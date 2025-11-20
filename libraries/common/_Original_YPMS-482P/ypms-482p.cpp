/*!
 * ypms-482p.cpp - Library for YPMS-482P Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "ypms-482p.h"
#include <utility>

// Register Address
namespace YPMS_482P_REG {
	constexpr uint16_t MEASUREMENT_VALUE = 0x0002;
	constexpr uint16_t PRODUCT_NAME      = 0x7000;
}

constexpr uint8_t SUCCESS = 0;

bool YPMS_482P::begin(uint8_t id, uint32_t timeout_ms) {
	constexpr uint16_t READ_REG_NUM = 0x10;
	char pname[33]                  = {0x00};
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);
	
	// Read device info
	err = modbus.readHoldingRegister(id,
	                                 YPMS_482P_REG::PRODUCT_NAME,
	                                 (uint16_t *)pname,
	                                 READ_REG_NUM);
	if (err != SUCCESS) {
		// Serial.printf("ERROR %d\n", (int)err);
		return false;
	}
	for (int i = 0; i < 32; i += 2) {
		std::swap(pname[i], pname[i + 1]);
	}
	// HAL.printf("\n%s\n", pname);

	if (strcmp(pname, "YPMS-482P") == 0) {
		return true;
	}

	return false;
}

// 測定値取得
bool YPMS_482P::fetchPh(uint8_t id, double *data) {
	constexpr uint16_t READ_REG_NUM = 0x02;
	union c2f {
		uint8_t tmp[4];
		float val;
	};
	union c2f res;
	uint8_t err;

	// 初期化
	memset(res.tmp, 0x00, 4);

	delay(50);
	err = modbus.readHoldingRegister(id,
	                                 YPMS_482P_REG::MEASUREMENT_VALUE,
	                                 (uint16_t *)res.tmp,
	                                 READ_REG_NUM);
	if (err != SUCCESS) {
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
