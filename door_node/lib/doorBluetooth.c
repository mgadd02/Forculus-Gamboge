#include "doorBluetooth.h"
#include "txBluetooth.h"
#include "localVariables.h"
#include <zephyr/kernel.h>

struct bt_uuid_128 tx_device_service_uuid = BT_UUID_INIT_128(
    0xaa, 0xbb, 0xcc, 0xdd,
    0xee, 0xff,
    0x00, 0x11,
    0x22, 0x33,
    0x44, 0x55, 0x66, 0x77
);

struct bt_uuid_128 tx_device_char_uuid = BT_UUID_INIT_128(
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
        struct pmodkypd_data_t *pmodkypd_data = k_fifo_get(&PMODKYPD_fifo, K_NO_WAIT);
        if (pmodkypd_data != NULL) {
            char msg[32];
            snprintf(msg, sizeof(msg), "pin,%s", pmodkypd_data->pin_code);
            send_msg(msg);
            k_free(pmodkypd_data);
        }
        

        struct ultrasonic_data_t *ultrasonic_data = k_fifo_get(&ULTRASONIC_fifo, K_NO_WAIT);
        if (ultrasonic_data != NULL) {
            char msg[32];
            snprintf(msg, sizeof(msg), "ultrasonic,%c", ultrasonic_data->proximity ? '1' : '0');
            send_msg(msg);
            k_free(ultrasonic_data);
        }

        struct magnetometer_data_t *magnetometer_data = k_fifo_get(&MAGNETOMETER_fifo, K_NO_WAIT);
        if (magnetometer_data != NULL) {
            char msg[32];
            snprintf(msg, sizeof(msg), "magnetometer,%c", magnetometer_data->door_opened ? '1' : '0');
            send_msg(msg);
            k_free(magnetometer_data);
        }
        struct ultrasonic_sample_data_t *ultrasonic_sample_data = k_fifo_get(&ULTRASONIC_SAMPLE_fifo, K_NO_WAIT);
        if (ultrasonic_sample_data != NULL) {
            char msg[32];
            snprintf(msg, sizeof(msg), "ultrasonic_s,%d", ultrasonic_sample_data->distance_cm);
            send_msg(msg);
            k_free(ultrasonic_sample_data);
        }
        struct magnetometer_sample_data_t *magnetometer_sample_data = k_fifo_get(&MAGNETOMETER_SAMPLE_fifo, K_NO_WAIT);
        if (magnetometer_sample_data != NULL) {
            char msg[32];
            snprintf(msg, sizeof(msg), "magnetometer_s,%d", magnetometer_sample_data->avg_magnetometer_value);
            send_msg(msg);
            k_free(magnetometer_sample_data);
        }
        k_sleep(K_MSEC(50));
    }
}