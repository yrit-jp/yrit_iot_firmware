#ifndef I2C_CT_UNIT_H
#define I2C_CT_UNIT_H

#include "Arduino.h"
#include "Wire.h"

class I2C_CT_Unit {
public:
	uint8_t DEVICE_ADDRESS;
	bool init(TwoWire *wire = &Wire, uint8_t address = 0x3F);
	String getDeviceName();
	bool fetchMilliAmpere(uint32_t *data);
	bool fetchAdRef(uint16_t *data);
	bool fetchAdVal(uint16_t *data);

private:
	TwoWire *_wire;

	const uint8_t addr_milli_ampere = 0x00;
	const uint8_t addr_adval        = 0x04;
	const uint8_t addr_adref        = 0x06;
	const uint8_t addr_who_am_i     = 0x08;

	typedef union {
		struct {
			uint32_t milli_ampere; // 4byte
			uint16_t ad_val;
			uint16_t ad_ref;
			uint8_t who_am_i; // 1byte
		};
		uint8_t packet[9];
	} DataReg;
	DataReg rx_data;
};

#endif