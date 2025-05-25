#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bmp680_sensor, LOG_LEVEL_INF);

int main(void)
{
    const struct device *bmp680 = DEVICE_DT_GET_ONE(bosch_bme680);

    if (!device_is_ready(bmp680)) {
        LOG_ERR("bmp680 device not ready");
        return -1;
    }

    LOG_INF("bmp680 driver ready");

    while (1) {
        struct sensor_value temp, press, hum, gas;

        if (sensor_sample_fetch(bmp680) < 0) {
            LOG_ERR("Failed to fetch sample");
            return -1;
        }

        if (sensor_channel_get(bmp680, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
            LOG_ERR("Failed to get temperature");
            return -1;
        }

        if (sensor_channel_get(bmp680, SENSOR_CHAN_PRESS, &press) < 0) {
            LOG_ERR("Failed to get pressure");
            return -1;
        }

        if (sensor_channel_get(bmp680, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
            LOG_ERR("Failed to get humidity");
            return -1;
        }

        if (sensor_channel_get(bmp680, SENSOR_CHAN_GAS_RES, &gas) < 0) {
            LOG_ERR("Failed to get gas resistance");
            return -1;
        }

        LOG_INF("Temperature: %0.3f °C", sensor_value_to_double(&temp));
        LOG_INF("Pressure: %0.3f hPa",sensor_value_to_double(&press));
        LOG_INF("Humidity: %0.3f %%", sensor_value_to_double(&hum));
        LOG_INF("Gas Resistance: %0.3f Ohm", sensor_value_to_double(&gas));
        
        k_sleep(K_MSEC(1000));
    }
}
