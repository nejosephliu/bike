// Blink app
//
// Blinks the LEDs on Buckler

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


#include "nrf_drv_clock.h"
#include "nrf_uarte.h"
#include "nrf_serial.h"
#include "app_timer.h"

#include "buckler.h"

#include "grove_display.h"
#include "buckler_accelerometer.h"

#include "mpu9250.h"

//extern const nrf_serial_t * serial_ref;

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

NRF_SERIAL_DRV_UART_CONFIG_DEF(m_uart0_drv_config_2,
                      BUCKLER_UART_RX, BUCKLER_UART_TX,
                      0, 0,
                      NRF_UART_HWFC_DISABLED, NRF_UART_PARITY_EXCLUDED,
                      NRF_UART_BAUDRATE_9600,
                      UART_DEFAULT_CONFIG_IRQ_PRIORITY);

  #define SERIAL_FIFO_TX_SIZE 32
  #define SERIAL_FIFO_RX_SIZE 32

  NRF_SERIAL_QUEUES_DEF(serial_queues_2, SERIAL_FIFO_TX_SIZE, SERIAL_FIFO_RX_SIZE);

  #define SERIAL_BUFF_TX_SIZE 1
  #define SERIAL_BUFF_RX_SIZE 1

NRF_SERIAL_BUFFERS_DEF(serial_buffs_2, SERIAL_BUFF_TX_SIZE, SERIAL_BUFF_RX_SIZE);

NRF_SERIAL_CONFIG_DEF(serial_config_2, NRF_SERIAL_MODE_POLLING,
                        NULL, NULL, NULL, NULL);
//                      &serial_queues_2, &serial_buffs_2, NULL, NULL);


NRF_SERIAL_UART_DEF(serial_uart_2, 0);

const nrf_serial_t * serial_ref_2 = &serial_uart_2;

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  init_tm1637(0);
  init_tm1637(1);

  clearDisplay(0);
  clearDisplay(1);
  
  displayStr("boP", 0);
  displayStr("HEL0", 1);
  
  nrf_delay_ms(1000);

  init_accelerometer();


  error_code = nrf_serial_init(&serial_uart_2, &m_uart0_drv_config_2, &serial_config_2);
  printf("First error code: %ld\n", error_code);
      APP_ERROR_CHECK(error_code);

  nrf_delay_ms(1000);
  

  // initialization complete
  printf("Buckler initialized!\n");
    //uint8_t header_buf[2];
    //int status = 0;
    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = BUCKLER_SENSORS_SCL;
    i2c_config.sda = BUCKLER_SENSORS_SDA;
    i2c_config.frequency = NRF_TWIM_FREQ_100K;
    error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    APP_ERROR_CHECK(error_code);
    mpu9250_init(&twi_mngr_instance);
    printf("IMU initialized!\n");
    mpu9250_start_gyro_integration();

    //float x_cancel_rate = 0.18955;
    //float x_cancel = x_cancel_rate;

    //uint32_t i = 0;

    nrf_delay_ms(1000);
    printf("1\n");
    nrf_delay_ms(1000);
    printf("Before Read\n");

    

    char c;
    error_code = nrf_serial_read(serial_ref_2, &c, sizeof(c), NULL, 5000);
    printf("error code: %ld, success: %u\n", error_code, NRF_SUCCESS);
    printf("After Read\n");


    while (true) {
        error_code = nrf_serial_read(serial_ref_2, &c, sizeof(c), NULL, 2000);
        printf("error code: %ld, success: %u\n", error_code, NRF_SUCCESS);
        nrf_delay_ms(2000);
        printf("error code: %ld, success: %u\n", error_code, NRF_SUCCESS);
        printf("error base: %d\n", NRF_ERROR_BASE_NUM);
        printf("COMMON error base: %u\n", NRF_ERROR_SDK_COMMON_ERROR_BASE);
        printf("NRF INVALID STATE: %u\n", NRF_ERROR_INVALID_STATE);
        printf("module not initialized: %u\n", NRF_ERROR_MODULE_NOT_INITIALIZED);
        printf("\nCHAR C: %c\n", c);
        printf("\n");
        if (error_code != NRF_SUCCESS)
        {
            continue;
        } else {

        }
    }


    /*while (true)
    {
        char c;
        error_code = nrf_serial_read(serial_ref_2, &c, sizeof(c), NULL, 5000);
        i += 1;
        if (error_code != NRF_SUCCESS)
        {
            printf("Error! %d\n", i);
            printf("Char: %c\n", c);
            //continue;
        } else {
          printf("No error! %d\n", i);
          printf("Char: %c\n", c);
          break;
        }
        
        //(void)nrf_serial_write(&serial_uart, &c, sizeof(c), NULL, 0);
        //(void)nrf_serial_flush(&serial_uart, 0);
        nrf_delay_ms(100);
    }*/

  /*while (1) {
    calculate_values();

    mpu9250_measurement_t measurement = mpu9250_read_gyro_integration();
    float x_angle = measurement.x_axis;
    
    x_angle += x_cancel;
    x_cancel += x_cancel_rate;

    if (x_angle < -350) {
        mpu9250_stop_gyro_integration();
        mpu9250_start_gyro_integration();
    }
    
    printf("X ANgle: %f\n", x_angle);
    
    float y_angle = measurement.y_axis;
    printf("Y ANgle: %f\n", y_angle);
    
    float z_angle = measurement.z_axis;
    printf("Z ANgle: %f\n", z_angle);

    float x_val = get_x_g();
    printf("X value: %f\n", x_val);
    float y_val = get_y_g();
    printf("Y value: %f\n", y_val);
    float z_val = get_z_g();
    printf("Z value: %f\n", z_val);

    float x_degree = get_x_degree();
    displayNum(x_degree, 0, true, 0);

    float y_degree = get_y_degree();

    if (y_degree > 17) {
        displayStr("---A", 1);
    } else if (y_degree < -17) {
        displayStr("A---", 1);
    } else {
        displayNum(y_degree, 0, true, 1);
    }
    /*printf("1\n");
    status = nrf_serial_read(serial_ref, header_buf, sizeof(header_buf), NULL, 1000);
    if(status != NRF_SUCCESS) {
        printf("error: %d\n", status);        
    } else {
        printf("success!\n");
    }
    printf("2\n");

    nrf_delay_ms(50);
  }*/
}
