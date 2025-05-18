/*
 * Copyright (c) 2022 Joao Dullius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/kernel.h>
 #include <zephyr/drivers/gpio.h>
 
 #define LED0_NODE   DT_ALIAS(led0)
 #define BUTTON_NODE DT_ALIAS(sw0)
 
 static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
 static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
 
 static struct gpio_callback button_cb_data;

 void enable_interrupt_work(struct k_work *work) 
 {
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_LEVE_ACTIVE);
 }
 K_WORK_DELAYABLE_DEFINE(debounce_work, enable_interrupt_work);
 
 void button_pressed_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
 {
     gpio_pin_toggle_dt(&led);
     printk("Button pressed!\n");
     gpio_pin_interrupt_configure_dt(&button, GPIO_INT_DISABLE);
     k_work_schedule(&debounce_work, K_MSEC(500));
 }
 
 int main(void)
 {
     int ret;
 
     printk("Hello Button + LED (Interrupt)!\n");
 
     if (!device_is_ready(led.port) || !device_is_ready(button.port)) {
         return -1;
     }
 
     ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
     if (ret < 0) {
         return -1;
     }
 
     ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
     if (ret < 0) {
         return -1;
     }
 
     ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_LEVEL_ACTIVE);
     if (ret < 0) {
         return -1;
     }
 
     gpio_init_callback(&button_cb_data, button_pressed_isr, BIT(button.pin));
     gpio_add_callback(button.port, &button_cb_data);
 
     while (1) {
         k_sleep(K_FOREVER);
     }
 }
 