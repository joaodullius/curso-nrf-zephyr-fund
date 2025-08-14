#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(bmp180_beacon, LOG_LEVEL_INF);

const struct device *bmp180 = DEVICE_DT_GET_ONE(bosch_bmp180);

#define MSGQ_SIZE 10
struct bmp180_msg {
    struct sensor_value temp;
};
K_MSGQ_DEFINE(bmp180_q, sizeof(struct bmp180_msg), MSGQ_SIZE, 4);

/* Manufacturer data: 0xFFFF ID + 2 bytes temperature */
static uint8_t mfg_data[4] = { 0xFF, 0xFF, 0x00, 0x00 };

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_NO_BREDR | BT_LE_AD_GENERAL)),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

static void sensor_timer_handler(struct k_timer *timer)
{
    struct bmp180_msg msg;

    if (sensor_sample_fetch(bmp180) == 0 &&
        sensor_channel_get(bmp180, SENSOR_CHAN_DIE_TEMP, &msg.temp) == 0) {
        k_msgq_put(&bmp180_q, &msg, K_NO_WAIT);
    }
}

K_TIMER_DEFINE(sensor_timer, sensor_timer_handler, NULL);

void adv_thread(void)
{
    struct bmp180_msg msg;
    int err;

    while (1) {
        k_msgq_get(&bmp180_q, &msg, K_FOREVER);

        int32_t temp_centi = (int32_t)(sensor_value_to_double(&msg.temp) * 100);
        sys_put_le16((uint16_t)temp_centi, &mfg_data[2]);

        err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Advertising data update failed (%d)", err);
        } else {
            LOG_INF("Temp: %d.%02d C", temp_centi / 100, temp_centi % 100);
        }
    }
}

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_DEFINE(adv_tid, STACK_SIZE, adv_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
    int err;

    LOG_INF("BMP180 beacon example");

    if (!device_is_ready(bmp180)) {
        LOG_ERR("BMP180 device not ready");
        return 0;
    }

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (%d)", err);
        return 0;
    }

    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (%d)", err);
        return 0;
    }

    k_timer_start(&sensor_timer, K_SECONDS(1), K_SECONDS(5));

    return 0;
}
