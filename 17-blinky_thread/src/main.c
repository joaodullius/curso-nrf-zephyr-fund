#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

// Thread 1: pisca LED0 a cada 500 ms
void thread_led0(void)
{
    while (1) {
        gpio_pin_toggle_dt(&led0);
        k_msleep(500);
    }
}

// Thread 2: pisca LED1 a cada 1000 ms
void thread_led1(void)
{
    while (1) {
        gpio_pin_toggle_dt(&led1);
        k_msleep(1000);
    }
}

#define STACK_SIZE 512
#define PRIORITY 5

K_THREAD_DEFINE(led0_tid, STACK_SIZE, thread_led0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(led1_tid, STACK_SIZE, thread_led1, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    printk("Inicializando LEDs...\n");

    if (!device_is_ready(led0.port) || !device_is_ready(led1.port)) {
        printk("Erro: LED não está pronto\n");
        return -1;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);

    // Threads já estão definidas com K_THREAD_DEFINE e iniciam automaticamente

    return 0;
}
