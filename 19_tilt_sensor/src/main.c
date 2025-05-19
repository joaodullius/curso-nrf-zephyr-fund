#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(tilt_sensor, CONFIG_TILT_SENSOR_LOG_LEVEL);

#define SENSOR_NODE DT_ALIAS(accel)
const struct device *accel_dev = DEVICE_DT_GET(SENSOR_NODE);
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led_dev = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#ifdef CONFIG_BOARD_BBC_MICROBIT_V2
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led_ctrl = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#endif

#include <zephyr/sys/util.h>

#define REF_X CONFIG_TILT_REF_X
#define REF_Y CONFIG_TILT_REF_Y
#define REF_Z CONFIG_TILT_REF_Z
#define TOL_PCT CONFIG_TILT_TOLERANCE_PERCENT

bool is_upright(const struct sensor_value *accel)
{
    int32_t acc_x = accel[0].val1 * 1000 + accel[0].val2 / 1000;
    int32_t acc_y = accel[1].val1 * 1000 + accel[1].val2 / 1000;
    int32_t acc_z = accel[2].val1 * 1000 + accel[2].val2 / 1000;
    LOG_DBG("acc_x=%d, acc_y=%d, acc_z=%d", acc_x, acc_y, acc_z);
    LOG_DBG("REF_X=%d, REF_Y=%d, REF_Z=%d", REF_X, REF_Y, REF_Z);

    const int32_t base = 10000; // fundo de escala de 10 m/s²

    int32_t tol_x = MAX(abs(REF_X), base) * TOL_PCT / 100;
    int32_t tol_y = MAX(abs(REF_Y), base) * TOL_PCT / 100;
    int32_t tol_z = MAX(abs(REF_Z), base) * TOL_PCT / 100;

    LOG_DBG("tol_x=%d, tol_y=%d, tol_z=%d", tol_x, tol_y, tol_z);

    bool ok_x = abs(acc_x - REF_X) <= tol_x;
    bool ok_y = abs(acc_y - REF_Y) <= tol_y;
    bool ok_z = abs(acc_z - REF_Z) <= tol_z;
    LOG_DBG("ok_x=%d -> acc_x %d - REF_X %d <= %d",
            ok_x, acc_x, REF_X, tol_x);
    LOG_DBG("ok_y=%d -> acc_y %d - REF_Y %d <= %d",
            ok_y, acc_y, REF_Y, tol_y);
    LOG_DBG("ok_z=%d -> acc_z %d - REF_Z %d <= %d",
            ok_z, acc_z, REF_Z, tol_z);

    return ok_x && ok_y && ok_z;
}



int main(void)
{

    struct sensor_value odr_attr;
    int ret;

    LOG_INF("Starting tilt sensor example...");
    LOG_INF("Using sensor: %s", accel_dev->name);
    LOG_INF("Reference: X=%d Y=%d Z=%d, Tolerance=%d%%", REF_X, REF_Y, REF_Z, TOL_PCT);

    if (!device_is_ready(accel_dev)) {
        LOG_ERR("Sensor device not ready");
        return -1;
    }

    /* set accel/gyro sampling frequency to 104 Hz */
    odr_attr.val1 = 104;
    odr_attr.val2 = 0;

    if (sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_WRN("Accel sampling frequency not configurable, skipping.");
    }

    if (sensor_attr_set(accel_dev, SENSOR_CHAN_GYRO_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_WRN("Gyro sampling frequency not configurable, skipping.");
    }

    if (sensor_sample_fetch(accel_dev) < 0) {
        LOG_ERR("Sensor sample update error");
        return -1;
    }

    if (!gpio_is_ready_dt(&led_dev)) {
        LOG_ERR("LED device not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&led_dev, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

//Codigo para o micro:bit V2. Seta o LED1 (coluna) como HIGH
#ifdef CONFIG_BOARD_BBC_MICROBIT_V2
    if (!gpio_is_ready_dt(&led_ctrl)) {
        LOG_ERR("LED1 (column control) not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&led_ctrl, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED1");
        return ret;
    }

    ret = gpio_pin_set_dt(&led_ctrl, 1);
    if (ret < 0) {
        LOG_ERR("Failed to activate LED1");
        return ret;
    }
#endif

    while (1) {
            struct sensor_value accel[3];

            if (sensor_sample_fetch(accel_dev) < 0 ||
                sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
                LOG_ERR("Sensor read error");
                continue;
            }

            LOG_INF("Accel [m/s^2]: X=%.3f Y=%.3f Z=%.3f",
                    sensor_value_to_double(&accel[0]),
                    sensor_value_to_double(&accel[1]),
                    sensor_value_to_double(&accel[2]));

            if (is_upright(accel)) {
                LOG_INF("Position: UPRIGHT");
                gpio_pin_set_dt(&led_dev, 0);
                k_sleep(K_MSEC(500));
            } else {
                LOG_INF("Position: TILTED");
                gpio_pin_toggle_dt(&led_dev);
                k_sleep(K_MSEC(100));
            }
        }
}
