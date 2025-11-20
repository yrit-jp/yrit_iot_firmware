/*!
 * wld_pa.cpp - Library for WLD-PA Modbus Device
 *
 * Copyright (c) 2025 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "wld_pa.h"
#include <utility>

// Register Address
namespace WLD_PA_REG {
	constexpr uint16_t INSTANTANEOUS_ELECTRIC_POWER = 0x0380; // 有効電力（瞬時値）
	constexpr uint16_t INTEGRAL_ELECTRIC_POWER      = 0x0500; // 積算有効電力量（受電）
	constexpr uint16_t PRODUCT_NAME                 = 0x07D0; // 形式読出し
}

constexpr uint8_t SUCCESS = 0;

bool WLD_PA::begin(uint8_t id, uint32_t timeout_ms) {
	constexpr uint16_t READ_REG_NUM = 0x0A;
	char pname[21]                  = {0x00};
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);

	// backup current value
	err = modbus.readInputRegister(id,
	                               WLD_PA_REG::PRODUCT_NAME,
	                               (uint16_t *)pname,
	                               READ_REG_NUM);
	if (err != SUCCESS) {
		// Serial.println("Failed Read");
		return false;
	}

	for (int i = 0; i < 20; i += 2) {
		std::swap(pname[i], pname[i + 1]);
	}
	// Serial.printf("%s\n", pname);

	if (strncmp(pname, "WLD-PA", 6) == 0) {
		return true;
	}

	return false;
}

// 積算電力値の取得
bool WLD_PA::fetchIntegralEP(uint8_t id, double *data) {
	uint16_t tmp[4] = {0x00};
	uint64_t val    = 0;
	uint8_t err;

	delay(50);
	err = modbus.readInputRegister(id,
	                               WLD_PA_REG::INTEGRAL_ELECTRIC_POWER,
	                               (uint16_t *)tmp,
	                               0x04);
	if (err != SUCCESS) {
		// Serial.printf("Err %d\n", err);
		return false;
	}
	val = tmp[3] + ((uint64_t)tmp[2] << 16) + ((uint64_t)tmp[1] << 32) + ((uint64_t)tmp[0] << 48);

	*data = val;
	*data /= 1000.0;

	return true;
}

// 瞬時電力値を取得
bool WLD_PA::fetchInstantaneousEP(uint8_t id, double *data) {
	uint16_t tmp[4] = {0x00};
	int64_t val     = 0;
	uint8_t err;

	delay(50);
	err = modbus.readInputRegister(id,
	                               WLD_PA_REG::INSTANTANEOUS_ELECTRIC_POWER,
	                               (uint16_t *)tmp,
	                               0x04);
	if (err != SUCCESS) {
		// Serial.printf("Err %d\n", err);
		return false;
	}
	val = tmp[3] + ((uint64_t)tmp[2] << 16) + ((uint64_t)tmp[1] << 32) + ((uint64_t)tmp[0] << 48);

	*data = val;
	*data /= 100.0;

	return true;
}