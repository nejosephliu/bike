#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "nrf.h"
#include "nrfx_gpiote.h"
#include "nrf_gpio.h"
#include "nrfx_pwm.h"
#include "led_strip.h"

static uint16_t numLEDs = 0;  // Number of LEDs to control
static uint8_t *pixels = 0;   // Pixel array
static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);     // PWM Driver

int led_init(uint16_t numLED, nrfx_gpiote_pin_t pin) {
  // Setting number of LEDs, returning error if already set
  if (numLEDs != 0) {
	return 1;
  }

  // Initialize GPIO
  ret_code_t error_code = NRF_SUCCESS;
  if (!nrfx_gpiote_is_init()) {
	error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // Configure GPIO pin
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  error_code = nrfx_gpiote_out_init(pin, &out_config);
  APP_ERROR_CHECK(error_code);

  // Initialize PWM driver
  nrfx_pwm_config_t const config = {
   .output_pins = {pin, // NRFX_PWM_PIN_INVERTED makes no difference
				   NRFX_PWM_PIN_NOT_USED,
				   NRFX_PWM_PIN_NOT_USED,
				   NRFX_PWM_PIN_NOT_USED},
   .irq_priority = APP_IRQ_PRIORITY_LOW,
   .base_clock = NRF_PWM_CLK_16MHz,
   .count_mode = NRF_PWM_MODE_UP,
   .top_value = 21,
   .load_mode = NRF_PWM_LOAD_COMMON,
   .step_mode = NRF_PWM_STEP_AUTO
  };
  error_code = nrfx_pwm_init(&m_pwm0, &config, NULL);
  APP_ERROR_CHECK(error_code);

  // Malloc enough bytes for pixels
  uint16_t numBytes = numLED * 3;
  pixels = (uint8_t*) malloc(numBytes);
  if (pixels == NULL) {
	return 1;
  }
  memset(pixels, 0, numBytes);

  numLEDs = numLED;
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
  if (first + num > numLEDs) return;
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
	  // For low bits, it is 7 (note: 6 fails badly).
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
