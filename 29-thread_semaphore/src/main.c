#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(prod_cons_sem, LOG_LEVEL_INF);

static uint32_t shared_data;
K_SEM_DEFINE(data_sem, 0, 1);

void producer_thread(void)
{
    while (1) {
        shared_data = sys_rand32_get();
        LOG_INF("Produced data: %u", shared_data);
        k_sem_give(&data_sem);
        k_msleep(1000);
    }
}

void consumer_thread(void)
{
    while (1) {
        k_sem_take(&data_sem, K_FOREVER);
        LOG_INF("Consumed data: %u", shared_data);
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(prod_tid, STACK_SIZE, producer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(cons_tid, STACK_SIZE, consumer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using semaphore");
    return 0;
}
