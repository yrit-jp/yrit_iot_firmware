#include "i2c_ct_unit.h"

bool I2C_CT_Unit::init(TwoWire *wire, uint8_t address) {
	_wire          = wire;
	DEVICE_ADDRESS = address;
	uint8_t size   = 1;

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(addr_who_am_i);
	_wire->endTransmission();

	uint8_t num = _wire->requestFrom((uint8_t)DEVICE_ADDRESS, size, (uint8_t)true);
	// HAL.printf("Recv Num: %d\n", num);
	if (num != size) {
		return false;
	}

	for (int i = addr_who_am_i; i < addr_who_am_i + size; i++) {
		rx_data.packet[i] = _wire->read();
	}
	uint8_t res = rx_data.who_am_i;

	// HAL.printf("res: %d\n", res);
	if (res == DEVICE_ADDRESS) {
		return true;
	}
	return false;
}

String I2C_CT_Unit::getDeviceName() {
	return "I2C_CT_Unit";
}

bool I2C_CT_Unit::fetchMilliAmpere(uint32_t *data) {
	uint8_t size = 4;

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(addr_milli_ampere);
	_wire->endTransmission();

	uint8_t num = _wire->requestFrom((uint8_t)DEVICE_ADDRESS, size, (uint8_t)true);
	if (num != size) {
		return false;
	}

	for (uint8_t i = addr_milli_ampere; i < addr_milli_ampere + size; i++) {
		if (_wire->available()) {
			rx_data.packet[i] = _wire->read();
		} else {
			return false;
		}
	}

	*data = rx_data.milli_ampere;
	return true;
}

bool I2C_CT_Unit::fetchAdRef(uint16_t *data) {
	uint8_t size = 2;

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(addr_adref);
	_wire->endTransmission();

	uint8_t num = _wire->requestFrom((uint8_t)DEVICE_ADDRESS, size, (uint8_t)true);
	if (num != size) {
		return false;
	}

	for (uint8_t i = addr_adref; i < addr_adref + size; i++) {
		if (_wire->available()) {
			rx_data.packet[i] = _wire->read();
		} else {
			return false;
		}
	}

	*data = rx_data.ad_ref;
	return true;
}

bool I2C_CT_Unit::fetchAdVal(uint16_t *data) {
	uint8_t size = 2;

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(addr_adval);
	_wire->endTransmission();

	uint8_t num = _wire->requestFrom((uint8_t)DEVICE_ADDRESS, size, (uint8_t)true);
	if (num != size) {
		return false;
	}

	for (uint8_t i = addr_adval; i < addr_adval + size; i++) {
		if (_wire->available()) {
			rx_data.packet[i] = _wire->read();
		} else {
			return false;
		}
	}

	*data = rx_data.ad_val;
	return true;
}