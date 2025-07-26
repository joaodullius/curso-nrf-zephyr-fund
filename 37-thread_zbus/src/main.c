#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(prod_cons_zbus, LOG_LEVEL_INF);

struct data_sample {
    uint32_t timestamp;
    uint32_t data;
};

ZBUS_LISTENER_DECLARE(consumer_lis);

ZBUS_CHAN_DEFINE(data_chan,
                 struct data_sample,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(consumer_lis),
                 ZBUS_MSG_INIT(0, 0));

static void consumer_callback(const struct zbus_channel *chan)
{
    struct data_sample sample;

    if (zbus_chan_read(chan, &sample, K_NO_WAIT) == 0) {
        LOG_INF("%u | Consumed data: %u", sample.timestamp, sample.data);
    }
}

ZBUS_LISTENER_DEFINE(consumer_lis, 4, consumer_callback);
ZBUS_LISTENER_THREAD_DEFINE(consumer_lis, 1024, 5);

static void producer_thread(void)
{
    struct data_sample sample;
    uint32_t timestamp = 0;

    while (1) {
        sample.timestamp = timestamp++;
        sample.data = sys_rand32_get();

        zbus_chan_pub(&data_chan, &sample, K_NO_WAIT);
        LOG_INF("%u | Produced data: %u", sample.timestamp, sample.data);

        k_msleep(1000);
    }
}

#define P_STACK_SIZE 1024
#define P_PRIORITY 5

K_THREAD_DEFINE(prod_tid, P_STACK_SIZE, producer_thread, NULL, NULL, NULL, P_PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using Zbus");
    return 0;
}
