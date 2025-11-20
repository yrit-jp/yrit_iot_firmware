#ifndef DS2485_H
#define DS2485_H

#include "I2C_OneWire.h"
#include <Arduino.h>

class DS2485 : public I2C_OneWire {
public:
	// DS2485() : I2C_OneWire(){};
	DS2485(TwoWire *wire, uint8_t address) : I2C_OneWire(wire, address) {}

	void read_bytes(uint8_t *buf, uint16_t count) override;
	void select(const uint8_t rom[8]) override;
	void skip() override;

	void reset_search() override;
	bool search(uint8_t *newAddr, bool search_mode = true) override;

	void setRPUP();
	bool deviceReset();
	bool readProtectionPageStatus(uint8_t *buf);
	bool readDeviceVersion(uint16_t &result);
	bool readManufacturerId(uint16_t &result);

	// uint8_t test() {
	// 	return wireReset();
	// }

private:
	uint8_t _reg_1wire_search = 0x00;
	uint8_t _st               = 0x00; // 1-wire master status result

	uint8_t wireReset();
	uint8_t wireReadStatus() { return _st; }
	void wireWriteByte(uint8_t data);
	uint8_t wireReadByte();
	void wireWriteBit(uint8_t v);
	uint8_t wireReadBit();
	void wireWriteTriplet(uint8_t v);
	void busyWait() {};

	bool getOneWireBusy(uint8_t status) { return false; }
	uint8_t getSingleBitRead(uint8_t status);
	uint8_t getTripletSecondBit(uint8_t status);
	uint8_t getBranchDirectionTaken(uint8_t status);
	uint8_t getPresencePulseDetect(uint8_t status);

	bool readDeviceStatus(uint8_t command, uint8_t *data);
};

#endif