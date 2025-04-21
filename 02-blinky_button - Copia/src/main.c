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
 
 int main(void)
 {
     int ret;
 
     printk("Hello Button + LED!\n");
 
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
 
     int last_state = gpio_pin_get_dt(&button);
 
     while (1) {
         int current = gpio_pin_get_dt(&button);
         if (current == 0 && last_state == 1) {
             gpio_pin_toggle_dt(&led);
         }
         last_state = current;
         k_msleep(20); // Para não travar o while loop
     }
 }
 