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
#include "quaternionFilters.h"
#include "si7021.h"

#define LED_PWM NRF_GPIO_PIN_MAP(0, 17)     // GPIO pin to control LED signal

// Buttons to select state instead of the voice detector (for now)
#define NRF_BUTTON0 NRF_GPIO_PIN_MAP(0, 13) // State = stop
#define NRF_BUTTON1 NRF_GPIO_PIN_MAP(0, 14) // State = left
#define NRF_BUTTON2 NRF_GPIO_PIN_MAP(0, 15) // State = right

// Put the buttons into an array
static uint8_t BUTTONS[3] = {NRF_BUTTON0, NRF_BUTTON1, NRF_BUTTON2};

/*For now, let's create a single timer that we will use to 
get the velocity from the Hall sensor and get the delta-T for the
AHRS algo.
*/

// Hall effect variables
volatile int hall_effect_mag_detects = 0;

APP_TIMER_DEF(hall_velocity_calc);

float hall_effect_timer_callback(void *p_context) {
	// Code to get velocity
	hall_effect_mag_detects = 0;
	// return velocity (as float)
}

void init_hall_effect_timer(void) {
	
}

// Create TWI manager instance to read the IMU
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

