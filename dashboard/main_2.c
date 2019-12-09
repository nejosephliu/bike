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

#include "buckler.h"

// Timer includes
#include "app_timer.h"
#include "nrf_drv_clock.h"

// Project includes
#include "states.h"
#include "led_strip.h"
#include "led_pattern.h"
#include "grove_display.h"
#include "IMU.h"
#include "quaternionFilters.h"
#include "si7021.h"
#include <math.h>

// Constants:

// Number of AHRS readings that we want to smooth
#define smooth_num 300

// Define Pi
#define PI 3.14159265359

// Bike wheel radius (in centimeters)
// I'm assuming standard road bike tires with 622mm diamteter
#define bike_radius 31.1

// Arc length in centimeters per each 1/4 turn of the bike wheel
#define arc_length (bike_radius*PI)/180.0

// GPIO defines
#define LED_PWM NRF_GPIO_PIN_MAP(0, 17)     // GPIO pin to control LED signal

// Buttons to select state instead of the voice detector (for now)
#define NRF_BUTTON0 NRF_GPIO_PIN_MAP(0, 13) // State = brake
#define NRF_BUTTON1 NRF_GPIO_PIN_MAP(0, 14) // State = left
#define NRF_BUTTON2 NRF_GPIO_PIN_MAP(0, 15) // State = right

// Put the buttons into an array
static uint8_t BUTTONS[3] = {NRF_BUTTON0, NRF_BUTTON1, NRF_BUTTON2};

// Hall sensor pin
#define HALL_PIN NRF_GPIO_PIN_MAP(0, 11)

/*For now, let's create a single timer that we will use to
get the velocity from the Hall sensor and get the delta-T for the
AHRS algo.
*/

// Hall effect variables
#define HALL_EFFECT_TIME_MS 1000 // Update the hall values 1x/second (for now)
volatile int hall_revolutions = 0;

APP_TIMER_DEF(hall_velocity_calc);

float hall_effect_timer_callback(void *p_context) {
    float distance_rotated = (float)hall_revolutions * arc_length;
    printf("Hall Revs: %i\n", hall_revolutions);
    printf("Bike wheel distance: %f\n", distance_rotated);
    hall_revolutions = 0;
    return distance_rotated; // return velocity (as float)
}

void start_lfclock(void) {
    // Enable low frequency clock for timers
    // Do this *once* in the entire program
    ret_code_t error_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(error_code);
    nrf_drv_clock_lfclk_request(NULL);
}

void init_hall_effect_timer(void) {
    app_timer_init();
    ret_code_t error_code = app_timer_create(&hall_velocity_calc,
                            APP_TIMER_MODE_REPEATED,
                            hall_effect_timer_callback);
    APP_ERROR_CHECK(error_code);
}

// Setup hall effect sensor interrupt callback
void setup_Hall_GPIO_interrupt(void) {
    nrfx_gpiote_in_config_t hall_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    ret_code_t error_code = nrfx_gpiote_in_init(HALL_PIN, &hall_config, hall_effect_GPIO_callback);
    APP_ERROR_CHECK(error_code);
    nrfx_gpiote_in_event_enable(HALL_PIN, true);
}

void hall_effect_GPIO_callback(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    hall_revolutions += 1;
}

volatile bool IMU_data_ready = false;
// IMU interrupt callback function
void IMU_interrupt_callback(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    IMU_data_ready = true;
}

// Function to setup read of mpu9250 on an interrupt
void setup_IMU_interrupt(void) {
    ret_code_t error_code;
    nrfx_gpiote_in_config_t IMU_int_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    error_code = nrfx_gpiote_in_init(BUCKLER_IMU_INTERUPT, &IMU_int_config, IMU_interrupt_callback);
    APP_ERROR_CHECK(error_code);
    nrfx_gpiote_in_event_enable(BUCKLER_IMU_INTERUPT, true);
}

// Function to setup the 3 state buttons as input pins
void setup_buttons(void) {
    ret_code_t error_code;

    // Do not use high accuracy per Leland
    nrfx_gpiote_in_config_t button_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    button_config.pull = NRF_GPIO_PIN_PULLUP;

    for (int i = 0; i < 3; i++) {
        error_code = nrfx_gpiote_in_init(BUTTONS[i], &button_config, button_callback);
        APP_ERROR_CHECK(error_code);
        nrfx_gpiote_in_event_enable(BUTTONS[i], true);
    }
}

// Callback function for 3 buttons to check state
// FOR NOW WE CONSIDER THE BUTTON CALLBACK TO BE A STAND IN
// FOR THE VOICE SENSOR!!
// I removed the button for state IDLE; this could be our default state
// that the system falls back on if need be
void button_callback(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    switch (pin) {
    case NRF_BUTTON0:
        voice_recognition_state = BRAKE;
        break;
    case NRF_BUTTON1:
        voice_recognition_state = LEFT;
        break;
    case NRF_BUTTON2:
        voice_recognition_state = RIGHT;
        break;
    }
}

// Function for taking sliding average of IMU data arrays
float sliding_averager_float_array(float *input_array_pointer, int array_size) {
    float output = 0;
    for (int i = 0; i < array_size; i++) {
        output += input_array_pointer[i];
    }
    return output / array_size;
}
// Function to convert timer ticks to milliseconds

float get_msecs_from_ticks(uint32_t tick_diff) {
    // Convert ticks to milliseconds
    // https://devzone.nordicsemi.com/f/nordic-q-a/10414/milliseconds-since-startup
    // ticks * ( (APP_TIMER_PRESCALER|RTC Clock Prescalar + 1 ) * 1000 ) / APP_TIMER_CLOCK_FREQ;
    // https://devzone.nordicsemi.com/f/nordic-q-a/26197/how-to-covert-app_timer-ticks-to-ms
    // NOTE: I'm assuing an RTC prescalar of 0 & clock freq of 32768Hz
    return ((float)tick_diff * ((0.0 + 1.0) * 1000.0)) / 32768.0;
}
// Create TWI manager instance to read the IMU
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

// Main FSM state variable
states current_system_state = IDLE;

// NOTE SURE WHETHER WE NEED THESE LAST TWO STATE VARIABLES!!!
// Kinematics state variable
states kinematics_state = IDLE;

// Voice recognition state variable
states voice_recognition_state = IDLE;

// Initialize Grove voice recognition serial interface??????



int main(void) {
    ret_code_t error_code = NRF_SUCCESS; // Don't need to redeclare error_code again

    // initialize GPIO driver
    if (!nrfx_gpiote_is_init()) {
        error_code = nrfx_gpiote_init();
    }
    APP_ERROR_CHECK(error_code);

    // Initialize I2C

    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = BUCKLER_SENSORS_SCL; // From Kobuki code
    i2c_config.sda = BUCKLER_SENSORS_SDA;
    i2c_config.frequency = NRF_TWIM_FREQ_400K; // Changed to 400k to match the datasheet for MPU9250
    error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    APP_ERROR_CHECK(error_code);

    // Start the IMU and run calibration
    start_IMU_i2c_connection(&twi_mngr_instance);
    calibrate_gyro_and_accel();

    // The mag values will have to be hardcoded for final implementation
    // Mag cal is very slow...
    calibrate_magnetometer();
    debug(); // Disable in GM build. Used as sanity check on IMU I2C reads

    // Initialize the grove displays
    init_tm1637_display(0);
    init_tm1637_display(1);
    clearDisplay(0);
    clearDisplay(1);

    // Initialize the LEDs
    uint16_t numLEDs = 8;
    led_init(numLEDs, LED_PWM); // assume success
    pattern_init(numLEDs);      // assume success
    pattern_start();

    // Initialize GPIO devices and timers in preparation for run:

    // Start low frequency clock
    start_lfclock();

    // Init buttons
    setup_buttons();

    // Setup IMU interrupt
    setup_IMU_interrupt();

    // Setup hall effect sensor
    setup_Hall_GPIO_interrupt();

    // Start hall timer
    init_hall_effect_timer();
    error_code = app_timer_start(hall_velocity_calc, APP_TIMER_TICKS(HALL_EFFECT_TIME_MS), NULL);
    APP_ERROR_CHECK(error_code);

    // Init variables for AHRS integration time
    uint32_t current_time = 0;
    uint32_t previous_time = 0;

    // Madewick algo hyperparameters; do not change during execution
    float GyroMeasError = PI * (4.0f / 180.0f);   // gyroscope measurement error in rads/s (start at 40 deg/s)
    float beta = sqrt(3.0f / 4.0f) * GyroMeasError;   // compute beta

    // Variables for AHRS calculation
    float pitch, yaw, roll;
    float a12, a22, a31, a32, a33; // rotation matrix coefficients for Euler angles and gravity components
    float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
    float lin_ax, lin_ay, lin_az;             // linear acceleration (acceleration with gravity component subtracted)
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion

    // Arrays to hold smoothed AHRS data
    float smooth_roll_array[smooth_num] = {0};
    float smooth_x_accel_array[smooth_num] = {0};
    uint16_t smoother_array_index = 0;

    float smoothed_roll = 0;
    float smoothed_x_accel = 0;

    while(true) {
        // Get current RTC tick count from hall effect timer
        current_time = app_timer_cnt_get();
        uint32_t tick_diff = app_timer_cnt_diff_compute(current_time, previous_time);
        float time_diff_msec = get_msecs_from_ticks(tick_diff);

        // Read the IMU if new data is available
        if (IMU_data_ready) {
            IMU_data_ready = false;
            read_accelerometer_pointer(&ax, &ay, &az);
            read_gyro_pointer(&gx, &gy, &gz);
            read_magnetometer_pointer(&mx, &my, &mz);

        }
        // Run Madgwick's algorithm
        MadgwickQuaternionUpdate(q, beta, time_diff_msec, -ax, ay, az, gx * PI / 180.0f, -gy * PI / 180.0f, -gz * PI / 180.0f,  my,  -mx, mz);

        // Get Euler's angles
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

        // Input AHRS output into smoothing array
        smooth_roll_array[smoother_array_index % smooth_num] = roll;
        smooth_x_accel_array[smoother_array_index % smooth_num] = lin_ax;
        smoother_array_index++;

        smoothed_roll = sliding_averager_float_array(smoothed_roll, smooth_num);
        smoothed_x_accel = sliding_averager_float_array(smoothed_x_accel, smooth_num);
        // FSM update logic goes here
        // Cascade of if..else if..else statements
        // to find the ultimate current_system_state

        // Main FSM
        switch(current_system_state) {
        case IDLE:
            // Show speed and distance to rider
            //CHECK FOR BRAKING SHOULD COME FIRST!!!!
            if (smoothed_roll > 2.0) {
                printf("Move to flash right\n");
                current_system_state = RIGHT;
            } else if (smoothed_roll < -2.0) {
                printf("Move to flash left\n");
                current_system_state = LEFT;
            }
            break;
        case BRAKE:
            // Flash red
            break;
        case LEFT:
            // Flash Left_green
            // Again check for breaking
            if (smoothed_roll > -2.0) {
                printf("Back to IDLE from LEFT\n");
            }
            break;
        case RIGHT:
            // Flash Right_green
            // Again check for breaking
            if (smoothed_roll < 2.0) {
                printf("Back to IDLE from RIGHT\n");
            }
            break;
        }
        previous_time = current_time;
    }
}

