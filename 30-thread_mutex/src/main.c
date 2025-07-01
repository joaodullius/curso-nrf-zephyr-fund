#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mutex_counter, LOG_LEVEL_INF);

static uint32_t counter;
K_MUTEX_DEFINE(counter_mutex);

void increment_thread(void)
{
    while (1) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        counter++;
        LOG_INF("Incremented counter: %u", counter);
        k_mutex_unlock(&counter_mutex);
        k_msleep(1000);
    }
}

void decrement_thread(void)
{
    while (1) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        counter--;
        LOG_INF("Decremented counter: %u", counter);
        k_mutex_unlock(&counter_mutex);
        k_msleep(1500);
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(inc_tid, STACK_SIZE, increment_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dec_tid, STACK_SIZE, decrement_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Counter protected with mutex");
    return 0;
}
