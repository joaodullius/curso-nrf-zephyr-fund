#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(adxl362_sensor, LOG_LEVEL_INF);

static void imu_trigger_handler(const struct device *dev,
	const struct sensor_trigger *trig)
{
	struct sensor_value accel[3];
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    LOG_INF("Accel [m/s^2]: X=%.3f Y=%.3f Z=%.3f",
            sensor_value_to_double(&accel[0]),
            sensor_value_to_double(&accel[1]),
            sensor_value_to_double(&accel[2]));
}


int main() {

    const struct device *imu = DEVICE_DT_GET_ONE(adi_adxl362);
    if (!device_is_ready(imu)) {
        printf("imu device not ready\n");
        return 1;
    }

	// Set scale to ±2g
	struct sensor_value scale_val = {
		.val1 = 2,
		.val2 = 0,
	};
	sensor_attr_set(imu, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &scale_val);

	// Set ODR to 400 Hz
	struct sensor_value odr_val = {
		.val1 = 400,
		.val2 = 0,
	};
	sensor_attr_set(imu, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_val);

	struct sensor_trigger trig = {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ACCEL_XYZ,
    };

    sensor_trigger_set(imu, &trig, imu_trigger_handler);
}
