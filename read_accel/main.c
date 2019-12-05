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

#include <math.h>
#include "buckler.h"
#include "display.h"
#include "mpu9250.h"
#include "quaternionFilters.h"
#include "grove_display.h"

// Math constants

#define PI 3.14159265359

#define smooth_num 300

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

APP_TIMER_DEF(main_loop_timer);

static void main_loop_timer_callback(void *p_context) {}  // Do nothing

static void create_main_loop_timer(void) {
    ret_code_t error_code = app_timer_create(&main_loop_timer,
                            APP_TIMER_MODE_REPEATED,
                            main_loop_timer_callback);
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

float sliding_averager_float_array(float *input_array_pointer, int array_size) {
    float output = 0;
    for (int i = 0; i < array_size; i++) {
        output += input_array_pointer[i];
    }
    return output / array_size;
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
    char display_buf[16] = {0};

    init_tm1637_display(0);
    init_tm1637_display(1);


    // Allocate arrays for mpu9250 measurements
    mpu9250_measurement_t acc_array[16] = {0};
    mpu9250_measurement_t avg_output = {0};
    uint16_t measurement_array_counter = 0;
        
    // Smooth over 1 sec of data
    float smoother_array[smooth_num] = {0};
    uint16_t smoother_array_index = 0;



    // Bike kinematics
    float current_velocity = 0;

    // Start the timer
    int delta_t_msec = 1000;


    // Init timers:
    lfclk_request(); // start clock
    app_timer_init();
    create_main_loop_timer(); //create our timer
    error_code = app_timer_start(main_loop_timer, APP_TIMER_TICKS(delta_t_msec), NULL);
    APP_ERROR_CHECK(error_code);

    // Setup variables for getting integration time
    uint32_t current_time = 0;
    uint32_t previous_time = 0;

    // Call MPU9250 init code with the I2C manager instance
    mpu9250_init(&twi_mngr_instance);

    uint16_t print_counter = 0;

    // Madewick algo hyperparameters; do not change during execution
    float GyroMeasError = PI * (4.0f / 180.0f);   // gyroscope measurement error in rads/s (start at 40 deg/s)
    float beta = sqrt(3.0f / 4.0f) * GyroMeasError;   // compute beta

    // Variables for AHRS calculation
    float pitch, yaw, roll;
    float a12, a22, a31, a32, a33; // rotation matrix coefficients for Euler angles and gravity components
    float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
    float lin_ax, lin_ay, lin_az;             // linear acceleration (acceleration with gravity component subtracted)
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion

    // loop forever
    while (1) {
        current_time = app_timer_cnt_get();
        uint32_t time_diff = app_timer_cnt_diff_compute(current_time, previous_time);

        // Convert ticks to milliseconds
        // https://devzone.nordicsemi.com/f/nordic-q-a/10414/milliseconds-since-startup
        // ticks * ( (APP_TIMER_PRESCALER|RTC Clock Prescalar + 1 ) * 1000 ) / APP_TIMER_CLOCK_FREQ;
        // https://devzone.nordicsemi.com/f/nordic-q-a/26197/how-to-covert-app_timer-ticks-to-ms
        // NOTE: I'm assuing an RTC prescalar of 0 & clock freq of 32768Hz!!!
        float time_diff_msec = ((float)time_diff * ((0.0 + 1.0) * 1000.0)) / 32768.0;


        if (read_accel) {
            read_accel = false;
            mpu9250_read_accelerometer_pointer(&ax, &ay, &az);
            mpu9250_read_gyro_pointer(&gx, &gy, &gz);
            mpu9250_read_magnetometer_pointer(&mx, &my, &mz);
            print_counter += 1;
        }
        MadgwickQuaternionUpdate(q, beta, time_diff_msec, -ax, ay, az, gx * PI / 180.0f, -gy * PI / 180.0f, -gz * PI / 180.0f,  my,  -mx, mz);

        a12 =   2.0f * (q[1] * q[2] + q[0] * q[3]);
        a22 =   q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3];
        a31 =   2.0f * (q[0] * q[1] + q[2] * q[3]);
        a32 =   2.0f * (q[1] * q[3] - q[0] * q[2]);
        a33 =   q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];
        pitch = -asinf(a32);
        roll  = atan2f(a31, a33);
        yaw   = atan2f(a12, a22);
        pitch *= 180.0f / PI;
        yaw   *= 180.0f / PI;
        yaw   += 13.2f; // Declination in Belmont, California is 13 degrees 20 minutes 2019-11-30
        if(yaw < 0) yaw   += 360.0f; // Ensure yaw stays between 0 and 360
        roll  *= 180.0f / PI;
        lin_ax = ax + a31;
        lin_ay = ay + a32;
        lin_az = az - a33;

        // Put variable into our smoother array

        smoother_array[smoother_array_index % smooth_num] = roll;
        smoother_array_index++;

        if ((print_counter % 100) == 0) {

            // Various things you can print out to RTT. Uncomment as needed
            //printf("X: %f\nY: %f\nZ: %f\n\n", ax, ay, az); // Raw accel
            //printf("X: %f\nY: %f\nZ: %f\n\n", lin_ax, lin_ay, lin_az); // Accel corrected for gravity and oreitnation
            //printf("Pitch: %f\nYaw: %f\n Roll: %f\n\n", pitch, yaw, roll); // Pitch, yaw, and roll.
            // snprintf(display_buf, 16, "Yaw: %f", yaw);
            // display_write(display_buf, DISPLAY_LINE_0);
            //printf("Smooth roll: %f\n", sliding_averager_float_array(smoother_array, smooth_num));
        }
        float smoothed_roll = sliding_averager_float_array(smoother_array, smooth_num);
        displayNum((int)smoothed_roll, 0, false, 1);
        if (smoothed_roll > 2.0) {
            // snprintf(display_buf, 16, "----A");
            // display_write(display_buf, DISPLAY_LINE_0);
            displayStr("---A", 0);
        } else if (smoothed_roll < -2.0) {
            // snprintf(display_buf, 16, "A----");
            // display_write(display_buf, DISPLAY_LINE_0);
            displayStr("A---", 0);
        } else {
            // snprintf(display_buf, 16, "A---A");
            // display_write(display_buf, DISPLAY_LINE_0);
            displayStr("A--A", 0);

        }

        previous_time = current_time;
    }
}