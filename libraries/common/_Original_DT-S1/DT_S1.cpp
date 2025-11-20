#include "DT_S1.h"

bool DT_S1::begin(uint8_t address) {
	DEVICE_ADDRESS = address;

	return true;
}

bool DT_S1::getTemperature(double *temperature) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x01); // read command
	if (_wire->endTransmission(false) != 0) {
		// Error
		return false;
	}

	delay(1);

	constexpr uint8_t DATA_NUM = 4;

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)DATA_NUM, (uint8_t) true);
	if (num != DATA_NUM) {
		return false;
	}

	int32_t raw = 0;
	for (int i = 0; i < DATA_NUM; i++) {
		raw = (raw << 8) + _wire->read();
	}

	*temperature = raw / 100.0;
	return true;
}