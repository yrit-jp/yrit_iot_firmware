/*!
 * ypms-482d.cpp - Library for YPMS-482D Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "ypms-482d.h"
#include <utility>

// Register Address
namespace YPMS_482D_REG {
	constexpr uint16_t TEMPERATURE_VALUE      = 0x0006; // 温度
	constexpr uint16_t DO_VALUE               = 0x0012; // 溶存酸素量
	constexpr uint16_t O2_CONCENTRATION_VALUE = 0x0014; // 酸素濃度
	constexpr uint16_t O2_SATURATION_VALUE    = 0x0016; // 酸素飽和度
	constexpr uint16_t PRESSURE_VALUE         = 0x0018; // 大気圧
	constexpr uint16_t PRODUCT_NAME           = 0x7000;
}

constexpr uint8_t SUCCESS = 0;

bool YPMS_482D::begin(uint8_t id, uint32_t timeout_ms) {
	constexpr uint16_t READ_REG_NUM = 0x10;
	char pname[33]                  = {0x00};
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);

	// Read device info
	err = modbus.readHoldingRegister(id,
	                                 YPMS_482D_REG::PRODUCT_NAME,
	                                 (uint16_t *)pname,
	                                 READ_REG_NUM);
	if (err != SUCCESS) {
		// Serial.printf("ERROR %d\n", (int)err);
		return false;
	}
	for (int i = 0; i < 32; i += 2) {
		std::swap(pname[i], pname[i + 1]);
	}
	// Serial.printf("\n%s\n", pname);

	if (strcmp(pname, "YPMS-482D") == 0) {
		return true;
	}

	return false;
}

// DO 測定値取得
bool YPMS_482D::fetchDO(uint8_t id, double *data) {
	return fetchValue(YPMS_482D_REG::DO_VALUE, id, data);
}

// 酸素濃度 測定値取得
bool YPMS_482D::fetchOxygenConcentration(uint8_t id, double *data) {
	return fetchValue(YPMS_482D_REG::O2_CONCENTRATION_VALUE, id, data);
}

// 酸素飽和度 測定値取得
bool YPMS_482D::fetchOxygenSaturation(uint8_t id, double *data) {
	return fetchValue(YPMS_482D_REG::O2_SATURATION_VALUE, id, data);
}

// 大気圧 測定値取得
bool YPMS_482D::fetchPressure(uint8_t id, double *data) {
	return fetchValue(YPMS_482D_REG::PRESSURE_VALUE, id, data);
}

// 温度 測定値取得
bool YPMS_482D::fetchTemperature(uint8_t id, double *data) {
	return fetchValue(YPMS_482D_REG::TEMPERATURE_VALUE, id, data);
}

/* ===================================================== */

// 測定値取得
bool YPMS_482D::fetchValue(const uint16_t target_register, uint8_t id, double *data) {
	constexpr uint16_t READ_REG_NUM = 0x02;
	union c2f {
		uint8_t tmp[4];
		float val;
	};
	union c2f res;
	uint8_t err;

	// 初期化
	memset(res.tmp, 0x00, 4);

	delay(100);
	err = modbus.readHoldingRegister(id,
	                                 target_register,
	                                 (uint16_t *)res.tmp,
	                                 READ_REG_NUM);
	if (err != SUCCESS) {
		// Serial.printf("ERROR 0x%02X -> %d\n", target_register, err);
		return false;
	}

	// for (int i=0; i<4; i++) {
	// 	HAL.printf("0x%02X ", res.tmp[i]);
	// }
	// HAL.println("");

	// エンディアン & バイトオーダー の調整
	std::swap(res.tmp[0], res.tmp[2]);
	std::swap(res.tmp[1], res.tmp[3]);

	*data = (double)res.val;

	return true;
}