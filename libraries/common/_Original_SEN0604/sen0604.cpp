/*!
 * sen0604.cpp - Library for DFRobot SEN0604 Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#include "sen0604.h"

// Register Address
namespace SEN0604_REG {
	constexpr uint16_t MOISTURE_CONTENT                     = 0x0000; // 水分含有量
	constexpr uint16_t TEMPERATURE                          = 0x0001; // 温度
	constexpr uint16_t CONDUCTIVITY                         = 0x0002; // 電気伝導率
	constexpr uint16_t PH                                   = 0x0003; // pH
	constexpr uint16_t SALINITY                             = 0x0007; // 塩分濃度
	constexpr uint16_t TDS                                  = 0x0008; // 総溶解固形分(TDS)
	constexpr uint16_t CONDUCTIVITY_TEMPERATURE_COEFFICIENT = 0x0022; // 電気伝導率温度係数
	constexpr uint16_t SALINITY_COEFFICIENT                 = 0x0023; // 塩分係数
	constexpr uint16_t TDS_COEFFICIENT                      = 0x0024; // TDS係数
	constexpr uint16_t TEMPERATURE_CALIBRATION_VALUE        = 0x0050; // 温度校正値
	constexpr uint16_t WATER_CONTENT_CALIBRATION_VALUE      = 0x0051; // 水分含有量校正値
	constexpr uint16_t CONDUCTIVITY_CALIBRATION_VALUE       = 0x0052; // 電気伝導率校正値
	constexpr uint16_t PH_CALIBRATION_VALUE                 = 0x0053; // pH校正値
	constexpr uint16_t DEVICE_ADDRESS                       = 0x07D0; // デバイスアドレス
	constexpr uint16_t DEVICE_BAUD_RATE                     = 0x07D1; // ボーレート

}

constexpr uint8_t SUCCESS = 0;

bool SEN0604::begin(uint8_t id, uint32_t timeout_ms) {
	uint16_t device_address   = 0xFF;
	uint16_t baud_rate_number = 0xFF;
	uint8_t err;

	modbus.setTimeoutTimeMs(timeout_ms);

	// check device address
	err = modbus.readHoldingRegister(id,
	                                 SEN0604_REG::DEVICE_ADDRESS,
	                                 &device_address,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}

	if (device_address != id) {
		return false;
	}

	// check baudrate number
	err = modbus.readHoldingRegister(id,
	                                 SEN0604_REG::DEVICE_BAUD_RATE,
	                                 &baud_rate_number,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}

	if (2 < baud_rate_number) {
		return false;
	}

	return true;
}

bool SEN0604::fetchMoisture(uint8_t id, double *data) {
	uint16_t value = 0x00;
	uint8_t err;

	err = modbus.readHoldingRegister(id,
	                                 SEN0604_REG::MOISTURE_CONTENT,
	                                 &value,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}

	*data = value / 10.0;
	return true;
}

bool SEN0604::fetchTemperature(uint8_t id, double *data) {
	uint16_t value = 0x00;
	uint8_t err;

	err = modbus.readHoldingRegister(id,
	                                 SEN0604_REG::TEMPERATURE,
	                                 &value,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}

	*data = value / 10.0;
	return true;
}

bool SEN0604::fetchElectricalConductivity(uint8_t id, int32_t *data) {
	uint16_t value = 0x00;
	uint8_t err;

	err = modbus.readHoldingRegister(id,
	                                 SEN0604_REG::CONDUCTIVITY,
	                                 &value,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}

	*data = value;
	return true;
}

bool SEN0604::fetchPh(uint8_t id, double *data) {
	uint16_t value = 0x00;
	uint8_t err;

	err = modbus.readHoldingRegister(id,
	                                 SEN0604_REG::PH,
	                                 &value,
	                                 1);

	if (err != SUCCESS) {
		return false;
	}

	*data = value / 10.0;
	return true;
}