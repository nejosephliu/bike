#pragma once

#include "nrf.h"

int  initLED(uint16_t numLED, nrfx_pwm_t pwm); // Only call once

void setPixelRGB(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
void setPixelColor(uint16_t n, uint32_t c);

void fill(uint16_t first, uint16_t num, uint32_t c);
void clear();

void show();
