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

#include "buckler.h"

// Timer includes
#include "nrf_drv_clock.h"
#include "app_timer.h"
// Project includes
#include "states.h"
#include "grove_display.h"
#include "buckler_accelerometer.h"
#include "gyro.h"
#include "mpu9250.h"

states state = IDLE;

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  init_tm1637_display(0);
  init_tm1637_display(1);
  clearDisplay(0);
  clearDisplay(1);
  
  displayStr("boP", 0);
  displayStr("HEL0", 1);
  
  nrf_delay_ms(1000);

  init_accelerometer();
  init_gyro();
  
  // initialization complete
  printf("Buckler initialized!\n");

  while (1) {
    calculate_accelerometer_values();

    float x_val = get_x_g();
    float y_val = get_y_g();
    float z_val = get_z_g();
    printf("X: %f\t Y: %f\t Z: %f\n", x_val, y_val, z_val);

    float y_degree = get_y_degree();

    if (y_degree > 17) {
        displayStr("---A", 1);
    } else if (y_degree < -17) {
        displayStr("A---", 1);
    } else {
        displayNum(y_degree, 0, true, 1);
    }
    
    nrf_delay_ms(50);
  }
}
