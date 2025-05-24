#ifndef GLOBALVARIABLES_H
#define GLOBALVARIABLES_H

#include <stdint.h>

extern struct k_fifo ULTRASONIC_fifo;

struct ultrasonic_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	int distance_cm;
};

#endif // GLOBALVARIABLES_H

