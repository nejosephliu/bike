#pragma once

typedef enum {
  NORMAL,
  RIGHT,
  LEFT,
  BRAKE,
} states;

// Initializes LED pattern.  Only call this once.
int pattern_init(uint16_t numLED);

// Starts displaying pattern
ret_code_t pattern_start();
// Stops displaying pattern
void pattern_stop();

// Update FSM State
void pattern_update_state(states state);
