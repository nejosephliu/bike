#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "nrf.h"
#include "nrfx_pwm.h"
#include "led.h"

static uint16_t numLEDs = 0;  // Number of LEDs to control
static uint8_t *pixels = 0;   // Pixel array
static nrfx_pwm_t m_pwm0;            // PWM Driver

int led_init(uint16_t numLED, nrfx_pwm_t pwm) {
  // Setting number of LEDs, returning error if already set
  if (numLEDs != 0) {
	return 1;
  }
  numLEDs = numLED;

  // Malloc enough bytes for pixels
  uint16_t numBytes = numLEDs * 3;
  pixels = (uint8_t*) malloc(numBytes);
  if (pixels == NULL) {
	return 1;
  }
  memset(pixels, 0, numBytes);

  m_pwm0 = pwm;
  return 0;
}

void led_set_pixel_RGB(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n < numLEDs) {
	uint8_t* p = &pixels[n*3];
	p[0] = g;
	p[1] = r;
	p[2] = b;
  }
}

// Color is composed of bytes {0, R, G, B}
void led_set_pixel_color(uint16_t n, uint32_t c) {
  if (n < numLEDs) {
	uint8_t* p = &pixels[n*3];
	p[0] = (uint8_t) (c >> 8);
	p[1] = (uint8_t) (c >> 16);
	p[2] = (uint8_t) c;
  }
}

// Fills LEDs from first (inclusive) to num (exclusive) with color
void led_fill(uint16_t first, uint16_t num, uint32_t c) {
  if (first + num > numLEDs) {
	return;
  }
  for (uint16_t i = first; i < first + num; i++) {
	led_set_pixel_color(i, c);
  }
}

void led_clear() {
  memset(pixels, -1, numLEDs*3);
}

void led_show() {
  if (!numLEDs) return;

  // Creating PWM Sequence Values
  // 3 RGB Values * 8 bits/byte * PWM sequence size + zero padding
  uint32_t pattern_len = numLEDs*3*8 + 2;
  uint16_t* pattern = (uint16_t*) malloc(pattern_len * sizeof(uint16_t));
  if (pattern == NULL) {
	return; // Error logging?
  }

  // Filling PWM Sequence Values, zero-padding at the end
  uint16_t pos = 0;
  for (uint16_t i=0; i<numLEDs*3; i++) {
	uint8_t pixel = pixels[i];
	for (uint8_t mask=0x80; mask>0; mask >>= 1) {
	  // For high bits, pwm duty is cycle is 13 out of 20.
	  // For low bits, it is 7 (note: 6 does not work).
	  pattern[pos] = (pixel & mask) ? (uint16_t) 13 : (uint16_t) 7;
	  pos++;
	}
  }
  pattern[pos++] = (uint16_t) 0 | (0x8000);
  pattern[pos++] = (uint16_t) 0 | (0x8000);

  // Creating PWM Sequence
  nrf_pwm_sequence_t seq = {
   .values.p_common = pattern,
   .length = pattern_len,
   .repeats = 0,
   .end_delay = 0
  };

  // (Finally) Calling the PWM library
  APP_ERROR_CHECK(nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, 0));
  free(pattern);
}
