#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bmp280_raw, LOG_LEVEL_INF);

#define I2C_NODE DT_ALIAS(work_i2c)
#define BMP280_ADDR 0x76

#define BMP280_REG_ID         0xD0
#define BMP280_REG_TEMP_MSB   0xFA
#define BMP280_REG_CALIB      0x88

struct bmp280_calib_param {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
};

static struct bmp280_calib_param calib;
static int32_t t_fine;

int bmp280_read_calib(const struct device *i2c_dev)
{
    uint8_t buf[6];
    int ret = i2c_burst_read(i2c_dev, BMP280_ADDR, BMP280_REG_CALIB, buf, 6);
    if (ret < 0) return ret;

    calib.dig_T1 = (buf[1] << 8) | buf[0];
    calib.dig_T2 = (buf[3] << 8) | buf[2];
    calib.dig_T3 = (buf[5] << 8) | buf[4];

    return 0;
}

int32_t bmp280_compensate_temp(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
              ((int32_t)calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

int main(void)
{
    const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -1;
    }

    uint8_t id = 0;
    if (i2c_reg_read_byte(i2c_dev, BMP280_ADDR, BMP280_REG_ID, &id) < 0 || id != 0x58) {
        LOG_ERR("BMP280 not found or wrong ID (0x%02x)", id);
        return -1;
    }

    LOG_INF("BMP280 detected. ID = 0x%02x", id);

    i2c_reg_write_byte(i2c_dev, BMP280_ADDR, 0xF4, 0x27); // Normal mode

    if (bmp280_read_calib(i2c_dev) < 0) {
        LOG_ERR("Failed to read calibration data");
        return -1;
    }

    while (1) {
        uint8_t data[3];
        if (i2c_burst_read(i2c_dev, BMP280_ADDR, BMP280_REG_TEMP_MSB, data, 3) < 0) {
            LOG_ERR("Failed to read temperature");
            return -1;
        }

        int32_t adc_T = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
        int32_t temp = bmp280_compensate_temp(adc_T);

        LOG_INF("Temperature: %d.%02d °C", temp / 100, temp % 100);
        k_sleep(K_MSEC(1000));
    }
}
