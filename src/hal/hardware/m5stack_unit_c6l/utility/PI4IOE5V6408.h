/*!
 * PI4IOE5V6408.h - Library for PI4IOE5V6408
 *
 * Copyright (c) 2026 takkaO
 *
 * This software is released under the MIT License.
 * see https://opensource.org/license/MIT
 */

#if defined(ARDUINO_M5STACK_UNIT_C6L)

#ifndef PI4IOE5V6408_H
#define PI4IOE5V6408_H

#include "Wire.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// レジスタアドレス（全レジスタはリード/ライト対応、アドレスはすべて奇数）
// ---------------------------------------------------------------------------
namespace PI4IOE5V6408_REG {
	constexpr uint8_t DEVICE_ID_AND_CONTROL = 0x01; // D[7:5]=MfrID, D[4:2]=FwRev, D[1]=RST_INT, D[0]=SW_RST
	constexpr uint8_t IO_DIRECTION          = 0x03; // 1=Output, 0=Input (per pin)
	constexpr uint8_t OUTPUT_STATE          = 0x05; // 1=High,  0=Low    (per pin)
	constexpr uint8_t OUTPUT_HIGH_IMPEDANCE = 0x07; // 1=Hi-Z,  0=Normal (per pin)
	constexpr uint8_t INPUT_DEFAULT_STATE   = 0x09; // 1=High, 0=Low    (per pin)
	constexpr uint8_t PULL_UP_DOWN_ENABLE   = 0x0B; // 1=有効,  0=無効   (per pin)
	constexpr uint8_t PULL_UP_DOWN_SELECT   = 0x0D; // 1=Pull-Up, 0=Pull-Down (per pin)
	constexpr uint8_t INPUT_STATUS          = 0x0F; // 入力ピンの現在値（Read-Only）
	constexpr uint8_t INTERRUPT_MASK        = 0x11; // 1=Masked（割り込み無効）, 0=Unmasked
	constexpr uint8_t INTERRUPT_STATUS      = 0x13; // 割り込み発生ピンのビットフラグ（Read-Only）
}

class PI4IOE5V6408 {
public:
	static constexpr uint8_t DEFAULT_ADDRESS = 0x43;

	bool begin(TwoWire &wire, uint8_t address = DEFAULT_ADDRESS);

	bool pinMode(uint8_t pin, uint8_t mode);
	bool digitalWrite(uint8_t pin, uint8_t value);
	int8_t digitalRead(uint8_t pin);

	bool setPullEnable(uint8_t pin, bool enable);
	bool setPullSelect(uint8_t pin, bool pullUp);
	bool setHighImpedance(uint8_t pin, bool enable);
	bool setInputDefaultState(uint8_t pin, bool defaultHigh);

	bool setInterruptMask(uint8_t pin, bool masked);
	int16_t getInterruptStatus();

	bool writeRegister(uint8_t reg, uint8_t value);
	int16_t readRegister(uint8_t reg);

private:
	TwoWire *_wire   = nullptr;
	uint8_t _address = DEFAULT_ADDRESS;

	bool _updateRegisterBit(uint8_t reg, uint8_t pin, bool value);
};

#endif // PI4IOE5V6408_H

#endif // HAL_API_M5STACK_UNIT_C6L_H