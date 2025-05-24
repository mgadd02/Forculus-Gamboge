#include "baseBluetooth.h"
#include "rxBluetooth.h"
#include "txBluetooth.h"
#include <zephyr/kernel.h>

struct relay_msg_t {
    char payload[21];
};

K_FIFO_DEFINE(relay_fifo);

struct bt_uuid_128 rx_device_service_uuid = BT_UUID_INIT_128(
    0xaa, 0xbb, 0xcc, 0xdd,
    0xee, 0xff,
    0x00, 0x11,
    0x22, 0x33,
    0x44, 0x55, 0x66, 0x77
);

struct bt_uuid_128 rx_device_char_uuid = BT_UUID_INIT_128(
    0x12, 0x34, 0x56, 0x78,
    0x90, 0xab,
    0xcd, 0xef,
    0x12, 0x34,
    0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
);

struct bt_uuid_128 tx_device_service_uuid = BT_UUID_INIT_128(
    0xdd, 0xbb, 0xcc, 0xdd,
    0xee, 0xff,
    0x00, 0x11,
    0x22, 0x33,
    0x44, 0x55, 0x66, 0x77
);

struct bt_uuid_128 tx_device_char_uuid = BT_UUID_INIT_128(
    0xdd, 0x34, 0x56, 0x78,
    0x90, 0xab,
    0xcd, 0xef,
    0x12, 0x34,
    0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
);

char device_name[] = "display_node";

void bluetooth_receiver0(void)
{
    bluetooth_advertiser();
    char current_msg[21] = {0};

    while (1) {
        const char *msg = get_received_data();

        if (strncmp(current_msg, msg, sizeof(current_msg)) != 0) {
            strncpy(current_msg, msg, sizeof(current_msg) - 1);
            current_msg[sizeof(current_msg) - 1] = '\0';
            printk("Received from received_data: %s\n", current_msg);

            // Check for "pin" prefix
            if (strncmp(current_msg, "pin,", 4) == 0 && current_msg[4] != '\0') {
                printk("Receiver pin: %s\n", &current_msg[4]);
            }
            // Check for "ultrasonic" prefix
            else if (strncmp(current_msg, "ultrasonic,", 11) == 0 && current_msg[11] != '\0') {
                if (current_msg[11] == '1') {
                    printk("Someone is in proximity\n");
                } else if (current_msg[11] == '0') {
                    printk("Someone left proximity\n");
                } else {
                    printk("Unknown ultrasonic state: %c\n", current_msg[11]);
                }
            }
        }

        k_sleep(K_MSEC(10));
    }
}



