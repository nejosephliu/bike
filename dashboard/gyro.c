#include "gyro.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

void init_gyro() {
    ret_code_t error_code = NRF_SUCCESS;

    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = BUCKLER_SENSORS_SCL;
    i2c_config.sda = BUCKLER_SENSORS_SDA;
    i2c_config.frequency = NRF_TWIM_FREQ_100K;
    error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    APP_ERROR_CHECK(error_code);
    mpu9250_init(&twi_mngr_instance);
    printf("IMU initialized!\n");
}