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
void init_tm1637();
int writeByte(int8_t wr_data);
void start();
void stop();
void display(uint8_t BitAddr, int8_t DispData);
void displayNum(float num, int decimal, bool show_minus);
void displayStr(const char str[]);
void clearDisplay();
void point(bool PointFlag);
void coding2(int8_t DispData[]);
int8_t coding(int8_t DispData);
void bitDelay();
uint8_t char2segments(char c);
void clockPinLow();
void clockPinHigh();
void dataPinLow();
void dataPinHigh();
void clockPinOutput();
void clockPinInput();
void dataPinOutput();
void dataPinInput();