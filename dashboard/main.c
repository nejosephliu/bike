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

#include "nrf_drv_clock.h"
#include "app_timer.h"

#include "buckler.h"

#include "speech_recognizer.h"
#include "grove_display.h"
#include "buckler_accelerometer.h"
#include "gyro.h"

#include "mpu9250.h"
#include "si7021.h"

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

  //si7021_init()


  
  // initialization complete
  printf("Buckler initialized!\n");
    
  mpu9250_start_gyro_integration();

  //float x_cancel_rate = 0.18955;
  //float x_cancel = x_cancel_rate;

  while (1) {
    calculate_accelerometer_values();

    float x_val = get_x_g();
    printf("X value: %f\n", x_val);
    float y_val = get_y_g();
    printf("Y value: %f\n", y_val);
    float z_val = get_z_g();
    printf("Z value: %f\n", z_val);

    float x_degree = get_x_degree();
    displayNum(x_degree, 0, true, 0);

    float y_degree = get_y_degree();

    if (y_degree > 17) {
        displayStr("---A", 1);
    } else if (y_degree < -17) {
        displayStr("A---", 1);
    } else {
        displayNum(y_degree, 0, true, 1);
    }


    
    nrf_delay_ms(50);


    /*mpu9250_measurement_t measurement = mpu9250_read_gyro_integration();
    float x_angle = measurement.x_axis;
    x_angle += x_cancel;
    x_cancel += x_cancel_rate;
    if (x_angle < -350) {
        mpu9250_stop_gyro_integration();
        mpu9250_start_gyro_integration();
    }
    printf("X Angle: %f\n", x_angle);
    float y_angle = measurement.y_axis;
    printf("Y Angle: %f\n", y_angle);
    float z_angle = measurement.z_axis;
    printf("Z Angle: %f\n", z_angle);*/
  }
}
