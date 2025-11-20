/*!
 * kw7m.cpp - Library for KW7M Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "kw7m.h"

// Register Address
namespace KW7M_REG {
	constexpr uint16_t RATE                           = 0x0032;
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER_L      = 0x0064;
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER_H      = 0x0065;
	constexpr uint16_t L1_CURRENT                     = 0x006B;
	constexpr uint16_t L2_CURRENT                     = 0x006D;
	constexpr uint16_t CT_TYPE                        = 0x003C;
	constexpr uint16_t PULSE_OUTPUT_UNIT_LSB          = 0x003D;
	constexpr uint16_t PULSE_OUTPUT_UNIT_MSB          = 0x003E;
	constexpr uint16_t PRIMARY_SIDE_CURRENT_CT5A      = 0x003F;
	constexpr uint16_t ALARM_VALUE_L                  = 0x0040;
	constexpr uint16_t ALARM_VALUE_H                  = 0x0041;
	constexpr uint16_t VT_RATIO                       = 0x0042;
	constexpr uint16_t CUTOFF_CURRENT                 = 0x0044;
	constexpr uint16_t CURRENT_ALARM                  = 0x0045;
	constexpr uint16_t STAND_BY_CURRENT_ALARM         = 0x004D;
	constexpr uint16_t STAND_BY_TIME_ALARM            = 0x004E;
	constexpr uint16_t VOLTAGE_1_2_L                  = 0x00AA;
	constexpr uint16_t VOLTAGE_1_2_H                  = 0x00AB;
	constexpr uint16_t VOLTAGE_2_3_L                  = 0x00AC;
	constexpr uint16_t VOLTAGE_2_3_H                  = 0x00AD;
	constexpr uint16_t INSTANTANEOUS_ELECTRIC_POWER_L = 0x00B0;
	constexpr uint16_t INSTANTANEOUS_ELECTRIC_POWER_H = 0x00B1;
}

constexpr uint8_t SUCCESS = 0;

bool KW7M::begin(uint8_t id, uint32_t timeout_ms) {
	uint16_t origin_value               = 0;
	uint16_t tmp                        = 0;
	uint16_t VALIDATION_VALUE = 9999;
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);

	// backup current value
	err = modbus.readHoldingRegister(id,
	                                 KW7M_REG::RATE,
	                                 &origin_value,
	                                 1);
	if (err != SUCCESS) {
		// Serial.println("Failed Backup");
		return false;
	}
	// printf("Rate: %d\n", origin_value);
	delay(50);

	// write test value
	err = modbus.writeHoldingRegister(id,
	                                  KW7M_REG::RATE,
	                                  &VALIDATION_VALUE,
	                                  1);
	if (err != SUCCESS) {
		// Serial.printf("Failed Write: %d\n", err);
		return false;
	}
	delay(50);

	// read test value
	modbus.readHoldingRegister(id, KW7M_REG::RATE, &tmp, 1);
	if (err != SUCCESS) {
		// Serial.println("Failed Read");
		return false;
	}
	delay(50);

	// read value is correct?
	if (tmp != VALIDATION_VALUE) {
		// Serial.printf("Failed check: %d\n", tmp);
		modbus.writeHoldingRegister(id,
		                            KW7M_REG::RATE,
		                            &origin_value,
		                            1);
		return false;
	}
	delay(50);

	// restore first value
	err = modbus.writeHoldingRegister(id,
	                                  KW7M_REG::RATE,
	                                  &origin_value,
	                                  1);
	if (err != SUCCESS) {
		// Serial.println("Failed restore");
		return false;
	}
	delay(50);

	return true;
}

// 積算電力値の取得
bool KW7M::fetchIntegralEP(uint8_t id, double *data) {
	uint16_t tmp;
	uint32_t val = 0;
	uint8_t err;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KW7M_REG::INTEGRAL_ELECTRIC_POWER_H,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val = tmp;
	val = val << 16;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KW7M_REG::INTEGRAL_ELECTRIC_POWER_L,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val += tmp;

	*data = val;
	*data /= 100.0;

	return true;
}

// 瞬時電力値を取得
bool KW7M::fetchInstantaneousEP(uint8_t id, double *data) {
	uint16_t tmp;
	uint32_t val = 0;
	uint8_t err;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KW7M_REG::INSTANTANEOUS_ELECTRIC_POWER_H,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val = tmp;
	val = val << 16;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KW7M_REG::INSTANTANEOUS_ELECTRIC_POWER_L,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val += tmp;

	*data = val;
	*data /= 100.0;

	return true;
}