/*!
 * km_n1_flk.h - Library for KM-N1-FLK Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef KM_N1_FLK_H
#define KM_N1_FLK_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class KM_N1_FLK {
public:
	KM_N1_FLK(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	// EP mean Electric Power
	bool fetchIntegralEP(uint8_t id, double *data);
	bool fetchActivePower(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;
};

#endif /* KM_N1_FLK_H */