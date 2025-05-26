#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "baseBluetooth.h"
#include "servo.h"
#include "CLIshell.h"
/* scheduling parameters */
#define STACKSIZE 				4096
#define PRIORITY 				7
#define PRIORITY_SENSOR			3

void main_task(void) {
    register_shell_commands();
}

K_THREAD_DEFINE(main_task_id, STACKSIZE, main_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(bluetooth_receiver0_id, STACKSIZE, bluetooth_receiver0, NULL, NULL, NULL, PRIORITY, 0, 0);