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

bool si7021_init();
float read_temperature();
float read_humidity();

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

static uint8_t SI7021_RESET_CMD = 0xFE;
static uint8_t SI7021_MEASTEMP_NOHOLD_CMD = 0xF3;
static uint8_t SI7021_MEASRH_NOHOLD_CMD = 0xF5;

static uint8_t temp_read_buf[3] = {0};
static uint8_t humidity_read_buf[3] = {0};

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized\n");
  
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = BUCKLER_SENSORS_SCL;
  i2c_config.sda = BUCKLER_SENSORS_SDA;
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  APP_ERROR_CHECK(error_code);
  
  while(true) {
    si7021_init();

    nrf_delay_ms(200);

    float temperature = read_temperature();
    printf("temperature (C): %f\n", temperature);
    printf("temperature (F): %f\n", temperature * 9.0 / 5.0 + 32);
    
    nrf_delay_ms(200);

    float humidity = read_humidity();
    printf("humidity: %.2f%%\n", humidity);

    nrf_delay_ms(5000);
  }
}

static nrf_twi_mngr_transfer_t const reset_i2c[] = {
  NRF_TWI_MNGR_WRITE(SI7021_ADDR, &SI7021_RESET_CMD, 1, NRF_TWI_MNGR_NO_STOP)
};

static nrf_twi_mngr_transfer_t const send_temp_command_i2c[] = {
  NRF_TWI_MNGR_WRITE(SI7021_ADDR, &SI7021_MEASTEMP_NOHOLD_CMD, 1, NRF_TWI_MNGR_NO_STOP)
};

static nrf_twi_mngr_transfer_t const read_temp_i2c[] = {
  NRF_TWI_MNGR_READ(SI7021_ADDR, temp_read_buf, 3, 0)
};

static nrf_twi_mngr_transfer_t const send_humidity_command_i2c[] = {
  NRF_TWI_MNGR_WRITE(SI7021_ADDR, &SI7021_MEASRH_NOHOLD_CMD, 1, NRF_TWI_MNGR_NO_STOP)
};

static nrf_twi_mngr_transfer_t const read_humidity_i2c[] = {
  NRF_TWI_MNGR_READ(SI7021_ADDR, humidity_read_buf, 3, 0)
};

bool si7021_init() {
  int error = nrf_twi_mngr_perform(&twi_mngr_instance, NULL, reset_i2c, sizeof(reset_i2c)/sizeof(reset_i2c[0]), NULL);
  printf("init error: %d \n", error);
  return true;
}

// At 55 degrees F (Apple), temperature reading stabilized at 61 degrees F, ±1 degree F
float read_temperature() {
  int error = nrf_twi_mngr_perform(&twi_mngr_instance, NULL, send_temp_command_i2c, sizeof(send_temp_command_i2c)/sizeof(send_temp_command_i2c[0]), NULL);
  printf("temp 1 error: %d \n", error);
  nrf_delay_ms(20);
  error = nrf_twi_mngr_perform(&twi_mngr_instance, NULL, read_temp_i2c, sizeof(read_temp_i2c)/sizeof(read_temp_i2c[0]), NULL);
  printf("temp 2 error: %d \n", error);

  uint16_t temp = (temp_read_buf[0] << 8) | temp_read_buf[1];

  float temperature = temp;
  temperature *= 175.72;
  temperature /= 65536;
  temperature -= 46.85;  

  return temperature;
}

// At 80% humidity (Apple), humidity reading stabilized at 73.5% ± 1%
float read_humidity() {
  int error = nrf_twi_mngr_perform(&twi_mngr_instance, NULL, send_humidity_command_i2c, sizeof(send_humidity_command_i2c)/sizeof(send_humidity_command_i2c[0]), NULL);
  printf("humidity 1 error: %d \n", error);
  nrf_delay_ms(20);
  error = nrf_twi_mngr_perform(&twi_mngr_instance, NULL, read_humidity_i2c, sizeof(read_humidity_i2c)/sizeof(read_humidity_i2c[0]), NULL);
  printf("humidity 2 error: %d \n", error);

  uint16_t hum = (humidity_read_buf[0] << 8) | humidity_read_buf[1];

  float humidity = hum;
  humidity *= 125;
  humidity /= 65536;
  humidity -= 6;

  return humidity;
}