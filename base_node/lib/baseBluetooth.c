#include "baseBluetooth.h"
#include "rxBluetooth.h"
#include "txBluetooth.h"
#include <zephyr/kernel.h>
#include "servo.h"
#include <stdbool.h>
#include <stdlib.h>
#include "localVariables.h"

volatile int latest_distance_cm = -1;
volatile int latest_avg_value = -1;
volatile bool door_locked = -1;

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

        // Check for new message
        if (strncmp(current_msg, msg, sizeof(current_msg)) != 0) {
            strncpy(current_msg, msg, sizeof(current_msg) - 1);
            current_msg[sizeof(current_msg) - 1] = '\0';
            char type[16];
            char value;

            if (sscanf(current_msg, "%20[^,],%c", type, &value) == 2) {

                if (strcmp(type, "pin") == 0) {
                    const char *pin = &current_msg[4];
                    printk("pin: %s\n", pin);
                } else if (strcmp(type, "ultrasonic") == 0) {
                    if (value == '1') {
                        set_servo_locked(false);
                        door_locked = false;
                    } else if (value == '0') {
                        set_servo_locked(true);
                        door_locked = true;
                    } 
                } else if (strcmp(type, "magnetometer") == 0) {
                    if (value == '1') {
                    } else if (value == '0') {
                    } 
                } else if (strcmp(type, "ultrasonic_s") == 0) {
                    latest_distance_cm = atoi(&current_msg[13]);
                } else if (strcmp(type, "magnetometer_s") == 0) {
                    latest_avg_value = atoi(&current_msg[15]);
                } else {
                }
            } else {
            }
        }

        k_sleep(K_MSEC(50));
    }
}



