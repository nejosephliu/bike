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
#include "nrf_drv_clock.h"

#include "buckler.h"

#define OP_QUEUES_SIZE          3
#define APP_TIMER_PRESCALER     NRF_SERIAL_APP_TIMER_PRESCALER

static void sleep_handler(void)
{
    __WFE();
    __SEV();
    __WFE();
}

NRF_SERIAL_DRV_UART_CONFIG_DEF(m_uart0_drv_config5,
                      BUCKLER_UART_RX, BUCKLER_UART_TX,
                      0, 0,
                      NRF_UART_HWFC_ENABLED, NRF_UART_PARITY_EXCLUDED,
                      NRF_UART_BAUDRATE_9600,
                      UART_DEFAULT_CONFIG_IRQ_PRIORITY);

#define SERIAL_FIFO_TX_SIZE 32
#define SERIAL_FIFO_RX_SIZE 32

NRF_SERIAL_QUEUES_DEF(serial_queues5, SERIAL_FIFO_TX_SIZE, SERIAL_FIFO_RX_SIZE);

#define SERIAL_BUFF_TX_SIZE 1
#define SERIAL_BUFF_RX_SIZE 1

NRF_SERIAL_BUFFERS_DEF(serial_buffs5, SERIAL_BUFF_TX_SIZE, SERIAL_BUFF_RX_SIZE);

NRF_SERIAL_CONFIG_DEF(serial_config5, NRF_SERIAL_MODE_DMA,
                      &serial_queues5, &serial_buffs5, NULL, sleep_handler);

NRF_SERIAL_UART_DEF(serial_uart5, 0);

int main(void) {
  ret_code_t ret;

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

    nrf_drv_clock_lfclk_request(NULL);
    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    ret = nrf_serial_init(&serial_uart5, &m_uart0_drv_config5, &serial_config5);
    APP_ERROR_CHECK(ret);

    while (true)
    {
        char c;
        ret = nrf_serial_read(&serial_uart5, &c, sizeof(c), NULL, 5000);
        if (ret != NRF_SUCCESS)
        {
          if (ret == NRF_ERROR_INVALID_STATE) {
            printf("Fail: NRF_ERROR_INVALID_STATE\n");
          } else {
            printf("Fail: %lu\n", ret);
          }
        } else {
          printf("Success! %lu\n", ret);
          printf("char: %i\n", c);
          nrf_delay_ms(500);
        }
      nrf_delay_ms(50);
    }
}