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
#include "buckler_accelerometer.h"
#include "gyro.h"
#include "mpu9250.h"

#define LED_PWM NRF_GPIO_PIN_MAP(0, 17)     // GPIO pin to control LED signal
#define NRF_BUTTON0 NRF_GPIO_PIN_MAP(0, 13) // Buttons to select state
#define NRF_BUTTON1 NRF_GPIO_PIN_MAP(0, 14)
#define NRF_BUTTON2 NRF_GPIO_PIN_MAP(0, 15)
#define NRF_BUTTON3 NRF_GPIO_PIN_MAP(0, 16)
static uint8_t BUTTONS[4] = {NRF_BUTTON0, NRF_BUTTON1, NRF_BUTTON2, NRF_BUTTON3};
#define HALL_PIN NRF_GPIO_PIN_MAP(0, 11)

APP_TIMER_DEF(fsm_timer_id);
// APP_TIMER_DEF(voice_timer_id);  // TO BE IMPLEMENTED
APP_TIMER_DEF(accel_timer_id);


states curr_state = IDLE;
states button_next_state;  // Button-controlled
// states voice_next_state;   // Voice-controlled
states accel_next_state;   // Accelerometer/Automatically controlled

int hall_revolutions = 0;  // Hall sensor counter

// Main FSM Callback
void fsm_timer_callback(void* p_context) {
  if (curr_state != button_next_state) {
	pattern_update_state(button_next_state);
  }
  curr_state = button_next_state;  // TEMPORARY

  // Grove Display (PROBABLY SHOULD BE MOVED TO ITS OWN LIBRARY)
  switch (curr_state) {
  case LEFT: displayStr("A---", 1);
	break;
  case RIGHT: displayStr("---A", 1);
	break;
  default: displayStr("----", 1);
  }

  displayNum(hall_revolutions, 0, false, 0);
  printf("Hall revolutions: %d\n", hall_revolutions);
  hall_revolutions = 0;
}

void button_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  switch (pin) {
  case NRF_BUTTON0: button_next_state = IDLE;
	break;
  case NRF_BUTTON1: button_next_state = LEFT;
	break;
  case NRF_BUTTON2: button_next_state = RIGHT;
	break;
  case NRF_BUTTON3: button_next_state = BRAKE;
	break;
  }
}

void accel_timer_callback(void* p_context) {
  /*
  calculate_accelerometer_values();

  float x_val = get_x_g();
  float y_val = get_y_g();
  float z_val = get_z_g();
  printf("X: %f\t Y: %f\t Z: %f\n", x_val, y_val, z_val);

  float y_degree = get_y_degree();
  if (y_degree > 17) {
	accel_next_state = RIGHT;
  } else if (y_degree < -17) {
	accel_next_state = LEFT;
  } else {
	accel_next_state = IDLE;
  }
  */
}

void hall_switch_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  hall_revolutions += 1;
}

// General clock callback (not used)
static void clock_handler(nrfx_clock_evt_type_t event) {
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;
  printf("\n\n\nInitialization started!\n");

  // initialize power management
  error_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(error_code);

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // configure GPIO pins (buttons and hall switch)
  // weird behavior between IN_EVENT and PORT_EVENT
  nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;
  for (int i=0; i<4; i++) {
	error_code = nrfx_gpiote_in_init(BUTTONS[i], &in_config, button_handler);
	APP_ERROR_CHECK(error_code);
	nrfx_gpiote_in_event_enable(BUTTONS[i], true);
  }
  nrfx_gpiote_in_config_t in_config2 = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
  error_code = nrfx_gpiote_in_init(HALL_PIN, &in_config2, hall_switch_handler);
  APP_ERROR_CHECK(error_code);
  nrfx_gpiote_in_event_enable(HALL_PIN, true);

  // initialize clock and timer
  error_code = nrfx_clock_init(&clock_handler);
  APP_ERROR_CHECK(error_code);
  if (!nrfx_clock_lfclk_is_running()) {
	nrfx_clock_lfclk_start();
  }
  app_timer_init();

  // initialize Grove Display
  init_tm1637_display(0);
  init_tm1637_display(1);
  clearDisplay(0);
  clearDisplay(1);

  // initialize Accelerometer/Gyro (May need to be rewritten)
  // init_accelerometer();
  // init_gyro();

  // initialize LEDs
  uint16_t numLEDs = 8;
  led_init(numLEDs, LED_PWM); // assume success
  pattern_init(numLEDs);      // assume success
  pattern_start();
  
  // initialization complete
  printf("Initialization Finished!\n");

  // create timers (FSM, accelerometer, voice (?))
  error_code = app_timer_create(&fsm_timer_id, APP_TIMER_MODE_REPEATED,
								&fsm_timer_callback);
  APP_ERROR_CHECK(error_code);
  error_code = app_timer_start(fsm_timer_id, APP_TIMER_TICKS(250), NULL);
  APP_ERROR_CHECK(error_code);
  error_code = app_timer_create(&accel_timer_id, APP_TIMER_MODE_REPEATED,
								&accel_timer_callback);
  APP_ERROR_CHECK(error_code);
  error_code = app_timer_start(accel_timer_id, APP_TIMER_TICKS(1000), NULL);

  while (1) {
	nrf_pwr_mgmt_run();
  }
}
