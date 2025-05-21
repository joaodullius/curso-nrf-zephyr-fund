#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Substitua esse alias pelo correto no seu overlay
#define SPI_NODE DT_ALIAS(adxl362_spi)

#if !DT_NODE_HAS_STATUS(SPI_NODE, okay)
#error "SPI device not defined in devicetree"
#endif


#define SPIOP (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER)
static const struct spi_dt_spec spi_adxl362 = SPI_DT_SPEC_GET(SPI_NODE, SPIOP, 0);

// Comandos do ADXL362
#define ADXL362_CMD_WRITE_REG   0x0A
#define ADXL362_CMD_READ_REG    0x0B
#define ADXL362_REG_DEVID_AD    0x00

#define ADXL362_REG_POWER_CTL  0x2D
#define ADXL362_MEASURE_MODE   0x02

#define ADXL362_REG_XDATA_L   0x0E
#define ADXL362_REG_YDATA_L   0x10
#define ADXL362_REG_ZDATA_L   0x12

#define GRAVITY_M_S2 9.80665f


int adxl362_read_reg(uint8_t reg, uint8_t *value)
{
    uint8_t tx_buf[3] = {
        ADXL362_CMD_READ_REG,
        reg,
        0x00 // Dummy
    };
    uint8_t rx_buf[3] = {0};

    const struct spi_buf tx_spi_buf = {
        .buf = tx_buf,
        .len = sizeof(tx_buf),
    };
    const struct spi_buf_set tx_set = {
        .buffers = &tx_spi_buf,
        .count = 1,
    };

    struct spi_buf rx_spi_buf = {
        .buf = rx_buf,
        .len = sizeof(rx_buf),
    };
    const struct spi_buf_set rx_set = {
        .buffers = &rx_spi_buf,
        .count = 1,
    };

    int ret = spi_transceive_dt(&spi_adxl362, &tx_set, &rx_set);
    if (ret == 0) {
        *value = rx_buf[2]; // O valor lido está na terceira posição
    }
    LOG_DBG("Read reg 0x%02X: 0x%02X", reg, *value);
    return ret;
}

int adxl362_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx_buf[3] = {
        ADXL362_CMD_WRITE_REG,
        reg,
        value
    };

    const struct spi_buf tx_spi_buf = {
        .buf = tx_buf,
        .len = sizeof(tx_buf),
    };
    const struct spi_buf_set tx_set = {
        .buffers = &tx_spi_buf,
        .count = 1,
    };

    return spi_write_dt(&spi_adxl362, &tx_set);
}

int16_t read_axis(uint8_t reg_base)
{
    uint8_t lsb, msb;
    if (adxl362_read_reg(reg_base, &lsb) < 0) return 0;
    if (adxl362_read_reg(reg_base + 1, &msb) < 0) return 0;

    int16_t value = (int16_t)((msb << 8) | lsb);  // Sign-extended by hardware
    LOG_DBG("Axis 0x%02X: LSB=0x%02X, MSB=0x%02X -> %d mg", reg_base, lsb, msb, value);

    return value;
}



int main(void)
{
    int ret;
    uint8_t devid = 0;

    if (!device_is_ready(spi_adxl362.bus)) {
        LOG_ERR("SPI bus not ready");
        return 1;
    }

    ret = adxl362_read_reg(ADXL362_REG_DEVID_AD, &devid);
    if (ret == 0) {
        LOG_INF("ADXL362 Device ID: 0x%02X", devid);
        if (devid != 0xAD) {
            LOG_WRN("Unexpected device ID!");
        }
    } else {
        LOG_ERR("Failed to read from ADXL362 (err %d)", ret);
    }

    ret = adxl362_write_reg(ADXL362_REG_POWER_CTL, ADXL362_MEASURE_MODE);
    if (ret == 0) {
        LOG_INF("Measurement mode enabled");
        k_msleep(10);  // Pequeno delay para o sensor estabilizar
    } else {
        LOG_ERR("Failed to set measurement mode");
    }

    uint8_t power_ctl;
    adxl362_read_reg(ADXL362_REG_POWER_CTL, &power_ctl);
    LOG_DBG("POWER_CTL = 0x%02X", power_ctl);

    while (1) {
        int16_t ax = read_axis(ADXL362_REG_XDATA_L);
        int16_t ay = read_axis(ADXL362_REG_YDATA_L);
        int16_t az = read_axis(ADXL362_REG_ZDATA_L);

        float ax_mg = ax * 1.0f;
        float ay_mg = ay * 1.0f;
        float az_mg = az * 1.0f;

        float ax_ms2 = ax_mg * GRAVITY_M_S2 / 1000.0f;
        float ay_ms2 = ay_mg * GRAVITY_M_S2 / 1000.0f;
        float az_ms2 = az_mg * GRAVITY_M_S2 / 1000.0f;

        LOG_INF("Accel [m/s^2]: X=%.2f Y=%.2f Z=%.2f", (double)ax_ms2, (double)ay_ms2, (double)az_ms2);

        k_msleep(500);
    }

    return 0;
}

