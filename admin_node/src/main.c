// #include "mqtt_client.h"

// int main(void) {
// 	// Start the MQTT client
// 	if (start_mqtt_client() != 0) {
// 		return -1; // Error starting MQTT client
// 	}

// 	// Main loop can be added here if needed
// 	while (1) {
// 		// Process MQTT events or other tasks
// 		// k_sleep(K_SECONDS(1)); // Sleep to avoid busy-waiting
// 	}

// 	return 0; // Should never reach here
// }

#include "lvgl_display.h"
#include "mqtt_client.h"

#include <zephyr/kernel.h>

/* scheduling parameters */
#define STACKSIZE 				4096
#define SMALL_STACKSIZE 		2048

#define PRIORITY 				7

K_THREAD_DEFINE(lvgl_display, STACKSIZE, run_lvgl_display, NULL, NULL, NULL, PRIORITY, 0, 0);
// K_THREAD_DEFINE(bluetooth_sender0_id, STACKSIZE, bluetooth_sender0, NULL, NULL, NULL, PRIORITY, 0, 0);

K_THREAD_DEFINE(mqtt_client, SMALL_STACKSIZE, start_mqtt_client, NULL, NULL, NULL, PRIORITY, 0, 0);