#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_int, LOG_LEVEL_INF);

#define UART_DEVICE_NODE DT_ALIAS(work_uart)
static const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define MSG_SIZE 64
static char rx_buf[MSG_SIZE];
static int rx_buf_pos = 0;

// Define a tarefa de trabalho para processar a linha recebida
void handle_line_work(struct k_work *work);
void print_uart(const char *buf);
K_WORK_DEFINE(line_work, handle_line_work);

/*
 * Workqueue handler — executado fora da ISR
 */
void handle_line_work(struct k_work *work)
{
    print_uart("Echo: ");
    print_uart(rx_buf);
    print_uart("\r\n");

    rx_buf_pos = 0;
}

/*
 * Callback da interrupção UART
 */
void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(dev)) {
        return;
    }

    while (uart_irq_rx_ready(dev)) {
        if (uart_fifo_read(dev, &c, 1) != 1) {
            continue;
        }

        if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
            rx_buf[rx_buf_pos] = '\0';
            k_work_submit(&line_work); // envia para a system workqueue
        } else if (rx_buf_pos < MSG_SIZE - 1) {
            rx_buf[rx_buf_pos++] = c;
        }
    }
}

/*
 * Envia string pela UART
 */
void print_uart(const char *buf)
{
    while (*buf) {
        uart_poll_out(uart_dev, *buf++);
    }
}

int main(void)
{
    LOG_INF("UART Interrupt Example (with k_work)");

    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready!\n");
        return -1;
    }

    uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    uart_irq_rx_enable(uart_dev);

    print_uart("Hello! I'm your echo bot.\r\n");
    print_uart("Type something and press enter:\r\n");

    return 0; // nada de while(1), tudo via eventos
}
