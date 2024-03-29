#include "si7021.h"

static const nrf_twi_mngr_t* twi_mngr_instance;

static uint8_t SI7021_RESET_CMD = 0xFE;
static uint8_t SI7021_MEASTEMP_NOHOLD_CMD = 0xF3;
static uint8_t SI7021_MEASRH_NOHOLD_CMD = 0xF5;

static uint8_t temp_read_buf[3] = {0};
static uint8_t humidity_read_buf[3] = {0};

void si7021_init(const nrf_twi_mngr_t* instance) {    
  twi_mngr_instance = instance;

  ret_code_t error_code = NRF_SUCCESS;

  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = BUCKLER_SENSORS_SCL;
  i2c_config.sda = BUCKLER_SENSORS_SDA;
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  
  //error_code = nrf_twi_mngr_init(twi_mngr_instance, &i2c_config);
  //APP_ERROR_CHECK(error_code);  
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

void si7021_reset() {
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, reset_i2c, sizeof(reset_i2c)/sizeof(reset_i2c[0]), NULL);
  if (error != 0) {
    printf("Error: %d \n", error);
  }
}

// At 55 degrees F (Apple), temperature reading stabilized at 61 degrees F, ±1 degree F
float read_temperature() {
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, send_temp_command_i2c, sizeof(send_temp_command_i2c)/sizeof(send_temp_command_i2c[0]), NULL);
  
  if (error != 0) {
    printf("Error: %d \n", error);
  }
  nrf_delay_ms(20);
  error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_temp_i2c, sizeof(read_temp_i2c)/sizeof(read_temp_i2c[0]), NULL);
  if (error != 0) {
    printf("Error: %d \n", error);
  }
  uint16_t temp = (temp_read_buf[0] << 8) | temp_read_buf[1];

  float temperature = temp;
  temperature *= 175.72;
  temperature /= 65536;
  temperature -= 46.85;

  // Minus 6 because of bias
  temperature = (9.0 / 5.0) * temperature + 32.0 - 6.0;

  return temperature;
}

// At 80% humidity (Apple), humidity reading stabilized at 73.5% ± 1%
float read_humidity() {
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, send_humidity_command_i2c, sizeof(send_humidity_command_i2c)/sizeof(send_humidity_command_i2c[0]), NULL);
  if (error != 0) {
    printf("Error: %d \n", error);
  }
  nrf_delay_ms(20);
  error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_humidity_i2c, sizeof(read_humidity_i2c)/sizeof(read_humidity_i2c[0]), NULL);
  if (error != 0) {
    printf("Error: %d \n", error);
  }

  uint16_t hum = (humidity_read_buf[0] << 8) | humidity_read_buf[1];

  float humidity = hum;
  humidity *= 125;
  humidity /= 65536;
  humidity -= 6;

  return humidity;
}