#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

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

#include "buckler.h"

#define POINT_ON 1
#define POINT_OFF 0

bool _PointFlag;
void init_tm1637(int port_number);
int writeByte(int8_t wr_data, int port_number);
void start(int port_number);
void stop(int port_number);
void display(uint8_t BitAddr, int8_t DispData, int port_number);
void displayNum(float num, int decimal, bool show_minus, int port_number);
void displayStr(const char str[], int port_number);
void clearDisplay(int port_number);
void point(bool PointFlag);
void coding2(int8_t DispData[]);
int8_t coding(int8_t DispData);
void bitDelay();
uint8_t char2segments(char c);
void clockPinLow(int port_number);
void clockPinHigh(int port_number);
void dataPinLow(int port_number);
void dataPinHigh(int port_number);
void clockPinOutput(int port_number);
void clockPinInput(int port_number);
void dataPinOutput(int port_number);
void dataPinInput(int port_number);
void setBrightness(uint8_t new_brightness);