#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/random.h>

LOG_MODULE_REGISTER(prod_cons_bad, LOG_LEVEL_INF);

static uint32_t shared_data;

void producer_thread(void)
{
    while (1) {
        shared_data = sys_rand32_get();
        LOG_INF("Produced data: %u", shared_data);
        k_msleep(1000);
    }
}

void consumer_thread(void)
{
    while (1) {
        LOG_INF("Consumed data: %u", shared_data);
        k_msleep(100);
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(prod_tid, STACK_SIZE, producer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(cons_tid, STACK_SIZE, consumer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using shared variable");
    return 0;
}
