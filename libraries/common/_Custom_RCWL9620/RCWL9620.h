/*!
 * @brief Aa ultrasonic distance sensor From M5Stack
 * @copyright Copyright (c) 2022 by M5Stack[https://m5stack.com]
 *
 * @Links [Unit Sonic I2C](https://docs.m5stack.com/en/unit/sonic.i2c)
 * @Links [Unit Sonic IO](https://docs.m5stack.com/en/unit/sonic.io)
 * @version  V0.0.2
 * @date  2022-07-21
 */

/*!
 * @modified: takkaO
 * @license: MIT License
 * @reference: https://github.com/m5stack/M5Unit-Sonic/tree/master
 */

#ifndef RCWL9620_H
#define RCWL9620_H

#include <Arduino.h>
#include <Wire.h>

class RCWL9620 {
public:
	uint8_t DEVICE_ADDRESS;
	RCWL9620(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}
	bool begin(uint8_t address);
	bool getDistance(double *distance);

private:
	TwoWire *_wire;
};

#endif