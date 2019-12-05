#pragma once

#include "nrf.h"
#include "nrfx_gpiote.h"

// Initializes the number of LEDs.  Only call this once
int  led_init(uint16_t numLED, nrfx_gpiote_pin_t pin);

// Set a specific pixel through RGB or through Color {0,R,G,B}
void led_set_pixel_RGB(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
void led_set_pixel_color(uint16_t n, uint32_t c);

// Fill from first (inclusive) to num (exclusive) with color
void led_fill(uint16_t first, uint16_t num, uint32_t c);

void led_clear(); // Clears all LEDs
void led_show();  // Send the signal to display the colors
