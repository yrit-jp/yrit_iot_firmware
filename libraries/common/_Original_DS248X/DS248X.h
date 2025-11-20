#ifndef DS248X_H
#define DS248X_H

#include <Arduino.h>
#include "I2C_OneWire.h"

class DS248X : public I2C_OneWire {
public:
	DS248X() : I2C_OneWire() {};
	DS248X(TwoWire *wire, uint8_t address) : I2C_OneWire(wire, address) {};
private:
	uint8_t wireReset();
	uint8_t wireReadStatus();
	void wireWriteByte(uint8_t data);
	uint8_t wireReadByte();
	void wireWriteBit(uint8_t v);
	uint8_t wireReadBit();
	void wireWriteTriplet(uint8_t v);
	void busyWait();

	bool getOneWireBusy(uint8_t status);
	uint8_t getSingleBitRead(uint8_t status);
	uint8_t getTripletSecondBit(uint8_t status);
	uint8_t getBranchDirectionTaken(uint8_t status);
	uint8_t getPresencePulseDetect(uint8_t status);
};

#endif