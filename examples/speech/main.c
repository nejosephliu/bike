#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "nrf.h"
#include "app_timer.h"
#include "nrfx_clock.h"

#include "speech_recognizer_v2.h"

// General clock callback (not used)
static void clock_handler(nrfx_clock_evt_type_t event) {
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // Init clock and timer for serial speech recognizer
  error_code = nrfx_clock_init(&clock_handler);
  APP_ERROR_CHECK(error_code);
  if (!nrfx_clock_lfclk_is_running()) {
	nrfx_clock_lfclk_start();
  }
  app_timer_init();

  // Init speech
  speech_init();
  printf("\n\n\nInitialized\n");

  uint8_t voice_input = 255;
  while(1) {
	voice_input = speech_read();
	if (voice_input != 255) {
	  // printf("Input Number: %i\n", voice_input);
	  printf("%s\n", speech_convert_reading(voice_input));
	}
  }
}
