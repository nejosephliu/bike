// Blink app
//
// Blinks the LEDs on Buckler

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
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"
#include "nrfx_pwm.h"

#include "buckler.h"
#include "led.h"

// LED array on NRF (4 total)
#define NRF_LED0 NRF_GPIO_PIN_MAP(0,17)
#define NRF_LED1 NRF_GPIO_PIN_MAP(0,18)
#define NRF_LED2 NRF_GPIO_PIN_MAP(0,19)
#define NRF_LED3 NRF_GPIO_PIN_MAP(0,20)
static uint8_t LEDS[4] = {NRF_LED0, NRF_LED1, NRF_LED2};

#define LED_PWM NRF_GPIO_PIN_MAP(0, 16)
static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

// LED array on Buckler (3 total)
// static uint8_t LEDS[3] = {BUCKLER_LED0, BUCKLER_LED1, BUCKLER_LED2};

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

  // configure leds
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  for (int i=0; i<3; i++) {
    error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
    APP_ERROR_CHECK(error_code);
  }
  
  error_code = nrfx_gpiote_out_init(NRF_LED3, &out_config);
  APP_ERROR_CHECK(error_code);
  error_code = nrfx_gpiote_out_init(LED_PWM, &out_config);
  APP_ERROR_CHECK(error_code);

  // initialize PWM driver
  nrfx_pwm_config_t const config = {
   .output_pins = {NRF_LED3, // NRFX_PWM_PIN_INVERTED
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
  initLED(numLEDs, m_pwm0);
  nrf_gpio_pin_clear(NRF_LED3);
  nrf_delay_ms(20);

  clear();
  show();
  nrf_delay_ms(1000);

  for (int i=0; i<numLEDs; i++) {
	setPixelRGB(i, 250, 255, 255);
  }
  setPixelRGB(5, 255, 255, 255);
  show();

  // loop forever
  while (1) {
    for (int i=0; i<3; i++) {
      nrf_gpio_pin_toggle(LEDS[i]);
      nrf_delay_ms(400);
    }
  }
}

