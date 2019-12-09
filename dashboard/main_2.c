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
#include "app_timer.h"
#include "nrf_drv_clock.h"

// Project includes
#include "states.h"
#include "led_strip.h"
#include "led_pattern.h"
#include "grove_display.h"
#include "gyro.h"
#include "IMU.h"
#include "si7021.h"

#define LED_PWM NRF_GPIO_PIN_MAP(0, 17)     // GPIO pin to control LED signal
#define NRF_BUTTON0 NRF_GPIO_PIN_MAP(0, 13) // Buttons to select state
#define NRF_BUTTON1 NRF_GPIO_PIN_MAP(0, 14)
#define NRF_BUTTON2 NRF_GPIO_PIN_MAP(0, 15)