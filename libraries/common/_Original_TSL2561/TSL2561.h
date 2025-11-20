#ifndef TSL2561_H
#define TSL2561_H

#include "Arduino.h"
#include "Wire.h"

class TSL2561 {
public:
	TSL2561(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}
	bool begin(uint8_t device_address = 0x29);
	bool readVisibleLux(uint32_t *lux);

private:
	TwoWire *_wire;
	union Channel_Data {
		uint16_t data;
		struct {
			uint8_t low;
			uint8_t high;
		} BYTE;
	};
	union Channel_Data _ch0;
	union Channel_Data _ch1;

	uint8_t DEVICE_ADDRESS;
	enum PARAMETERS {
		LUX_SCALE     = 14,     // scale by 2^14
		RATIO_SCALE   = 9,      // scale ratio by 2^9
		CH_SCALE      = 10,     // scale channel values by 2^10
		CHSCALE_TINT0 = 0x7517, // 322/11 * 2^CH_SCALE
		CHSCALE_TINT1 = 0x0fe7  // 322/81 * 2^CH_SCALE
	};

	// COMMAND(0x80) | register;
	enum REGISTERS {
		TSL2561REG_CONTROL        = 0x80,
		TSL2561REG_TIMING         = 0x81,
		TSL2561REG_THRESHLOWLOW   = 0x82,
		TSL2561REG_THRESHLOWHIGH  = 0x83,
		TSL2561REG_THRESHHIGHLOW  = 0x84,
		TSL2561REG_THRESHHIGHHIGH = 0x85,
		TSL2561REG_INTERRUPT      = 0x86,
		TSL2561REG_CRC            = 0x88,
		TSL2561REG_ID             = 0x8A,
		TSL2561REG_DATA0LOW       = 0x8C,
		TSL2561REG_DATA0HIGH      = 0x8D,
		TSL2561REG_DATA1LOW       = 0x8E,
		TSL2561REG_DATA1HIGH      = 0x8F
	};

	enum CONST_VALUE {
		K1T = 0x0040, // 0.125 * 2^RATIO_SCALE
		B1T = 0x01f2, // 0.0304 * 2^LUX_SCALE
		M1T = 0x01be, // 0.0272 * 2^LUX_SCALE
		K2T = 0x0080, // 0.250 * 2^RATIO_SCA
		B2T = 0x0214, // 0.0325 * 2^LUX_SCALE
		M2T = 0x02d1, // 0.0440 * 2^LUX_SCALE
		K3T = 0x00c0, // 0.375 * 2^RATIO_SCALE
		B3T = 0x023f, // 0.0351 * 2^LUX_SCALE
		M3T = 0x037b, // 0.0544 * 2^LUX_SCALE
		K4T = 0x0100, // 0.50 * 2^RATIO_SCALE
		B4T = 0x0270, // 0.0381 * 2^LUX_SCALE
		M4T = 0x03fe, // 0.0624 * 2^LUX_SCALE
		K5T = 0x0138, // 0.61 * 2^RATIO_SCALE
		B5T = 0x016f, // 0.0224 * 2^LUX_SCALE
		M5T = 0x01fc, // 0.0310 * 2^LUX_SCALE
		K6T = 0x019a, // 0.80 * 2^RATIO_SCALE
		B6T = 0x00d2, // 0.0128 * 2^LUX_SCALE
		M6T = 0x00fb, // 0.0153 * 2^LUX_SCALE
		K7T = 0x029a, // 1.3 * 2^RATIO_SCALE
		B7T = 0x0018, // 0.00146 * 2^LUX_SCALE
		M7T = 0x0012, // 0.00112 * 2^LUX_SCALE
		K8T = 0x029a, // 1.3 * 2^RATIO_SCALE
		B8T = 0x0000, // 0.000 * 2^LUX_SCALE
		M8T = 0x0000, // 0.000 * 2^LUX_SCALE

		K1C = 0x0043, // 0.130 * 2^RATIO_SCALE
		B1C = 0x0204, // 0.0315 * 2^LUX_SCALE
		M1C = 0x01ad, // 0.0262 * 2^LUX_SCALE
		K2C = 0x0085, // 0.260 * 2^RATIO_SCALE
		B2C = 0x0228, // 0.0337 * 2^LUX_SCALE
		M2C = 0x02c1, // 0.0430 * 2^LUX_SCALE
		K3C = 0x00c8, // 0.390 * 2^RATIO_SCALE
		B3C = 0x0253, // 0.0363 * 2^LUX_SCALE
		M3C = 0x0363, // 0.0529 * 2^LUX_SCALE
		K4C = 0x010a, // 0.520 * 2^RATIO_SCALE
		B4C = 0x0282, // 0.0392 * 2^LUX_SCALE
		M4C = 0x03df, // 0.0605 * 2^LUX_SCALE
		K5C = 0x014d, // 0.65 * 2^RATIO_SCALE
		B5C = 0x0177, // 0.0229 * 2^LUX_SCALE
		M5C = 0x01dd, // 0.0291 * 2^LUX_SCALE
		K6C = 0x019a, // 0.80 * 2^RATIO_SCALE
		B6C = 0x0101, // 0.0157 * 2^LUX_SCALE
		M6C = 0x0127, // 0.0180 * 2^LUX_SCALE
		K7C = 0x029a, // 1.3 * 2^RATIO_SCALE
		B7C = 0x0037, // 0.00338 * 2^LUX_SCALE
		M7C = 0x002b, // 0.00260 * 2^LUX_SCALE
		K8C = 0x029a, // 1.3 * 2^RATIO_SCALE
		B8C = 0x0000, // 0.000 * 2^LUX_SCALE
		M8C = 0x0000  // 0.000 * 2^LUX_SCALE
	};

	enum PACKAGE_TYPE {
		TYPE_T_PACKAGE  = 0,
		TYPE_CS_PACKAGE = 1
	};
	enum PACKAGE_TYPE _package_type;

	enum GAIN {
		GAIN_X1  = 0,
		GAIN_X16 = 1
	};
	enum GAIN _gain;

	enum INTEGRATION_TIME {
		INT_13MS  = 0,
		INT_101MS = 1,
		INT_402MS = 3
	};
	enum INTEGRATION_TIME _i_time;
	
	bool readRegister(int address, uint8_t *value);
	bool writeRegister(int address, uint8_t value);
	bool getData();
	uint32_t calculateLux(GAIN gain, INTEGRATION_TIME i_time, PACKAGE_TYPE package);
};

#endif /* TSL2561_H */