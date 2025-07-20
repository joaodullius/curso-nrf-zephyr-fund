#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(tilt_sensor, CONFIG_TILT_SENSOR_LOG_LEVEL);

struct accel_sample {
    struct sensor_value x;
    struct sensor_value y;
    struct sensor_value z;
    uint32_t timestamp;
};

#define SENSOR_NODE DT_ALIAS(accel)
const struct device *accel_dev = DEVICE_DT_GET(SENSOR_NODE);
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led_dev = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#ifdef CONFIG_BOARD_BBC_MICROBIT_V2
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led_ctrl = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#endif

#define REF_X CONFIG_TILT_REF_X
#define REF_Y CONFIG_TILT_REF_Y
#define REF_Z CONFIG_TILT_REF_Z
#define TOL_PCT CONFIG_TILT_TOLERANCE_PERCENT

static bool is_upright(const struct accel_sample *sample)
{
    int32_t acc_x = sample->x.val1 * 1000 + sample->x.val2 / 1000;
    int32_t acc_y = sample->y.val1 * 1000 + sample->y.val2 / 1000;
    int32_t acc_z = sample->z.val1 * 1000 + sample->z.val2 / 1000;
    LOG_DBG("acc_x=%d, acc_y=%d, acc_z=%d", acc_x, acc_y, acc_z);
    LOG_DBG("REF_X=%d, REF_Y=%d, REF_Z=%d", REF_X, REF_Y, REF_Z);

    const int32_t base = 10000; /* fundo de escala de 10 m/s² */

    int32_t tol_x = MAX(abs(REF_X), base) * TOL_PCT / 100;
    int32_t tol_y = MAX(abs(REF_Y), base) * TOL_PCT / 100;
    int32_t tol_z = MAX(abs(REF_Z), base) * TOL_PCT / 100;

    LOG_DBG("tol_x=%d, tol_y=%d, tol_z=%d", tol_x, tol_y, tol_z);

    bool ok_x = abs(acc_x - REF_X) <= tol_x;
    bool ok_y = abs(acc_y - REF_Y) <= tol_y;
    bool ok_z = abs(acc_z - REF_Z) <= tol_z;
    LOG_DBG("ok_x=%d -> acc_x %d - REF_X %d <= %d", ok_x, acc_x, REF_X, tol_x);
    LOG_DBG("ok_y=%d -> acc_y %d - REF_Y %d <= %d", ok_y, acc_y, REF_Y, tol_y);
    LOG_DBG("ok_z=%d -> acc_z %d - REF_Z %d <= %d", ok_z, acc_z, REF_Z, tol_z);

    return ok_x && ok_y && ok_z;
}

K_MSGQ_DEFINE(accel_q, sizeof(struct accel_sample), 10, 4);

static void sampling_thread(void)
{
    struct sensor_value accel[3];
    struct accel_sample sample;
    static uint32_t counter = 0;

    while (1) {
        if (sensor_sample_fetch(accel_dev) == 0 &&
            sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_XYZ, accel) == 0) {
            
            sample.x = accel[0];
            sample.y = accel[1];
            sample.z = accel[2];
            sample.timestamp = counter++;
            
            if (k_msgq_put(&accel_q, &sample, K_NO_WAIT) != 0) {
                LOG_WRN("Message queue full, dropping sample");
            }
        } else {
            LOG_ERR("Sensor read error");
        }
        k_msleep(100);
    }
}

static void tilt_thread(void)
{
    struct accel_sample sample;

    while (1) {
        k_msgq_get(&accel_q, &sample, K_FOREVER);

        LOG_INF("Accel [m/s^2]: X=%.3f Y=%.3f Z=%.3f (timestamp=%u)",
                sensor_value_to_double(&sample.x),
                sensor_value_to_double(&sample.y),
                sensor_value_to_double(&sample.z),
                sample.timestamp);

        if (is_upright(&sample)) {
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

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(sample_tid, STACK_SIZE, sampling_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(tilt_tid, STACK_SIZE, tilt_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    struct sensor_value odr_attr;
    int ret;

    LOG_INF("Starting tilt sensor example with k_msgq...");
    LOG_INF("Using sensor: %s", accel_dev->name);
    LOG_INF("Reference: X=%d Y=%d Z=%d, Tolerance=%d%%", REF_X, REF_Y, REF_Z, TOL_PCT);

    if (!device_is_ready(accel_dev)) {
        LOG_ERR("Sensor device not ready");
        return -1;
    }

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
        return ret;
    }

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

    return 0;
}
