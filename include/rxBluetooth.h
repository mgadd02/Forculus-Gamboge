
#ifndef RXBLUETOOTH_H
#define RXBLUETOOTH_H

#include <zephyr/bluetooth/uuid.h>


extern struct bt_uuid_128 rx_device_service_uuid;
extern struct bt_uuid_128 rx_device_char_uuid;
const char *get_received_data(void);
void bluetooth_advertiser(void);

#endif // RXBLUETOOTH_H
