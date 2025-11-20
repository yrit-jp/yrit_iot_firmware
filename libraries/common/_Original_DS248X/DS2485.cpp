#include "DS2485.h"

#define DS2485_CMD_RESET 0x62 // Device reset
#define DS2485_CMD_WRITE_MEMORY 0x96
#define DS2485_CMD_READ_MEMORY 0x44
#define DS2485_CMD_SET_PAGE_PROTECTION 0xC3
#define DS2485_CMD_READ_STATUS 0xAA
#define DS2485_CMD_SET_I2C_ADDRESS 0x75
#define DS2485_CMD_READ_ONEWIRE_PORT_CONFIG 0x52
#define DS2485_CMD_WRITE_ONEWIRE_PORT_CONFIG 0x99
#define DS2485_CMD_ONEWIRE_SCRIPT 0x88
#define DS2485_CMD_ONEWIRE_BLOCK 0xAB
#define DS2485_CMD_ONEWIRE_READ_BLOCK 0x50
#define DS2485_CMD_ONEWIRE_WRITE_BLOCK 0x68
#define DS2485_CMD_ONEWIRE_SEARCH 0x11
#define DS2485_CMD_FULL_CMD_SEQ 0x57
#define DS2485_CMD_CRC16 0xCC

#define WRITE_LENGTH_BYTE_1 (1)
#define READ_STATUS_PROTECTION_PAGE (0)
#define READ_STATUS_MANUFACTURER_ID (1)
#define READ_STATUS_DEVICE_VERSION (2)

#define DS248X_STATUS_IGNORES (1 << 0)
#define DS248X_STATUS_PPD (1 << 1)
#define DS248X_STATUS_SD (1 << 2)
#define DS248X_STATUS_LL (1 << 3)
#define DS248X_STATUS_RST (1 << 4)
#define DS248X_STATUS_SBR (1 << 5)
#define DS248X_STATUS_TSB (1 << 6)
#define DS248X_STATUS_DIR (1 << 7)

#define WAIT_TRM (50)  // ms
#define WAIT_TWM (100) // ms
#define WAIT_TWS (15)  // ms
#define WAIT_TOP (1)

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//   APIs (Public)
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool DS2485::deviceReset() {
	delay(WAIT_TWM);
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_RESET);
	_wire->endTransmission();

	delay(WAIT_TOP);

	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)2, (uint8_t) true);
	if (num != 2) {
		return false;
	}
	_wire->read(); // Length byte

	// Check Result byte | 0xAA is success
	return ((_wire->read() == 0xAA) ? true : false);
}

void DS2485::setRPUP() {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_WRITE_ONEWIRE_PORT_CONFIG);
	_wire->write(3);
	_wire->write(0x11);
	_wire->write(0x06);
	_wire->write(0x00);
	_wire->endTransmission();

	delay(WAIT_TOP);

	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_WRITE_ONEWIRE_PORT_CONFIG);
	_wire->write(3);    // Length byte
	_wire->write(0x00); // Register
	_wire->write(0x00); // LSB
	_wire->write(0x08); // Enable APU
	_wire->endTransmission();

	delay(WAIT_TOP);
}

bool DS2485::readProtectionPageStatus(uint8_t *buf) {
	return readDeviceStatus(READ_STATUS_DEVICE_VERSION, buf);
}

bool DS2485::readDeviceVersion(uint16_t &result) {
	uint8_t buf[3] = {0};
	bool res       = readDeviceStatus(READ_STATUS_DEVICE_VERSION, buf);
	result         = ((uint16_t)buf[1] << 8) + buf[2];
	return res;
}

bool DS2485::readManufacturerId(uint16_t &result) {
	uint8_t buf[3] = {0};
	bool res       = readDeviceStatus(READ_STATUS_MANUFACTURER_ID, buf);
	result         = ((uint16_t)buf[1] << 8) + buf[2];
	return res;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_//_/_/_/_/_/_/_/_/_/*/
//
//   APIs (Private)
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_//_/_/_/_/_/_/_/_/_/*/

uint8_t DS2485::wireReset() {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(2);
	_wire->write(0x00);
	_wire->write(0x80); // RP
	_wire->endTransmission();

	delay(WAIT_TOP * 2 + 2);

	// Receive data
	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)4, (uint8_t) true);
	if (num != 4) {
		return false;
	}

	_wire->read();                  // length
	uint8_t result = _wire->read(); // result byte
	_wire->read();                  // 0x00
	_st = _wire->read();            // status result

	return ((result == 0xAA) && (getPresencePulseDetect(_st)));
}

void DS2485::wireWriteByte(uint8_t data) {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(2);
	_wire->write(0x03);
	_wire->write(data); // TX
	_wire->endTransmission();

	delay(WAIT_TOP * 2 + 2);
}

uint8_t DS2485::wireReadByte() {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(1);
	_wire->write(0x04);
	_wire->endTransmission();

	delay(WAIT_TOP + 2);

	// Receive data
	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)4, (uint8_t) true);
	if (num != 4) {
		return false;
	}

	_wire->read();                  // length
	uint8_t result = _wire->read(); // result byte
	_wire->read();                  // 0x04
	uint8_t data = _wire->read();   // rx result

	return data;
}

void DS2485::wireWriteBit(uint8_t v) {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(2);
	_wire->write(0x01);
	_wire->write(v);
	_wire->endTransmission();

	delay(WAIT_TOP * 2);

	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)4, (uint8_t) true);
	// if (num != 4) {
	// 	return false;
	// }

	_wire->read();               // Length
	if (_wire->read() == 0xAA) { // result
		// Success
		_wire->read();       // 0x01
		_st = _wire->read(); // ST
	}
}

uint8_t DS2485::wireReadBit() {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(1);
	_wire->write(0x02);
	_wire->endTransmission();

	delay(WAIT_TOP);

	// Receive data
	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)4, (uint8_t) true);
	// if (num != 4) {
	// 	return false;
	// }

	_wire->read();                  // length
	uint8_t result = _wire->read(); // result byte
	_wire->read();                  // 0x02
	_st = _wire->read();            // ST

	return getSingleBitRead(_st);
}

void DS2485::wireWriteTriplet(uint8_t v) {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(2);
	_wire->write(0x05);
	_wire->write(v);
	_wire->endTransmission();

	delay(WAIT_TOP * 2);

	_wire->requestFrom((uint8_t)_address, (uint8_t)4, (uint8_t) true);

	_wire->read();               // Length
	if (_wire->read() == 0xAA) { // result
		// Success
		_wire->read();       // 0x05
		_st = _wire->read(); // ST
	}
}

void DS2485::read_bytes(uint8_t *buf, uint16_t count) {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(2);
	_wire->write(0x08);
	_wire->write(0x09);
	_wire->endTransmission();

	delay(WAIT_TOP * (12 + 2 + 1));

	// Receive data
	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)13, (uint8_t) true);
	if (num != 13) {
		return;
	}

	_wire->read();                  // length
	uint8_t result = _wire->read(); // result byte
	_wire->read();                  // 0x08
	_wire->read();                  // 0x09
	for (int i = 0; i < count; i++) {
		buf[i] = _wire->read(); // rx data
	}

	// Serial.printf("RBS: 0x%02X\n", result);
}

void DS2485::select(const uint8_t rom[8]) {
	// match
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(0x02);
	_wire->write(0x03); // OW_WRITE_BYTE
	_wire->write(0x55);
	_wire->endTransmission();

	delay(WAIT_TOP * 2);

	// send address
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(0x0A);
	_wire->write(0x09); // OW_WRITE_BLOCK
	_wire->write(0x08);
	for (int i = 0; i < 8; i++) {
		_wire->write(rom[i]);
	}
	_wire->endTransmission();

	delay(WAIT_TOP * 10);
}

void DS2485::skip() {
	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_ONEWIRE_SCRIPT);
	_wire->write(1);
	_wire->write(0x07);
	_wire->endTransmission();

	delay(WAIT_TOP);

	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)4, (uint8_t) true);
	// if (num != 4) {
	// 	return false;
	// }

	_wire->read();               // Length
	if (_wire->read() == 0xAA) { // result
		// Success
		_wire->read();       // 0x07
		_st = _wire->read(); // ST
	}
}

void DS2485::reset_search() {
	_searchLastDeviceFlag  = 0;
	_searchLastDiscrepancy = 0;
	for (int i = 0; i < 8; i++) {
		_searchAddress[i] = 0;
	}

	_reg_1wire_search = 0b00000101;
}

bool DS2485::search(uint8_t *newAddr, bool search_mode /* =true */) {
	uint8_t direction;
	uint8_t last_zero = 0;

	if (_searchLastDeviceFlag)
		return 0;

	if (!reset()) {
		return 0;
	}

	wireWriteByte((search_mode) ? 0xF0 : 0xEC);

	for (uint8_t i = 0; i < 64; i++) {
		int searchByte = i / 8;
		int searchBit  = 1 << i % 8;

		if (i < _searchLastDiscrepancy)
			direction = _searchAddress[searchByte] & searchBit;
		else
			direction = (i == _searchLastDiscrepancy);

		wireWriteTriplet(direction ? 0x01 : 0x00); // DS248X とはレジスタマップが違うので注意

		uint8_t status = wireReadStatus();

		uint8_t id      = getSingleBitRead(status);
		uint8_t comp_id = getTripletSecondBit(status);
		direction       = getBranchDirectionTaken(status);

		if (id && comp_id) {
			return 0;
		} else {
			if (!id && !comp_id && !direction) {
				last_zero = i;
			}
		}

		if (direction)
			_searchAddress[searchByte] |= searchBit;
		else
			_searchAddress[searchByte] &= ~searchBit;
	}

	_searchLastDiscrepancy = last_zero;

	if (!last_zero)
		_searchLastDeviceFlag = 1;

	for (uint8_t i = 0; i < 8; i++) {
		newAddr[i] = _searchAddress[i];
	}

	return 1;
}

/// なぜかうまく動かない専用コマンド版
// bool DS2485::search(uint8_t *newAddr, bool search_mode /* =true */) {
// 	if (_searchLastDeviceFlag) {
// 		return 0;
// 	}

// 	Serial.printf("_reg_1wire_search => %02X\n", _reg_1wire_search);
// 	_wire->beginTransmission(_address);
// 	_wire->write(DS2485_CMD_ONEWIRE_SEARCH);
// 	_wire->write(2);
// 	_wire->write(_reg_1wire_search);
// 	_wire->write(0xF0);
// 	_wire->endTransmission();
// 	delay(WAIT_TOP + 64);

// 	// Receive data
// 	uint8_t num = _wire->requestFrom((uint8_t)_address, (uint8_t)11, (uint8_t) true);
// 	if (num != 11) {
// 		return false;
// 	}

// 	_wire->read();                  // length
// 	uint8_t result = _wire->read(); // result byte
// 	for (int i = 0; i < 8; i++) {
// 		newAddr[i] = _wire->read();
// 		Serial.printf("%02X", newAddr[i]);
// 	}
// 	Serial.printf("\n");
// 	_searchLastDeviceFlag = _wire->read(); // last device flag

// 	_reg_1wire_search = 0b00000000;
// 	return (result == 0xAA);
// }

uint8_t DS2485::getSingleBitRead(uint8_t status) {
	return (status & DS248X_STATUS_SBR) ? 1 : 0;
}

uint8_t DS2485::getTripletSecondBit(uint8_t status) {
	return (status & DS248X_STATUS_TSB) ? 1 : 0;
}

uint8_t DS2485::getBranchDirectionTaken(uint8_t status) {
	return (status & DS248X_STATUS_DIR) ? 1 : 0;
}

uint8_t DS2485::getPresencePulseDetect(uint8_t status) {
	return (status & DS248X_STATUS_PPD) ? 1 : 0;
}

bool DS2485::readDeviceStatus(uint8_t command, uint8_t *data) {
	uint8_t read_length = 0;
	switch (command) {
	case READ_STATUS_PROTECTION_PAGE:
		read_length = 7;
		break;
	case READ_STATUS_MANUFACTURER_ID:
		read_length = 3;
		break;
	case READ_STATUS_DEVICE_VERSION:
		read_length = 3;
		break;
	default:
		return false;
	}

	_wire->beginTransmission(_address);
	_wire->write(DS2485_CMD_READ_STATUS);
	_wire->write(0x01);
	_wire->write(command);
	_wire->endTransmission();

	delay(WAIT_TRM * 2);

	// Receive data
	uint8_t num = _wire->requestFrom((uint8_t)_address, read_length, (uint8_t) true);
	if (num != read_length) {
		return false;
	}

	_wire->read(); // data length
	for (int i = 0; i < read_length; i++) {
		data[i] = _wire->read();
	}
	return ((data[0] == 0xAA) ? true : false);
}