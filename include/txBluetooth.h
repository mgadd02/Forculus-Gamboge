
#ifndef TXBLUETOOTH_H
#define TXBLUETOOTH_H
#include <zephyr/bluetooth/uuid.h>

extern struct bt_uuid_128 device_service_uuid;
extern struct bt_uuid_128 device_char_uuid;
extern char device_name[];
extern uint16_t discovered_handle;


void bluetooth_scanner(void);
void send_msg(const char *msg);

#endif // TXBLUETOOTH_H