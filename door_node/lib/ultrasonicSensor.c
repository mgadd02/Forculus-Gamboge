#include "ultrasonicSensor.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include "localVariables.h"
#include <zephyr/sys_clock.h> 

// Define nodes for data and clock
#define ULTRASONIC_TRIGGER_NODE  DT_ALIAS(ultrasonictrig)
#define ULTRASONIC_ECHO_NODE DT_ALIAS(ultrasonicecho)

// Initialize global gpio_dt_spec structures
struct gpio_dt_spec ultrasonic_trig = GPIO_DT_SPEC_GET(ULTRASONIC_TRIGGER_NODE, gpios);
struct gpio_dt_spec ultrasonic_echo = GPIO_DT_SPEC_GET(ULTRASONIC_ECHO_NODE, gpios);

/**
 * @brief Initialize the ultrasonic sensor
 * 
 * This function configures the GPIO pins for the ultrasonic sensor.
 * The trigger pin is set as output and the echo pin is set as input.
 */
void UltrasonicSensorInit(void)
{
    // Configure the trigger pin as output
    gpio_pin_configure_dt(&ultrasonic_trig, GPIO_OUTPUT_INACTIVE);
    
    // Configure the echo pin as input
    gpio_pin_configure_dt(&ultrasonic_echo, GPIO_INPUT);
    printk("Ultrasonic Sensor Initialized\n");
}



void UltrasonicSensorRead(void)
{
    UltrasonicSensorInit();

    while (1) {
        gpio_pin_set_dt(&ultrasonic_trig, 0);
        k_busy_wait(2);
        gpio_pin_set_dt(&ultrasonic_trig, 1);
        k_busy_wait(10);
        gpio_pin_set_dt(&ultrasonic_trig, 0);

        while (!gpio_pin_get_dt(&ultrasonic_echo)) { }
        uint32_t start = k_cycle_get_32();

        while (gpio_pin_get_dt(&ultrasonic_echo)) { }
        uint32_t end = k_cycle_get_32();

        uint32_t cycles = end - start;
        uint64_t duration_us = (uint64_t)cycles * 1000000U / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

        uint32_t dist_cm = duration_us / 58; // Standard formula for HC-SR04

        printk("duration: %llu µs, distance: %u cm\n", duration_us, dist_cm);

        struct ultrasonic_data_t *ultrasonic_data = k_malloc(sizeof(struct ultrasonic_data_t));
        if (!ultrasonic_data) {
            printk("Failed to allocate memory for ultrasonic data\n");
            continue;
        }
        ultrasonic_data->distance_cm = dist_cm;
        k_fifo_put(&ULTRASONIC_fifo, ultrasonic_data);
        k_sleep(K_MSEC(100));
    }
}
