#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(prod_cons_zbus, LOG_LEVEL_INF);

struct data_sample {
    uint32_t timestamp;
    uint32_t data;
};

static void consumer_callback(const struct zbus_channel *chan)
{
    const struct data_sample *sample;

    LOG_INF("Listener callback triggered!");
    
    /* Get direct pointer to channel message data */
    sample = zbus_chan_const_msg(chan);
    
    LOG_INF("%u | Consumed data: %u", sample->timestamp, sample->data);
}

ZBUS_LISTENER_DEFINE(consumer_lis, consumer_callback);

ZBUS_CHAN_DEFINE(data_chan,
                 struct data_sample,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(consumer_lis),
                 ZBUS_MSG_INIT(.timestamp = 0, .data = 0));

static void producer_thread(void)
{
    struct data_sample sample;
    uint32_t timestamp = 0;

    /* Wait for system to be ready */
    k_msleep(1000);

    while (1) {
        sample.timestamp = timestamp++;
        sample.data = sys_rand32_get();

        int ret = zbus_chan_pub(&data_chan, &sample, K_NO_WAIT);
        if (ret == 0) {
            LOG_INF("%u | Produced data: %u", sample.timestamp, sample.data);
        } else {
            LOG_ERR("Failed to publish data, ret: %d", ret);
        }

        k_msleep(1000);
    }
}

#define P_STACK_SIZE 1024
#define P_PRIORITY 5

K_THREAD_DEFINE(prod_tid, P_STACK_SIZE, producer_thread, NULL, NULL, NULL, P_PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Producer-Consumer example using Zbus with Listener");
    return 0;
}
