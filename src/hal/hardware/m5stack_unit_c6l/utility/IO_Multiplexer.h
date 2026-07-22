#if defined(ARDUINO_M5STACK_UNIT_C6L)

#ifndef IO_MULTIPLEXER_H
#define IO_MULTIPLEXER_H

#include "PI4IOE5V6408.h" // IO Expander
#include <Arduino.h>

namespace IO_Multiplexer {
	bool initialize(TwoWire &two_wire);
	int readPin(uint8_t pin);
	void writePin(uint8_t pin, bool state);
	void enableLNA(bool state);
	void switchRF(bool state);
	void setNRST(bool state);
}

#endif // IO_MULTIPLEXER_H
#endif // HAL_API_M5STACK_UNIT_C6L_H