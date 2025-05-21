
#ifndef RXBLUETOOTH_H
#define RXBLUETOOTH_H

#include <zephyr/bluetooth/uuid.h>

extern struct bt_uuid_128 device_service_uuid;
extern struct bt_uuid_128 device_char_uuid;

void BluetoothAdvertiser(void);

#endif // RXBLUETOOTH_H
