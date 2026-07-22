/*!
 * PI4IOE5V6408.cpp - Library for PI4IOE5V6408
 *
 * Copyright (c) 2026 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#if defined(ARDUINO_M5STACK_UNIT_C6L)

#include "PI4IOE5V6408.h"

bool PI4IOE5V6408::begin(TwoWire &wire, uint8_t address) {
	_wire    = &wire;
	_address = address;

	// ソフトウェアリセットを発行してレジスタを初期値に戻す（bit[0]=1 で起動、完了後自動クリア）
	if (!writeRegister(PI4IOE5V6408_REG::DEVICE_ID_AND_CONTROL, 0x01)) {
		return false;
	}
	delay(1); // リセット完了を待つ（データシート: t_rst ≤ 150 ns、余裕を持って 1 ms 待機）

	// Manufacturer ID を読んで正しいデバイスかを確認する
	// デフォルト値 bits[7:5] = 0b101
	int16_t val = readRegister(PI4IOE5V6408_REG::DEVICE_ID_AND_CONTROL);
	if (val < 0) {
		return false;
	}
	uint8_t mfr_id = ((uint8_t)val >> 5) & 0b111;
	return mfr_id == 0b101;
}

bool PI4IOE5V6408::pinMode(uint8_t pin, uint8_t mode) {
	// IO_DIRECTION: 0=Input, 1=Output（電源投入時デフォルト 0x00 = 全ピン Input）
	return _updateRegisterBit(PI4IOE5V6408_REG::IO_DIRECTION, pin, mode == OUTPUT);
}

bool PI4IOE5V6408::digitalWrite(uint8_t pin, uint8_t value) {
	return _updateRegisterBit(PI4IOE5V6408_REG::OUTPUT_STATE, pin, value != 0);
}

int8_t PI4IOE5V6408::digitalRead(uint8_t pin) {
	if (pin > 7) {
		return -1;
	}
	int16_t val = readRegister(PI4IOE5V6408_REG::INPUT_STATUS);
	if (val < 0) {
		return -1;
	}
	return (val >> pin) & 0x01;
}

bool PI4IOE5V6408::setPullEnable(uint8_t pin, bool enable) {
	return _updateRegisterBit(PI4IOE5V6408_REG::PULL_UP_DOWN_ENABLE, pin, enable);
}

bool PI4IOE5V6408::setPullSelect(uint8_t pin, bool pullUp) {
	// PULL_UP_DOWN_SELECT: 1=Pull-Up, 0=Pull-Down
	return _updateRegisterBit(PI4IOE5V6408_REG::PULL_UP_DOWN_SELECT, pin, pullUp);
}

bool PI4IOE5V6408::setHighImpedance(uint8_t pin, bool enable) {
	return _updateRegisterBit(PI4IOE5V6408_REG::OUTPUT_HIGH_IMPEDANCE, pin, enable);
}

bool PI4IOE5V6408::setInputDefaultState(uint8_t pin, bool defaultHigh) {
	return _updateRegisterBit(PI4IOE5V6408_REG::INPUT_DEFAULT_STATE, pin, defaultHigh);
}

bool PI4IOE5V6408::setInterruptMask(uint8_t pin, bool masked) {
	return _updateRegisterBit(PI4IOE5V6408_REG::INTERRUPT_MASK, pin, masked);
}

int16_t PI4IOE5V6408::getInterruptStatus() {
	return readRegister(PI4IOE5V6408_REG::INTERRUPT_STATUS);
}

bool PI4IOE5V6408::writeRegister(uint8_t reg, uint8_t value) {
	_wire->beginTransmission(_address);
	_wire->write(reg);
	_wire->write(value);
	return _wire->endTransmission() == 0;
}

int16_t PI4IOE5V6408::readRegister(uint8_t reg) {
	// レジスタアドレスを書き込んでポインタを設定し、STOP 後に別トランザクションで読み出す
	// （SoftWire は Repeated Start を正しくサポートしない場合があるため STOP+START を使用）
	_wire->beginTransmission(_address);
	_wire->write(reg);
	if (_wire->endTransmission(false) != 0) {
		return -1;
	}
	if (_wire->requestFrom(_address, (uint8_t)1) != 1) {
		return -1;
	}
	return _wire->read();
}

bool PI4IOE5V6408::_updateRegisterBit(uint8_t reg, uint8_t pin, bool value) {
	if (pin > 7) {
		return false;
	}
	// Read-Modify-Write でターゲットビットのみ変更する
	int16_t current = readRegister(reg);
	if (current < 0) {
		return false;
	}
	uint8_t updated;
	if (value) {
		updated = (uint8_t)current | (1 << pin);
	} else {
		updated = (uint8_t)current & ~(1 << pin);
	}
	// 変更がなければ不要な Write を省略する
	if (updated == (uint8_t)current) {
		return true;
	}
	return writeRegister(reg, updated);
}

#endif // ARDUINO_M5STACK_UNIT_C6L