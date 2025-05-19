#ifndef ULTRASONICSENSOR_H
#define ULTRASONICSENSOR_H

// Global declarations of GPIO specifications
extern struct gpio_dt_spec ultrasonic_trig;   // Trigger pin
extern struct gpio_dt_spec ultrasonic_echo;  // Echo pin

void UltrasonicSensorRead(void);
#endif // ULTRASONICSENSOR_H