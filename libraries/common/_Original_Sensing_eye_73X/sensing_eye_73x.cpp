/*!
 * sensing_eye_73x.cpp - Library for Sensing Eye 73X Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "sensing_eye_73x.h"

// Register Address
namespace SENSING_EYE_73X_REG {
	constexpr uint16_t WATER_QUARITY_RAW = 0x0000; // 30001
	constexpr uint16_t WATER_TEMPERATURE = 0x0001; // 30002
	constexpr uint16_t UNIT_ADDRESS      = 0x0015; // 40022
	constexpr uint16_t TX_MODE           = 0x0016; // 40023
}

constexpr uint8_t SUCCESS = 0;

bool SENSING_EYE_73X::begin(uint8_t id, uint32_t timeout_ms) {
	uint16_t unit_address              = 0;
	uint16_t tx_mode                   = 0;
	constexpr uint16_t PROTOCOL_MODBUS = 0x0001;
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);
	
	// Read protocol
	err = modbus.readHoldingRegister(id,
	                                 SENSING_EYE_73X_REG::TX_MODE,
	                                 &tx_mode,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	// HAL.printf("Protocol: 0x%04X\n", tx_mode);
	if (tx_mode != PROTOCOL_MODBUS) {
		return false;
	}
	delay(50);

	// Read buffer size
	modbus.readHoldingRegister(id,
	                           SENSING_EYE_73X_REG::UNIT_ADDRESS,
	                           &unit_address,
	                           1);
	if (err != SUCCESS) {
		return false;
	}
	// HAL.printf("Unit Address: 0x%04X\n", unit_address);
	if (unit_address != (uint16_t)id) {
		return false;
	}
	delay(50);

	return true;
}

bool SENSING_EYE_73X::fetchWaterQuality(uint8_t id, int32_t *data) {
	uint16_t tmp;
	uint8_t err;

	delay(50);
	err = modbus.readInputRegister(id,
	                               SENSING_EYE_73X_REG::WATER_QUARITY_RAW,
	                               &tmp,
	                               1);
	if (err != SUCCESS) {
		return false;
	}

	*data = tmp;

	return true;
}

bool SENSING_EYE_73X::fetchWaterTemperature(uint8_t id, double *data) {
	uint16_t tmp;
	uint8_t err;

	delay(50);
	err = modbus.readInputRegister(id,
	                               SENSING_EYE_73X_REG::WATER_TEMPERATURE,
	                               &tmp,
	                               1);
	if (err != SUCCESS) {
		return false;
	}

	*data = tmp;
	*data /= 10.0;

	return true;
}