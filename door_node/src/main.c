#include <zephyr/kernel.h>
#include <zephyr/device.h>
// #include "ultrasonicSensor.h"
// #include "bluetooth.h"
#include "pmodkypd.h"
/* scheduling parameters */
#define STACKSIZE 				1024
#define PRIORITY 				7
#define PRIORITY_SENSOR			3

// K_THREAD_DEFINE(UltrasonicSensorRead_id, STACKSIZE, UltrasonicSensorRead, NULL, NULL, NULL, PRIORITY, 0, 0);
// K_THREAD_DEFINE(UltrasonicSender_id, STACKSIZE, UltrasonicSender, NULL, NULL, NULL, PRIORITY_SENSOR, 0, 0);
K_THREAD_DEFINE(PmodKypdListener_id, STACKSIZE, PmodKypdListener, NULL, NULL, NULL, PRIORITY_SENSOR, 0, 0);