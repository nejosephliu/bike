#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"
#include "nrfx_gpiote.h"
#include "nrfx_saadc.h"

#include "buckler.h"

// ADC channels
#define X_CHANNEL 0
#define Y_CHANNEL 1
#define Z_CHANNEL 2

void saadc_callback (nrfx_saadc_evt_t const * p_event);
nrf_saadc_value_t sample_value (uint8_t channel);

void init_accelerometer();
void calculate_values();
float get_x_degree();
float get_y_degree();
float get_z_degree();
float get_x_g();
float get_y_g();
float get_z_g();