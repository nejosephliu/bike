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
#include "nrfx_clock.h"

// Project includes
#include "states.h"
#include "led_strip.h"
#include "led_pattern.h"
#include "grove_display.h"
#include "IMU.h"
#include "quaternion_filter.h"
#include "speech_recognizer_v2.h"
#include "si7021.h"
#include <math.h>

// Constants:

// Number of AHRS readings that we want to smooth
#define smooth_num 300

// Define Pi
#define PI 3.14159265359

// m/s to MPH
#define MS_TO_MPH_CONVERSION_FACTOR 2.237

// Bike wheel radius (in centimeters)
// I'm assuming standard road bike tires with 622mm diamteter
#define bike_radius .39

// Arc length in centimeters per each 1/2 turn of the bike wheel
#define arc_length bike_radius * PI * (2.0 / 9.0)

// GPIO defines
#define LED_PWM NRF_GPIO_PIN_MAP(0, 17)     // GPIO pin to control LED signal

// Hall sensor pin
#define HALL_PIN NRF_GPIO_PIN_MAP(0, 11)

#define BRAKING_THRESHOLD -0.7
#define LEFT_THRESHOLD 14.0
#define RIGHT_THRESHOLD -1 * LEFT_THRESHOLD

// Main FSM state variable
states current_system_state = IDLE;
// Voice recognition state variable
states voice_recognition_state = IDLE;
// Kinematics state variable
states kinematics_state = IDLE;///IDLE or BRAKE

/*For now, let's create a single timer that we will use to
get the velocity from the Hall sensor and get the delta-T for the
AHRS algo.
*/

// Hall effect variables
#define HALL_EFFECT_TIME_MS 250 // Update the hall values 2x/second (for now)

volatile int hall_revolutions = 0;
volatile int hall_revolution_history[4] = {0};

volatile uint32_t hall_revolution_array_index = 0;

volatile float velocity_readings_in_last_callback[100] = {0};

volatile float velocity_reading_history[4] = {0};
volatile uint32_t velocity_reading_array_index = 0;

volatile int num_readings_in_last_callback = 0;

APP_TIMER_DEF(hall_velocity_calc);

volatile float distance_rotated = 0;

// Display Mode Enum
#define NUM_DISPLAY_MODES 4
#define DISPLAY_MODE_VELOCITY_MPH 0
#define DISPLAY_MODE_DISTANCE_METERS 1
#define DISPLAY_MODE_TEMP 2
#define DISPLAY_MODE_HUMIDITY 3

uint8_t si7021_is_init = 0;
int display_mode = DISPLAY_MODE_VELOCITY_MPH;


// Voice Commands Enum
#define VOICE_COMMAND_NEXT 5
#define VOICE_COMMAND_PREV 6
#define VOICE_COMMAND_LEFT 16
#define VOICE_COMMAND_RIGHT 17
#define VOICE_COMMAND_STOP 18

float get_msecs_from_ticks(uint32_t tick_diff);

float hall_last_time = -1;
float hall_new_time = -1;

void hall_effect_timer_callback(void *p_context) {
    // in meters
    distance_rotated += ((float)hall_revolutions * arc_length);

    if (display_mode == DISPLAY_MODE_DISTANCE_METERS) {
        displayNum(distance_rotated, 0, false, 0);
        displayStr("NN", 1);
    }

    float velocity_sum = 0;
    for (int i = 0; i < num_readings_in_last_callback; i++) {
        velocity_sum += velocity_readings_in_last_callback[i];
    }

    float avg_velocity;

    if (num_readings_in_last_callback == 0) {
        avg_velocity = 0;
    } else {
        avg_velocity = velocity_sum / num_readings_in_last_callback;
    }

    

    //printf("Hall Revs: %i\n", hall_revolutions);
    //displayNum((int)distance_rotated, 0, false, 0);

    // velocity printout in MPH
    // float velocity_mph = (float)hall_revolutions * arc_length * MS_TO_MPH_CONVERSION_FACTOR * ((float)HALL_EFFECT_TIME_MS / 1000.0); // Multiply by .5 because we're going by 1/2 sec update

    if (display_mode == DISPLAY_MODE_VELOCITY_MPH) {
        displayNum(avg_velocity, 2, false, 0);
        //displayNum(num_readings_in_last_callback, 0, false, 1);
        displayStr("NNPH", 1);
    } else if (display_mode == DISPLAY_MODE_DISTANCE_METERS) {
        displayNum(distance_rotated, 0, false, 0);
        displayStr("NN", 1);
    }

    num_readings_in_last_callback = 0;

    //printf("Bike wheel distance: %f\n", distance_rotated);
    hall_revolution_array_index++;
    hall_revolution_history[hall_revolution_array_index % 4] = hall_revolutions;
    hall_revolutions = 0;

    velocity_reading_array_index++;
    velocity_reading_history[velocity_reading_array_index % 4] = avg_velocity;
}

// General clock callback (not used)
static void clock_handler(nrfx_clock_evt_type_t event) {
}

void start_lfclock(void) {
    // Enable low frequency clock for timers
    // Do this *once* in the entire program
    ret_code_t error_code = nrfx_clock_init(&clock_handler);
    APP_ERROR_CHECK(error_code);
    if (!nrfx_clock_lfclk_is_running()) {
        nrfx_clock_lfclk_start();
    }
}

void init_hall_effect_timer(void) {
    app_timer_init();
    ret_code_t error_code = app_timer_create(&hall_velocity_calc,
                            APP_TIMER_MODE_REPEATED,
                            &hall_effect_timer_callback);
    APP_ERROR_CHECK(error_code);
}

void hall_effect_GPIO_callback(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    hall_revolutions += 1;

    hall_last_time = hall_new_time;
    hall_new_time = app_timer_cnt_get();

    if (hall_last_time != -1) {
        uint32_t tick_diff = app_timer_cnt_diff_compute(hall_new_time, hall_last_time);
        float time_diff_msec = get_msecs_from_ticks(tick_diff);

        printf("time diff msec: %f\n", time_diff_msec);

        float velocity_mph = arc_length / time_diff_msec * 1000 * MS_TO_MPH_CONVERSION_FACTOR;
        printf("velocity: %f\n\n", velocity_mph);

        if (velocity_mph < 30) {
            velocity_readings_in_last_callback[num_readings_in_last_callback++] = velocity_mph;
        }

        // if (display_mode == DISPLAY_MODE_VELOCITY_MPH) {
        //     displayNum(velocity_mph, 2, false, 0);
        //     displayStr("NNPH", 1);
        // } else if (display_mode == DISPLAY_MODE_DISTANCE_METERS) {
        //     displayNum(distance_rotated, 0, false, 0);
        //     displayStr("NN", 1);
        // }
    }
}

// Setup hall effect sensor interrupt callback
void setup_Hall_GPIO_interrupt(void) {
    nrfx_gpiote_in_config_t hall_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    ret_code_t error_code = nrfx_gpiote_in_init(HALL_PIN, &hall_config, hall_effect_GPIO_callback);
    APP_ERROR_CHECK(error_code);
    nrfx_gpiote_in_event_enable(HALL_PIN, true);
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

// Function for taking sliding average of IMU data arrays
float sliding_averager_float_array(float *input_array_pointer, int array_size) {
    float output = 0;
    for (int i = 0; i < array_size; i++) {
        output += input_array_pointer[i];
    }
    return output / (float)array_size;
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

int main(void) {
    ret_code_t error_code = NRF_SUCCESS; // Don't need to redeclare error_code again

    // Start low frequency clock
    start_lfclock();

    // initialize GPIO driver
    if (!nrfx_gpiote_is_init()) {
        error_code = nrfx_gpiote_init();
    }
    APP_ERROR_CHECK(error_code);

    // Initialize the grove displays
    init_tm1637_display(0);
    init_tm1637_display(1);
    clearDisplay(0);
    clearDisplay(1);

    // Initialize GPIO devices and timers in preparation for run:

    // Setup IMU interrupt
    setup_IMU_interrupt();

    // Setup hall effect sensor
    setup_Hall_GPIO_interrupt();

    // Start hall timer
    init_hall_effect_timer();
    error_code = app_timer_start(hall_velocity_calc, APP_TIMER_TICKS(HALL_EFFECT_TIME_MS), NULL);
    APP_ERROR_CHECK(error_code);

    // Initialize I2C

    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = BUCKLER_SENSORS_SCL; // From Kobuki code
    i2c_config.sda = BUCKLER_SENSORS_SDA;
    i2c_config.frequency = NRF_TWIM_FREQ_400K; // Changed to 400k to match the datasheet for MPU9250
    error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    APP_ERROR_CHECK(error_code);
    nrf_delay_ms(50);



    // Start the IMU and run calibration
    start_IMU_i2c_connection(&twi_mngr_instance);
    calibrate_gyro_and_accel();

    // Init Si7021 Temperature/Humidity Sensor
    si7021_init(&twi_mngr_instance);
    si7021_is_init = 1;
    si7021_reset();

    // calibrate_magnetometer(); // Run to generate magnetometer calibration values
    restore_calibrated_magnetometer_values();

    // Initialize the LEDs
    uint16_t numLEDs = 9;
    led_init(numLEDs, LED_PWM); // assume success
    pattern_init(numLEDs);      // assume success
    pattern_start();


    // Initialize the Grove speech recognizer
    speech_init();

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
    float smooth_lin_y_accel_array[smooth_num] = {0};
    uint16_t smoother_array_index = 0;

    float smoothed_roll = 0;
    float smoothed_lin_y_accel = 0;

    uint16_t IMU_read_counter = 0;

    bool turn_locked = false;
    uint32_t speech_sensor_triggered_time = 0;
    float speech_sensor_triggered_time_diff = 0;    

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
            IMU_read_counter++;
        }

        uint8_t speech_input = speech_read();

        if (speech_input != 255) {
            if(speech_input == VOICE_COMMAND_STOP) {
                voice_recognition_state = BRAKE;
            }
            if(speech_input == VOICE_COMMAND_LEFT) {
                voice_recognition_state = LEFT;
            }
            if(speech_input == VOICE_COMMAND_RIGHT) {
                voice_recognition_state = RIGHT;
            }
            if(speech_input == VOICE_COMMAND_NEXT) {
                display_mode = (display_mode + 1) % NUM_DISPLAY_MODES;
                printf("Display Mode (Next): %d\n", display_mode);
            }
            if(speech_input == VOICE_COMMAND_PREV) {
                display_mode = (display_mode + NUM_DISPLAY_MODES - 1) % NUM_DISPLAY_MODES;
                printf("Display Mode (Prev): %d\n", display_mode);
            }
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
        smooth_lin_y_accel_array[smoother_array_index % smooth_num] = lin_ay;
        smoother_array_index++;

        smoothed_roll = sliding_averager_float_array(smooth_roll_array, smooth_num);
        smoothed_lin_y_accel = sliding_averager_float_array(smooth_lin_y_accel_array, smooth_num);

        if ((IMU_read_counter % 100) == 0) {
            //printf("Smoothed Y Accel: %f\n", smoothed_lin_y_accel);
            // printf("Smoothed roll: %f\n", smoothed_roll); add back

            if (si7021_is_init == 1) {
                float temp = read_temperature();
                float hum = read_humidity();
                //printf("Temperature: %f\n", temp);
                //printf("Humidity: %f\n", hum);

                if (display_mode == DISPLAY_MODE_TEMP) {
                    displayNum(temp, 0, false, 0);
                    displayStr("*F", 1);
                } else if (display_mode == DISPLAY_MODE_HUMIDITY) {
                    displayNum(hum, 0, false, 0);
                    displayStr("*Io", 1);
                }
            }

            __disable_irq();
            // float current_speed = (float)hall_revolution_history[hall_revolution_array_index % 4];
            // float recent_speed_1 = (float)hall_revolution_history[(hall_revolution_array_index - 1) % 4];
            // float recent_speed_2 = (float)hall_revolution_history[(hall_revolution_array_index - 2) % 4];
            // float recent_speed_3 = (float)hall_revolution_history[(hall_revolution_array_index - 3) % 4];

            float current_speed = (float)velocity_reading_history[velocity_reading_array_index % 4];
            float recent_speed_1 = (float)velocity_reading_history[(velocity_reading_array_index - 1) % 4];
            float recent_speed_2 = (float)velocity_reading_history[(velocity_reading_array_index - 2) % 4];
            float recent_speed_3 = (float)velocity_reading_history[(velocity_reading_array_index - 3) % 4];

            __enable_irq();

            float speed_diff = current_speed - ((recent_speed_1 * 0.1) + (recent_speed_2 * 0.4) + (recent_speed_3 * 0.5));
            printf("Weighted Speed Diff: %f\n", speed_diff);

            //displayNum(speed_diff, 2, true, 0);

            
            //displayNum(smoothed_roll, 2, true, 1);


            switch(current_system_state) {
            case IDLE:
                // Show speed and distance to rider
                //CHECK FOR BRAKING SHOULD COME FIRST!!!!
                //displayStr("A--A", 1);
                if ((speed_diff < BRAKING_THRESHOLD) | (voice_recognition_state == BRAKE)) {
                    voice_recognition_state = IDLE;
                    speech_sensor_triggered_time = app_timer_cnt_get();
                    current_system_state = BRAKE;
                } else if ((smoothed_roll > LEFT_THRESHOLD) | (voice_recognition_state == LEFT)) {
                    speech_sensor_triggered_time = app_timer_cnt_get();
                    current_system_state = LEFT;
                } else if ((smoothed_roll < RIGHT_THRESHOLD) | (voice_recognition_state == RIGHT)) {
                    speech_sensor_triggered_time = app_timer_cnt_get();
                    current_system_state = RIGHT;
                }
                break;

            case BRAKE:
                speech_sensor_triggered_time_diff = get_msecs_from_ticks(app_timer_cnt_diff_compute(app_timer_cnt_get(), speech_sensor_triggered_time));
                if (speech_sensor_triggered_time_diff < 3000.0) {
                    break;
                } else if ((speech_sensor_triggered_time_diff > 3000.0) && ((voice_recognition_state == BRAKE) | (speed_diff < -8.0))) {
                    voice_recognition_state = IDLE;
                    speech_sensor_triggered_time = app_timer_cnt_get();
                    break;
                }

                if (smoothed_roll < RIGHT_THRESHOLD) {
                    current_system_state = RIGHT;
                    break;
                } else if (smoothed_roll > LEFT_THRESHOLD) {
                    current_system_state = LEFT;
                    break;
                } else {
                    current_system_state = IDLE;
                }
                break;

            case RIGHT:
                // Flash Left_green
                // Again check for breaking
                //displayStr("A---", 1);
                if (speed_diff < BRAKING_THRESHOLD) {
                    voice_recognition_state = IDLE;
                    turn_locked = false;
                    current_system_state = BRAKE;
                    break;
                }
                if (smoothed_roll < RIGHT_THRESHOLD) {
                    turn_locked = true;
                }

                speech_sensor_triggered_time_diff = get_msecs_from_ticks(app_timer_cnt_diff_compute(app_timer_cnt_get(), speech_sensor_triggered_time));

                if ((voice_recognition_state == RIGHT) && (turn_locked == false) && speech_sensor_triggered_time_diff < 10000.0) {
                    break;
                } else if ((voice_recognition_state == RIGHT) && (turn_locked == false) && speech_sensor_triggered_time_diff > 10000.0) {
                    turn_locked = false;
                    voice_recognition_state = IDLE;
                    current_system_state = IDLE;
                    break;
                }
                /*if (smoothed_roll > 10.0) {
                    turn_locked = false;
                    voice_recognition_state = IDLE;
                    current_system_state = LEFT;
                } else */if (smoothed_roll > -1.0) { // Changed hysteresis for BENCH TESTING!!!
                    turn_locked = false;
                    voice_recognition_state = IDLE;
                    current_system_state = IDLE;
                }
                break;

            case LEFT:
                // Flash Right_green
                // Again check for breaking
                //displayStr("A---", 1);
                if (speed_diff < BRAKING_THRESHOLD) {
                    voice_recognition_state = IDLE;
                    turn_locked = false;
                    current_system_state = BRAKE;
                    break;
                }
                if (smoothed_roll > LEFT_THRESHOLD) {
                    turn_locked = true;
                }

                speech_sensor_triggered_time_diff = get_msecs_from_ticks(app_timer_cnt_diff_compute(app_timer_cnt_get(), speech_sensor_triggered_time));

                if ((voice_recognition_state == LEFT) && (turn_locked == false) && speech_sensor_triggered_time_diff < 10000.0) {
                    break;
                } else if ((voice_recognition_state == LEFT) && (turn_locked == false) && speech_sensor_triggered_time_diff > 10000.0) {
                    turn_locked = false;
                    voice_recognition_state = IDLE;
                    current_system_state = IDLE;
                    break;
                }
                /*if (smoothed_roll < -10.0) {
                    turn_locked = false;
                    voice_recognition_state = IDLE;
                    current_system_state = RIGHT;
                } else */if (smoothed_roll < 1.0) { // Changed hysteresis for BENCH TESTING!!!
                    turn_locked = false;
                    voice_recognition_state = IDLE;
                    current_system_state = IDLE;
                }
                break;
            }
            pattern_update_state(current_system_state);
        }

        previous_time = current_time;
    }
}

