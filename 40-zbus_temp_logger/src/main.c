#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(zbus_temp, LOG_LEVEL_INF);

/* LEDs */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* Sensor device */
static const struct device *bmp180 = DEVICE_DT_GET_ONE(bosch_bmp180);

/* Threshold for LED0 */
#define TEMP_THRESHOLD 28.0f

static void led1_callback(const struct zbus_channel *chan)
{
    gpio_pin_toggle_dt(&led1);
}

ZBUS_LISTENER_DEFINE(led1_listener, led1_callback);

ZBUS_SUBSCRIBER_DEFINE(sensor_sub, 4);

/* Zbus definitions */
ZBUS_CHAN_DEFINE(tick_chan,
                 bool,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(led1_listener, sensor_sub),
                 ZBUS_MSG_INIT(false));

ZBUS_SUBSCRIBER_DEFINE(process_sub, 1);
ZBUS_MSG_SUBSCRIBER_DEFINE(logger_sub);

ZBUS_CHAN_DEFINE(temp_chan,
                 float,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(process_sub, logger_sub),
                 ZBUS_MSG_INIT(0.0f));

/* Timer thread publishes tick */
void timer_thread(void)
{
    bool tick = true;

    while (1) {
        zbus_chan_pub(&tick_chan, &tick, K_NO_WAIT);
        k_sleep(K_SECONDS(1));
    }
}

/* Sensor thread waits for tick and reads temperature */
void sensor_thread(void)
{
    const struct zbus_channel *chan;
    struct sensor_value temp_val;
    float temp;

    while (1) {
        zbus_sub_wait(&sensor_sub, &chan, K_FOREVER);
        if (chan == &tick_chan) {
            if (sensor_sample_fetch(bmp180) == 0 &&
                sensor_channel_get(bmp180, SENSOR_CHAN_DIE_TEMP, &temp_val) == 0) {
                temp = (float)sensor_value_to_double(&temp_val);
                zbus_chan_pub(&temp_chan, &temp, K_NO_WAIT);
            } else {
                LOG_ERR("BMP180 read failed");
            }
        }
    }
}

/* Process thread controls LED0 */
void process_thread(void)
{
    const struct zbus_channel *chan;
    float temp;

    while (1) {
        zbus_sub_wait(&process_sub, &chan, K_FOREVER);
        if (chan == &temp_chan) {
            if (zbus_chan_read(&temp_chan, &temp, K_NO_WAIT) == 0) {
                gpio_pin_set_dt(&led0, temp > TEMP_THRESHOLD);
            }
        }
    }
}

/* Logger thread prints all samples */
void logger_thread(void)
{
    const struct zbus_channel *chan;
    float temp;

    while (1) {
        zbus_sub_wait_msg(&logger_sub, &chan, &temp, K_FOREVER);
        if (chan == &temp_chan) {
            LOG_INF("Temp: %.2f C", (double)temp);
        }
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(timer_tid, STACK_SIZE, timer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(sensor_tid, STACK_SIZE, sensor_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(process_tid, STACK_SIZE, process_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(logger_tid, STACK_SIZE, logger_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    if (!device_is_ready(bmp180)) {
        LOG_ERR("BMP180 not ready");
        return -1;
    }

    if (!device_is_ready(led0.port) || !device_is_ready(led1.port)) {
        LOG_ERR("LED devices not ready");
        return -1;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);

    LOG_INF("Zbus temperature logger example");
    return 0;
}

