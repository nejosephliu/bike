#pragma once

#include "nrf.h"

int  led_init(uint16_t numLED, nrfx_pwm_t pwm); // Only call once

void led_set_pixel_RGB(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
void led_set_pixel_color(uint16_t n, uint32_t c);

void led_fill(uint16_t first, uint16_t num, uint32_t c);
void led_clear();

void led_show();
