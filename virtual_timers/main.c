// Virtual timers
//
// Uses a single hardware timer to create an unlimited supply of virtual
//  software timers

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"

#include "buckler.h"
#include "virtual_timer.h"

void led0_toggle() {
    nrf_gpio_pin_toggle(BUCKLER_LED0);
}

void led1_toggle() {
    nrf_gpio_pin_toggle(BUCKLER_LED1);
}

void led2_toggle() {
    nrf_gpio_pin_toggle(BUCKLER_LED2);
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Board initialized!\n");

  // You can use the NRF GPIO library to test your timers
  nrf_gpio_pin_dir_set(BUCKLER_LED0, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(BUCKLER_LED1, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(BUCKLER_LED2, NRF_GPIO_PIN_DIR_OUTPUT);

  // led0_toggle();
  // led1_toggle();
  // led2_toggle();

  // Don't forget to initialize your timer library
  virtual_timer_init();
  nrf_delay_ms(3000);
  // uint32_t timer = virtual_timer_start_repeated(1000000, led0_toggle);
  // printf("timer set: %lu\n", timer);

  // virtual_timer_start(1000000, led0_toggle);
  // virtual_timer_start(2000000, led1_toggle);
  // virtual_timer_start(3000000, led2_toggle);
  // virtual_timer_start(4000000, led0_toggle);
  // virtual_timer_start(5000000, led1_toggle);
  // virtual_timer_start(6000000, led2_toggle);

  virtual_timer_start_repeated(1000, led0_toggle);
  virtual_timer_start_repeated(2000, led1_toggle);



  nrf_delay_ms(5000);
  // virtual_timer_cancel(timer);
  // printf("timer canceled %lu\n", timer);

  while(1) {
    nrf_delay_ms(1000);
    printf("%lu\n", read_timer());
  }





    // Setup some timers and see what happens
  //virtual_timer_start_repeated(1000000, led0_toggle);
  //virtual_timer_start_repeated(2000000, led1_toggle);

}

