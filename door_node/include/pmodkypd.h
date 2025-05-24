#ifndef PMODKYPD_H
#define PMODKYPD_H

extern struct gpio_dt_spec pmodkypd_col1;
extern struct gpio_dt_spec pmodkypd_col2;
extern struct gpio_dt_spec pmodkypd_col3;
extern struct gpio_dt_spec pmodkypd_col4;
extern struct gpio_dt_spec pmodkypd_row1;
extern struct gpio_dt_spec pmodkypd_row2;
extern struct gpio_dt_spec pmodkypd_row3;
extern struct gpio_dt_spec pmodkypd_row4;


void PmodKypdListener(void);

#endif // PMODKYPD_H