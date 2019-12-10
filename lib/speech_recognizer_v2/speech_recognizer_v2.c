#include <stdint.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_serial.h"
#include "app_timer.h"
#include "nrfx_clock.h"

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

// Assumes app timer has already been started
void speech_init(void) {
    ret_code_t ret = nrf_serial_init(&serial_uart, &uart_config, &serial_config);
    APP_ERROR_CHECK(ret);
}

uint8_t speech_read(void) {
    ret_code_t ret;
    uint8_t input_number = 0;
    ret = nrf_serial_read(&serial_uart, &input_number, sizeof(input_number), NULL, 0);

    if(ret == NRF_SUCCESS) {
        return input_number;
    } else {
        return 255;
    }
    
}

const char *speech_convert_reading(uint8_t reading) {
    switch (reading) {
    case 0x01:
        return "Turn on the light";
    case 0x02:
        return "Turn off the light";
    case 0x03:
        return "Play music";
    case 0x04:
        return "Pause";
    case 0x05:
        return "Next";
    case 0x06:
        return "Previous";
    case 0x07:
        return "Up";
    case 0x08:
        return "Down";
    case 0x09:
        return "Turn on the TV";
    case 0x0a:
        return "Turn off the TV";
    case 0x0b:
        return "Increase Temperature";
    case 0x0c:
        return "Decrease Temperature";
    case 0x0d:
        return "What's the time";
    case 0x0e:
        return "Open the door";
    case 0x0f:
        return "Close the door";
    case 0x10:
        return "Left";
    case 0x11:
        return "Right";
    case 0x12:
        return "Stop";
    case 0x13:
        return "Start";
    case 0x14:
        return "Mode 1";
    case 0x15:
        return "Mode 2";
    case 0x16:
        return "Go";
    }
    return "Invalid";
}
