#include "pmodkypd.h"
#include <localVariables.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define PMODKYPD_COL1_NODE  DT_ALIAS(pmodkypdcol1)
#define PMODKYPD_COL2_NODE  DT_ALIAS(pmodkypdcol2)
#define PMODKYPD_COL3_NODE  DT_ALIAS(pmodkypdcol3)
#define PMODKYPD_COL4_NODE  DT_ALIAS(pmodkypdcol4)
#define PMODKYPD_ROW1_NODE  DT_ALIAS(pmodkypdrow1)
#define PMODKYPD_ROW2_NODE  DT_ALIAS(pmodkypdrow2)
#define PMODKYPD_ROW3_NODE  DT_ALIAS(pmodkypdrow3)
#define PMODKYPD_ROW4_NODE  DT_ALIAS(pmodkypdrow4)

static struct gpio_dt_spec cols[4];
static struct gpio_dt_spec rows[4];


struct gpio_dt_spec pmodkypd_col1 = GPIO_DT_SPEC_GET(PMODKYPD_COL1_NODE, gpios);
struct gpio_dt_spec pmodkypd_col2 = GPIO_DT_SPEC_GET(PMODKYPD_COL2_NODE, gpios);
struct gpio_dt_spec pmodkypd_col3 = GPIO_DT_SPEC_GET(PMODKYPD_COL3_NODE, gpios);
struct gpio_dt_spec pmodkypd_col4 = GPIO_DT_SPEC_GET(PMODKYPD_COL4_NODE, gpios);
struct gpio_dt_spec pmodkypd_row1 = GPIO_DT_SPEC_GET(PMODKYPD_ROW1_NODE, gpios);
struct gpio_dt_spec pmodkypd_row2 = GPIO_DT_SPEC_GET(PMODKYPD_ROW2_NODE, gpios);
struct gpio_dt_spec pmodkypd_row3 = GPIO_DT_SPEC_GET(PMODKYPD_ROW3_NODE, gpios);
struct gpio_dt_spec pmodkypd_row4 = GPIO_DT_SPEC_GET(PMODKYPD_ROW4_NODE, gpios);


void PmodKyodInit(void)
{
    cols[0] = pmodkypd_col1;
    cols[1] = pmodkypd_col2;
    cols[2] = pmodkypd_col3;
    cols[3] = pmodkypd_col4;

    rows[0] = pmodkypd_row1;
    rows[1] = pmodkypd_row2;
    rows[2] = pmodkypd_row3;
    rows[3] = pmodkypd_row4;

    for (int i = 0; i < 4; i++) {
        int ret;
    
        ret = gpio_pin_configure_dt(&cols[i], GPIO_OUTPUT_HIGH);
        if (ret != 0) {
            printk("Failed to configure col %d: %d\n", i, ret);
        }
    
        ret = gpio_pin_configure_dt(&rows[i], GPIO_INPUT | GPIO_PULL_UP);
        if (ret != 0) {
            printk("Failed to configure row %d: %d\n", i, ret);
        }
    }
    for (int i = 0; i < 4; i++) {
        int val = gpio_pin_get_dt(&rows[i]);
        printk("Initial row[%d] = %d\n", i, val);
    }
    

    printk("PmodKypd Initialized\n");
}


const char *keymap[4][4] = {
    {"1", "2", "3", "A"},
    {"4", "5", "6", "B"},
    {"7", "8", "9", "C"},
    {"0", "F", "E", "D"},
};

void PmodKypdListener(void)
{
    PmodKyodInit();

    char pin_code[6]; // 5 digits + null terminator
    int pin_index = 0;
    bool waiting_for_B = true;

    while (1) {
        for (int col = 0; col < 4; col++) {
            gpio_pin_set_dt(&cols[col], 0);

            for (int row = 0; row < 4; row++) {
                if (gpio_pin_get_dt(&rows[row]) == 0) {
                    const char *key = keymap[row][col];

                    // Debounce wait
                    k_msleep(200);

                    if (waiting_for_B) {
                        if (strcmp(key, "B") == 0) {
                            printk("Please type in a 5 pin code.\n");
                            waiting_for_B = false;
                            pin_index = 0;
                        }
                    } else {
                        if (pin_index < 5) {
                            pin_code[pin_index++] = key[0];
                            printk("Digit %d: %c\n", pin_index, key[0]);
                        }

                        if (pin_index == 5) {
                            pin_code[5] = '\0'; // null-terminate string
                            printk("PIN entered: %s\n", pin_code);

                            struct pmodkypd_data_t *pmodkypd_data = k_malloc(sizeof(struct pmodkypd_data_t));
                            if (!pmodkypd_data) {
                                printk("Failed to allocate memory for pmodkypd data\n");
                                continue;
                            }
                            memcpy(pmodkypd_data->pin_code, pin_code, sizeof(pin_code));
                            k_fifo_put(&PMODKYPD_fifo, pmodkypd_data);

                            waiting_for_B = true;  // reset for next session
                        }
                    }

                    // Wait until key is released before continuing
                    while (gpio_pin_get_dt(&rows[row]) == 0) {
                        k_msleep(10);
                    }
                }
            }

            gpio_pin_set_dt(&cols[col], 1);
        }

        k_msleep(50);
    }
}
