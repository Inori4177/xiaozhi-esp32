#ifndef FT6336_TOUCH_H
#define FT6336_TOUCH_H

#include <driver/i2c_master.h>
#include <stdint.h>

#define FT6336_I2C_ADDR         0x38

void ft6336_touch_init(i2c_master_bus_handle_t bus_handle, uint16_t width, uint16_t height, uint8_t rotation);
void ft6336_touch_read(void);
bool ft6336_touch_get_point(int *x, int *y);

#endif
