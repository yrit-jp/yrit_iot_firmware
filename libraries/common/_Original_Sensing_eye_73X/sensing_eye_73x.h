/*!
 * sensing_eye_73x.h - Library for Sensing Eye 73X Modbus Device
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#ifndef SENSING_EYE_73X_H
#define SENSING_EYE_73X_H

#include "Arduino.h"
#include "DFRobot_RTU.h"

class SENSING_EYE_73X {
public:
	SENSING_EYE_73X(Stream *theSerial) : modbus(theSerial) {}
	bool begin(uint8_t id, uint32_t timeout_ms);

	bool fetchWaterQuality(uint8_t id, int32_t *data);
	bool fetchWaterTemperature(uint8_t id, double *data);

protected:
	DFRobot_RTU modbus;
};

#endif /* SENSING_EYE_73X_H */