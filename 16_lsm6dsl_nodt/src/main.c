#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <stdint.h>

LOG_MODULE_REGISTER(lsm6dsl_raw, LOG_LEVEL_INF);

#define I2C_NODE DT_ALIAS(work_i2c)
#define LSM6DSL_ADDR 0x6A

#define REG_WHO_AM_I    0x0F
#define REG_CTRL1_XL    0x10
#define REG_OUTX_L_XL   0x28

#define LSM6DSL_SENSITIVITY_2G  (2.0f * 10.0f / 32768.0f)

static int16_t to_int16(uint8_t lsb, uint8_t msb)
{
    return (int16_t)((msb << 8) | lsb);
}

int main(void)
{
    const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -1;
    }

    // Verificar WHO_AM_I
    uint8_t id = 0;
    if (i2c_reg_read_byte(i2c_dev, LSM6DSL_ADDR, REG_WHO_AM_I, &id) < 0 || id != 0x6A) {
        LOG_ERR("LSM6DSL not found or wrong ID (0x%02x)", id);
        return -1;
    }

    LOG_INF("LSM6DSL detected. WHO_AM_I = 0x%02x", id);

    // Configurar acelerômetro: 104 Hz, ±2g, BW = 100 Hz
    // Valor = ODR_XL[3:0]=0100 (104 Hz), FS_XL[1:0]=00 (±2g), BW_XL[1:0]=00 (100Hz)
    if (i2c_reg_write_byte(i2c_dev, LSM6DSL_ADDR, REG_CTRL1_XL, 0x40) < 0) {
        LOG_ERR("Failed to write CTRL1_XL");
        return -1;
    }
    LOG_INF("Acelerômetro ativado (104 Hz, ±2g)");

    while (1) {
        uint8_t data[6];
        if (i2c_burst_read(i2c_dev, LSM6DSL_ADDR, REG_OUTX_L_XL, data, 6) < 0) {
            LOG_ERR("Failed to read acceleration data");
            return -1;
        }

        int16_t ax = to_int16(data[0], data[1]);
        int16_t ay = to_int16(data[2], data[3]);
        int16_t az = to_int16(data[4], data[5]);

        float ax_mg = ax * LSM6DSL_SENSITIVITY_2G;
        float ay_mg = ay * LSM6DSL_SENSITIVITY_2G;
        float az_mg = az * LSM6DSL_SENSITIVITY_2G;

        LOG_INF("Accel [mg]: X=%.2f Y=%.2f Z=%.2f", 
                (double)ax_mg, (double)ay_mg, (double)az_mg);

        k_sleep(K_MSEC(500));
    }
}
