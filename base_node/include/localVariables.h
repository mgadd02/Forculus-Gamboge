#ifndef LOCAL_VARIABLES_H
#define LOCAL_VARIABLES_H

#include <zephyr/kernel.h>
#include <stdbool.h>

extern volatile int latest_distance_cm;
extern volatile int latest_avg_value;
extern volatile bool door_locked;

#endif // LOCAL_VARIABLES_H
