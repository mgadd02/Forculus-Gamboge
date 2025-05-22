#include "txBluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <string.h>

static struct bt_conn *default_conn;
uint16_t discovered_handle = 0;

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_write_params write_params;
static uint16_t svc_start_handle = 0, svc_end_handle = 0;


static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params) {
    if (err) {
        printk("Write failed: 0x%02x\n", err);
    } else {
        printk("Write successful\n");
    }
}

void send_msg(const char *msg) {
    if (!discovered_handle) {
        printk("Characteristic handle not discovered yet.\n");
        return;
    }

    write_params.handle = discovered_handle;
    write_params.offset = 0;
    write_params.data = msg;
    write_params.length = strlen(msg);
    write_params.func = write_cb;

    printk("Sending '%s' to handle 0x%04x (len %d)\n", msg, discovered_handle, write_params.length);

    int err = bt_gatt_write(default_conn, &write_params);
    if (err) {
        printk("bt_gatt_write failed (err %d)\n", err);
    }
}
static bool adv_data_has_name(struct net_buf_simple *ad, const char *target_name) {
    while (ad->len > 1) {
        uint8_t len = net_buf_simple_pull_u8(ad);
        if (len == 0 || len > ad->len) break;

        uint8_t type = net_buf_simple_pull_u8(ad);
        if ((type == BT_DATA_NAME_COMPLETE || type == BT_DATA_NAME_SHORTENED) &&
            len - 1 == strlen(target_name) &&
            memcmp(ad->data, target_name, len - 1) == 0) {
            printk("Found match: %.*s\n", len - 1, ad->data);

            return true;
        }

        net_buf_simple_pull(ad, len - 1);
    }
    return false;
}
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad) {
    if (adv_data_has_name(ad, device_name)) {
        char addr_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
        printk("Found node: %s (RSSI %d)\n", addr_str, rssi);

        bt_le_scan_stop();
        struct bt_le_conn_param *param = BT_LE_CONN_PARAM_DEFAULT;
        int err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
        if (err) {
            printk("Connection failed (err %d)\n", err);
        }
    }
}

static void auth_cancel(struct bt_conn *conn)
{
    printk("Pairing cancelled\n");
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
    printk("Pairing confirmation\n");
    bt_conn_auth_pairing_confirm(conn);
}

static struct bt_conn_auth_cb auth_cb = {
    .cancel = auth_cancel,
    .pairing_confirm = auth_pairing_confirm,
};

static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    default_conn = bt_conn_ref(conn);  // ✅ Properly store connection
    printk("Connected\n");

    int auth_err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (auth_err) {
        printk("Failed to set security: %d\n", auth_err);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason %u)\n", reason);
    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static uint8_t discover_char_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params) {
    if (!attr) {
        printk("Characteristic discovery complete\n");
        return BT_GATT_ITER_STOP;
    }

    const struct bt_gatt_chrc *chrc = attr->user_data;
    if (bt_uuid_cmp(chrc->uuid, &tx_device_char_uuid.uuid) == 0) {
        discovered_handle = chrc->value_handle;
        printk("Discovered characteristic handle: 0x%04x\n", discovered_handle);
    }

    return BT_GATT_ITER_CONTINUE;
}



static uint8_t discover_service_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params) {
    if (!attr) {
        printk("Service discovery complete\n");
        if (svc_start_handle && svc_end_handle) {
            discover_params.uuid = &tx_device_char_uuid.uuid;
            discover_params.func = discover_char_func;
            discover_params.start_handle = svc_start_handle;
            discover_params.end_handle = svc_end_handle;
            discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

            int err = bt_gatt_discover(conn, &discover_params);
            if (err) {
                printk("Characteristic discovery failed (err %d)\n", err);
            }
        }
        return BT_GATT_ITER_STOP;
    }

    const struct bt_gatt_service_val *service = attr->user_data;
    svc_start_handle = attr->handle + 1;
    svc_end_handle = service->end_handle;

    printk("Found service from 0x%04x to 0x%04x\n", svc_start_handle, svc_end_handle);

    return BT_GATT_ITER_CONTINUE;
}


void discover_device_char(void) {
    memset(&discover_params, 0, sizeof(discover_params));
    discover_params.uuid = &tx_device_service_uuid.uuid;
    discover_params.func = discover_service_func;
    discover_params.start_handle = 0x0001;
    discover_params.end_handle = 0xffff;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    int err = bt_gatt_discover(default_conn, &discover_params);
    if (err) {
        printk("Service discovery failed (err %d)\n", err);
    }
}

volatile bool discovery_ready = false;

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    printk("Pairing complete, bonded: %d\n", bonded);
    discover_device_char();
    discovery_ready = true;  // ✅ Set flag when discovery is initiated
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    printk("Pairing failed (reason %d)\n", reason);
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static struct bt_conn_auth_info_cb auth_info_cb = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed,
};


void bluetooth_scanner(void) {
    int err = bt_enable(NULL);
    if (err && err != -120) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");
    bt_conn_auth_cb_register(&auth_cb); 
    bt_conn_auth_info_cb_register(&auth_info_cb);

    struct bt_le_scan_param scan_param = {
        .type = BT_HCI_LE_SCAN_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = 0x0010,
        .window = 0x0010,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        printk("Scan start failed (err %d)\n", err);
    } else {
        printk("Scanning...\n");
    }
}

