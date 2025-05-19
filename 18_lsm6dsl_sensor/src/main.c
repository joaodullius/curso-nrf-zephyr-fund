#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lsm6dsl_driver, LOG_LEVEL_INF);

#define SENSOR_NODE DT_NODELABEL(lsm6ds3tr_c)

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(SENSOR_NODE);
    struct sensor_value odr_attr;

    if (!device_is_ready(dev)) {
        LOG_ERR("Sensor device not ready");
        return -1;
    }

	/* set accel/gyro sampling frequency to 104 Hz */
	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return 0;
	}

	if (sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return 0;
	}

	if (sensor_sample_fetch(dev) < 0) {
		printk("Sensor sample update error\n");
		return 0;
	}
    while (1) {
        struct sensor_value accel[3];

        if (sensor_sample_fetch(dev) < 0) {
            LOG_ERR("Failed to fetch sensor data");
            return -1;
        }

        if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
            LOG_ERR("Failed to get sensor channel data");
            return -1;
        }

        LOG_INF("Accel [m/s^2]: X=%.3f Y=%.3f Z=%.3f",
                sensor_value_to_double(&accel[0]),
                sensor_value_to_double(&accel[1]),
                sensor_value_to_double(&accel[2]));

        k_sleep(K_MSEC(500));
    }
}
