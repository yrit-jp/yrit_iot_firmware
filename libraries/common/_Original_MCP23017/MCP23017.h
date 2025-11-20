#ifndef MCP23017_H
#define MCP23017_H

#include <Arduino.h>
#include <Wire.h>

class MCP23017 {
public:
	uint8_t DEVICE_ADDRESS;

	MCP23017(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}
	bool init(uint8_t address);
	bool getStatusGpioA(uint32_t *data);
	bool getStatusGpioB(uint32_t *data);

private:
	TwoWire *_wire;
};

#endif