#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <lis3mdl.h>
#include <math.h>
#include <string.h>
#include "localVariables.h"
#include <stdbool.h>

#define MAGNETOMETER_THRESHOLD 2.5
#define MAGNETOMETER_SAMPLE_INTERVAL_MS 100
#define MAGNETOMETER_PRINT_INTERVAL_COUNT (1000 / MAGNETOMETER_SAMPLE_INTERVAL_MS)  // 10

static double compute_avg_magnitude(double x, double y, double z) {
    return (fabs(x) + fabs(y) + fabs(z)) / 3.0;
}

void MagnetometerSensorRead(void)
{
    const struct device *dev = DEVICE_DT_GET_ONE(st_lis3mdl_magn);
    if (!device_is_ready(dev)) {
        printk("Magnetometer device not ready\n");
        return;
    }

    bool was_open = false;
    printk("Starting Magnetometer Sensor Read\n");
    while (1) {
        if (sensor_sample_fetch(dev) < 0) {
            printk("Failed to fetch LIS3MDL sample\n");
            k_sleep(K_MSEC(MAGNETOMETER_SAMPLE_INTERVAL_MS));
            continue;
        }

        struct sensor_value x_val, y_val, z_val;
        if (sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &x_val) < 0 ||
            sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &y_val) < 0 ||
            sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &z_val) < 0) {
            printk("Failed to read LIS3MDL axes\n");
            k_sleep(K_MSEC(MAGNETOMETER_SAMPLE_INTERVAL_MS));
            continue;
        }

        double x = sensor_value_to_double(&x_val);
        double y = sensor_value_to_double(&y_val);
        double z = sensor_value_to_double(&z_val);

        double avg = compute_avg_magnitude(x, y, z);
        bool door_open = avg < MAGNETOMETER_THRESHOLD;

        
        // If door state changes, send to FIFO
        if (door_open != was_open) {
            was_open = door_open;
            printk("Door state changed: %s\n", door_open ? "Open" : "Closed");

            struct magnetometer_data_t *data = k_malloc(sizeof(struct magnetometer_data_t));
            if (!data) {
                printk("Failed to allocate magnetometer_data_t\n");
                continue;
            }

            data->door_opened = door_open;
            k_fifo_put(&MAGNETOMETER_fifo, data);
        }
        struct magnetometer_sample_data_t *sample_data = k_malloc(sizeof(struct magnetometer_sample_data_t));
        if (!sample_data) {
            printk("Failed to allocate magnetometer_sample_data_t\n");
            continue;
        }
        sample_data->avg_magnetometer_value = (int)(avg*100); // Store as integer percentage
        k_fifo_put(&MAGNETOMETER_SAMPLE_fifo, sample_data);

        k_sleep(K_MSEC(500));
    }
}
