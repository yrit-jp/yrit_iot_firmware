#include "TSL2561.h"

bool TSL2561::begin(uint8_t device_address) {
	DEVICE_ADDRESS = device_address;
	uint8_t id     = 0x00;
	if (!readRegister(TSL2561REG_ID, &id)) {
		return false;
	}

	switch (id & 0xF0) {
	case 0b00000000:
		// TSL2560CS
		// fallthrough
	case 0b00010000:
		// TSL2561CS
		_package_type = TYPE_CS_PACKAGE;
		break;
	case 0b01000000:
		// TSL2560T
		// fallthrough
	case 0b01010000:
		// TSL2561T
		_package_type = TYPE_T_PACKAGE;
		break;
	default:
		return false;
	}

	// Power ON
	if (!writeRegister(TSL2561REG_CONTROL, 0x03)) {
		return false;
	}

	// Gain x1
	// 13ms cycle
	_gain   = GAIN_X1;
	_i_time = INT_13MS;
	if (!writeRegister(TSL2561REG_TIMING, 0x00)) {
		return false;
	}

	// Disable interrupt
	if (!writeRegister(TSL2561REG_INTERRUPT, 0x00)) {
		return false;
	}

	// Power OFF
	return writeRegister(TSL2561REG_CONTROL, 0x00);
}

bool TSL2561::readVisibleLux(uint32_t *lux) {
	// Power ON
	if (!writeRegister(TSL2561REG_CONTROL, 0x03)) {
		return false;
	}

	delay(14); // Wait integration time
	if (!getData()) {
		return false;
	}

	// Power OFF
	if (!writeRegister(TSL2561REG_CONTROL, 0x00)) {
		return false;
	}

	if (_ch1.data == 0) {
		*lux = 0;
		return true;
	}
	if (_ch0.data / _ch1.data < 2 && _ch0.data > 4900) {
		return false; // ch0 out of range, but ch1 not. the lux is not valid in this situation.
	}

	*lux = calculateLux(_gain, _i_time, _package_type);
	return true;
}

/****************************************************
 * Private functions
 * **************************************************/

bool TSL2561::readRegister(int address, uint8_t *value) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(address); // register to read
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	uint8_t num = _wire->requestFrom((uint8_t)DEVICE_ADDRESS, (uint8_t)1, (uint8_t) true);
	if (num != 1) {
		return false;
	}
	while (_wire->available()) {
		*value = _wire->read();
	}
	return true;
}

bool TSL2561::writeRegister(int address, uint8_t value) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(address); // register to read
	_wire->write(value);   // send value to write
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}
	return true;
}

bool TSL2561::getData() {
	if (!readRegister(TSL2561REG_DATA0LOW, &_ch0.BYTE.low)) {
		return false;
	}
	if (!readRegister(TSL2561REG_DATA0HIGH, &_ch0.BYTE.high)) {
		return false;
	}

	if (!readRegister(TSL2561REG_DATA1LOW, &_ch1.BYTE.low)) {
		return false;
	}
	if (!readRegister(TSL2561REG_DATA1HIGH, &_ch1.BYTE.high)) {
		return false;
	}
	return true;
}

uint32_t TSL2561::calculateLux(GAIN gain, INTEGRATION_TIME i_time, PACKAGE_TYPE package) {
	uint32_t chScale, b, m;

	switch (i_time) {
	case INT_13MS: // 13.7 msec
		chScale = CHSCALE_TINT0;
		break;
	case INT_101MS: // 101 msec
		chScale = CHSCALE_TINT1;
		break;
	default: // assume no scaling
		chScale = (1 << CH_SCALE);
		break;
	}
	if (gain == GAIN_X16) {
		chScale = chScale << 4; // scale 1X to 16X
	}
	// scale the channel values
	uint32_t channel0 = (_ch0.data * chScale) >> CH_SCALE;
	uint32_t channel1 = (_ch1.data * chScale) >> CH_SCALE;

	uint32_t ratio1 = 0;
	if (channel0 != 0) {
		ratio1 = (channel1 << (RATIO_SCALE + 1)) / channel0;
	}
	// round the ratio value
	unsigned long ratio = (ratio1 + 1) >> 1;

	switch (package) {
	case TYPE_T_PACKAGE: // T package
		if ((ratio >= 0) && (ratio <= K1T)) {
			b = B1T;
			m = M1T;
		} else if (ratio <= K2T) {
			b = B2T;
			m = M2T;
		} else if (ratio <= K3T) {
			b = B3T;
			m = M3T;
		} else if (ratio <= K4T) {
			b = B4T;
			m = M4T;
		} else if (ratio <= K5T) {
			b = B5T;
			m = M5T;
		} else if (ratio <= K6T) {
			b = B6T;
			m = M6T;
		} else if (ratio <= K7T) {
			b = B7T;
			m = M7T;
		} else if (ratio > K8T) {
			b = B8T;
			m = M8T;
		}
		break;
	case TYPE_CS_PACKAGE: // CS package
		if ((ratio >= 0) && (ratio <= K1C)) {
			b = B1C;
			m = M1C;
		} else if (ratio <= K2C) {
			b = B2C;
			m = M2C;
		} else if (ratio <= K3C) {
			b = B3C;
			m = M3C;
		} else if (ratio <= K4C) {
			b = B4C;
			m = M4C;
		} else if (ratio <= K5C) {
			b = B5C;
			m = M5C;
		} else if (ratio <= K6C) {
			b = B6C;
			m = M6C;
		} else if (ratio <= K7C) {
			b = B7C;
			m = M7C;
		}
	}

	uint32_t temp = ((channel0 * b) - (channel1 * m));
	if (temp < 0) {
		temp = 0;
	}
	temp += (1 << (LUX_SCALE - 1));
	// strip off fractional portion
	uint32_t lux = temp >> LUX_SCALE;
	return (lux);
}