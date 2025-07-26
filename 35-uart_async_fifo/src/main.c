#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_async_fifo, LOG_LEVEL_INF);

/* Reserve first word for k_fifo */
struct uart_item {
    void *fifo_reserved;
    size_t len;
    char data[];
};

K_FIFO_DEFINE(uart_fifo);

#define UART_NODE DT_ALIAS(work_uart)
static const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);

#define BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

#define RX_BUF_SIZE 64
static uint8_t rx_buf[RX_BUF_SIZE];
static size_t rx_pos;

static void dump_fifo(void)
{
    struct uart_item *item;

    while ((item = k_fifo_get(&uart_fifo, K_NO_WAIT)) != NULL) {
        LOG_INF("Received: %s", log_strdup(item->data));
        k_free(item);
        /* Comment out the line above to see the wrong way of using k_fifo */
    }
}

static void button_pressed_isr(const struct device *dev, struct gpio_callback *cb,
                               uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    LOG_INF("Button pressed: dumping fifo");
    dump_fifo();
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type) {
    case UART_RX_RDY:
        for (size_t i = 0; i < evt->data.rx.len; i++) {
            char c = rx_buf[evt->data.rx.offset + i];
            if (c == '\n' || c == '\r') {
                struct uart_item *item =
                    k_malloc(sizeof(struct uart_item) + rx_pos + 1);
                /* Comment out the line above to see the wrong way of using k_fifo */
                if (!item) {
                    LOG_ERR("Allocation failed");
                    rx_pos = 0;
                    break;
                }

                item->len = rx_pos;
                memcpy(item->data, rx_buf, rx_pos);
                item->data[rx_pos] = '\0';

                k_fifo_put(&uart_fifo, item);

                rx_pos = 0;
                uart_rx_disable(uart_dev);
            } else if (rx_pos < RX_BUF_SIZE - 1) {
                rx_buf[rx_pos++] = c;
            }
        }
        break;

    case UART_RX_DISABLED:
        uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50);
        break;

    default:
        break;
    }
}

int main(void)
{
    int ret;

    LOG_INF("UART async FIFO sample");

    if (!device_is_ready(uart_dev) || !device_is_ready(button.port)) {
        LOG_ERR("Devices not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure button");
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button interrupt");
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    ret = uart_callback_set(uart_dev, uart_cb, NULL);
    if (ret) {
        LOG_ERR("Failed to set UART callback");
        return ret;
    }

    ret = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50);
    if (ret) {
        LOG_ERR("Failed to enable UART RX");
        return ret;
    }

    return 0; /* everything runs via callbacks */
}

