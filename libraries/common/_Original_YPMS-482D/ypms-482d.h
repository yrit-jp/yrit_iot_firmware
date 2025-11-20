/*!
 * ypms-482d.h - Library for YPMS-482D Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef YPMS_482D_H
#define YPMS_482D_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class YPMS_482D {
public:
	YPMS_482D(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	bool fetchDO(uint8_t id, double *data);
	bool fetchOxygenConcentration(uint8_t id, double *data);
	bool fetchOxygenSaturation(uint8_t id, double *data);
	bool fetchPressure(uint8_t id, double *data);
	bool fetchTemperature(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;

	bool fetchValue(const uint16_t target_register, uint8_t id, double *data);
};

#endif /* YPMS_482D_H */