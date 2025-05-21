#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tilt_sensor, CONFIG_TILT_SENSOR_LOG_LEVEL);

#define SENSOR_NODE DT_ALIAS(accel)
#define LED0_NODE DT_ALIAS(led0)

const struct device *accel_dev = DEVICE_DT_GET(SENSOR_NODE);
static const struct gpio_dt_spec led_dev = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct k_work_delayable stop_motion_work;
static bool motion_active = false;

void stop_motion_handler(struct k_work *work)
{
    motion_active = false;
    gpio_pin_set_dt(&led_dev, 0);
    LOG_INF("Motion timeout: LED stopped");
}

void adxl362_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
    if (trigger->type == SENSOR_TRIG_MOTION) {
        LOG_INF("Trigger: MOTION");
        motion_active = true;
        gpio_pin_set_dt(&led_dev, 1);

        // (Re)agenda desligamento do LED após 5 segundos
        k_work_reschedule(&stop_motion_work, K_SECONDS(5));
    }
}

int main(void)
{
    struct sensor_trigger trig_motion = {
        .type = SENSOR_TRIG_MOTION,
        .chan = SENSOR_CHAN_ACCEL_XYZ
    };

    struct sensor_value odr_attr;
    struct sensor_value duration;
    int ret;

    LOG_INF("Starting ADXL362 motion + timeout example...");

    if (!device_is_ready(accel_dev)) {
        LOG_ERR("Sensor not ready");
        return -1;
    }

    if (!gpio_is_ready_dt(&led_dev)) {
        LOG_ERR("LED not ready");
        return -1;
    }

    if (gpio_pin_configure_dt(&led_dev, GPIO_OUTPUT_INACTIVE) < 0) {
        LOG_ERR("Failed to configure LED");
        return -1;
    }

    // Inicializa o delayed work
    k_work_init_delayable(&stop_motion_work, stop_motion_handler);

    // Define ODR = 12.5 Hz
    odr_attr.val1 = 12;
    odr_attr.val2 = 500000;
    sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);

    // Duração do movimento = 3 amostras
    duration.val1 = 3;
    duration.val2 = 0;
    sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_HYSTERESIS, &duration);

    // Ativa apenas o trigger de motion
    ret = sensor_trigger_set(accel_dev, &trig_motion, adxl362_trigger_handler);
    if (ret < 0) {
        LOG_ERR("Failed to set motion trigger");
        return -1;
    }

}
