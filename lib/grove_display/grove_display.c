#include "grove_display.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_gpiote.h"
#include "nrf_gpio.h"

#include "buckler.h"

const int DIGITS = 4;

uint32_t clock_pin_0 = BUCKLER_GROVE_A0;
uint32_t data_pin_0 = BUCKLER_GROVE_A1;

uint32_t clock_pin_1 = BUCKLER_GROVE_D0;
uint32_t data_pin_1 = BUCKLER_GROVE_D1;

uint8_t brightness = 7;

static int8_t tube_tab[] =  {0x3f, 0x06, 0x5b, 0x4f,
                            0x66, 0x6d, 0x7d, 0x07,
                            0x7f, 0x6f, 0x77, 0x7c,
                            0x39, 0x5e, 0x79, 0x71
                            }; //0~9,A,b,C,d,E,F

static uint8_t char2segments(char c) {
  switch (c)
  {
    case '_' : return 0x08;
    case '^' : return 0x01; // ¯
    case '-' : return 0x40;
    case '*' : return 0x63; // °
    case ' ' : return 0x00; // space
    case 'A' : return 0x77;
    case 'b' : return 0x7c;
    case 'c' : return 0x58;
    case 'C' : return 0x39;
    case 'd' : return 0x5e;
    case 'E' : return 0x79;
    case 'F' : return 0x71;
    case 'h' : return 0x74;
    case 'H' : return 0x76;
    case 'I' : return 0x30;
    case 'J' : return 0x0e;
    case 'L' : return 0x38;
    case 'n' : return 0x54;
    case 'N' : return 0x37; // like ∩
    case 'o' : return 0x5c;
    case 'P' : return 0x73;
    case 'q' : return 0x67;
    case 'u' : return 0x1c;
    case 'U' : return 0x3e;
    case 'y' : return 0x66; // =4
  }
  return 0;
}

static int8_t coding(int8_t disp_data) {
  if (disp_data == 0x7f)
    disp_data = 0x00; // Clear digit
  else if (disp_data >= 0 && disp_data < (int)(sizeof(tube_tab)/sizeof(*tube_tab)))
    disp_data = tube_tab[disp_data];
  else if ( disp_data >= '0' && disp_data <= '9' )
    disp_data = tube_tab[(int)(disp_data)-48]; // char to int (char "0" = ASCII 48)
  else
    disp_data = char2segments(disp_data);
  disp_data += _PointFlag == POINT_ON ? 0x80 : 0;

  return disp_data;
}

// static void coding2(int8_t disp_data[]) {
//   for (uint8_t i = 0; i < DIGITS; i++)
//     disp_data[i] = coding(disp_data[i]);
// }

static void bitDelay() {
  nrf_delay_us(50);
}

static void clockPinLow(int port_number) {
    uint32_t clock_pin;
    if (port_number == 0) {
        clock_pin = clock_pin_0;
    } else {
        clock_pin = clock_pin_1;
    }
    nrf_gpio_pin_clear(clock_pin);
}

static void clockPinHigh(int port_number) {
    uint32_t clock_pin;
    if (port_number == 0) {
        clock_pin = clock_pin_0;
    } else {
        clock_pin = clock_pin_1;
    }
    nrf_gpio_pin_set(clock_pin);
}

static void dataPinLow(int port_number) {
    uint32_t data_pin;
    if (port_number == 0) {
        data_pin = data_pin_0;
    } else {
        data_pin = data_pin_1;
    }
    nrf_gpio_pin_clear(data_pin);
}

static void dataPinHigh(int port_number) {
    uint32_t data_pin;
    if (port_number == 0) {
        data_pin = data_pin_0;
    } else {
        data_pin = data_pin_1;
    }
    nrf_gpio_pin_set(data_pin);
}

static void clockPinOutput(int port_number) {
    uint32_t clock_pin;
    if (port_number == 0) {
        clock_pin = clock_pin_0;
    } else {
        clock_pin = clock_pin_1;
    }
    nrf_gpio_pin_dir_set(clock_pin, NRF_GPIO_PIN_DIR_OUTPUT);
}

// static void clockPinInput(int port_number) {
//     uint32_t clock_pin;
//     if (port_number == 0) {
//         clock_pin = clock_pin_0;
//     } else {
//         clock_pin = clock_pin_1;
//     }
//     nrf_gpio_pin_dir_set(clock_pin, NRF_GPIO_PIN_DIR_INPUT);
// }

static void dataPinOutput(int port_number) {
    uint32_t data_pin;
    if (port_number == 0) {
        data_pin = data_pin_0;
    } else {
        data_pin = data_pin_1;
    }
    nrf_gpio_pin_dir_set(data_pin, NRF_GPIO_PIN_DIR_OUTPUT);
}

static void dataPinInput(int port_number) {
    uint32_t data_pin;
    if (port_number == 0) {
        data_pin = data_pin_0;
    } else {
        data_pin = data_pin_1;
    }
    nrf_gpio_pin_dir_set(data_pin, NRF_GPIO_PIN_DIR_INPUT);
}


void init_tm1637_display(int port_number) {
    printf("Initializing Grove 4 Digit Display.\n");

    ret_code_t error_code = NRF_SUCCESS;

    // initialize GPIO driver
    if (!nrfx_gpiote_is_init()) {
        error_code = nrfx_gpiote_init();
    }
    APP_ERROR_CHECK(error_code);

    // configure grove pins
    // manually-controlled (simple) output, initially set
    nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
    
    uint32_t data_pin;
    uint32_t clock_pin;
    if (port_number == 0) {
        data_pin = data_pin_0;
        clock_pin = clock_pin_0;
    } else {
        data_pin = data_pin_1;
        clock_pin = clock_pin_1;
    }
    
    error_code = nrfx_gpiote_out_init(clock_pin, &out_config);
    APP_ERROR_CHECK(error_code);
    error_code = nrfx_gpiote_out_init(data_pin, &out_config);
    APP_ERROR_CHECK(error_code);

    clockPinOutput(port_number);
    dataPinOutput(port_number);
}

static int writeByte(int8_t wr_data, int port_number) {
  for (uint8_t i = 0; i < 8; i++) {  // Sent 8bit data
    clockPinLow(port_number);

    if (wr_data & 0x01)
      dataPinHigh(port_number); // LSB first
    else
      dataPinLow(port_number);

    wr_data >>= 1;
    clockPinHigh(port_number);
  }

  clockPinLow(port_number); // Wait for the ACK
  dataPinHigh(port_number);
  clockPinHigh(port_number);
  dataPinInput(port_number);

  bitDelay();
  uint32_t data_pin;
  if (port_number == 0) {
      data_pin = data_pin_0;
  } else {
      data_pin = data_pin_1;
  }
  uint8_t ack = nrf_gpio_pin_read(data_pin);

  if (ack == 0) {
    dataPinOutput(port_number);
    dataPinLow(port_number);
  }

  bitDelay();
  dataPinOutput(port_number);
  bitDelay();

  return ack;
}

static void start(int port_number) {
  clockPinHigh(port_number);
  dataPinHigh(port_number);
  dataPinLow(port_number);
  clockPinLow(port_number);
}

static void stop(int port_number) {
  clockPinLow(port_number);
  dataPinLow(port_number);
  clockPinHigh(port_number);
  dataPinHigh(port_number);
}

void display(uint8_t bit_addr, int8_t disp_data, int port_number) {
  int8_t seg_data;

  seg_data = coding(disp_data);
  start(port_number);               // Start signal sent to TM1637 from MCU
  uint8_t ADDR_FIXED = 0x44;
  writeByte(ADDR_FIXED, port_number); // Command1: Set data
  stop(port_number);
  start(port_number);
  writeByte(bit_addr | 0xc0, port_number); // Command2: Set data (fixed address)
  writeByte(seg_data, port_number);        // Transfer display data 8 bits
  stop(port_number);
  start(port_number);
  uint8_t cmd_disp_ctrl = 0x88 + brightness;
  writeByte(cmd_disp_ctrl, port_number); // Control display
  stop(port_number);
}

void displayNum(float num, int decimal, bool show_minus, int port_number) {
  // Displays number with decimal places (no decimal point implementation)
  // Colon is used instead of decimal point if decimal == 2
  // Be aware of int size limitations (up to +-2^15 = +-32767)

  int number = fabs(num) * pow(10, decimal);

  for (int i = 0; i < DIGITS - (show_minus && num < 0 ? 1 : 0); ++i) {
    int j = DIGITS - i - 1;

    if (number != 0)
      display(j, number % 10, port_number);
    else
      display(j, 0x7f, port_number); // display nothing

    number /= 10;
  }

  if (show_minus && num < 0)
    display(0, '-', port_number); // Display '-'

  if (decimal == 2)
    point(true);
  else
    point(false);
}

void displayStr(const char str[], int port_number) {
  for (int i = 0; i < (int)(strlen(str)); i++) {
    if (i + 1 > DIGITS) {
      nrf_delay_ms(500); //loop delay
      for (int d = 0; d < DIGITS; d++) {
        display(d, str[d + i + 1 - DIGITS], port_number); //loop display
      }
    } else {
      display(i, str[i], port_number);
    }
  }

  // display nothing
  for (int i = strlen(str); i < DIGITS; i++) {
    display(i, 0x7f, port_number);
  }
}

void clearDisplay(int port_number) {
  display(0x00, 0x7f, port_number);
  display(0x01, 0x7f, port_number);
  display(0x02, 0x7f, port_number);
  display(0x03, 0x7f, port_number);
}

void point(bool PointFlag) {
  _PointFlag = PointFlag;
}

void setBrightness(uint8_t new_brightness) {
    brightness = new_brightness;
}
