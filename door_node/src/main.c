#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "pmodkypd.h"
#include "doorBluetooth.h"
#include "ultrasonicSensor.h"
#include "lis3mdl.h"

/* scheduling parameters */
#define STACKSIZE 				4096
#define PRIORITY 				7
#define PRIORITY_SENSOR			3

K_THREAD_DEFINE(PmodKypdListener_id, STACKSIZE, PmodKypdListener, NULL, NULL, NULL, PRIORITY_SENSOR, 0, 0);
K_THREAD_DEFINE(UltrasonicSensorRead_id, STACKSIZE, UltrasonicSensorRead, NULL, NULL, NULL, PRIORITY_SENSOR, 0, 0);
K_THREAD_DEFINE(MagnetometerSensorRead_id, STACKSIZE, MagnetometerSensorRead, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(bluetooth_sender0_id, STACKSIZE, bluetooth_sender0, NULL, NULL, NULL, PRIORITY_SENSOR, 0, 0);