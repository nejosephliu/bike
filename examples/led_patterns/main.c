// LED Pattern application
//
// Uses timers to create patterns on the LED strip
// Uses the NRF buttons to select which state

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
#include "nrf_pwr_mgmt.h"

#include "states.h"
#include "led_strip.h"
#include "led_pattern.h"

#define LED_PWM NRF_GPIO_PIN_MAP(0, 17) // GPIO pin to control LED signal
static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

#define NRF_BUTTON0 NRF_GPIO_PIN_MAP(0, 13) // Buttons to select state
#define NRF_BUTTON1 NRF_GPIO_PIN_MAP(0, 14)
#define NRF_BUTTON2 NRF_GPIO_PIN_MAP(0, 15)
#define NRF_BUTTON3 NRF_GPIO_PIN_MAP(0, 16)
static uint8_t BUTTONS[4] = {NRF_BUTTON0, NRF_BUTTON1, NRF_BUTTON2, NRF_BUTTON3};

states state;

void button_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  switch (pin) {
  case NRF_BUTTON0: state = IDLE;
	break;
  case NRF_BUTTON1: state = LEFT;
	break;
  case NRF_BUTTON2: state = RIGHT;
	break;
  case NRF_BUTTON3: state = BRAKE;
	break;
  }
  // printf("Reached pin %ld\n", pin);
  pattern_update_state(state);
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize power management
  error_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(error_code);

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

  // configure GPIO pins
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  error_code = nrfx_gpiote_out_init(LED_PWM, &out_config);
  APP_ERROR_CHECK(error_code);
  nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;
  for (int i=0; i<4; i++) {
	error_code = nrfx_gpiote_in_init(BUTTONS[i], &in_config, button_handler);
	APP_ERROR_CHECK(error_code);
	nrfx_gpiote_in_event_enable(BUTTONS[i], true);
  }

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

  // nrf_delay_ms(1000);
  pattern_start();
  printf("Ready");

  // loop forever
  while (1) {
	nrf_pwr_mgmt_run();
  }
}

