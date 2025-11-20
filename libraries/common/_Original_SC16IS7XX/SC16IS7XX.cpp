#include "SC16IS7XX.h"

constexpr uint8_t REG_RHR_THR    = 0x00; // 送受信保持レジスタ
constexpr uint8_t REG_IER        = 0x01; // 割り込み有効レジスタ
constexpr uint8_t REG_IIR_FCR    = 0x02; // 割り込み識別/FIFO制御レジスタ
constexpr uint8_t REG_LCR        = 0x03; // ライン制御レジスタ
constexpr uint8_t REG_MCR        = 0x04; // モデムコントロールレジスタ
constexpr uint8_t REG_LSR        = 0x05; // ラインステータスレジスタ
constexpr uint8_t REG_MSR        = 0x06; // モデムステータスレジスタ
constexpr uint8_t REG_SPR        = 0x07; // スクラッチパッドレジスタ
constexpr uint8_t REG_TCR        = 0x06; // 伝送制御レジスタ
constexpr uint8_t REG_TLR        = 0x07; // トリガレベルレジスタ
constexpr uint8_t REG_TXLVL      = 0x08; // 送信FIFOレベルレジスタ
constexpr uint8_t REG_RXLVL      = 0x09; // 受信FIFOレベルレジスタ
constexpr uint8_t REG_IO_DIR     = 0x0A;
constexpr uint8_t REG_IO_STATE   = 0x0B;
constexpr uint8_t REG_IO_INT_ENA = 0x0C;
constexpr uint8_t REG_IO_CONTROL = 0x0E; // IO制御レジスタ
constexpr uint8_t REG_EFCR       = 0x0F; // 追加機能制御レジスタ
constexpr uint8_t REG_DLL        = 0x00; // divisor ラッチレジスタ LSB
constexpr uint8_t REG_DLH        = 0x01; // divisor ラッチレジスタ MSB
constexpr uint8_t REG_EFR        = 0x02; // 追加機能レジスタ
constexpr uint8_t REG_XON1       = 0x04;
constexpr uint8_t REG_XON2       = 0x05;
constexpr uint8_t REG_XOFF1      = 0x06;
constexpr uint8_t REG_XOFF2      = 0x07;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Public
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool SC16IS7XX::begin(uint8_t addr,
                      uint32_t oscillator_hz,
                      uint32_t baudrate,
                      uint8_t data_bit,
                      uint8_t stop_bit,
                      ParityBit parity) {

	DEVICE_ADDRESS = addr;
	OSCILLATOR_HZ  = oscillator_hz;

	reset();
	delay(5); // least 3 us

	if (!setBaudrate(baudrate) |
	    !setParam(data_bit, stop_bit, parity) |
	    !enableFIFO()) {

		return false;
	}

	return true;
}

void SC16IS7XX::reset() {
	IO_Control_Reg reg;
	readRegister(REG_IO_CONTROL, &reg.Byte);
	reg.Bit.reset = 1;
	writeRegister(REG_IO_CONTROL, reg.Byte);
}

bool SC16IS7XX::setBaudrate(uint32_t baudrate) {
	MCR_Reg mcr;

	if (!readRegister(REG_MCR, &mcr.Byte)) {
		return false;
	}
	uint8_t prescaler = (mcr.Bit.clock_divisor == 1) ? 4 : 1;

	uint16_t divisor = (OSCILLATOR_HZ / prescaler) / (baudrate * 16);

	LCR_Reg lcr;
	if (!readRegister(REG_LCR, &lcr.Byte)) {
		return false;
	}
	// Because accessible only LCR[7] is 1
	lcr.Bit.divisor_latch_enable = 1;
	if (!writeRegister(REG_LCR, lcr.Byte)) {
		return false;
	}

	if (!writeRegister(REG_DLL, (uint8_t)divisor)) {
		return false;
	}
	if (!writeRegister(REG_DLH, (uint8_t)(divisor >> 8))) {
		return false;
	}

	lcr.Bit.divisor_latch_enable = 0;
	if (!writeRegister(REG_LCR, lcr.Byte)) {
		return false;
	}
	return true;
}

bool SC16IS7XX::setParam(uint8_t data_bit, uint8_t stop_bit, ParityBit parity) {
	LCR_Reg lcr;
	lcr.Byte = 0;

	switch (data_bit) {
	case 5:
		lcr.Bit.word_length = 0b00;
		break;
	case 6:
		lcr.Bit.word_length = 0b01;
		break;
	case 7:
		lcr.Bit.word_length = 0b10;
		break;
	case 8:
		lcr.Bit.word_length = 0b11;
		break;
	default:
		// default is 8 bits
		lcr.Bit.word_length = 0b11;
		break;
	}

	switch (stop_bit) {
	case 1:
		lcr.Bit.number_of_stop_bit = 0;
		break;
	case 2:
		lcr.Bit.number_of_stop_bit = 1;
		break;
	default:
		// default is 1 bit
		lcr.Bit.number_of_stop_bit = 0;
		break;
	}

	switch (parity) {
	case ParityBit::None:
		lcr.Bit.parity_enable = 0;
		break;
	case ParityBit::Even:
		lcr.Bit.parity_enable = 1;
		lcr.Bit.parity_type   = 1;
		lcr.Bit.set_parity    = 0;
		break;
	case ParityBit::Odd:
		lcr.Bit.parity_enable = 1;
		lcr.Bit.parity_type   = 0;
		lcr.Bit.set_parity    = 0;
		break;
	default:
		lcr.Bit.parity_enable = 0;
		break;
	}

	lcr.Bit.break_control = 0;

	if (!writeRegister(REG_LCR, lcr.Byte)) {
		return false;
	}
	return true;
}

bool SC16IS7XX::enableFIFO() {
	FCR_Reg fcr;
	if (!readRegister(REG_IIR_FCR, &fcr.Byte)) {
		return false;
	}

	fcr.Bit.FIFO_enable   = 1;
	fcr.Bit.reset_TX_FIFO = 1;
	fcr.Bit.reset_RX_FIFO = 1;

	if (!writeRegister(REG_IIR_FCR, fcr.Byte)) {
		return false;
	}
	return true;
}

bool SC16IS7XX::disableFIFO() {
	FCR_Reg fcr;
	if (!readRegister(REG_IIR_FCR, &fcr.Byte)) {
		return false;
	}

	fcr.Bit.FIFO_enable = 0;
	if (!writeRegister(REG_IIR_FCR, fcr.Byte)) {
		return false;
	}
	return true;
}

bool SC16IS7XX::clearFIFO(bool tx, bool rx) {
	FCR_Reg fcr;
	if (!readRegister(REG_IIR_FCR, &fcr.Byte)) {
		return false;
	}

	fcr.Bit.reset_TX_FIFO = (uint8_t)tx;
	fcr.Bit.reset_RX_FIFO = (uint8_t)rx;

	if (!writeRegister(REG_IIR_FCR, fcr.Byte)) {
		return false;
	}
	return true;
}

uint8_t SC16IS7XX::rxAvailable() {
	uint8_t data = 0;

	if (!readRegister(REG_RXLVL, &data)) {
		return 0;
	}
	return data;
}

uint8_t SC16IS7XX::txReady() {
	uint8_t data = 0;

	if (!readRegister(REG_TXLVL, &data)) {
		return 0;
	}
	return data;
}

bool SC16IS7XX::writeByte(uint8_t data) {
	if (txReady() == 0) {
		return false;
	}

	if (!writeRegister(REG_RHR_THR, data)) {
		return false;
	}
	return true;
}

bool SC16IS7XX::readByte(uint8_t *data) {
	if (rxAvailable() == 0) {
		return false;
	}

	if (!readRegister(REG_RHR_THR, data)) {
		return false;
	}
	return true;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Private
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool SC16IS7XX::writeRegister(uint8_t reg_addr, uint8_t data) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(reg_addr << 3);
	_wire->write(data);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}
	return true;
}

bool SC16IS7XX::readRegister(uint8_t reg_addr, uint8_t *data) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(reg_addr << 3);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)1, (uint8_t) true);
	if (num != 1) {
		return false;
	}

	*data = _wire->read();
	return true;
}