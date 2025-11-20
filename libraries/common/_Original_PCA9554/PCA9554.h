/*
 * Author: takkaO
 * Creation Date: 2024-05-26
 * License: MIT License
 *
 *
 * License Details:
 * MIT License
 * Copyright (c) 2024 takkaO
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef PCA9554_H
#define PCA9554_H

#include <Arduino.h>
#include <Wire.h>

class PCA9554 {
public:
	static constexpr uint8_t ALL_INPUT  = 0xFF;
	static constexpr uint8_t ALL_OUTPUT = 0x00;

	PCA9554(uint8_t address, TwoWire *theWire = &Wire) {
		DEVICE_ADDRESS = address;
		_wire          = theWire;
	}

	bool begin();
	bool readRegister(uint8_t reg, uint8_t &result);
	bool writeRegister(uint8_t reg, uint8_t value);

	void setPinMode(uint8_t pin, uint8_t rw);
	void setAllPinMode(uint8_t value);

	bool readPin(uint8_t pin, uint8_t &result);
	bool readAllPin(uint8_t &result);

	bool writePin(uint8_t pin, uint8_t hl);
	bool writeAllPin(uint8_t value);

private:
	TwoWire *_wire;
	uint8_t DEVICE_ADDRESS;
};

#endif /* PCA9554_H */