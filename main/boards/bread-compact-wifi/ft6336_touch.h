#ifndef FT6336_TOUCH_H
#define FT6336_TOUCH_H

#include <driver/i2c_master.h>
#include <stdint.h>

#define FT6336_I2C_ADDR         0x38

/* Align with LVGL_Demos/FT6336-arduino/FT6336.h (TFT setRotation(1) => ROTATION_RIGHT) */
#define FT6336_ROTATION_NORMAL    0
#define FT6336_ROTATION_INVERTED  2
#define FT6336_ROTATION_RIGHT     1
#define FT6336_ROTATION_LEFT      3

void ft6336_touch_init(i2c_master_bus_handle_t bus_handle, uint16_t width, uint16_t height, uint8_t rotation);
void ft6336_touch_read(void);
bool ft6336_touch_get_point(int *x, int *y);

/** 上次 ft6336_touch_read() 是否检测到按下（无需再次 I2C） */
bool ft6336_touch_is_pressed(void);

#endif
