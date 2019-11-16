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

#include "app_timer.h"
# include "nrf_drv_clock.h"

#include "buckler.h"

// LED array on NRF (4 total)
// #define NRF_LED0 NRF_GPIO_PIN_MAP(0,17)
// #define NRF_LED1 NRF_GPIO_PIN_MAP(0,18)
// #define NRF_LED2 NRF_GPIO_PIN_MAP(0,19)
// #define NRF_LED3 NRF_GPIO_PIN_MAP(0,20)
// static uint8_t LEDS[4] = {NRF_LED0, NRF_LED1, NRF_LED2, NRF_LED3};

// LED array on Buckler (3 total)
static uint8_t LEDS[3] = {BUCKLER_LED0, BUCKLER_LED1, BUCKLER_LED2};

APP_TIMER_DEF(accel_timer_id); // Create timer with timer ID
volatile bool read_accel;

static void accel_read_callback(void *p_context) {
    read_accel = true;
}

static void lfclk_request(void) // Enable clock to allow timers to work
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

static void create_accel_timer(void) {
    ret_code_t error_code = app_timer_create(&accel_timer_id,
                                  APP_TIMER_MODE_REPEATED,
                                  accel_read_callback);
    APP_ERROR_CHECK(error_code);
}

int main(void) {
    ret_code_t error_code = NRF_SUCCESS; // Don't need to redeclar error_code again

    // Init timers:
    lfclk_request(); // start clock
    app_timer_init(); 
    create_accel_timer(); //create our timer

    // initialize GPIO driver
    if (!nrfx_gpiote_is_init()) {
        error_code = nrfx_gpiote_init();
    }
    APP_ERROR_CHECK(error_code);

    // configure leds
    // manually-controlled (simple) output, initially set
    nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  
    
    for (int i = 0; i < 3; i++) {
        error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
        APP_ERROR_CHECK(error_code);
    }

    // Start the timer
    error_code = app_timer_start(accel_timer_id, APP_TIMER_TICKS(100), NULL);
    APP_ERROR_CHECK(error_code);

    // loop forever
    while (1) {
      //printf("Accel is true\n");
        if (read_accel) {
            read_accel = false;
            nrf_gpio_pin_toggle(LEDS[0]);
        }
    }
}

