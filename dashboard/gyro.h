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

#include "mpu9250.h"

void init_gyro();