#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adxl362_raw, LOG_LEVEL_INF);

#define SPI_DEV_NAME     "SPI3"
#define CS_GPIO_DEV_NAME "GPIO_0"
#define CS_PIN           22
#define CS_FLAGS         GPIO_ACTIVE_LOW

#define REG_PARTID      0x02
#define REG_FILTER_CTL  0x2C
#define REG_POWER_CTL   0x2D
#define REG_XDATA_L     0x0E

static int spi_reg_read_byte(const struct device *dev, const struct spi_config *cfg,
                             uint8_t reg, uint8_t *val)
{
    uint8_t tx_buf[2] = {0x0B, reg}; // Read command + register
    struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
    struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};

    struct spi_buf rx_bufs[2] = {
        {.buf = NULL, .len = sizeof(tx_buf)}, // discard echoed command
        {.buf = val, .len = 1},               // read one byte
    };
    struct spi_buf_set rx_set = {.buffers = rx_bufs, .count = 2};

    return spi_transceive(dev, cfg, &tx_set, &rx_set);
}

static int spi_reg_write_byte(const struct device *dev, const struct spi_config *cfg,
                              uint8_t reg, uint8_t val)
{
    uint8_t tx_buf[3] = {0x0A, reg, val}; // Write command
    struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
    struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};
    return spi_write(dev, cfg, &tx_set);
}

static int spi_burst_read(const struct device *dev, const struct spi_config *cfg,
                          uint8_t start_reg, uint8_t *data, size_t len)
{
    uint8_t cmd_buf[2] = {0x0B, start_reg};
    struct spi_buf tx_bufs[1] = {{.buf = cmd_buf, .len = sizeof(cmd_buf)}};
    struct spi_buf rx_bufs[2] = {
        {.buf = NULL, .len = sizeof(cmd_buf)}, // discard echoed command
        {.buf = data, .len = len},
    };

    struct spi_buf_set tx = {.buffers = tx_bufs, .count = 1};
    struct spi_buf_set rx = {.buffers = rx_bufs, .count = 2};

    return spi_transceive(dev, cfg, &tx, &rx);
}

int ain(void)
{
    const struct device *spi_dev = device_get_binding(SPI_DEV_NAME);
    const struct device *gpio_dev = device_get_binding(CS_GPIO_DEV_NAME);

    if (!device_is_ready(spi_dev) || !device_is_ready(gpio_dev)) {
        LOG_ERR("SPI or GPIO device not ready");
        return;
    }

    struct spi_cs_control cs_ctrl = {
        .gpio = {
            .port = gpio_dev,
            .pin = CS_PIN,
            .dt_flags = CS_FLAGS,
        },
        .delay = 0,
    };

    struct spi_config spi_cfg = {
        .frequency = 4000000,
        .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
                     SPI_MODE_CPOL | SPI_MODE_CPHA,
        .slave = 0,
        .cs = cs_ctrl,
    };

    if (spi_cs_is_gpio(&spi_cfg)) {
        LOG_INF("CS is controlled via GPIO");
    }

    uint8_t id = 0;
    if (spi_reg_read_byte(spi_dev, &spi_cfg, REG_PARTID, &id) < 0 || id != 0xF2) {
        LOG_ERR("ADXL362 not found or wrong ID (0x%02x)", id);
        return -1;
    }

    LOG_INF("ADXL362 detected. PARTID = 0x%02x", id);

    spi_reg_write_byte(spi_dev, &spi_cfg, REG_FILTER_CTL, 0x05); // ODR = 12.5Hz
    spi_reg_write_byte(spi_dev, &spi_cfg, REG_POWER_CTL, 0x02);  // Measurement mode

    while (1) {
        uint8_t raw[6];
        if (spi_burst_read(spi_dev, &spi_cfg, REG_XDATA_L, raw, 6) == 0) {
            int16_t x = (raw[1] << 8) | raw[0];
            int16_t y = (raw[3] << 8) | raw[2];
            int16_t z = (raw[5] << 8) | raw[4];
            LOG_INF("Accel X: %d, Y: %d, Z: %d", x, y, z);
        }

        // Exemplo de uso do spi_release() ao final da transação (opcional)
        spi_release(spi_dev, &spi_cfg);

        k_sleep(K_MSEC(500));
    }
}
