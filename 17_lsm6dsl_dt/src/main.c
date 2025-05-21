#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <stdint.h>

LOG_MODULE_REGISTER(lsm6dsl_raw, LOG_LEVEL_INF);

#define LSM6DSL_NODE DT_NODELABEL(lsm6dsl)
static const struct i2c_dt_spec lsm6dsl = I2C_DT_SPEC_GET(LSM6DSL_NODE);

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
    if (!device_is_ready(lsm6dsl.bus)) {
        LOG_ERR("I2C bus not ready");
        return -1;
    }

    uint8_t id = 0;
    if (i2c_reg_read_byte_dt(&lsm6dsl, REG_WHO_AM_I, &id) < 0 || id != 0x6A) {
        LOG_ERR("LSM6DSL not found or wrong ID (0x%02x)", id);
        return -1;
    }

    LOG_INF("LSM6DSL detected. WHO_AM_I = 0x%02x", id);

    if (i2c_reg_write_byte_dt(&lsm6dsl, REG_CTRL1_XL, 0x40) < 0) {
        LOG_ERR("Failed to write CTRL1_XL");
        return -1;
    }
    LOG_INF("Acelerômetro ativado (104 Hz, ±2g)");

    while (1) {
        uint8_t data[6];
        if (i2c_burst_read_dt(&lsm6dsl, REG_OUTX_L_XL, data, 6) < 0) {
            LOG_ERR("Failed to read acceleration data");
            return -1;
        }

        int16_t ax = to_int16(data[0], data[1]);
        int16_t ay = to_int16(data[2], data[3]);
        int16_t az = to_int16(data[4], data[5]);

        float ax_mg = ax * LSM6DSL_SENSITIVITY_2G;
        float ay_mg = ay * LSM6DSL_SENSITIVITY_2G;
        float az_mg = az * LSM6DSL_SENSITIVITY_2G;

        LOG_INF("Accel [m/s^2]: X=%.2f Y=%.2f Z=%.2f", 
                (double)ax_mg, (double)ay_mg, (double)az_mg);

        k_sleep(K_MSEC(500));
    }
}
