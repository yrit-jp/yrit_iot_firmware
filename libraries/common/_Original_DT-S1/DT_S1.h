#ifndef DT_S1_H
#define DT_S1_H

#include <Arduino.h>
#include <Wire.h>

class DT_S1 {
public:
	uint8_t DEVICE_ADDRESS;

	DT_S1(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}
	bool begin(uint8_t address = 0x09);
	bool getTemperature(double *temperature);

private:
	TwoWire *_wire;

};

#endif /* DT_S1_H */