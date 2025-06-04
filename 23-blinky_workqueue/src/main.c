/*
 * Copyright (c) 2022 Joao Dullius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(workqueue_sample, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void work_handler(struct k_work *work)
{
    gpio_pin_toggle_dt(&led);
    LOG_INF("LED toggled!");
}
K_WORK_DEFINE(my_work, work_handler);

void timer_handler(struct k_timer *timer_id)
{
    k_work_submit(&my_work);  // Envia para a System Workqueue
}
K_TIMER_DEFINE(my_timer, timer_handler, NULL);

int main(void)
{
    int ret;

    LOG_INF("Sample: System Workqueue com timer e LED");

    if (!device_is_ready(led.port)) {
        LOG_ERR("LED GPIO port %s is not ready", led.port->name);
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED pin");
        return -1;
    }

    k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(1));

    LOG_INF("Timer iniciado. LED vai piscar a cada 1s.");
}
