/*!
 * wld_pa.h - Library for WLD-PA Modbus Device
 *
 * Copyright (c) 2025 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef WLD_PA_H
#define WLD_PA_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class WLD_PA {
public:
WLD_PA(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	// EP mean Electric Power
	bool fetchIntegralEP(uint8_t id, double *data);
	bool fetchInstantaneousEP(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;
};

#endif /* WLD_PA */