#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

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


void work_handler_led2(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(my_work_led2, work_handler_led2);

void work_handler_led2(struct k_work *work)
{
    k_work_schedule(&my_work_led2, K_MSEC(250));
    gpio_pin_toggle_dt(&led2);
}

void work_handler_led3(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(my_work_led3, work_handler_led3);

void work_handler_led3(struct k_work *work)
{
    k_work_schedule(&my_work_led3, K_MSEC(750));
    gpio_pin_toggle_dt(&led3);
}


int main(void)
{
    printk("Inicializando LEDs...\n");

    if (!device_is_ready(led0.port) || !device_is_ready(led1.port)
        || !device_is_ready(led2.port) || !device_is_ready(led3.port)) {
        printk("Erro: LED não está pronto\n");
        return -1;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

    // Threads já estão definidas com K_THREAD_DEFINE e iniciam automaticamente

    k_work_schedule(&my_work_led2, K_MSEC(250)); 
    k_work_schedule(&my_work_led3, K_MSEC(750)); 


    return 0;
}
