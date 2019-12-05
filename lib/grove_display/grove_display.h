#include <stdbool.h>
#include <stdint.h>

#define POINT_ON 1
#define POINT_OFF 0

bool _PointFlag;
void init_tm1637_display(int port_number);

void display(uint8_t BitAddr, int8_t DispData, int port_number);
void displayNum(float num, int decimal, bool show_minus, int port_number);
void displayStr(const char str[], int port_number);

void clearDisplay(int port_number);
void point(bool PointFlag);

void setBrightness(uint8_t new_brightness);
