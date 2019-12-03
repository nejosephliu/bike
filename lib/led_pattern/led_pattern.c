#include <stdlib.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"

#include "app_timer.h"
#include "nrfx_clock.h"

#include "states.h"
#include "led_strip.h"
#include "led_pattern.h"

static uint16_t numLEDs = 0;

static states state = IDLE;
static uint16_t iteration = 0;

APP_TIMER_DEF(pattern_timer_id);

// Clear lights
static void clear_pattern() {
  led_clear();
  led_show();
  nrf_delay_ms(2);
}

// Callbacks for specific states
static void idle_callback() {
  clear_pattern();
  for (int i=0; i < numLEDs; i++) {
	led_set_pixel_RGB(i, 0, 100, 200); // Red-orange
  }
  led_show();
}

static void right_callback() {
  iteration = iteration % numLEDs;  
  clear_pattern();
  led_fill(0, iteration + 1, 0x00FFFF7F);
  led_show();
  iteration = (iteration + 1) % numLEDs;
}

static void left_callback() {
  iteration = iteration % numLEDs;
  clear_pattern();
  led_fill(numLEDs - 1 - iteration, iteration + 1, 0x00FF7FFF);
  led_show();
  iteration = (iteration + 1) % numLEDs;
}

static void brake_callback() { // Flash red lights
  clear_pattern();

  if (iteration % 2 == 0) {
	led_fill(0, numLEDs, 0x003FFFFF); // Red
  } else {
	led_fill(0, numLEDs, 0x00FFFFFF); // Empty
  }
  led_show();
  iteration = (iteration + 1) % 2;
}

// General Timer callback
static void pattern_timer_callback(void* p_context) {
  switch (state) {
  case IDLE: idle_callback();
	break;
  case RIGHT: right_callback();
	break;
  case LEFT: left_callback();
	break;
  case BRAKE: brake_callback();
	break;
  default: idle_callback();
	break;
  }
}

// General clock callback (not used)
static void clock_handler(nrfx_clock_evt_type_t event) {
}

// Sets up the timer for the LED pattern
int pattern_init(uint16_t numLED) {
  if (numLEDs != 0) return 1;

  // Initialize clock
  ret_code_t err_code = nrfx_clock_init(&clock_handler);
  APP_ERROR_CHECK(err_code);
  nrfx_clock_lfclk_start();

  // Create timer
  app_timer_init();
  err_code = app_timer_create(&pattern_timer_id,
							  APP_TIMER_MODE_REPEATED,
							  &pattern_timer_callback);
  APP_ERROR_CHECK(err_code);

  numLEDs = numLED;
  return 0;
}

// Starts the timer (and the LED pattern)
ret_code_t pattern_start() {
  ret_code_t err_code = app_timer_start(pattern_timer_id,
										APP_TIMER_TICKS(250), NULL);
  APP_ERROR_CHECK(err_code);
  return err_code;
}

// Stops the timer (and the LED pattern)
void pattern_stop() {
  app_timer_stop(pattern_timer_id);
}

// Update FSM state to change LED pattern output
void pattern_update_state(states new_state) {
  state = new_state;
  iteration = 0;
}

