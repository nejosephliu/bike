#include "si7021.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized\n");
  
  si7021_init(&twi_mngr_instance);

  while(true) {
    si7021_reset();

    nrf_delay_ms(200);

    float temperature = read_temperature();
    printf("temperature (C): %f\n", temperature);
    printf("temperature (F): %f\n", temperature * 9.0 / 5.0 + 32);
    
    nrf_delay_ms(200);

    float humidity = read_humidity();
    printf("humidity: %.2f%%\n", humidity);

    nrf_delay_ms(3000);
  }
}
