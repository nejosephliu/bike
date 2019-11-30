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
#include "nrf_serial.h"
#include "nrf_twi_mngr.h"

#include "app_timer.h"
#include "nrf_drv_clock.h"

#include "buckler.h"
#include "display.h"
#include "mpu9250.h"

// LED array on NRF (4 total)
// #define NRF_LED0 NRF_GPIO_PIN_MAP(0,17)
// #define NRF_LED1 NRF_GPIO_PIN_MAP(0,18)
// #define NRF_LED2 NRF_GPIO_PIN_MAP(0,19)
// #define NRF_LED3 NRF_GPIO_PIN_MAP(0,20)
// static uint8_t LEDS[4] = {NRF_LED0, NRF_LED1, NRF_LED2, NRF_LED3};

// LED array on Buckler (3 total)
static uint8_t LEDS[3] = {BUCKLER_LED0, BUCKLER_LED1, BUCKLER_LED2};

// Timer instantiation and callbacks

APP_TIMER_DEF(accel_timer_id);
volatile bool read_accel;

static void lfclk_request(void) { // Enable clock to allow timers to work
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

static void accel_read_callback_timer(void *p_context) {
    read_accel = true;
}

static void create_accel_timer(void) {
    ret_code_t error_code = app_timer_create(&accel_timer_id,
                            APP_TIMER_MODE_REPEATED,
                            accel_read_callback_timer);
    APP_ERROR_CHECK(error_code);
}

void sliding_averager(mpu9250_measurement_t *input_array_pointer, mpu9250_measurement_t *output_array_pointer, int array_size) {
    float temp_x = 0;
    float temp_y = 0;
    float temp_z = 0;

    for (int i = 0; i < array_size; i++) {
        temp_x += input_array_pointer[i].x_axis;
        temp_y += input_array_pointer[i].y_axis;
        temp_z += input_array_pointer[i].z_axis;
    }

    temp_x /= (float) array_size;
    temp_y /= (float) array_size;
    temp_z /= (float) array_size;

    // Save smoothed values into
    output_array_pointer->x_axis = temp_x;
    output_array_pointer->y_axis = temp_y;
    output_array_pointer->z_axis = temp_z;

    return;
}

float update_velocity(mpu9250_measurement_t *smoothed_acceleration, int delta_t_msec, float previous_velocity) {
    // TODO: Add an enum and switch/case statements for easy access selection
    // For now just hardcoded to z-axis
    float axis_accel_value = smoothed_acceleration->x_axis;

    if ((axis_accel_value < .05) && (axis_accel_value > -.05)) {
        return previous_velocity;
    }

    previous_velocity += (axis_accel_value * ((float) delta_t_msec / 1000.0));
    return previous_velocity;
}

void accel_read_IMU_interrupt_callback(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    read_accel = true;
}

// TWI Manager Instances
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

int main(void) {
    ret_code_t error_code = NRF_SUCCESS; // Don't need to redeclare error_code again

    // Init timers:
    lfclk_request(); // start clock
    app_timer_init();
    create_accel_timer(); //create our timer

    // initialize GPIO driver
    if (!nrfx_gpiote_is_init()) {
        error_code = nrfx_gpiote_init();
    }
    APP_ERROR_CHECK(error_code);

    // Poll mpu9250 on its interrupt pin

    nrfx_gpiote_in_config_t IMU_int_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    // IMU_int_config.pull = NRF_GPIO_PIN_PULLUP; // Disable pullup to see if accel INT pin active high will work

    error_code = nrfx_gpiote_in_init(BUCKLER_IMU_INTERUPT, &IMU_int_config, accel_read_IMU_interrupt_callback);
    APP_ERROR_CHECK(error_code);
    nrfx_gpiote_in_event_enable(BUCKLER_IMU_INTERUPT, true);


    // configure leds
    // manually-controlled (simple) output, initially set
    nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);


    for (int i = 0; i < 3; i++) {
        error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
        APP_ERROR_CHECK(error_code);
    }

    // Start the I2C interface

    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = BUCKLER_SENSORS_SCL; // From Kobuki code
    i2c_config.sda = BUCKLER_SENSORS_SDA;
    i2c_config.frequency = NRF_TWIM_FREQ_400K; // Changed to 400k to match the datasheet for MPU9250
    error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    APP_ERROR_CHECK(error_code);

    // Start SPI for Kobuki display
    nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);
    nrf_drv_spi_config_t spi_config = {
        .sck_pin = BUCKLER_LCD_SCLK,
        .mosi_pin = BUCKLER_LCD_MOSI,
        .miso_pin = BUCKLER_LCD_MISO,
        .ss_pin = BUCKLER_LCD_CS,
        .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .orc = 0,
        .frequency = NRF_DRV_SPI_FREQ_4M,
        .mode = NRF_DRV_SPI_MODE_2,
        .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
    };
    error_code = nrf_drv_spi_init(&spi_instance, &spi_config, NULL, NULL);
    APP_ERROR_CHECK(error_code);
    display_init(&spi_instance);
    display_write("Hello, Human!", DISPLAY_LINE_0);
    printf("Display initialized!\n");


    // Allocate arrays for mpu9250 measurements
    mpu9250_measurement_t acc_array[16] = {0};
    mpu9250_measurement_t avg_output = {0};

    uint16_t measurement_array_counter = 0;

    // Bike kinematics
    float current_velocity = 0;

    // Start the timer
    int delta_t_msec = 1000;

    // DO NOT enable polling accel on a timer; use interrupt pin
    //error_code = app_timer_start(accel_timer_id, APP_TIMER_TICKS(delta_t_msec), NULL);
    //APP_ERROR_CHECK(error_code);

    // Call MPU9250 init code with the I2C manager instance
    mpu9250_init(&twi_mngr_instance);

    uint8_t print_counter = 0;

    // loop forever
    while (1) {
        if (read_accel) {
            read_accel = false;
        }
    }
}