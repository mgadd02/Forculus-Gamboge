#include "displayBluetooth.h"
#include "rxBluetooth.h"
#include <zephyr/kernel.h>
#include <string.h>

struct bt_uuid_128 rx_device_service_uuid = BT_UUID_INIT_128(
    0xdd, 0xbb, 0xcc, 0xdd,
    0xee, 0xff,
    0x00, 0x11,
    0x22, 0x33,
    0x44, 0x55, 0x66, 0x77
);

struct bt_uuid_128 rx_device_char_uuid = BT_UUID_INIT_128(
    0xdd, 0x34, 0x56, 0x78,
    0x90, 0xab,
    0xcd, 0xef,
    0x12, 0x34,
    0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
);

void bluetooth_receiver0(void)
{
    printk("Display node thread started\n");

    bluetooth_advertiser();
    while (1)
    {
        k_sleep(K_SECONDS(5));
    }
}
