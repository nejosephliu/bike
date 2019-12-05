#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_twi_mngr.h"
#include "buckler.h"

#define SI7021_ADDR 0x40

void si7021_init();
void si7021_reset();
float read_temperature();
float read_humidity();