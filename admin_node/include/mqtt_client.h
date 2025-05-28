#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H


/** MQTT connection timeouts */
#define MSECS_NET_POLL_TIMEOUT	5000
#define MSECS_WAIT_RECONNECT	1000

#define MQTT_TOPIC "topic/test/esp32_sub"
#define CLIENT_ID "esp32_sub"

// #define MQTT_BROKER_ADDR "f466895c2a554c708ecd3d675b4db272.s1.eu.hivemq.cloud"
// #define MQTT_BROKER_PORT 8883

// #define MQTT_USERNAME "slarm"
// #define MQTT_PASSWORD "Slarmiscool1"

#define MQTT_BROKER_ADDR "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883

#define MQTT_USERNAME NULL
#define MQTT_PASSWORD NULL

extern int start_mqtt_client(void);

#endif // MQTT_CONFIG_H