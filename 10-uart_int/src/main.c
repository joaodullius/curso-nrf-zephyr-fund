#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define MSG_SIZE 64
static char rx_buf[MSG_SIZE];
static int rx_buf_pos = 0;
static volatile bool line_ready = false;

/*
 * Callback chamado pela interrupção da UART sempre que dados estão disponíveis.
 * Acumula caracteres em rx_buf até encontrar um \n ou \r.
 */
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(dev)) {
        return;
    }

    while (uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, &c, 1);

        if ((c == '\n' || c == '\r') && rx_buf_pos > 0 && !line_ready) {
            rx_buf[rx_buf_pos] = '\0';
            line_ready = true;
        } else if (rx_buf_pos < MSG_SIZE - 1 && !line_ready) {
            rx_buf[rx_buf_pos++] = c;
        }
    }
}

/*
 * Envia uma string pela UART (caractere a caractere).
 */
void print_uart(const char *buf)
{
    while (*buf) {
        uart_poll_out(uart_dev, *buf++);
    }
}

void main(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready!\n");
        return;
    }

    uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    uart_irq_rx_enable(uart_dev);

    print_uart("Hello! I'm your echo bot.\r\n");
    print_uart("Type something and press enter:\r\n");

    while (1) {
        if (line_ready) {
            print_uart("Echo: ");
            print_uart(rx_buf);
            print_uart("\r\n");

            rx_buf_pos = 0;
            line_ready = false;
        }

        k_msleep(10); // evita busy loop
    }
}
