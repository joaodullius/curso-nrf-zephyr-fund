#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define UART_NODE DT_NODELABEL(uart0)
const struct device *uart = DEVICE_DT_GET(UART_NODE);

void main(void)
{
    if (!device_is_ready(uart)) {
        return;
    }

    unsigned char c;

    while (1) {
        if (uart_poll_in(uart, &c) == 0) {
            uart_poll_out(uart, c);
        }
        k_yield();
    }
}
