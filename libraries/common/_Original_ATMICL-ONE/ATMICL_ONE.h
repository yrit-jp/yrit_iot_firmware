/*!
 * ATMICL-ONE.h - Library for ATMICL-ONE current sensor
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT.
 * see https://opensource.org/license/MIT
 */

#ifndef ATMICL_ONE_H
#define ATMICL_ONE_H

#include <Arduino.h>
#include <Wire.h>

class ATMICL_ONE {
public:
	ATMICL_ONE(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}

	uint8_t DEVICE_ADDRESS;
	bool begin(uint8_t address);
	bool readAcCurrent(double *data);
	bool readAcFrequency(double *data);
	bool readDcCounter(uint32_t *data);
	bool readDcState(uint16_t *data);
	bool readDcFrequency(uint16_t *data);

	bool changeAddress(uint8_t address);
	bool adjustOffset(int8_t adj);
	bool setZeroPoint();
	bool clearDcCounter();

private:
	TwoWire *_wire;
	unsigned long long _last_read_time = 0;

	struct ATMICL_ONE_AC {
		int16_t current;
		uint16_t frequency;
	};
	struct ATMICL_ONE_AC _ac;

	struct ATMICL_ONE_DC {
		bool state;
		uint32_t counter;
		uint16_t frequency;
	};
	struct ATMICL_ONE_DC _dc;

	bool readAC();
	bool readDC();
};

#endif