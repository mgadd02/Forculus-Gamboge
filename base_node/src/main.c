#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "baseBluetooth.h"
#include "servo.h"
/* scheduling parameters */
#define STACKSIZE 				4096
#define PRIORITY 				7
#define PRIORITY_SENSOR			3

K_THREAD_DEFINE(bluetooth_receiver0_id, STACKSIZE, bluetooth_receiver0, NULL, NULL, NULL, PRIORITY, 0, 0);