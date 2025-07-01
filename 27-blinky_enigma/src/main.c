#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

// Método 1: Thread dedicada com k_msleep()
void led0_thread(void)
{
    while (1) {
        gpio_pin_toggle_dt(&led0);
        k_msleep(1000); // bloqueia a thread por 1 segundo
    }
}

// Método 2: Uso de k_timer com callback
K_TIMER_DEFINE(led1_timer, NULL, NULL);

void led1_timer_handler(struct k_timer *timer)
{
    gpio_pin_toggle_dt(&led1); // executado no contexto do timer
}

// Método 3: Uso de k_work_delayable com re-agendamento
static struct k_work_delayable led2_work;

void led2_work_handler(struct k_work *work)
{
    gpio_pin_toggle_dt(&led2); // executado no contexto da system workqueue
    k_work_schedule(&led2_work, K_MSEC(1000)); // reagenda o trabalho para daqui a 1s
}

// Thread para o LED0
#define STACK_SIZE 512
#define PRIORITY 5
K_THREAD_DEFINE(led0_tid, STACK_SIZE, led0_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    printk("Enigma dos 3 LEDs!\n\n");

    printk("LED0: usa k_msleep() em thread dedicada\n");
    printk("LED1: usa k_timer com callback\n");
    printk("LED2: usa k_work_delayable com re-agendamento\n\n");

    if (!device_is_ready(led0.port) || !device_is_ready(led1.port) || !device_is_ready(led2.port)) {
        printk("Erro: algum LED não está pronto!\n");
        return -1;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);

    // Inicia timer do LED1
    k_timer_init(&led1_timer, led1_timer_handler, NULL);
    k_timer_start(&led1_timer, K_NO_WAIT, K_MSEC(1000));

    // Inicia work do LED2
    k_work_init_delayable(&led2_work, led2_work_handler);
    k_work_schedule(&led2_work, K_NO_WAIT); // inicia imediatamente

    // Mantém main vivo (por clareza didática)
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
