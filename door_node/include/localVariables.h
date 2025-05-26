#ifndef LOCALVARIABLES_H
#define LOCALVARIABLES_H

#include <stdint.h>
#include <stdbool.h>
extern struct k_fifo ULTRASONIC_fifo;
extern struct k_fifo PMODKYPD_fifo;
extern struct k_fifo MAGNETOMETER_fifo;

struct ultrasonic_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	bool proximity;
};

struct pmodkypd_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	char pin_code[6]; // 5 digits + null terminator

};

struct magnetometer_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	bool door_opened;
};
#endif // LOCALVARIABLES_H

