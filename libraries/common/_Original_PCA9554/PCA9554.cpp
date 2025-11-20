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

#include "PCA9554.h"

#define REG_INPUT_PORT (0x00)
#define REG_OUTPUT_PORT (0x01)
#define REG_POLARITY_INVERSION_PORT (0x02)
#define REG_CONFIG_PORT (0x03)

bool PCA9554::begin() {
	// POLARITY_INVERSION レジスタを設定することで、センサを識別する

	uint8_t value = 0x00;
	// 書き込み
	if (writeRegister(REG_POLARITY_INVERSION_PORT, 0xFF) == false) {
		return false;
	}
	// 読み込み
	if (readRegister(REG_POLARITY_INVERSION_PORT, value) == false) {
		return false;
	}
	// 値チェック
	if (value != 0xFF) {
		return false;
	}

	// 設定をデフォルトに戻す
	if (writeRegister(REG_POLARITY_INVERSION_PORT, 0x00) == false) {
		return false;
	}
	// 読み込み
	if (readRegister(REG_POLARITY_INVERSION_PORT, value) == false) {
		return false;
	}
	// 値チェック
	if (value != 0x00) {
		return false;
	}
	return true;
}

bool PCA9554::readRegister(uint8_t reg, uint8_t &value) {
	// 読み込み
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(reg);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}
	delay(50);

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)1, (uint8_t) true);
	if (num != 1) {
		// Error
		return false;
	}
	value = _wire->read();

	return true;
}

bool PCA9554::writeRegister(uint8_t reg, uint8_t value) {
	// 書き込み
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(reg);
	_wire->write(value);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}
	return true;
}

void PCA9554::setPinMode(uint8_t pin, uint8_t rw) {
	uint8_t value;
	readRegister(REG_CONFIG_PORT, value);

	uint8_t next;
	if (rw == 1) {
		// 指定されたビット位置を 1 に設定
		next = value | (1 << pin);
	} else {
		// 指定されたビット位置を 0 に設定
		next = value & ~(1 << pin);
	}

	writeRegister(REG_CONFIG_PORT, next);
}

void PCA9554::setAllPinMode(uint8_t value) {
	writeRegister(REG_CONFIG_PORT, value);
}

bool PCA9554::readPin(uint8_t pin, uint8_t &result) {
	uint8_t value;
	if (readRegister(REG_INPUT_PORT, value) == false) {
		return false;
	}

	result = (value & (1 << pin)) ? 1 : 0;
	return true;
}

bool PCA9554::readAllPin(uint8_t &result) {
	return readRegister(REG_INPUT_PORT, result);
}

bool PCA9554::writePin(uint8_t pin, uint8_t hl) {
	uint8_t value;
	if (readRegister(REG_OUTPUT_PORT, value) == false) {
		return false;
	}

	uint8_t next;
	if (hl == 1) {
		// 指定されたビット位置を 1 に設定
		next = value | (1 << pin);
	} else {
		// 指定されたビット位置を 0 に設定
		next = value & ~(1 << pin);
	}

	return writeRegister(REG_OUTPUT_PORT, next);
}

bool PCA9554::writeAllPin(uint8_t value) {
	return writeRegister(REG_OUTPUT_PORT, value);
}
