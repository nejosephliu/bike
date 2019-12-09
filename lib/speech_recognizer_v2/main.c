#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_drv_power.h"
#include "nrf_serial.h"
#include "app_timer.h"


#include "app_error.h"
#include "app_util.h"

#include "speech_recognizer_v2.h"

int main(void) {
    start_grove_speech_recognizer();
    uint8_t voice_input = 255;
    while(1) {
        voice_input = read_voice_command();
        if(voice_input != 255) {
            printf("Input Number: %i\n", voice_input);
        }
    }
}