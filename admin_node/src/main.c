#include "mqtt_client.h"

int main(void) {
	// Start the MQTT client
	if (start_mqtt_client() != 0) {
		return -1; // Error starting MQTT client
	}

	// Main loop can be added here if needed
	while (1) {
		// Process MQTT events or other tasks
		// k_sleep(K_SECONDS(1)); // Sleep to avoid busy-waiting
	}

	return 0; // Should never reach here
}