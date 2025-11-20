/*!
 * ypms-482p.h - Library for YPMS-482P Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef YPMS_482P_H
#define YPMS_482P_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class YPMS_482P {
public:
	YPMS_482P(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	bool fetchPh(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;
};

#endif /* YPMS_482P_H */