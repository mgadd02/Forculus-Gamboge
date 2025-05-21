#ifndef LOCALVARIABLES_H
#define LOCALVARIABLES_H

#include <stdint.h>

extern struct k_fifo ULTRASONIC_fifo;

struct ultrasonic_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	int distance_cm;
};

#endif // LOCALVARIABLES_H

