#include "MCP23017.h"
#define USE_BANK_0

#ifdef USE_BANK_0
#define ADDR_IODIRA (0x00)
#define ADDR_IODIRB (0x01)
#define ADDR_IPOLA (0x02)
#define ADDR_IPOLB (0x03)
#define ADDR_GPINTENA (0x04)
#define ADDR_GPINTENB (0x05)
#define ADDR_DEFVALA (0x06)
#define ADDR_DEFVALB (0x07)
#define ADDR_INTCONA (0x08)
#define ADDR_INTCONB (0x09)
#define ADDR_IOCON (0x0A)
#define ADDR_GPPUA (0x0C)
#define ADDR_GPPUB (0x0D)
#define ADDR_INTFA (0x0E)
#define ADDR_INTFB (0x0F)
#define ADDR_INTCAPA (0x10)
#define ADDR_INTCAPB (0x11)
#define ADDR_GPIOA (0x12)
#define ADDR_GPIOB (0x13)
#define ADDR_OLATA (0x14)
#define ADDR_OLATB (0x15)
#else
#define ADDR_IODIRA (0x00)
#define ADDR_IPOLA (0x01)
#define ADDR_GPINTENA (0x02)
#define ADDR_DEFVALA (0x03)
#define ADDR_INTCONA (0x04)
#define ADDR_IOCON (0x05)
#define ADDR_GPPUA (0x06)
#define ADDR_INTFA (0x07)
#define ADDR_INTCAPA (0x08)
#define ADDR_GPIOA (0x09)
#define ADDR_OLATA (0x0A)
#define ADDR_IODIRB (0x0B)
#define ADDR_IPOLB (0x0C)
#define ADDR_GPINTENB (0x0D)
#define ADDR_DEFVALB (0x0E)
#define ADDR_INTCONB (0x0F)
#define ADDR_GPPUB (0x11)
#define ADDR_INTFB (0x12)
#define ADDR_INTCAPB (0x13)
#define ADDR_GPIOB (0x14)
#define ADDR_OLATB (0x15)
#endif

bool MCP23017::init(uint8_t address) {
	DEVICE_ADDRESS = address;

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_IOCON);
	_wire->write(0x00);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_IODIRA); // IODIRA register
	_wire->write(0xFF);        // set all of port A to inputs
	_wire->write(0xFF);        // set all of port B to inputs
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_IPOLA); // IPOLA register
	_wire->write(0x00);       // GPIO register bit reflects the same logic state of the input pin. (port A)
	_wire->write(0x00);       // GPIO register bit reflects the same logic state of the input pin. (port B)
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_GPINTENA);
	_wire->write(0x00); // disable all port A interrupt
	_wire->write(0x00); // disable all port B interrupt
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_GPPUA);
	_wire->write(0xFF); // enable port A internal pull-up
	_wire->write(0xFF); // enable port B internal pull-up
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	return true;
}

bool MCP23017::getStatusGpioA(uint32_t *data) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_GPIOA);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)1, (uint8_t) true);
	if (num != 1) {
		return false;
	}

	while (_wire->available()) {
		*data = _wire->read();
	}
	return true;
}

bool MCP23017::getStatusGpioB(uint32_t *data) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(ADDR_GPIOB);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)1, (uint8_t) true);
	if (num != 1) {
		return false;
	}

	while (_wire->available()) {
		*data = _wire->read();
	}
	return true;
}
