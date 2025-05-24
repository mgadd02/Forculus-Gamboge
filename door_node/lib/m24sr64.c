#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#define I2C_DEV_NAME "I2C_2"
#define M24SR64_ADDR 0x56  // or 0x57 depending on GPO

static const struct device *i2c_dev = NULL;

#define RF_DISABLE_NODE DT_ALIAS(nfcreaderrfdisable)
static const struct gpio_dt_spec rf_disable = GPIO_DT_SPEC_GET(RF_DISABLE_NODE, gpios);

uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
        }
    }
    return ~crc;
}

int send_apdu(const uint8_t *apdu, size_t apdu_len, uint8_t *resp, size_t resp_len) {
    uint8_t tx_buf[64];
    if (!i2c_dev) return -ENODEV;

    tx_buf[0] = 0xA2;  // PCB
    memcpy(&tx_buf[1], apdu, apdu_len);

    uint16_t crc = crc16_ccitt(&tx_buf[1], apdu_len);
    tx_buf[apdu_len + 1] = crc & 0xFF;
    tx_buf[apdu_len + 2] = (crc >> 8) & 0xFF;

    int ret = i2c_write(i2c_dev, tx_buf, apdu_len + 3, M24SR64_ADDR);
    if (ret) return ret;

    int tries = 0;
    uint8_t dummy;
    do {
        ret = i2c_read(i2c_dev, &dummy, 1, M24SR64_ADDR);
        k_msleep(5);
    } while (ret && ++tries < 50);

    if (ret) return -ETIMEDOUT;
    return i2c_read(i2c_dev, resp, resp_len, M24SR64_ADDR);
}

void NFCRead(void) {
    uint8_t response[64];

    uint8_t get_session_cmd[] = {0x26};
    if (i2c_write(i2c_dev, get_session_cmd, sizeof(get_session_cmd), M24SR64_ADDR) != 0) {
        printk("GetSession write failed\n");
        return;
    }

    int ret, tries = 0;
    uint8_t dummy;
    do {
        ret = i2c_read(i2c_dev, &dummy, 1, M24SR64_ADDR);
        k_msleep(5);
    } while (ret && ++tries < 50);
    if (ret) {
        printk("GetSession timed out\n");
        return;
    } else {
        printk("GetSession succeeded\n");
    }

    uint8_t select_app[] = {
        0x00, 0xA4, 0x04, 0x00, 0x07,
        0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01
    };
    if (send_apdu(select_app, sizeof(select_app), response, 2) != 0 || response[0] != 0x90) {
        printk("Select app failed\n");
        return;
    }

    uint8_t select_ndef[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x04};
    if (send_apdu(select_ndef, sizeof(select_ndef), response, 2) != 0 || response[0] != 0x90) {
        printk("Select NDEF file failed\n");
        return;
    }

    uint8_t read_cmd[] = {0x00, 0xB0, 0x00, 0x00, 0x20};
    if (send_apdu(read_cmd, sizeof(read_cmd), response, 34) != 0 || response[32] != 0x90) {
        printk("ReadBinary failed\n");
        return;
    }

    printk("Tag detected\nNDEF Data:\n");
    for (int i = 0; i < 32; i++) {
        printk("%02x ", response[i]);
    }
    printk("\n");
}

void NFCReaderLoop(void) {
    i2c_dev = device_get_binding(I2C_DEV_NAME);
    if (!i2c_dev) {
        printk("I2C device %s not found\n", I2C_DEV_NAME);
        return;
    }

    if (!device_is_ready(rf_disable.port)) {
        printk("RF_DISABLE GPIO not ready\n");
        return;
    }

    gpio_pin_configure_dt(&rf_disable, GPIO_OUTPUT_ACTIVE);  // Disable RF (set PE2 high)
    k_msleep(10);  // Give the chip time to switch out of RF mode

    printk("Starting NFC reader loop...\n");
    while (1) {
        NFCRead();
        k_msleep(1000);
    }
}
