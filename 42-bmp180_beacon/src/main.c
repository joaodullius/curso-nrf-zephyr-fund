#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

LOG_MODULE_REGISTER(bmp180_beacon, LOG_LEVEL_INF);

const struct device *bmp180 = DEVICE_DT_GET_ONE(bosch_bmp180);

/* Manufacturer data: 0xFFFF ID + 2 bytes temperature */
static uint8_t mfg_data[4] = { 0xFF, 0xFF, 0x00, 0x00 };

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_NO_BREDR | BT_LE_AD_GENERAL)),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void sensor_work_handler(struct k_work *work)
{
    struct sensor_value temp;
    int err;

    LOG_INF("Fetching sensor data...");

    if (sensor_sample_fetch(bmp180) != 0) {
        LOG_ERR("sensor_sample_fetch failed");
        return;
    }
    if (sensor_channel_get(bmp180, SENSOR_CHAN_DIE_TEMP, &temp) != 0) {
        LOG_ERR("sensor_channel_get failed");
        return;
    }

    /* Converte sensor_value -> centi-°C (s16), truncando */
    int32_t centi = temp.val1 * 100 + temp.val2 / 10000;
    int16_t t_centi = (int16_t)centi;

    memcpy(&mfg_data[2], &t_centi, sizeof(t_centi));

    LOG_HEXDUMP_INF(mfg_data, sizeof(mfg_data), "Manufacturer Data");

    err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising data update failed (%d)", err);
    } else {
        LOG_INF("Updated Temp: %0.2f C in advertising data\n",
                sensor_value_to_double(&temp));
    }
}

K_WORK_DEFINE(sensor_work, sensor_work_handler);

static void sensor_timer_handler(struct k_timer *timer)
{
    k_work_submit(&sensor_work);
}

K_TIMER_DEFINE(sensor_timer, sensor_timer_handler, NULL);

static void bt_ready(int err)
{
    char addr_s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_t addr = {0};
    size_t count = 1;

    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    /* Start advertising */
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
                          sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    bt_id_get(&addr, &count);
    bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

    printk("Beacon started, advertising as %s\n", addr_s);
}

int main(void)
{
    int err;

    LOG_INF("BMP180 beacon example");

    if (!device_is_ready(bmp180)) {
        LOG_ERR("BMP180 device not ready");
        return 0;
    }

    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (%d)", err);
        return 0;
    }

    k_timer_start(&sensor_timer, K_SECONDS(1), K_SECONDS(5));

    return 0;
}
