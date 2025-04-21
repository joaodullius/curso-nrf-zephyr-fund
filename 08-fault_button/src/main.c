/*
 * Copyright (c) 2022 Joao Dullius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/kernel.h>
 #include <zephyr/drivers/gpio.h>
 #include <zephyr/logging/log.h>
 #include <inttypes.h>
 
 LOG_MODULE_REGISTER(fault_demo, LOG_LEVEL_INF);
 
 #define LED0_NODE   DT_ALIAS(led0)
 #define BUTTON_NODE DT_ALIAS(sw0)
 
 static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
 static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
 
 void crash_function(uint32_t *addr)
 {
     LOG_INF("Button pressed at %" PRIu32, k_cycle_get_32());
     LOG_INF("Coredump: %s", CONFIG_BOARD);
 
 #if !defined(CONFIG_CPU_CORTEX_M)
     *addr = 0;
 #else
     ARG_UNUSED(addr);
     __asm__ volatile("udf #0" : : : );
 #endif
 }
 
 static void cause_fault(void)
 {
     crash_function(0);
 }
 
 int main(void)
 {
     int ret;
 
     LOG_INF("Fault Button!");
 
     if (!device_is_ready(led.port) || !device_is_ready(button.port)) {
         LOG_ERR("Dispositivos LED ou botão não prontos.");
         return -1;
     }
 
     ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
     if (ret < 0) {
         LOG_ERR("Erro ao configurar LED.");
         return -1;
     }
 
     ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
     if (ret < 0) {
         LOG_ERR("Erro ao configurar botão.");
         return -1;
     }
 
     int last_state = gpio_pin_get_dt(&button);
 
     while (1) {
         int current = gpio_pin_get_dt(&button);
         if (current == 0 && last_state == 1) {
             gpio_pin_toggle_dt(&led);
 
             LOG_INF("Gerando fault proposital...");
             cause_fault();
         }
         last_state = current;
         k_msleep(20);
     }
 }
 