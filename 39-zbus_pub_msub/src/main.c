#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(prod_cons_zbus, LOG_LEVEL_INF);

struct data_sample {
    uint32_t timestamp;
    uint32_t data;
};

/* Define a message subscriber */
ZBUS_MSG_SUBSCRIBER_DEFINE(cons_msub);

/* Define the channel with the subscriber */
ZBUS_CHAN_DEFINE(data_chan,
                 struct data_sample,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(cons_msub),
                 ZBUS_MSG_INIT(.timestamp = 0, .data = 0));

int timestamp_counter = 0;

void producer_thread(void)
{
    struct data_sample sample;

    while (1) {
        sample.data = sys_rand32_get();
        sample.timestamp = timestamp_counter++;

        zbus_chan_pub(&data_chan, &sample, K_NO_WAIT);
        LOG_INF("%d | Produced data: %u", sample.timestamp, sample.data);

        k_msleep(1000);
    }
}

void consumer_thread(void)
{
    const struct zbus_channel *chan;
    struct data_sample sample;

    LOG_INF("Consumer thread started, waiting for messages...");
    while (1) {
        zbus_sub_wait_msg(&cons_msub, &chan, &sample, K_FOREVER);
        if (&data_chan == chan) {
            LOG_INF("%u | Consumed data: %u", sample.timestamp, sample.data);
        }    
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(prod_tid, STACK_SIZE, producer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(cons_tid, STACK_SIZE, consumer_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using Zbus");
    return 0;
}
