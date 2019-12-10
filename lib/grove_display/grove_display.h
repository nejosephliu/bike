#include <stdbool.h>
#include <stdint.h>

// Init Grove Display Module
// Port number refers to the display port to communicate on (either 0 or 1)
void init_tm1637_display(int port_number);

// Displays a char to the correct bit address and port number
void display(uint8_t BitAddr, int8_t DispData, int port_number);

/* Displays a number to Grove
 *
 * INPUTS:
 *   num - int or float to print to Grove
 *   decimal - number of decimal places to print.  Does NOT print the decimal
 *       point, unless decimal = 2, where it uses the colon as the point
 *   show_minus - displays a negative sign if the number is negative.  Note 
 *       that this takes up one position in the display
 *   port_number - the port to display to
 */
void displayNum(float num, int decimal, bool show_minus, int port_number);

// Displays a string to Grove.  Un-representable characters show as empty.
void displayStr(const char str[], int port_number);

void clearDisplay(int port_number);
void setBrightness(uint8_t new_brightness);
