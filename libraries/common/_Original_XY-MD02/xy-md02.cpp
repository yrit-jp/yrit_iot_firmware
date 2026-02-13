// ==============================================================
// xy-md02.cpp - Library for XY-MD02 Modbus Device
// Copyright (c) 2025 takkaO
// This software is released under the MIT License.
// see https://opensource.org/licenses/MIT
// =============================================================

#include "xy-md02.h"

namespace XY_MD02_REG {
	constexpr uint16_t TEMPERATURE            = 0x0001;
	constexpr uint16_t HUMIDITY               = 0x0002;
	constexpr uint16_t DEVICE_ADDRESS         = 0x0101;
	constexpr uint16_t BAUD_RATE              = 0x0102;
	constexpr uint16_t TEMPERATURE_CORRECTION = 0x0103;
	constexpr uint16_t HUMIDITY_CORRECTION    = 0x0104;
}

constexpr uint8_t SUCCESS = 0;

bool XY_MD02::begin(uint8_t id, uint32_t timeout_ms) {
	uint16_t origin_value     = 0;
	uint16_t tmp              = 0;
	uint16_t VALIDATION_VALUE = 9;
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);
	err = modbus.readHoldingRegister(id,
	                                 XY_MD02_REG::TEMPERATURE_CORRECTION,
	                                 &origin_value,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}
	delay(50);

	err = modbus.writeHoldingRegister(id,
	                                  XY_MD02_REG::TEMPERATURE_CORRECTION,
	                                  &VALIDATION_VALUE,
	                                  1);

	if (err != SUCCESS) {
		return false;
	}
	delay(50);

	// read value is correct?
	err = modbus.readHoldingRegister(id,
	                                 XY_MD02_REG::TEMPERATURE_CORRECTION,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	delay(50);

	if (tmp != VALIDATION_VALUE) {
		modbus.writeHoldingRegister(id,
		                            XY_MD02_REG::TEMPERATURE_CORRECTION,
		                            &origin_value,
		                            1);
		return false;
	}
	delay(50);

	// restore original value
	err = modbus.writeHoldingRegister(id,
	                                  XY_MD02_REG::TEMPERATURE_CORRECTION,
	                                  &origin_value,
	                                  1);
	if (err != SUCCESS) {
		return false;
	}
	delay(50);

	return true;
}

bool XY_MD02::fetchTemperature(uint8_t id, double *temperature) {
	uint16_t tmp;
	uint8_t err;

	delay(50);
	err = modbus.readInputRegister(id,
	                               XY_MD02_REG::TEMPERATURE,
	                               &tmp,
	                               1);
	if (err != SUCCESS) {
		return false;
	}

	*temperature = static_cast<double>(tmp) * 0.1; // Convert (/10)

	return true;
}

bool XY_MD02::fetchHumidity(uint8_t id, double *humidity) {
	uint16_t tmp;
	uint8_t err;

	delay(50);
	err = modbus.readInputRegister(id,
	                               XY_MD02_REG::HUMIDITY,
	                               &tmp,
	                               1);
	if (err != SUCCESS) {
		return false;
	}

	*humidity = static_cast<double>(tmp) * 0.1; // Convert (/10)

	return true;
}