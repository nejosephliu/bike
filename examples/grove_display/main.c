// Grove Display app
//
// Displays strings, ints, and floats to 2 Grove Display Modules

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

#include "buckler.h"
#include "grove_display.h"

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // configure grove display
  init_tm1637_display(0);
  init_tm1637_display(1);
  clearDisplay(0);
  clearDisplay(1);
  nrf_delay_ms(1000);

  displayStr("A*^_", 0);
  displayStr("H n3", 1);
  nrf_delay_ms(2000);

  displayNum(0, 0, false, 0);
  displayNum(1020, 0, false, 1);
  nrf_delay_ms(2000);

  displayNum(-2.345, 2, true, 0);      // Only prints "-2.34"
  displayStr("d_d", 1);
  nrf_delay_ms(2000);

  setBrightness(1);
  displayStr("0123456780AbcdEFh", 1);  // Sliding string
}
