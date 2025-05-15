#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_echo, LOG_LEVEL_DBG);

#define UART_NODE DT_ALIAS(work_uart)
const struct device *uart = DEVICE_DT_GET(UART_NODE);

int main(void)
{
    LOG_INF("Starting UART Echo\n");
    if (!device_is_ready(uart)) {
        return -1;
    }

    unsigned char c;

    while (1) {
        if (uart_poll_in(uart, &c) == 0) {
            uart_poll_out(uart, c);
            LOG_DBG("%c", c);
        }
        k_yield();
    }
}
