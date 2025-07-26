#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(prod_cons_fifo, LOG_LEVEL_INF);

struct data_sample {
    uint32_t timestamp;
    uint32_t data;
};

K_FIFO_DEFINE(data_fifo);

int timestamp_counter = 0;

void producer_thread(void)
{
    while (1) {
        struct data_sample *sample = k_malloc(sizeof(struct data_sample));
        /* Comment out the line above to see the wrong way of using k_fifo */
        if (!sample) {
            LOG_ERR("Failed to allocate sample");
            k_sleep(K_MSEC(200));
            continue;
        }

        sample->data = sys_rand32_get();
        sample->timestamp = timestamp_counter++;
        LOG_INF("Produced data: %u", sample->data);
        k_fifo_put(&data_fifo, sample);

        k_msleep(200); /* Producer runs faster */
    }
}

void consumer_thread(void)
{
    while (1) {
        for (int i = 0; i < 5; i++) {
            struct data_sample *sample = k_fifo_get(&data_fifo, K_FOREVER);
            LOG_INF("%d | Consumed data: %u", sample->timestamp, sample->data);
            k_free(sample);
            /* Comment out the line above to see the wrong way of using k_fifo */
        }
        k_msleep(1000); /* Consumer is slower */
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(prod_tid, STACK_SIZE, producer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(cons_tid, STACK_SIZE, consumer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using k_fifo");

    return 0;
}
