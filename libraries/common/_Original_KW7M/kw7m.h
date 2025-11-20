/*!
 * kw7m.h - Library for KW7M Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef KW7M_H
#define KW7M_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class KW7M {
public:
	KW7M(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	// EP mean Electric Power
	bool fetchIntegralEP(uint8_t id, double *data);
	bool fetchInstantaneousEP(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;
};

#endif /* KW7M_H */