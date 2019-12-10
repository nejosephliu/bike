#pragma once

// Start serial connection with speech recognizer
// Assumes app_timer has already been started
void speech_init(void);

// Return ID number of recognized speech, or -1 (255) if none
uint8_t speech_read(void);

// Converts ID number to the spoken message
const char* speech_convert_reading(uint8_t reading);
