#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE   DT_ALIAS(led0)
#define SW0_NODE    DT_ALIAS(sw0)
#define SW1_NODE    DT_ALIAS(sw1)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec sw1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);

static struct gpio_callback sw0_cb_data, sw1_cb_data;

static struct k_poll_signal sw0_signal = K_POLL_SIGNAL_INITIALIZER(sw0_signal);
static struct k_poll_signal sw1_signal = K_POLL_SIGNAL_INITIALIZER(sw1_signal);

void sw0_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    k_poll_signal_raise(&sw0_signal, 0);
}

void sw1_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    k_poll_signal_raise(&sw1_signal, 0);
}

int main(void)
{
    int ret;

    printk("k_poll() com SW0 ou SW1\n");

    if (!device_is_ready(led.port) || !device_is_ready(sw0.port) || !device_is_ready(sw1.port)) {
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;

    ret = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
    if (ret < 0) return ret;

    ret = gpio_pin_configure_dt(&sw1, GPIO_INPUT);
    if (ret < 0) return ret;

    ret = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) return ret;

    ret = gpio_pin_interrupt_configure_dt(&sw1, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) return ret;

    gpio_init_callback(&sw0_cb_data, sw0_pressed, BIT(sw0.pin));
    gpio_add_callback(sw0.port, &sw0_cb_data);

    gpio_init_callback(&sw1_cb_data, sw1_pressed, BIT(sw1.pin));
    gpio_add_callback(sw1.port, &sw1_cb_data);

    struct k_poll_event events[2] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sw0_signal),
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sw1_signal),
    };

    while (1) {
        k_poll(events, 2, K_FOREVER);

        if (events[0].state == K_POLL_STATE_SIGNALED) {
            printk("SW0 pressionado - Invertendo LED\n");
            k_poll_signal_reset(&sw0_signal);
        }

        if (events[1].state == K_POLL_STATE_SIGNALED) {
            printk("SW1 pressionado - Invertendo LED\n");
            k_poll_signal_reset(&sw1_signal);
        }

        gpio_pin_toggle_dt(&led);
    }
}
