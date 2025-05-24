#include "pmodkypd.h"
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

    while (1) {
        for (int col = 0; col < 4; col++) {
            // Drive current column LOW
            gpio_pin_set_dt(&cols[col], 0);

            // Check each row
            for (int row = 0; row < 4; row++) {
                if (gpio_pin_get_dt(&rows[row]) == 0) {
                    printk("Key pressed: %s\n", keymap[row][col]);
                    k_msleep(200);  // basic debounce
                }
            }

            // Drive current column back HIGH
            gpio_pin_set_dt(&cols[col], 1);
        }

        k_msleep(50);  // allow rest before next scan
    }
}
