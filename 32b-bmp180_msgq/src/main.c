#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bmp180_msgq, LOG_LEVEL_INF);

#define BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

const struct device *bmp180 = DEVICE_DT_GET_ONE(bosch_bmp180);

struct bmp180_msg {
    struct sensor_value temp;
};

K_MSGQ_DEFINE(bmp180_q, sizeof(struct bmp180_msg), 10, 4);

static void dump_queue(void)
{
    struct bmp180_msg msg;

    while (k_msgq_get(&bmp180_q, &msg, K_NO_WAIT) == 0) {
        LOG_INF("Temp: %0.2f C",
                sensor_value_to_double(&msg.temp));
    }
}

static void button_pressed_isr(const struct device *dev, struct gpio_callback *cb,
                              uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);
    LOG_INF("Button pressed: dumping queue");
    dump_queue();
}

void sensor_thread(void)
{
    struct bmp180_msg msg;

    while (1) {
        if (sensor_sample_fetch(bmp180) != 0) {
            LOG_ERR("sensor_sample_fetch failed");
        } else if (sensor_channel_get(bmp180, SENSOR_CHAN_DIE_TEMP, &msg.temp) != 0) {
            LOG_ERR("sensor_channel_get failed");
        } else {
            k_msgq_put(&bmp180_q, &msg, K_NO_WAIT);
        }
    }
}


#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(sensor_tid, STACK_SIZE, sensor_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    int ret;

    LOG_INF("BMP180 message queue example");

    if (!device_is_ready(bmp180)) {
        LOG_ERR("BMP180 device not ready");
        return -1;
    }

    if (!device_is_ready(button.port)) {
        LOG_ERR("Button device not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure button");
        return -1;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button interrupt");
        return -1;
    }

    gpio_init_callback(&button_cb_data, button_pressed_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    return 0;
}
