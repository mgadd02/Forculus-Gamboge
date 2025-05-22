#include "baseBluetooth.h"
#include "rxBluetooth.h"
#include "txBluetooth.h"
#include <zephyr/kernel.h>

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
    {
        const char *msg = get_received_data();

        if (strncmp(current_msg, msg, sizeof(current_msg)) != 0) {
            strncpy(current_msg, msg, sizeof(current_msg) - 1);
            current_msg[sizeof(current_msg) - 1] = '\0'; 
            printk("Received from received_data: %s\n", current_msg);
        }

        k_sleep(K_MSEC(10));
    }
}


void bluetooth_sender0(void){
    bluetooth_scanner();
    while (!discovery_ready || !discovered_handle) {
    printk("Waiting for discovery and pairing...\n");
    k_sleep(K_SECONDS(1));
}
    while (1)
    {
        send_msg("Hello from base_node");
        k_sleep(K_SECONDS(5));
    }
}
