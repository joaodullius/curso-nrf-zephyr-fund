#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(prod_cons_msgq, LOG_LEVEL_INF);

K_MSGQ_DEFINE(data_q, sizeof(uint32_t), 10, 4);

void producer_thread(void)
{
    uint32_t data;

    while (1) {
        data = sys_rand32_get();
        LOG_INF("Produced data: %u", data);
        k_msgq_put(&data_q, &data, K_FOREVER);
        k_msleep(1000);
    }
}

void consumer_thread(void)
{
    uint32_t data;

    while (1) {
        k_msgq_get(&data_q, &data, K_FOREVER);
        LOG_INF("Consumed data: %u", data);
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(prod_tid, STACK_SIZE, producer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(cons_tid, STACK_SIZE, consumer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using k_msgq");
    return 0;
}
