// Blink app
//
// Blinks the LEDs on Buckler

#include "grove_display.h"

const int DIGITS = 4;

uint32_t clock_pin;
uint32_t data_pin;

static int8_t tube_tab[] =  {0x3f, 0x06, 0x5b, 0x4f,
                            0x66, 0x6d, 0x7d, 0x07,
                            0x7f, 0x6f, 0x77, 0x7c,
                            0x39, 0x5e, 0x79, 0x71
                            }; //0~9,A,b,C,d,E,F

uint8_t char2segments(char c) {
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


void init_tm1637(uint32_t clock_pin_set, uint32_t data_pin_set) {
    clock_pin = clock_pin_set;
    data_pin = data_pin_set;

    ret_code_t error_code = NRF_SUCCESS;

    // initialize GPIO driver
    if (!nrfx_gpiote_is_init()) {
        error_code = nrfx_gpiote_init();
    }
    APP_ERROR_CHECK(error_code);

    // configure grove pins
    // manually-controlled (simple) output, initially set
    nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
    
    error_code = nrfx_gpiote_out_init(clock_pin, &out_config);
    APP_ERROR_CHECK(error_code);
    error_code = nrfx_gpiote_out_init(data_pin, &out_config);
    APP_ERROR_CHECK(error_code);

    clockPinOutput();
    dataPinOutput();
}

int writeByte(int8_t wr_data) {
  for (uint8_t i = 0; i < 8; i++) // Sent 8bit data
  {
    clockPinLow();

    if (wr_data & 0x01)
      dataPinHigh(); // LSB first
    else
      dataPinLow();

    wr_data >>= 1;
    clockPinHigh();
  }

  clockPinLow(); // Wait for the ACK
  dataPinHigh();
  clockPinHigh();
  dataPinInput();

  bitDelay();
  uint8_t ack = nrf_gpio_pin_read(data_pin);

  if (ack == 0)
  {
    dataPinOutput();
    dataPinLow();
  }

  bitDelay();
  dataPinOutput();
  bitDelay();

  return ack;
}

void start() {
  clockPinHigh();
  dataPinHigh();
  dataPinLow();
  clockPinLow();
}

void stop() {
  clockPinLow();
  dataPinLow();
  clockPinHigh();
  dataPinHigh();
}

void display(uint8_t bit_addr, int8_t disp_data) {
  int8_t seg_data;

  seg_data = coding(disp_data);
  start();               // Start signal sent to TM1637 from MCU
  uint8_t ADDR_FIXED = 0x44;
  writeByte(ADDR_FIXED); // Command1: Set data
  stop();
  start();
  writeByte(bit_addr | 0xc0); // Command2: Set data (fixed address)
  writeByte(seg_data);        // Transfer display data 8 bits
  stop();
  start();
  uint8_t cmd_disp_ctrl = 0x88 + 5;
  writeByte(cmd_disp_ctrl); // Control display
  stop();
}

void displayNum(float num, int decimal, bool show_minus) {
  // Displays number with decimal places (no decimal point implementation)
  // Colon is used instead of decimal point if decimal == 2
  // Be aware of int size limitations (up to +-2^15 = +-32767)

  int number = fabs(num) * pow(10, decimal);

  for (int i = 0; i < DIGITS - (show_minus && num < 0 ? 1 : 0); ++i)
  {
    int j = DIGITS - i - 1;

    if (number != 0)
      display(j, number % 10);
    else
      display(j, 0x7f); // display nothing

    number /= 10;
  }

  if (show_minus && num < 0)
    display(0, '-'); // Display '-'

  if (decimal == 2)
    point(true);
  else
    point(false);
}

void displayStr(const char str[])
{
  for (int i = 0; i < (int)(strlen(str)); i++) {
    if (i + 1 > DIGITS) {
      nrf_delay_ms(500); //loop delay
      for (int d = 0; d < DIGITS; d++) {
        display(d, str[d + i + 1 - DIGITS]); //loop display
      }
    } else {
      display(i, str[i]);
    }
  }

  // display nothing
  for (int i = strlen(str); i < DIGITS; i++) {
    display(i, 0x7f);
  }
}

void clearDisplay() {
  display(0x00, 0x7f);
  display(0x01, 0x7f);
  display(0x02, 0x7f);
  display(0x03, 0x7f);
}

void point(bool PointFlag) {
  _PointFlag = PointFlag;
}

void coding2(int8_t disp_data[]) {
  for (uint8_t i = 0; i < DIGITS; i++)
    disp_data[i] = coding(disp_data[i]);
}

int8_t coding(int8_t disp_data) {
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

void bitDelay() {
  nrf_delay_us(50);
}

void clockPinLow() {
  nrf_gpio_pin_clear(clock_pin);
}

void clockPinHigh() {
  nrf_gpio_pin_set(clock_pin);
}

void dataPinLow() {
  nrf_gpio_pin_clear(data_pin);
}

void dataPinHigh() {
  nrf_gpio_pin_set(data_pin);
}

void clockPinOutput() {
  nrf_gpio_pin_dir_set(clock_pin, NRF_GPIO_PIN_DIR_OUTPUT);
}

void clockPinInput() {
  nrf_gpio_pin_dir_set(clock_pin, NRF_GPIO_PIN_DIR_INPUT);
}

void dataPinOutput() {
  nrf_gpio_pin_dir_set(data_pin, NRF_GPIO_PIN_DIR_OUTPUT);
}

void dataPinInput() {
  nrf_gpio_pin_dir_set(data_pin, NRF_GPIO_PIN_DIR_INPUT);
}