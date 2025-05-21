#include "doorBluetooth.h"
#include "txBluetooth.h"
#include <zephyr/kernel.h>

struct bt_uuid_128 device_service_uuid = BT_UUID_INIT_128(
    0xaa, 0xbb, 0xcc, 0xdd,
    0xee, 0xff,
    0x00, 0x11,
    0x22, 0x33,
    0x44, 0x55, 0x66, 0x77
);

struct bt_uuid_128 device_char_uuid = BT_UUID_INIT_128(
    0x12, 0x34, 0x56, 0x78,
    0x90, 0xab,
    0xcd, 0xef,
    0x12, 0x34,
    0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
);

char device_name[] = "base_node";

void bluetooth_sender0(void)
{
    bluetooth_scanner();


    while (!discovered_handle) {
    printk("Waiting for discovery...\n");
    k_sleep(K_SECONDS(5));
    }
    while (1)
    {
        send_msg("Hello from door_node");
        k_sleep(K_SECONDS(5));
    }
}

