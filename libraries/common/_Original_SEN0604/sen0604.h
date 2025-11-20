/*!
 * sen0604.h - Library for DFRobot SEN0604 Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef SEN_0604_H
#define SEN_0604_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class SEN0604 {
public:
	SEN0604(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	bool fetchMoisture(uint8_t id, double *data);
	bool fetchTemperature(uint8_t id, double *data);
	bool fetchElectricalConductivity(uint8_t id, int32_t *data);
	bool fetchPh(uint8_t id, double *data);
	// bool fetchSalinity(uint8_t id, double *data);
	// bool fetchTotalDissolvedSolids(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;
};

#endif /* SEN_0604_H */