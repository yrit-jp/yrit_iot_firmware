/*! ==============================================================
 * xy-md02.h - Library for XY-MD02 Modbus Device
 *
 * Copyright (c) 2025 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/licenses/MIT
 * ============================================================= */

#ifndef XY_MD02_H
#define XY_MD02_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class XY_MD02 {
public:
	XY_MD02(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	bool fetchTemperature(uint8_t id, double *temperature);
	bool fetchHumidity(uint8_t id, double *humidity);

protected:
	DFRobot_RTU modbus;
};

#endif /* XY_MD02_H */
