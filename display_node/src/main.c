#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "displayBluetooth.h"
#include <string.h>
/* scheduling parameters */
#define STACKSIZE               2048
#define PRIORITY 				7
#define PRIORITY_SENSOR			3

int main(void)
{
    printk("Display node started\n");
    return 0;
}
K_THREAD_DEFINE(bluetooth_receiver0_id, STACKSIZE, bluetooth_receiver0, NULL, NULL, NULL, PRIORITY, 0, 0);