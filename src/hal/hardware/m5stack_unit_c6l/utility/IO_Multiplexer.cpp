#if defined(ARDUINO_M5STACK_UNIT_C6L)

#include "IO_Multiplexer.h"

namespace {
	PI4IOE5V6408 pi4ioe5v6408;

	struct rv_pair {
		uint8_t reg;
		uint8_t val;
	};

	bool _inited = false;
}

int IO_Multiplexer::readPin(uint8_t pin) {
	if (_inited == false or pin > 7) {
		return HIGH;
	}

	int result = pi4ioe5v6408.digitalRead(pin);
	if (result < 0) {
		return HIGH;
	}

	return result ? HIGH : LOW;
}

void IO_Multiplexer::writePin(uint8_t pin, bool state) {
	if (_inited == false or pin > 7) {
		return;
	}

	pi4ioe5v6408.digitalWrite(pin, state);
}

void IO_Multiplexer::enableLNA(bool state) {
	if (_inited == false) {
		return;
	}
	pi4ioe5v6408.digitalWrite(5, state);
}

void IO_Multiplexer::switchRF(bool state) {
	if (_inited == false) {
		return;
	}
	pi4ioe5v6408.digitalWrite(6, state);
}

void IO_Multiplexer::setNRST(bool state) {
	if (_inited == false) {
		return;
	}
	pi4ioe5v6408.digitalWrite(7, state);
}

bool IO_Multiplexer::initialize(TwoWire &_wire) {
	_inited = false;
	if (not pi4ioe5v6408.begin(_wire)) {
		return false;
	}

	// PI4IO E0 ピンアサイン（Unified, Power_Class.cpp）
	//  P0: BTN1 (UnitC6L + NessoN1)
	//  P1: BTN2 (NessoN1) / NC (UnitC6L)
	//  P2-P4: NC
	//  P5: LNA Enable
	//  P6: RF Switch
	//  P7: LoRa Reset
	const rv_pair default_set[] = {
	    {0x03, 0b11100000}, // IO_DIRECTION:          P5/P6/P7=Output, P0-P4=Input
	    {0x05, 0b10000000}, // OUTPUT_STATE:          P7(LoRa Reset)=High
	    {0x07, 0b00011100}, // OUTPUT_HIGH_IMPEDANCE: P2-P4=Hi-Z
	    {0x0D, 0b11000011}, // PULL_UP_DOWN_SELECT:   P0/P1/P6/P7=Pull-Up, others=Pull-Down
	    {0x0B, 0b11000011}, // PULL_UP_DOWN_ENABLE:   P0/P1/P6/P7=有効
	    {0x09, 0b00000011}, // INPUT_DEFAULT_STATE:   P0/P1 デフォルトHigh（アクティブLow）
	    {0x11, 0b11111100}, // INTERRUPT_MASK:        P0/P1 のみ割り込み有効
	};

	for (const auto &entry : default_set) {
		if (not pi4ioe5v6408.writeRegister(entry.reg, entry.val)) {
			return false;
		}
	}
	_inited = true;
	return true;
}

#endif // ARDUINO_M5STACK_UNIT_C6L