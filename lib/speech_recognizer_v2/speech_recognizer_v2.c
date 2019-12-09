#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_drv_power.h"
#include "nrf_serial.h"
#include "app_timer.h"

#include "app_error.h"
#include "app_util.h"

#include "buckler.h"

#include "speech_recognizer_v2.h"

#define OP_QUEUES_SIZE          3
#define APP_TIMER_PRESCALER     NRF_SERIAL_APP_TIMER_PRESCALER

NRF_SERIAL_DRV_UART_CONFIG_DEF(uart_config,
                               BUCKLER_UART_RX, BUCKLER_UART_TX,
                               0, 0,
                               NRF_UART_HWFC_ENABLED, NRF_UART_PARITY_EXCLUDED,
                               NRF_UART_BAUDRATE_9600,
                               UART_DEFAULT_CONFIG_IRQ_PRIORITY);

NRF_SERIAL_CONFIG_DEF(serial_config, NRF_SERIAL_MODE_POLLING,
                      NULL, NULL, NULL, NULL);

NRF_SERIAL_UART_DEF(serial_uart, 0);

void start_grove_speech_recognizer(void) {
	// Don't need to do this for the final version
    ret_code_t ret;

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

    nrf_drv_clock_lfclk_request(NULL);
    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    //

    ret = nrf_serial_init(&serial_uart, &uart_config, &serial_config);
    APP_ERROR_CHECK(ret);
    printf("Init is done\n");
}

uint8_t read_voice_command(void) {
	ret_code_t ret;
	uint8_t input_number = 0;
	ret = nrf_serial_read(&serial_uart, &input_number, sizeof(input_number), NULL, 0);

	if(ret == NRF_SUCCESS) {
		return input_number;
	} else {
		printf("%i\n", ret);
		return 255;
	}

}