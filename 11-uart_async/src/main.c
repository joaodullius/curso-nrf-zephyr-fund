
/*
 * Copyright (c) 2022 Joao Dullius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/kernel.h>
 #include <zephyr/device.h>
 #include <zephyr/drivers/uart.h>
 #include <zephyr/logging/log.h>
 #include <string.h>
 
 LOG_MODULE_REGISTER(uart_async, LOG_LEVEL_DBG);
 
 #define MSG_SIZE 32
 
 struct msgq_item_t {
	 char buffer[MSG_SIZE];
	 int length;
 };
 K_MSGQ_DEFINE(uart_msgq, sizeof(struct msgq_item_t), 10, 4);
 
 #define UART_DEVICE_NODE DT_ALIAS(work_uart)
 static const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
 
 static char rx_buf[MSG_SIZE];
 
 static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
 {
	 struct msgq_item_t rx_data;
 
	 switch (evt->type) {
	 case UART_TX_DONE:
		 LOG_DBG("UART_TX_DONE");
		 LOG_INF("Tx sent %d bytes", evt->data.tx.len);
		 break;
 
	 case UART_TX_ABORTED:
		 LOG_DBG("UART_TX_ABORTED");
		 LOG_ERR("Tx aborted");
		 break;
 
	 case UART_RX_RDY:
		 LOG_DBG("UART_RX_RDY");
		 LOG_INF("Received data %d bytes", evt->data.rx.len);
		 memcpy(rx_data.buffer, rx_buf, evt->data.rx.len);
		 rx_data.buffer[evt->data.rx.len] = '\0';  // garantir término da string
		 rx_data.length = evt->data.rx.len;
		 k_msgq_put(&uart_msgq, &rx_data, K_NO_WAIT);
		 uart_rx_disable(uart_dev);
		 break;
 
	 case UART_RX_BUF_REQUEST:
		 LOG_DBG("UART_RX_BUF_REQUEST");
		 break;
 
	 case UART_RX_BUF_RELEASED:
		 LOG_DBG("UART_RX_BUF_RELEASED");
		 break;
 
	 case UART_RX_DISABLED:
		 LOG_DBG("UART_RX_DISABLED");
		 uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 100);
		 break;
 
	 case UART_RX_STOPPED:
		 LOG_DBG("UART_RX_STOPPED");
		 break;
 
	 default:
		 break;
	 }
 }
 
 void uart_send_fn(void *a, void *b, void *c)
 {
	 struct msgq_item_t tx_data;
	 int err;
 
	 while (1) {
		 k_msgq_get(&uart_msgq, &tx_data, K_FOREVER);
		 LOG_DBG("Send Message");
		 err = uart_tx(uart_dev, tx_data.buffer, tx_data.length, SYS_FOREVER_MS);
		 if (err) {
			 LOG_ERR("Transmit Error");
		 }
	 }
 }
 
 #define THREAD_STACK_SIZE 2048
 K_THREAD_DEFINE(send_thread, THREAD_STACK_SIZE, uart_send_fn, NULL, NULL, NULL,
				 K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
 
 int main(void)
 {
	 int err;
 
	 LOG_INF("UART Asynchronous Sample");
 
	 if (uart_dev == NULL || !device_is_ready(uart_dev)) {
		 printk("UART device not found!\n");
		 return -1;
	 }
 
	 err = uart_callback_set(uart_dev, uart_cb, NULL);
	 __ASSERT(err == 0, "Callback Set Failed");
 
	 err = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 100);
	 if (err) {
		 printk("Enable Error!\n");
		 return -1;
	 }
 }
 