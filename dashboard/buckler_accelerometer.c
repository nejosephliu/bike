#include "buckler_accelerometer.h"

float x_degree = 0;
float y_degree = 0;
float z_degree = 0;

float x_g = 0;
float y_g = 0;
float z_g = 0;

// callback for SAADC events
void saadc_callback (nrfx_saadc_evt_t const * p_event) {
  // don't care about adc callbacks
}

// sample a particular analog channel in blocking mode
nrf_saadc_value_t sample_value (uint8_t channel) {
  nrf_saadc_value_t val;
  ret_code_t error_code = nrfx_saadc_sample_convert(channel, &val);
  APP_ERROR_CHECK(error_code);
  return val;
}

void init_accelerometer() {
    ret_code_t error_code = NRF_SUCCESS;

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    // initialize analog to digital converter
    nrfx_saadc_config_t saadc_config = NRFX_SAADC_DEFAULT_CONFIG;
    saadc_config.resolution = NRF_SAADC_RESOLUTION_12BIT;
    error_code = nrfx_saadc_init(&saadc_config, saadc_callback);
    APP_ERROR_CHECK(error_code);

    // initialize analog inputs
    // configure with 0 as input pin for now
    nrf_saadc_channel_config_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(0);
    channel_config.gain = NRF_SAADC_GAIN1_6; // input gain of 1/6 Volts/Volt, multiply incoming signal by (1/6)
    channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL; // 0.6 Volt reference, input after gain can be 0 to 0.6 Volts

    // specify input pin and initialize that ADC channel
    channel_config.pin_p = BUCKLER_ANALOG_ACCEL_X;
    error_code = nrfx_saadc_channel_init(X_CHANNEL, &channel_config);
    APP_ERROR_CHECK(error_code);

    // specify input pin and initialize that ADC channel
    channel_config.pin_p = BUCKLER_ANALOG_ACCEL_Y;
    error_code = nrfx_saadc_channel_init(Y_CHANNEL, &channel_config);
    APP_ERROR_CHECK(error_code);

    // specify input pin and initialize that ADC channel
    channel_config.pin_p = BUCKLER_ANALOG_ACCEL_Z;
    error_code = nrfx_saadc_channel_init(Z_CHANNEL, &channel_config);
    APP_ERROR_CHECK(error_code);
}

void calculate_values() {
    nrf_saadc_value_t x_val = sample_value(X_CHANNEL);
    nrf_saadc_value_t y_val = sample_value(Y_CHANNEL);
    nrf_saadc_value_t z_val = sample_value(Z_CHANNEL);

    float x_voltage = x_val*0.6/4095.0*6.0;
    float y_voltage = y_val*0.6/4095.0*6.0;
    float z_voltage = z_val*0.6/4095.0*6.0;

    x_g = (x_voltage - 1.5*2.84/3.0)/0.42*2.84/3.0;
    y_g = (y_voltage - 1.5*2.84/3.0)/0.42*2.84/3.0;
    z_g = (z_voltage - 1.5*2.84/3.0)/0.42*2.84/3.0;

    x_degree = atan(x_g / sqrt(y_g*y_g + z_g*z_g))*180/M_PI;
    y_degree = atan(y_g / sqrt(x_g*x_g + z_g*z_g))*180/M_PI;
    z_degree = atan(sqrt(x_g*x_g + y_g*y_g) / z_g)*180/M_PI;
}

float get_x_g() {
    return x_g;
}

float get_y_g() {
    return y_g;
}

float get_z_g() {
    return z_g;
}

float get_x_degree() {
    return x_degree;
}

float get_y_degree() {
    return y_degree;
}

float get_z_degree() {
    return z_degree;
}