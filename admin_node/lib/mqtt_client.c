#include <esp32_wifi_connect.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

#include "mqtt_client.h"

LOG_MODULE_REGISTER(mqtt_sub, LOG_LEVEL_INF);

int start_mqtt_client(void);

#define RX_BUFFER_SIZE 512
#define TX_BUFFER_SIZE 256

static struct mqtt_client client;
static struct sockaddr_storage broker;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t tx_buffer[TX_BUFFER_SIZE];
static struct k_sem wifi_connected;

static bool mqtt_connected = false;
static bool subscribed = false;

static struct pollfd fds[1];
static int nfds;

K_FIFO_DEFINE(mqtt_lvgl_fifo);

void parse_bracketed_pairs(const char *input, mqtt_lvgl_data_t *data) {
    const char *p = input;
    char key[32], value[32];

    while ((p = strchr(p, '[')) != NULL) {
        p++;  // move past '['

        // Find closing bracket
        const char *end = strchr(p, ']');
        if (!end) break;

        // Copy content between [ and ]
        size_t len = end - p;
        char pair[64];
        if (len >= sizeof(pair)) len = sizeof(pair) - 1;
        strncpy(pair, p, len);
        pair[len] = '\0';

        // Split key and value by comma
        char *comma = strchr(pair, ',');
        if (!comma) break;

        *comma = '\0';
        strncpy(key, pair, sizeof(key));
        strncpy(value, comma + 1, sizeof(value));

        // Assign to struct
        if (strcmp(key, "open") == 0)
            data->open = atoi(value);
        else if (strcmp(key, "locked") == 0)
            data->locked = atoi(value);
        else if (strcmp(key, "motion") == 0)
            data->motion_detected = atoi(value);
        else if (strcmp(key, "face") == 0)
            data->face_validated = atoi(value);
        else if (strcmp(key, "pin") == 0)
            data->pin_validated = atoi(value);
        else if (strcmp(key, "attempt") == 0)
            data->new_attempt = atoi(value);
        else if (strcmp(key, "face_name") == 0)
            strncpy(data->face_name, value, sizeof(data->face_name) - 1);
        else if (strcmp(key, "temp") == 0)
            strncpy(data->temperature, value, sizeof(data->temperature) - 1);
        else if (strcmp(key, "humidity") == 0)
            strncpy(data->humidity, value, sizeof(data->humidity) - 1);
        else if (strcmp(key, "air") == 0)
            strncpy(data->air_quality, value, sizeof(data->air_quality) - 1);

        p = end + 1;  // continue after ']'
    }
}

void parse_bracketed_triples(const char *input, mqtt_lvgl_data_t *data) {
    const char *p = input;
    char key[32], value[32];

    float eco2 = -1.0f;
    float etvoc = -1.0f;
    bool face_name_received = false;

    while ((p = strchr(p, '[')) != NULL) {
        p++;  // move past '['

        const char *end = strchr(p, ']');
        if (!end) break;

        size_t len = end - p;
        char triple[96];
        if (len >= sizeof(triple)) len = sizeof(triple) - 1;
        strncpy(triple, p, len);
        triple[len] = '\0';

        // Split into parts: device, key, value
        char *device = strtok(triple, ",");
        char *k = strtok(NULL, ",");
        char *v = strtok(NULL, ",");

        if (!device || !k) {
            p = end + 1;
            continue;
        }

        strncpy(key, k, sizeof(key));

        // Assign default or actual value
        if (v && *v != '\0') {
            strncpy(value, v, sizeof(value));
        // } else if (strcmp(key, "person") == 0) {

            // strncpy(value, "unknown", sizeof(value));
        } else {
            p = end + 1;
            continue;
        }

        // Only handle fields in mqtt_lvgl_data_t
        if (strcmp(key, "door_state") == 0) {
            data->locked = strcmp(value, "locked") == 0;
            data->open = !data->locked;
            data->pin_validated = !data->locked;
        } else if (strcmp(key, "person_present") == 0) {
            data->motion_detected = (strcmp(value, "1") == 0);
        } else if (strcmp(key, "attempt") == 0) {
            data->new_attempt = atoi(value);
        } else if (strcmp(key, "person") == 0) {
            strncpy(data->face_name, value, sizeof(data->face_name) - 1);
            data->face_name[sizeof(data->face_name) - 1] = '\0';
            face_name_received = true;
        } else if (strcmp(key, "Temp") == 0) {
            strncpy(data->temperature, value, sizeof(data->temperature) - 1);
            data->temperature[sizeof(data->temperature) - 1] = '\0';
		} else if (strcmp(key, "Hum") == 0) {
            strncpy(data->humidity, value, sizeof(data->humidity) - 1);
            data->humidity[sizeof(data->humidity) - 1] = '\0';
        } else if (strcmp(key, "eCO2") == 0) {
            eco2 = atof(value);
        } else if (strcmp(key, "eTVOC") == 0) {
            etvoc = atof(value);
        }

        p = end + 1;
    }

    // Set face validation status
    if (face_name_received && strcmp(data->face_name, "Unknown") != 0) {
        data->face_validated = true;
    } else {
        data->face_validated = false;
    }

    // Determine air quality from eCO2 and eTVOC
    if (eco2 > 0 && etvoc > 0) {
        if (eco2 < 800 && etvoc < 100)
            strncpy(data->air_quality, "Good", sizeof(data->air_quality));
        else if (eco2 < 1200 && etvoc < 300)
            strncpy(data->air_quality, "Moderate", sizeof(data->air_quality));
        else
            strncpy(data->air_quality, "Poor", sizeof(data->air_quality));
    } else {
        strncpy(data->air_quality, "Unknown", sizeof(data->air_quality));
    }
}

void send_mqtt_data(char *input) {
    // if (!data) {
    //     // handle allocation failure
    //     return;
    // }
	mqtt_lvgl_data_t *data = k_malloc(sizeof(mqtt_lvgl_data_t));
	if (!data) {
		LOG_ERR("Failed to allocate memory for MQTT LVGL data");
		return;
	}
	printk("HEREA");
	parse_bracketed_triples(input, data);
	printk("HEREB");


    k_fifo_put(&mqtt_lvgl_fifo, data);
	printk("HEREC");

}

/**
 * Prepare the file descriptor list for polling.
 * This is called before polling to ensure the correct file descriptors are set.
 * It sets up the MQTT client's TCP socket for reading.
 */
static void prepare_fds(struct mqtt_client *client) {

	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}

	fds[0].events = POLLIN;
	nfds = 1;
}

/**
 * Clear the file descriptor list.
 * This is called when the MQTT client disconnects to reset the state.
 */
static void clear_fds(void) {
	nfds = 0;
}

/**
 * MQTT event handler.
 * This function handles various MQTT events such as connection, disconnection, and message reception.
 */
static void mqtt_evt_handler(struct mqtt_client *const c, const struct mqtt_evt *evt) {
	switch (evt->type) {

	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
            LOG_INF("MQTT connect failed %d\n", evt->result);
            break;
        }
        LOG_INF("MQTT Connected");
        mqtt_connected = true;
        break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT disconnected");
		printk("MQTT client disconnected %d\n", evt->result);
        mqtt_connected = false;
		clear_fds();
		break;

	case MQTT_EVT_PUBLISH: {
	    const struct mqtt_publish_param *p = &evt->param.publish;
    	char buf[RX_BUFFER_SIZE];

    	printk("MQTT_EVT_PUBLISH received\n");
    	printk("Topic: %.*s\n", p->message.topic.topic.size, p->message.topic.topic.utf8);
    	printk("Payload length: %d\n", p->message.payload.len);

    	if (p->message.payload.len > 0) {
        	int rc = mqtt_read_publish_payload(&client, buf, MIN(p->message.payload.len, sizeof(buf) - 1));
        		if (rc < 0) {
            	printk("ERROR: Failed to read publish payload [%d]\n", rc);
            	break;
        	}
			
        	buf[MIN(p->message.payload.len, sizeof(buf) - 1)] = '\0';
        	// printk("Payload: %s\n", buf);
			printk("NEW PAYLOAD");
			send_mqtt_data(buf);  // Process the payload data

    	} else {
        	printk("Payload length is zero.\n");
    	}

    	break;
	}
	default:
		LOG_DBG("Unhandled MQTT event: %d", evt->type);
		break;
	}
}

/**
 * Initialize the MQTT client and set up the broker address.
 * This function resolves the broker address and prepares the MQTT client structure.
 * It also sets the client ID, username, and password if provided.
 */
static int mqtt_client_broker_init(void) {

	struct addrinfo hints = {
    	.ai_family = AF_INET,
    	.ai_socktype = SOCK_STREAM,
	};

	struct addrinfo *res;
	int err = getaddrinfo(MQTT_BROKER_ADDR, NULL, &hints, &res);
	if (err != 0 || res == NULL) {
    	LOG_ERR("Failed to resolve broker address");
    	return -1;
	}

	// Fill broker sockaddr
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
	memcpy(broker4, res->ai_addr, sizeof(struct sockaddr_in));
	broker4->sin_port = htons(MQTT_BROKER_PORT);  // Ensure correct port
	freeaddrinfo(res);  // Free the DNS result

	// Set up MQTT client parameters
	struct mqtt_utf8 client_id = {
		.utf8 = (uint8_t *)CLIENT_ID,
		.size = strlen(CLIENT_ID),
	};

	struct mqtt_utf8 username = {
		.utf8 = (uint8_t *)MQTT_USERNAME,
		.size = MQTT_USERNAME != NULL ? strlen(MQTT_USERNAME) : 0,  // Handle NULL username
	};

	struct mqtt_utf8 password = {
		.utf8 = (uint8_t *)MQTT_PASSWORD,
		.size = MQTT_PASSWORD != NULL ? strlen(MQTT_PASSWORD) : 0,  // Handle NULL password
	};

	memset(&client, 0, sizeof(client));
	mqtt_client_init(&client);

	client.broker = &broker;
	client.evt_cb = mqtt_evt_handler;
	client.client_id = client_id;
	client.user_name = &username;
	client.password = &password;
	client.protocol_version = MQTT_VERSION_3_1_1;
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

	return 0;
}

/**
 * Subscribe to a specific MQTT topic.
 * This function creates a subscription list and subscribes to the specified topic with QoS 1.
 * It returns 0 on success or an error code on failure.
 */
int mqtt_subscribe_to_topic(void) {

	// Subscribe to the desired topic
	struct mqtt_topic topic = {
		.topic = {
			.utf8 = (uint8_t *)MQTT_TOPIC,
			.size = strlen(MQTT_TOPIC),
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
	};

	const struct mqtt_subscription_list sub_list = {
		.list = &topic,
		.list_count = 1,
		.message_id = 1,
	};

	int rc = mqtt_subscribe(&client, &sub_list);

	if (rc != 0) {
		LOG_ERR("mqtt_subscribe failed: %d", rc);
		return rc;
	}

	return 0;
}

/* wifi event callback to set connection semaphore */
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface) {
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		LOG_INF("Wi-Fi connected with IP");
		k_sem_give(&wifi_connected);
	}
}

/**
 * Poll the MQTT socket for incoming data.
 * This function prepares the file descriptors and polls the MQTT socket for events.
 * It returns the number of events or an error code.
 */
static int poll_mqtt_socket(int timeout)
{
	int rc;

	prepare_fds(&client);

	if (nfds <= 0) {
		return -EINVAL;
	}

	rc = zsock_poll(fds, nfds, timeout);
	if (rc < 0) {
		LOG_ERR("Socket poll error [%d]", rc);
	}

	return rc;
}

/**
 * Connect to the MQTT broker.
 * This function initializes the MQTT client and connects to the broker.
 * It blocks until the MQTT connection is established or fails.
 * Returns 0 on success or an error code on failure.
 */
int app_mqtt_connect(void)
{
	int rc = 0;

	mqtt_connected = false;

	/* Block until MQTT CONNACK event callback occurs */
	while (!mqtt_connected) {

		mqtt_client_broker_init();
		/* Initialize MQTT client */

		rc = mqtt_connect(&client);
		if (rc != 0) {
			LOG_ERR("MQTT Connect failed [%d]", rc);
			k_msleep(MSECS_WAIT_RECONNECT);
			continue;
		}

		/* Poll MQTT socket for response */
		rc = poll_mqtt_socket(MSECS_NET_POLL_TIMEOUT);
		if (rc > 0) {
			mqtt_input(&client);
		}

		if (!mqtt_connected) {
			mqtt_abort(&client);
		}
	}

	if (!mqtt_connected) {
		return -EINVAL;
	}

	return 0;
}

/**
 * Process MQTT events and keep the connection alive.
 * This function checks for incoming MQTT data and processes it.
 * It also handles the MQTT keepalive mechanism.
 * Returns 0 on success or an error code on failure.
 */
int app_mqtt_process(void)
{
	int rc;

	rc = poll_mqtt_socket(mqtt_keepalive_time_left(&client));
	if (rc != 0) {
		if (fds[0].revents & ZSOCK_POLLIN) {
			/* MQTT data received */
			rc = mqtt_input(&client);

			if (mqtt_input(&client) == -EBUSY) {
    			LOG_WRN("MQTT client busy, skipping input");
    			return 0;  // Avoid treating it as fatal
			}	

			if (rc != 0) {
				LOG_ERR("MQTT Input failed [%d]", rc);
				return rc;
			}
			/* Socket error */
			if (fds[0].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
				LOG_ERR("MQTT socket closed / error");
				return -ENOTCONN;
			}
		}
	} else {
		/* Socket poll timed out, time to call mqtt_live() */
		rc = mqtt_live(&client);
		if (rc != 0) {
			LOG_ERR("MQTT Live failed [%d]", rc);
			return rc;
		}
	}

	return 0;
}

/**
 * Main function to initialize the Wi-Fi connection and MQTT client.
 * It waits for the Wi-Fi connection to be established, connects to the MQTT broker,
 * subscribes to a topic, and processes incoming MQTT messages.
 */
int start_mqtt_client(void) {

	k_sem_init(&wifi_connected, 0, 1);

	static struct net_mgmt_event_callback wifi_cb;
	net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&wifi_cb);

	start_wifi();  // Provided by esp32_wifi_connect.h

	LOG_INF("Waiting for Wi-Fi...");
	if (k_sem_take(&wifi_connected, K_SECONDS(15)) != 0) {
		LOG_ERR("Wi-Fi connection timed out");
		return -1;
	}
	// Retry to connect to Wi-Fi if needed?

	LOG_INF("Connecting to MQTT broker...");
	if (app_mqtt_connect() != 0) {
		LOG_ERR("Failed to connect to MQTT");
		return -1;
	}

	LOG_INF("Subscribing to MQTT topic...");
	if (mqtt_subscribe_to_topic() != 0) {
		LOG_ERR("Failed to subscribe to MQTT topic");
		return -1;
	}
	LOG_INF("Subscribed to topic %s", MQTT_TOPIC);
	int rc;

	while (mqtt_connected) {
		rc = app_mqtt_process();
		if (rc != 0) {
			break;
		}
		k_msleep(100);  // Adjust as needed for your application

		// how to publish...
	}
	mqtt_disconnect(&client);

	return 0;
}


