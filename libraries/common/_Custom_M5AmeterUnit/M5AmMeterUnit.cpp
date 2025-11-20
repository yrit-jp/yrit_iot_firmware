/*! ==============================================================
 * @file: M5AmMeterUnit.h
 * @author: M5Stack
 * @modified: takkaO
 * @license: MIT License
 * @reference: https://github.com/m5stack/M5-ProductExampleCodes/tree/1bc38736e61a362080d1dffc4020b3a2a1dff6ae/Unit/A_Meter_Unit/ammeter
 * ============================================================= */

#include "M5AmMeterUnit.h"

#define ADS1115_MV_6144 0.187500F
#define ADS1115_MV_4096 0.125000F
#define ADS1115_MV_2048 0.062500F // default
#define ADS1115_MV_1024 0.031250F
#define ADS1115_MV_512 0.015625F
#define ADS1115_MV_256 0.007813F

#define ADS1115_PGA_6144_CAL_ADDR 208
#define ADS1115_PGA_4096_CAL_ADDR 216
#define ADS1115_PGA_2048_CAL_ADDR 224
#define ADS1115_PGA_1024_CAL_ADDR 232
#define ADS1115_PGA_512_CAL_ADDR 240
#define ADS1115_PGA_256_CAL_ADDR 248

M5AmMeterUnit::M5AmMeterUnit(TwoWire *theWire) {
	_wire = theWire;
}

void M5AmMeterUnit::setGain(uint8_t gain) {
	resolution = getResolution(gain);

	int16_t hope   = 1;
	int16_t actual = 1;
	if (readCalibrationFromEEPROM(gain, &hope, &actual)) {
		calibration_factor = fabs((double)hope / actual);
	}
}

double M5AmMeterUnit::calculateAmpere(int16_t ad_value) {
	constexpr int MEASURING_DIR = -1;
	if (ad_value < 0) {
		ad_value *= MEASURING_DIR;
	}
	return resolution * calibration_factor * ad_value;
}

double M5AmMeterUnit::getResolution(uint8_t gain) {
	const double AMMETER_PRESSURE_COEFFICIENT = 0.05;

	switch (gain) {
	case ADS1115_PGA_6144:
		return ADS1115_MV_6144 / AMMETER_PRESSURE_COEFFICIENT;
	case ADS1115_PGA_4096:
		return ADS1115_MV_4096 / AMMETER_PRESSURE_COEFFICIENT;
	case ADS1115_PGA_2048:
		return ADS1115_MV_2048 / AMMETER_PRESSURE_COEFFICIENT;
	case ADS1115_PGA_1024:
		return ADS1115_MV_1024 / AMMETER_PRESSURE_COEFFICIENT;
	case ADS1115_PGA_512:
		return ADS1115_MV_512 / AMMETER_PRESSURE_COEFFICIENT;
	case ADS1115_PGA_256:
		return ADS1115_MV_256 / AMMETER_PRESSURE_COEFFICIENT;
	default:
		return ADS1115_MV_256 / AMMETER_PRESSURE_COEFFICIENT;
	};
}

/*! @brief Get EEPROM address
    @return EEPROM address.. */
uint8_t M5AmMeterUnit::getPGAEEEPROMAddr(uint8_t gain) {
	switch (gain) {
	case ADS1115_PGA_6144:
		return ADS1115_PGA_6144_CAL_ADDR;
	case ADS1115_PGA_4096:
		return ADS1115_PGA_4096_CAL_ADDR;
	case ADS1115_PGA_2048:
		return ADS1115_PGA_2048_CAL_ADDR;
	case ADS1115_PGA_1024:
		return ADS1115_PGA_1024_CAL_ADDR;
	case ADS1115_PGA_512:
		return ADS1115_PGA_512_CAL_ADDR;
	case ADS1115_PGA_256:
		return ADS1115_PGA_256_CAL_ADDR;
	default:
		return 0x00;
	};
}

bool M5AmMeterUnit::readCalibrationFromEEPROM(uint8_t gain,
                                              int16_t *hope,
                                              int16_t *actual) {
	uint8_t addr = getPGAEEEPROMAddr(gain);
	uint8_t buff[8];
	memset(buff, 0, 8);

	*hope   = 1;
	*actual = 1;

	// bool result = EEPORMRead(addr, buff, 8);
	bool result = i2cReadBytes(0x51, addr, buff, 8);

	if (result == false) {
		return false;
	}

	uint8_t xor_result = 0x00;
	for (uint8_t i = 0; i < 5; i++) {
		xor_result ^= buff[i];
	}

	if (xor_result != buff[5]) {
		return false;
	}

	*hope   = (buff[1] << 8) | buff[2];
	*actual = (buff[3] << 8) | buff[4];
	return true;
}

/*! @brief Read a certain length of data to the specified register address.
    @return True if the read was successful, otherwise false.. */
bool M5AmMeterUnit::i2cReadBytes(uint8_t addr,
                                 uint8_t reg_addr,
                                 uint8_t *buff,
                                 uint16_t len) {
	_wire->beginTransmission(addr);
	_wire->write(reg_addr);
	uint8_t i = 0;
	if (_wire->endTransmission(false) == 0 &&
	    _wire->requestFrom(addr, (uint8_t)len)) {
		while (_wire->available()) {
			buff[i++] = _wire->read();
		}
		return true;
	}

	return false;
}

/*! @brief Write a certain length of data to the specified register address.
    @return True if the write was successful, otherwise false.. */
bool M5AmMeterUnit::i2cWriteBytes(uint8_t addr,
                                  uint8_t reg_addr,
                                  uint8_t *buff,
                                  uint16_t len) {
	bool function_result = false;

	_wire->beginTransmission(addr);
	_wire->write(reg_addr);
	for (int i = 0; i < len; i++) {
		_wire->write(*(buff + i));
	}
	function_result = (_wire->endTransmission() == 0);
	return function_result;
}

/*! @brief Read a certain length of 16-bit data to the specified register
   address.
    @return True if the read was successful, otherwise false.. */
bool M5AmMeterUnit::i2cReadU16(uint8_t addr, uint8_t reg_addr, uint16_t *value) {
	uint8_t read_buf[2] = {0x00, 0x00};
	bool result         = i2cReadBytes(addr, reg_addr, read_buf, 2);
	*value              = (read_buf[0] << 8) | read_buf[1];
	return result;
}

/*! @brief Writes a certain length of 16-bit data to the specified register
   address.
    @return True if the read was successful, otherwise false.. */
bool M5AmMeterUnit::i2cWriteU16(uint8_t addr, uint8_t reg_addr, uint16_t value) {
	uint8_t write_buf[2];
	write_buf[0] = value >> 8;
	write_buf[1] = value & 0xff;
	return i2cWriteBytes(addr, reg_addr, write_buf, 2);
}