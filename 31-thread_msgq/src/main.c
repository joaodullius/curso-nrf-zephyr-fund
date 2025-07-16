#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(prod_cons_msgq, LOG_LEVEL_INF);

struct data_sample {
    uint32_t data;
};

K_MSGQ_DEFINE(data_q, sizeof(struct data_sample), 10, 4);

void producer_thread(void)
{
    struct data_sample sample;

    while (1) {
        sample.data = sys_rand32_get();
        LOG_INF("Produced data: %u", sample.data);
        k_msgq_put(&data_q, &sample, K_FOREVER);
        k_msleep(1000);
    }
}

void consumer_thread(void)
{
    struct data_sample sample;

    while (1) {
        k_msgq_get(&data_q, &sample, K_FOREVER);
        LOG_INF("Consumed data: %u", sample.data);
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
