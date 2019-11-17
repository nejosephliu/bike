#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "app_error.h"
#include "nrf.h"
#include "nrfx_pwm.h"
#include "led.h"

static uint16_t numLEDs = 0;  // Number of LEDs to control
static uint8_t *pixels = 0;   // Pixel array
static nrfx_pwm_t m_pwm0;            // PWM Driver

int initLED(uint16_t numLED, nrfx_pwm_t pwm) {
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

void setPixelRGB(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n < numLEDs) {
	uint8_t* p = &pixels[n*3];
	p[0] = g;
	p[1] = r;
	p[2] = b;
  }
}

// Color is composed of bytes {0, R, G, B}
void setPixelColor(uint16_t n, uint32_t c) {
  if (n < numLEDs) {
	uint8_t* p = &pixels[n*3];
	p[0] = (uint8_t) (c >> 8);
	p[1] = (uint8_t) (c >> 16);
	p[2] = (uint8_t) c;
  }
}

void fill(uint16_t first, uint16_t num, uint32_t c) {
  if (first + num > numLEDs) {
	return;
  }
  for (uint16_t i = first; i < first + num; i++) {
	setPixelColor(i, c);
  }
}

void clear() {
  memset(pixels, 0, numLEDs*3);
}

void show() {
  if (!numLEDs) return;

  // Creating PWM Sequence Values
  // 3 RGB Values * 8 bits/byte * PWM sequence size + zero padding
  uint32_t pattern_size = numLEDs*3*8*sizeof(uint16_t) + 2*sizeof(uint16_t);
  uint16_t* pattern = (uint16_t*) malloc(pattern_size);
  if (pattern == NULL) {
	return; // Error logging?
  }

  // Filling PWM Sequence Values, zero-padding at the end
  uint16_t pos = 0;
  for (uint16_t i=0; i<numLEDs*3; i++) {
	uint8_t pixel = pixels[i];

	for (uint8_t mask=0x80; mask>0; mask >>= 1) {
	  // For high bits, pwm duty is 13.  For low bits, it is 6.
	  pattern[pos] = (pixel & mask) ? (uint16_t) 13 : (uint16_t) 6;
	  pos++;
	}
  }
  pattern[pos++] = (uint16_t) 0 | (0x8000);
  pattern[pos++] = (uint16_t) 0 | (0x8000);

  printf("Pattern Start\n");
  for (uint16_t i=0; i< numLEDs*3*8 + 2; i++) {
	if (i % 24 == 23) {
	  printf("%d\n", pattern[i]);
	} else {
	  printf("%d ", pattern[i]);
	}
  }
  printf("Pattern End\n");
  // printf("%d\n", (uint32_t) NRF_PWM_VALUES_LENGTH(pattern));
  // printf("%d\n", pattern_size / sizeof(uint16_t));

  // Creating PWM Sequence
  nrf_pwm_sequence_t seq = {
   .values.p_common = pattern,
   .length = numLEDs*3*8 + 2,
   .repeats = 0,
   .end_delay = 0
  };
  // Calling the PWM (Finally)
  APP_ERROR_CHECK(nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, 0));
  printf("Called simple playback\n");

  free(pattern);
}
