// Hall Switch Example
//
// Uses a hall effect sensor and magnets to control an LED

#include <stdbool.h>
#include <stdio.h>

#include "app_error.h"
#include "nrf.h"
#include "nrfx_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

#include "buckler.h"

#define HALL_PIN NRF_GPIO_PIN_MAP(0,15)
#define NRF_LED0 NRF_GPIO_PIN_MAP(0,17) // Or use BUCKLER_LED0

volatile bool magnet = false;  // Global variable

// handler called whenever an input pin changes
void hall_switch_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  magnet = !magnet;
  if (magnet) {
	printf("Magnet sensed\n");
	nrfx_gpiote_out_clear(NRF_LED0);
  } else {
	printf("Magnet not detected\n\n");
	nrfx_gpiote_out_set(NRF_LED0);
  }
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize power management
  error_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(error_code);

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // configure led
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  error_code = nrfx_gpiote_out_init(NRF_LED0, &out_config);
  APP_ERROR_CHECK(error_code);

  // configure hall switch
  // input pin, trigger on either edge, low accuracy (allows low-power operation)
  nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
  error_code = nrfx_gpiote_in_init(HALL_PIN, &in_config, hall_switch_handler);
  APP_ERROR_CHECK(error_code);
  nrfx_gpiote_in_event_enable(HALL_PIN, true);

  // set initial states for LEDs
  nrfx_gpiote_out_set(NRF_LED0);
  
  // loop forever
  while (1) {
    // enter sleep mode
    nrf_pwr_mgmt_run();
  }
}

