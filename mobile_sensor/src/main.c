/* main.c — Thingy:52 “Base Node” broadcaster scanner */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/scan.h>

#include <zephyr/net/buf.h>
#include <string.h>

LOG_MODULE_REGISTER(base_node, LOG_LEVEL_INF);

#define COMPANY_ID     0xFFFF
#define SENSOR_COUNT   6

/* buffer for latest floats */
static float latest[SENSOR_COUNT];
static bool  seen = false;

/* scan callback: look for manufacturer data == COMPANY_ID */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi,
                    uint8_t adv_type, struct net_buf_simple *buf)
{
    struct net_buf_simple_state state;
    net_buf_simple_save(buf, &state);

    while (buf->len > 1) {
        uint8_t field_len = net_buf_simple_pull_u8(buf);
        if (field_len < 1) {
            break;
        }
        uint8_t ad_type = net_buf_simple_pull_u8(buf);

        if (ad_type == BT_DATA_MANUFACTURER_DATA
            && field_len >= 2 + SENSOR_COUNT * sizeof(float)) {
            uint8_t *data = buf->data;
            uint16_t comp = data[0] | (data[1] << 8);
            if (comp == COMPANY_ID) {
                for (int i = 0; i < SENSOR_COUNT; i++) {
                    memcpy(&latest[i],
                           &data[2 + i * sizeof(float)],
                           sizeof(float));
                }
                seen = true;

                char addr_str[BT_ADDR_LE_STR_LEN];
                bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
                LOG_INF("Update from %s RSSI %d", addr_str, rssi);
                break;
            }
        }

        /* skip remainder of this AD field */
        net_buf_simple_pull(buf, field_len - 1);
    }

    net_buf_simple_restore(buf, &state);
}

/* scan parameters: passive, no duplicate filter */
static struct bt_le_scan_param scan_param = {
    .type     = BT_LE_SCAN_TYPE_PASSIVE,
    .options  = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window   = BT_GAP_SCAN_FAST_WINDOW,
};

/* shell command: get latest sensor values */
static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
    if (!seen) {
        shell_print(sh, "No broadcast received yet");
        return 0;
    }
    shell_print(sh, "HTS221: Temp = %.2f °C, Hum = %.2f%%",
                latest[0], latest[1]);
    shell_print(sh, "LPS22HB: Press = %.2f kPa, Temp = %.2f °C",
                latest[2], latest[3]);
    shell_print(sh, "CCS811: eCO₂ = %.0f ppm, eTVOC = %.0f ppb",
                latest[4], latest[5]);
    return 0;
}
SHELL_CMD_REGISTER(get, NULL,
    "Get last‐seen sensor broadcast",
    cmd_get);

/* shell command: have we seen any valid broadcast? */
static int cmd_seen(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Broadcast seen: %s", seen ? "YES" : "NO");
    return 0;
}
SHELL_CMD_REGISTER(seen, NULL,
    "Check if any valid broadcast has been received",
    cmd_seen);

void main(void)
{
    int err;

    LOG_INF("Starting Base Node");

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }
    LOG_INF("Bluetooth initialized");

    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err) {
        LOG_ERR("Scanning start failed (err %d)", err);
        return;
    }
    LOG_INF("Scanning started; waiting for broadcasts...");
}
