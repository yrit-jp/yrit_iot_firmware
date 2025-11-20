#ifndef SC16IS7XX_H
#define SC16IS7XX_H

#include <Arduino.h>
#include <Wire.h>

enum class ParityBit : uint8_t {
	None,
	Even,
	Odd
};

typedef union {
	uint8_t Byte;
	struct {
		uint8_t FIFO_enable : 1; // LSB
		uint8_t reset_RX_FIFO : 1;
		uint8_t reset_TX_FIFO : 1;
		uint8_t _reserved : 1;
		uint8_t TX_trigger : 2;
		uint8_t RX_trigger : 2;
	} Bit;
} FCR_Reg;

typedef union {
	uint8_t Byte;
	struct {
		uint8_t word_length : 2; // LSB
		uint8_t number_of_stop_bit : 1;
		uint8_t parity_enable : 1;
		uint8_t parity_type : 1;
		uint8_t set_parity : 1;
		uint8_t break_control : 1;
		uint8_t divisor_latch_enable : 1;
	} Bit;
} LCR_Reg;

typedef union {
	uint8_t Byte;
	struct {
		uint8_t data_in_receiver : 1; // LSB
		uint8_t overrun_error : 1;
		uint8_t parity_error : 1;
		uint8_t framing_error : 1;
		uint8_t break_interrupt : 1;
		uint8_t THR_empty : 1;
		uint8_t THR_and_TSR_empty : 1;
		uint8_t FIFO_data_error : 1;
	} Bit;
} LSR_Reg;

typedef union {
	uint8_t Byte;
	struct {
		uint8_t DTR : 1; // LSB
		uint8_t RTS : 1;
		uint8_t TCR_and_TLR_enable : 1;
		uint8_t _reserved : 1;
		uint8_t enable_loopback : 1;
		uint8_t Xon_Any : 1;
		uint8_t lrDA_mode_enable : 1;
		uint8_t clock_divisor : 1;
	} Bit;
} MCR_Reg;

typedef union {
	uint8_t Byte;
	struct {
		uint8_t dCTS : 1; // LSB
		uint8_t dDSR : 1;
		uint8_t dRI : 1;
		uint8_t dCD : 1;
		uint8_t CTS : 1;
		uint8_t DSR : 1;
		uint8_t RI : 1;
		uint8_t CD : 1;
	} Bit;
} MSR_Reg;

typedef union {
	uint8_t Byte;
	struct {
		uint8_t latch : 1; // LSB
		uint8_t gpio : 1;
		uint8_t _reserved1 : 1;
		uint8_t reset : 1;
		uint8_t _reserved2 : 4;
	} Bit;
} IO_Control_Reg;

class SC16IS7XX {
public:
	SC16IS7XX(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}
	
	bool begin(uint8_t addr,
	           uint32_t oscillator_hz = 7372800,
	           uint32_t baudrate      = 9600,
	           uint8_t data_bit       = 8,
	           uint8_t stop_bit       = 1,
	           ParityBit parity       = ParityBit::None);
	void reset();
	bool setBaudrate(uint32_t baudrate);
	bool setParam(uint8_t data_bit, uint8_t stop_bit, ParityBit parity);
	bool enableFIFO();
	bool disableFIFO();
	bool clearFIFO(bool tx, bool rx);
	uint8_t rxAvailable();
	uint8_t txReady();
	bool writeByte(uint8_t data);
	bool readByte(uint8_t *data);

private:
	TwoWire *_wire;
	uint8_t DEVICE_ADDRESS;
	uint32_t OSCILLATOR_HZ;

	bool writeRegister(uint8_t reg_addr, uint8_t data);
	bool readRegister(uint8_t reg_addr, uint8_t *data);
};

#endif /* SC16IS7XX_H */