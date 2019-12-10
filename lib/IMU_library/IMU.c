// MPU-9250 driver
//
// Read from MPU-9250 3-axis accelerometer/gyro/magnetometer over I2C
// Register documentation: http://www.invensense.com/wp-content/uploads/2017/11/RM-MPU-9250A-00-v1.6.pdf
//
// Based on https://github.com/sparkfun/SparkFun_MPU-9250-DMP_Arduino_Library/blob/master/src/util/inv_mpu.c

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "IMU.h"

static uint8_t MPU_ADDRESS = 0x68;
static uint8_t MAG_ADDRESS = 0x0C;

static const nrf_twi_mngr_t *i2c_manager = NULL;

float factory_mag_sensitivity[3] = {0};
float software_mag_bias[3] = {0};
float software_mag_scale[3] = {0};

static uint8_t i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr) {
    uint8_t rx_buf = 0;
    nrf_twi_mngr_transfer_t const read_transfer[] = {
        NRF_TWI_MNGR_WRITE(i2c_addr, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
        NRF_TWI_MNGR_READ(i2c_addr, &rx_buf, 1, 0),
    };
    ret_code_t error_code = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 2, NULL);
    APP_ERROR_CHECK(error_code);
    return rx_buf;
}

static void i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data) {
    uint8_t buf[2] = {reg_addr, data};
    nrf_twi_mngr_transfer_t const write_transfer[] = {
        NRF_TWI_MNGR_WRITE(i2c_addr, buf, 2, 0),
    };
    ret_code_t error_code = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);
    APP_ERROR_CHECK(error_code);
}

// Function to read multiple bytes from mpu9250 over I2C
static void i2c_reg_read_N_bytes(uint8_t i2c_addr, uint8_t reg_addr, int num_bytes, uint8_t *buffer) {
    nrf_twi_mngr_transfer_t const read_transfer[] = {
        NRF_TWI_MNGR_WRITE(i2c_addr, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
        //NRF_TWI_MNGR_READ(i2c_addr, buffer, num_bytes, NRF_TWI_MNGR_NO_STOP),
        NRF_TWI_MNGR_READ(i2c_addr, buffer, num_bytes, 0),
    };
    ret_code_t error_code = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 2, NULL);
    APP_ERROR_CHECK(error_code);
}

void debug(void) {
    int16_t x_offset = (((uint16_t) i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_XOUT_H)) << 8 | i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_XOUT_L));
    int16_t y_offset = (((uint16_t) i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_YOUT_H)) << 8 | i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_YOUT_L));
    int16_t z_offset = (((uint16_t) i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_ZOUT_H)) << 8 | i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_ZOUT_L));

    printf("x %i\n", x_offset);
    printf("y %i\n", y_offset);
    printf("z %i\n", z_offset);

    uint8_t my_buffer[6] = {0};
    i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_ACCEL_XOUT_H, 6, my_buffer);

    x_offset = (((uint16_t) my_buffer[0]) << 8 | my_buffer[1]);
    y_offset = (((uint16_t) my_buffer[2]) << 8 | my_buffer[3]);
    z_offset = (((uint16_t) my_buffer[4]) << 8 | my_buffer[5]);
    printf("x %i\n", x_offset);
    printf("y %i\n", y_offset);
    printf("z %i\n", z_offset);

    uint8_t WAI = 0;
    i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_WHO_AM_I, 1, &WAI);
    printf("%#x\n", WAI);

}

void calibrate_gyro_and_accel(void) {
    // Delay by 2sec to allow the buckler to "settle" after being restarted
    nrf_delay_ms(2000);

    uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
    uint16_t i, packet_count, fifo_count;
    int32_t gyro_bias[3] = { 0, 0, 0 }, accel_bias[3] = { 0, 0, 0 };

    // reset device
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
    nrf_delay_ms(100);

    // get stable time source; Auto select clock source to be PLL gyroscope reference if ready
    // else use the internal oscillator, bits 2:0 = 001
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_1, 0x01);
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_2, 0x00);
    nrf_delay_ms(100);

    // Configure device for bias calculation
    i2c_reg_write(MPU_ADDRESS, MPU9250_INT_ENABLE, 0x00); // Disable all interrupts
    i2c_reg_write(MPU_ADDRESS, MPU9250_FIFO_EN, 0x00);      // Disable FIFO
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_1, 0x00); // Turn on internal clock source
    i2c_reg_write(MPU_ADDRESS, MPU9250_I2C_MST_CTRL, 0x00); // Disable I2C master
    i2c_reg_write(MPU_ADDRESS, MPU9250_USER_CTRL, 0x00); // Disable FIFO and I2C master modes
    i2c_reg_write(MPU_ADDRESS, MPU9250_USER_CTRL, 0x0C); // Reset FIFO and DMP
    nrf_delay_ms(100);

    // Configure MPU6050 gyro and accelerometer for bias calculation
    i2c_reg_write(MPU_ADDRESS, MPU9250_CONFIG, 0x01); // Set low-pass filter to 188 Hz
    i2c_reg_write(MPU_ADDRESS, MPU9250_SMPLRT_DIV, 0x00); // Set sample rate to 1 kHz
    i2c_reg_write(MPU_ADDRESS, MPU9250_GYRO_CONFIG, 0x00); // Set gyro full-scale to 250 degrees per second, maximum sensitivity
    i2c_reg_write(MPU_ADDRESS, MPU9250_ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity

    uint16_t gyrosensitivity = 131;   // = 131 LSB/degrees/sec
    uint16_t accelsensitivity = 16384;  // = 16384 LSB/g

    // Configure FIFO to capture accelerometer and gyro data for bias calculation
    i2c_reg_write(MPU_ADDRESS, MPU9250_USER_CTRL, 0x40);   // Enable FIFO
    i2c_reg_write(MPU_ADDRESS, MPU9250_FIFO_EN, 0x78); // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
    nrf_delay_ms(40); // accumulate 40 samples in 40 milliseconds = 480 bytes

    // At end of sample accumulation, turn off FIFO sensor read
    i2c_reg_write(MPU_ADDRESS, MPU9250_FIFO_EN, 0x00); // Disable gyro and accelerometer sensors for FIFO
    i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_FIFO_COUNTH, 2, &data[0]); // read FIFO sample count
    fifo_count = ((uint16_t) data[0] << 8) | data[1];
    packet_count = fifo_count / 12; // How many sets of full gyro and accelerometer data for averaging

    for (i = 0; i < packet_count; i++) {
        int16_t accel_temp[3] = { 0, 0, 0 }, gyro_temp[3] = { 0, 0, 0 };
        i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_FIFO_R_W, 12, &data[0]); // read data for averaging
        accel_temp[0] = (int16_t) (((int16_t) data[0] << 8) | data[1]); // Form signed 16-bit integer for each sample in FIFO
        accel_temp[1] = (int16_t) (((int16_t) data[2] << 8) | data[3]);
        accel_temp[2] = (int16_t) (((int16_t) data[4] << 8) | data[5]);
        gyro_temp[0] = (int16_t) (((int16_t) data[6] << 8) | data[7]);
        gyro_temp[1] = (int16_t) (((int16_t) data[8] << 8) | data[9]);
        gyro_temp[2] = (int16_t) (((int16_t) data[10] << 8) | data[11]);

        accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
        accel_bias[1] += (int32_t) accel_temp[1];
        accel_bias[2] += (int32_t) accel_temp[2];
        gyro_bias[0] += (int32_t) gyro_temp[0];
        gyro_bias[1] += (int32_t) gyro_temp[1];
        gyro_bias[2] += (int32_t) gyro_temp[2];

    }
    accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
    accel_bias[1] /= (int32_t) packet_count;
    accel_bias[2] /= (int32_t) packet_count;
    gyro_bias[0] /= (int32_t) packet_count;
    gyro_bias[1] /= (int32_t) packet_count;
    gyro_bias[2] /= (int32_t) packet_count;

    if (accel_bias[2] > 0L) {
        accel_bias[2] -= (int32_t) accelsensitivity;
    }  // Remove gravity from the z-axis accelerometer bias calculation
    else {
        accel_bias[2] += (int32_t) accelsensitivity;
    }

    // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
    data[0] = (-gyro_bias[0] / 4 >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
    data[1] = (-gyro_bias[0] / 4) & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
    data[2] = (-gyro_bias[1] / 4 >> 8) & 0xFF;
    data[3] = (-gyro_bias[1] / 4) & 0xFF;
    data[4] = (-gyro_bias[2] / 4 >> 8) & 0xFF;
    data[5] = (-gyro_bias[2] / 4) & 0xFF;

    // Push gyro biases to hardware registers

    i2c_reg_write(MPU_ADDRESS, MPU9250_XG_OFFSET_H, data[0]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_XG_OFFSET_L, data[1]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_YG_OFFSET_H, data[2]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_YG_OFFSET_L, data[3]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_ZG_OFFSET_H, data[4]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_ZG_OFFSET_L, data[5]);


    // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
    // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
    // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
    // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
    // the accelerometer biases calculated above must be divided by 8.

    int16_t accel_bias_reg[3] = { 0, 0, 0 }; // A place to hold the factory accelerometer trim biases
    int16_t mask_bit[3] = { 1, 1, 1 }; // Define array to hold mask bit for each accelerometer bias axis

    i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_XA_OFFSET_H, 2, &data[0]); // Read factory accelerometer trim values
    accel_bias_reg[0] = ((int16_t) data[0] << 8) | data[1];
    i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_YA_OFFSET_H, 2, &data[0]);
    accel_bias_reg[1] = ((int16_t) data[0] << 8) | data[1];
    i2c_reg_read_N_bytes(MPU_ADDRESS, MPU9250_ZA_OFFSET_H, 2, &data[0]);
    accel_bias_reg[2] = ((int16_t) data[0] << 8) | data[1];

    for (i = 0; i < 3; i++) {
        if (accel_bias_reg[i] % 2) {
            mask_bit[i] = 0;
        }
        accel_bias_reg[i] -= accel_bias[i] >> 3; // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g
        if (mask_bit[i]) {
            accel_bias_reg[i] = accel_bias_reg[i] & ~mask_bit[i]; // Preserve temperature compensation bit
        } else {
            accel_bias_reg[i] = accel_bias_reg[i] | 0x0001; // Preserve temperature compensation bit
        }
    }

    data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
    data[1] = (accel_bias_reg[0]) & 0xFF;
    data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
    data[3] = (accel_bias_reg[1]) & 0xFF;
    data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
    data[5] = (accel_bias_reg[2]) & 0xFF;

    // Push accelerometer biases to hardware registers

    i2c_reg_write(MPU_ADDRESS, MPU9250_XA_OFFSET_H, data[0]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_XA_OFFSET_L, data[1]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_YA_OFFSET_H, data[2]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_YA_OFFSET_L, data[3]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_ZA_OFFSET_H, data[4]);
    i2c_reg_write(MPU_ADDRESS, MPU9250_ZA_OFFSET_L, data[5]);

    // Put the correct settings back on////////
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_1, 0x01);

    // Register write to get the correct sampling rate

    i2c_reg_write(MPU_ADDRESS, MPU9250_CONFIG, 0x03); // 41Hz low pass filter
    i2c_reg_write(MPU_ADDRESS, MPU9250_SMPLRT_DIV, 0x04); // 200Hz update rate

    // enable bypass mode
    i2c_reg_write(MPU_ADDRESS, MPU9250_USER_CTRL, 0x00);
    nrf_delay_ms(3);
    i2c_reg_write(MPU_ADDRESS, MPU9250_INT_PIN_CFG, 0x12); // Default = 0x02
    nrf_delay_ms(3);
    i2c_reg_write(MPU_ADDRESS, MPU9250_INT_ENABLE, 0x01);
    nrf_delay_ms(3);

    // configure gyro range to +/- 2000 degrees per second
    // i2c_reg_write(MPU_ADDRESS, MPU9250_GYRO_CONFIG, 0x18);

    // gyro at +/- 250 */sec
    i2c_reg_write(MPU_ADDRESS, MPU9250_GYRO_CONFIG, 0x00);

    // configure accelerometer range to +/- 2 g
    i2c_reg_write(MPU_ADDRESS, MPU9250_ACCEL_CONFIG, 0x00);

    // reset magnetometer
    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL2, 0x01);
    nrf_delay_ms(100);

    // configure magnetometer, enable continuous measurement mode (8 Hz)
    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL1, 0x16);

}

void calibrate_magnetometer(void) {
    // First we pull the factory calibration values from the mag:

    uint8_t temp_data[7] = {0};

    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL1, 0x00); // Power down magnetometer
    nrf_delay_ms(10);
    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL1, 0x0F); // Enter Fuse ROM access mode
    nrf_delay_ms(10);

    i2c_reg_read_N_bytes(MAG_ADDRESS, AK8963_ASAX, 3, temp_data);  // Read the factory-set bias
    // Convert and save the biases for future use
    factory_mag_sensitivity[0] =  (float)(temp_data[0] - 128) / 256. + 1.;
    factory_mag_sensitivity[1] =  (float)(temp_data[1] - 128) / 256. + 1.;
    factory_mag_sensitivity[2] =  (float)(temp_data[2] - 128) / 256. + 1.;

    uint16_t ii = 0;
    int32_t mag_bias[3] = {0, 0, 0}, mag_scale[3] = {0, 0, 0};
    int16_t mag_max[3] = {-32767, -32767, -32767}, mag_min[3] = {32767, 32767, 32767};

    // Enable continuous read mode at 16-bit resolution
    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL1, 0x16);

    printf("Mag Cal: Wave device in a figure eight until done!\n");
    nrf_delay_ms(4000);

    // Collect 15 seconds of raw data
    // 100Hz collection rate-->10ms/sample-->1,500 samples needed

    int16_t mag_xyz_temp[3] = {0}; // Hold the temporary mag XYZ measurements

    for(ii = 0; ii < 1500; ii++) {

        i2c_reg_read_N_bytes(MAG_ADDRESS, AK8963_HXL, 7, temp_data);
        mag_xyz_temp[0] = (((uint16_t)temp_data[1]) << 8 | temp_data[0]);
        mag_xyz_temp[1] = (((uint16_t)temp_data[3]) << 8 | temp_data[2]);
        mag_xyz_temp[2] = (((uint16_t)temp_data[5]) << 8 | temp_data[4]);

        for (int jj = 0; jj < 3; jj++) {
            if(mag_xyz_temp[jj] > mag_max[jj]) {
                mag_max[jj] = mag_xyz_temp[jj];
            }
            if(mag_xyz_temp[jj] < mag_min[jj]) {
                mag_min[jj] = mag_xyz_temp[jj];
            };
        }
        nrf_delay_ms(10);
    }

    // Get hard iron correction
    mag_bias[0]  = (mag_max[0] + mag_min[0]) / 2; // get average x mag bias in counts
    mag_bias[1]  = (mag_max[1] + mag_min[1]) / 2; // get average y mag bias in counts
    mag_bias[2]  = (mag_max[2] + mag_min[2]) / 2; // get average z mag bias in counts

    float magnetometer_resolution = (10.0 * 4912.0) / 32760.0;

    software_mag_bias[0] = (float) mag_bias[0] * magnetometer_resolution * factory_mag_sensitivity[0]; // save mag biases in G for main program
    software_mag_bias[1] = (float) mag_bias[1] * magnetometer_resolution * factory_mag_sensitivity[1];
    software_mag_bias[2] = (float) mag_bias[2] * magnetometer_resolution * factory_mag_sensitivity[2];

    // Get soft iron correction estimate
    mag_scale[0]  = (mag_max[0] - mag_min[0]) / 2; // get average x axis max chord length in counts
    mag_scale[1]  = (mag_max[1] - mag_min[1]) / 2; // get average y axis max chord length in counts
    mag_scale[2]  = (mag_max[2] - mag_min[2]) / 2; // get average z axis max chord length in counts

    float avg_rad = mag_scale[0] + mag_scale[1] + mag_scale[2];
    avg_rad /= 3.0;

    software_mag_scale[0] = avg_rad / ((float)mag_scale[0]);
    software_mag_scale[1] = avg_rad / ((float)mag_scale[1]);
    software_mag_scale[2] = avg_rad / ((float)mag_scale[2]);
    printf("Factory Bias - X:%f Y:%f Z:%f\n", factory_mag_sensitivity[0], factory_mag_sensitivity[1], factory_mag_sensitivity[2]);
    printf("Software Bias - X:%f Y:%f Z:%f\n", software_mag_bias[0], software_mag_bias[1], software_mag_bias[2]);
    printf("Software Scale - X:%f Y:%f Z:%f\n", software_mag_scale[0], software_mag_scale[1], software_mag_scale[2]);
}

// initialization and configuration
void start_IMU_i2c_connection(const nrf_twi_mngr_t *i2c) {
    i2c_manager = i2c;

    // reset IMU
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_1, 0x80);
    nrf_delay_ms(100);

    // disable sleep mode and set clock source to internal high res PLL clock
    i2c_reg_write(MPU_ADDRESS, MPU9250_PWR_MGMT_1, 0x01);

    // Register write to get the correct sampling rate

    i2c_reg_write(MPU_ADDRESS, MPU9250_CONFIG, 0x03); // 41Hz low pass filter
    i2c_reg_write(MPU_ADDRESS, MPU9250_SMPLRT_DIV, 0x04); // 200Hz update rate

    // enable bypass mode
    i2c_reg_write(MPU_ADDRESS, MPU9250_USER_CTRL, 0x00);
    nrf_delay_ms(3);
    i2c_reg_write(MPU_ADDRESS, MPU9250_INT_PIN_CFG, 0x12); // Default = 0x02
    nrf_delay_ms(3);
    i2c_reg_write(MPU_ADDRESS, MPU9250_INT_ENABLE, 0x01);
    nrf_delay_ms(3);

    // configure gyro range to +/- 2000 degrees per second
    // i2c_reg_write(MPU_ADDRESS, MPU9250_GYRO_CONFIG, 0x18);

    // gyro at +/- 250 */sec
    i2c_reg_write(MPU_ADDRESS, MPU9250_GYRO_CONFIG, 0x00);

    // configure accelerometer range to +/- 2 g
    i2c_reg_write(MPU_ADDRESS, MPU9250_ACCEL_CONFIG, 0x00);

    // reset magnetometer
    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL2, 0x01);
    nrf_delay_ms(100);

    // configure magnetometer, enable continuous measurement mode (8 Hz)
    i2c_reg_write(MAG_ADDRESS, AK8963_CNTL1, 0x16);
}

void restore_calibrated_magnetometer_values(void) {
    factory_mag_sensitivity[0] = 1.214844;
    factory_mag_sensitivity[1] = 1.218750;
    factory_mag_sensitivity[2] = 1.175781;
    
    software_mag_bias[0] = 249.548782;
    software_mag_bias[1] = -180.910721;
    software_mag_bias[2] = -431.923737;
    
    software_mag_scale[0] = 1.231183;
    software_mag_scale[1] = 1.052874;
    software_mag_scale[2] = 0.807760;
}

void read_accelerometer_pointer(float *ax, float *ay, float *az) {
    // read values
    int16_t x_val = (((uint16_t)i2c_reg_read(MPU_ADDRESS, MPU9250_ACCEL_XOUT_H)) << 8) | i2c_reg_read(MPU_ADDRESS, MPU9250_ACCEL_XOUT_L);
    int16_t y_val = (((uint16_t)i2c_reg_read(MPU_ADDRESS, MPU9250_ACCEL_YOUT_H)) << 8) | i2c_reg_read(MPU_ADDRESS, MPU9250_ACCEL_YOUT_L);
    int16_t z_val = (((uint16_t)i2c_reg_read(MPU_ADDRESS, MPU9250_ACCEL_ZOUT_H)) << 8) | i2c_reg_read(MPU_ADDRESS, MPU9250_ACCEL_ZOUT_L);

    // convert to g at +/- 2g resolution

    *ax = ((float)x_val) / 16384;
    *ay = ((float)y_val) / 16384;
    *az = ((float)z_val) / 16384;
}

void read_gyro_pointer(float *gx, float *gy, float *gz) {
    // read values
    int16_t x_val = (((uint16_t)i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_XOUT_H)) << 8) | i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_XOUT_L);
    int16_t y_val = (((uint16_t)i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_YOUT_H)) << 8) | i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_YOUT_L);
    int16_t z_val = (((uint16_t)i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_ZOUT_H)) << 8) | i2c_reg_read(MPU_ADDRESS, MPU9250_GYRO_ZOUT_L);

    // Convert to degrees at +/- 250deg/sec resolution
    *gx = ((float)x_val) / 131;
    *gy = ((float)y_val) / 131;
    *gz = ((float)z_val) / 131;
}

void read_magnetometer_pointer(float *mx, float *my, float *mz) {
    // read data
    // must read 8 bytes starting at the first status register
    uint8_t reg_addr = AK8963_ST1;
    uint8_t rx_buf[8] = {0};
    nrf_twi_mngr_transfer_t const read_transfer[] = {
        NRF_TWI_MNGR_WRITE(MAG_ADDRESS, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
        NRF_TWI_MNGR_READ(MAG_ADDRESS, rx_buf, 8, 0),
    };
    ret_code_t error_code = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 2, NULL);
    APP_ERROR_CHECK(error_code);

    // determine values
    int16_t x_val = (((uint16_t)rx_buf[2]) << 8) | rx_buf[1];
    int16_t y_val = (((uint16_t)rx_buf[4]) << 8) | rx_buf[3];
    int16_t z_val = (((uint16_t)rx_buf[6]) << 8) | rx_buf[5];

    // convert to milligaus --> (1mg = 1000uT)

    float x = x_val * ((10.0 * 4912.0) / 32760.0) * factory_mag_sensitivity[0] - software_mag_bias[0];
    float y = y_val * ((10.0 * 4912.0) / 32760.0) * factory_mag_sensitivity[1] - software_mag_bias[1];
    float z = z_val * ((10.0 * 4912.0) / 32760.0) * factory_mag_sensitivity[2] - software_mag_bias[2];

    *mx = x * software_mag_scale[0];
    *my = y * software_mag_scale[1];
    *mz = z * software_mag_scale[2];
}