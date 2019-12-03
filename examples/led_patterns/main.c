// LED Pattern application
//
// Uses timers to create patterns on the LED strip

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

#include "states.h"
#include "led_strip.h"
#include "led_pattern.h"

#define LED_PWM NRF_GPIO_PIN_MAP(0, 16) // GPIO pin to control LED signal
static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

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

  // configure GPIO pin
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  error_code = nrfx_gpiote_out_init(LED_PWM, &out_config);
  APP_ERROR_CHECK(error_code);

  // initialize PWM driver
  nrfx_pwm_config_t const config = {
   .output_pins = {LED_PWM, // NRFX_PWM_PIN_INVERTED makes no difference
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

  uint16_t numLEDs = 8;
  led_init(numLEDs, m_pwm0); // assume success
  pattern_init(numLEDs);     // assume success

  nrf_delay_ms(1000);
  states state;
  pattern_start();

  // loop forever
  while (1) {
	state = IDLE;
	pattern_update_state(state);
	nrf_delay_ms(5000);
	state = LEFT;
	pattern_update_state(state);
	nrf_delay_ms(5000);
	state = RIGHT;
	pattern_update_state(state);
	nrf_delay_ms(5000);
	state = BRAKE;
	pattern_update_state(state);
	nrf_delay_ms(5000);
  }
}

