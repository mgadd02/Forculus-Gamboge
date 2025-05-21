
#include "rxBluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

static uint8_t data_buffer[20] = "Default msg";

static ssize_t read_handler(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, data_buffer, sizeof(data_buffer));
}

static ssize_t write_handler(struct bt_conn *conn, const struct bt_gatt_attr *attr,const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
        memcpy(data_buffer + offset, buf, len);
    printk("Received: %.*s\n", len, (char*)buf);
    return len;
}

BT_GATT_SERVICE_DEFINE(generic_svc,
    BT_GATT_PRIMARY_SERVICE(&device_service_uuid.uuid),

    BT_GATT_CHARACTERISTIC(&device_char_uuid.uuid,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
        read_handler, write_handler, NULL),
);

static void connected(struct bt_conn *conn, uint8_t err) {
    printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason %u)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

void BluetoothAdvertiser(void) {
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");


const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
};


    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed (err %d)\n", err);
    } else {
        printk("Advertising started\n");
    }
}
