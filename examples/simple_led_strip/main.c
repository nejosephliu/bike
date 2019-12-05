// LED strip app
//
// Controls an Adafruit Neopixel LED Strip using a PWM driver
//
// Setup:
// Attach 5V power supply to LED strip through a 100 Ohm resistor
// Attach data of LED strip to GPIO Pin 16
// Attach ground to LED strip and connect it to NRF

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrfx_pwm.h"

#include "led_strip.h"

#define LED_PWM NRF_GPIO_PIN_MAP(0, 16) // GPIO pin to control LED signal

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("\n\n\n\nlog initialized\n");

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // GPIO and PWM configured in led_init
  uint16_t numLEDs = 8;
  led_init(numLEDs, LED_PWM); // assume success

  led_clear();
  led_show();
  nrf_delay_ms(1000);

  // loop forever
  while (1) {
	for (int i=0; i<numLEDs; i++) {
	  led_set_pixel_RGB(i, 127, 255, 255); // Red
	}
	led_show();
	nrf_delay_ms(999);
	led_clear();
	led_show();
	nrf_delay_ms(1);

	for (int i=0; i<numLEDs; i++) {
	  led_set_pixel_color(i, (uint32_t) 0x00FFF0FF); // Green
	}
	led_show();
	nrf_delay_ms(999);
	led_clear();
	led_show();
	nrf_delay_ms(1);

    led_fill(0, numLEDs, (uint32_t) 0x00FFFF7F); // Blue
	led_show();
	nrf_delay_ms(999);
	led_clear();
	led_show();
	nrf_delay_ms(1);
  }
}

