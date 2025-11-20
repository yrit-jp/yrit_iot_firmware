/*!
 * km_n1_flk.cpp - Library for KM-N1-FLK Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "km_n1_flk.h"

// Register Address
namespace KM_N1_FLK_REG {
	constexpr uint16_t PROTOCOL_H                    = 0x2200;
	constexpr uint16_t PROTOCOL_L                    = 0x2201;
	constexpr uint16_t BUFFER_SIZE_H                 = 0x240A;
	constexpr uint16_t BUFFER_SIZE_L                 = 0x240B;
	constexpr uint16_t ACTIVE_POWER_H                = 0x0010;
	constexpr uint16_t ACTIVE_POWER_L                = 0x0011;
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER_WH_H  = 0x0200;
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER_WH_L  = 0x0201;
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER_KWH_H = 0x0220;
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER_KWH_L = 0x0221;
}

constexpr uint8_t SUCCESS = 0;

bool KM_N1_FLK::begin(uint8_t id, uint32_t timeout_ms) {
	uint16_t protocol                  = 0;
	uint16_t buffer_size               = 0;
	constexpr uint16_t FLK_BUFFER_SIZE = 0x00E6;
	constexpr uint16_t PROTOCOL_MODBUS = 0x0001;
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);
	
	// Read protocol
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::PROTOCOL_L,
	                                 &protocol,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	// HAL.printf("Protocol: 0x%04X\n", protocol);
	if (protocol != PROTOCOL_MODBUS) {
		return false;
	}
	delay(50);

	// Read buffer size
	modbus.readHoldingRegister(id,
	                           KM_N1_FLK_REG::BUFFER_SIZE_L,
	                           &buffer_size,
	                           1);
	if (err != SUCCESS) {
		return false;
	}
	// HAL.printf("Buffer Size: 0x%04X\n", buffer_size);
	if (buffer_size != FLK_BUFFER_SIZE) {
		return false;
	}
	delay(50);

	return true;
}

// 積算有効電力量の取得
bool KM_N1_FLK::fetchIntegralEP(uint8_t id, double *data) {
	uint16_t tmp;
	uint32_t val = 0;
	uint8_t err;

	// kWh データの取得
	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::INTEGRAL_ELECTRIC_POWER_KWH_H,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val = tmp;
	val = val << 16;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::INTEGRAL_ELECTRIC_POWER_KWH_L,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val += tmp;

	*data = val;

	// 小数点以下（Wh）のデータ取得
	val = 0;
	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::INTEGRAL_ELECTRIC_POWER_WH_H,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val = tmp;
	val = val << 16;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::INTEGRAL_ELECTRIC_POWER_WH_L,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val += tmp;

	// kWh 表示の時の小数点以下を算出＆結合
	*data += (val % 1000) * 0.001;

	return true;
}

bool KM_N1_FLK::fetchActivePower(uint8_t id, double *data) {
	uint16_t tmp;
	uint32_t val = 0;
	uint8_t err;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::ACTIVE_POWER_H,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val = tmp;
	val = val << 16;

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 KM_N1_FLK_REG::ACTIVE_POWER_L,
	                                 &tmp,
	                                 1);
	if (err != SUCCESS) {
		return false;
	}
	val += tmp;

	*data = val;
	*data /= 10.0;

	return true;
}