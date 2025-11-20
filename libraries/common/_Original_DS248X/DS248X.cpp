#include "DS248X.h"

#define DS248X_CMD_RESET 0xF0 // Device reset

#define DS248X_CMD_SRP 0xE1 // Set read pointer
#define DS248X_POINTER_STATUS 0xF0
#define DS248X_STATUS_BUSY (1 << 0)
#define DS248X_STATUS_PPD (1 << 1)
#define DS248X_STATUS_SD (1 << 2)
#define DS248X_STATUS_LL (1 << 3)
#define DS248X_STATUS_RST (1 << 4)
#define DS248X_STATUS_SBR (1 << 5)
#define DS248X_STATUS_TSB (1 << 6)
#define DS248X_STATUS_DIR (1 << 7)
#define DS248X_POINTER_DATA 0xE1
#define DS248X_POINTER_CONFIG 0xC3

#define DS248X_CMD_WRITECONFIG 0xD2
#define DS248X_CMD_RESETWIRE 0xB4
#define DS248X_CMD_WRITEBYTE 0xA5
#define DS248X_CMD_READBYTE 0x96
#define DS248X_CMD_SINGLEBIT 0x87
#define DS248X_CMD_TRIPLET 0x78

#define WIRE_CMD_SKIP 0xCC
#define WIRE_CMD_SELECT 0x55
#define WIRE_CMD_SEARCH 0xF0

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_//_/_/_/_/_/_/_/_/_/*/
//
//   APIs (Private)
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_//_/_/_/_/_/_/_/_/_/*/

uint8_t DS248X::wireReset() {
	busyWait();
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_RESETWIRE);
	_wire->endTransmission();

	return getPresencePulseDetect(wireReadStatus());
}

uint8_t DS248X::wireReadStatus() {
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_SRP);
	_wire->write(DS248X_POINTER_STATUS);
	_wire->endTransmission();

	_wire->requestFrom((uint8_t)_address, (uint8_t)1, (uint8_t) true);
	return _wire->read();
}

void DS248X::wireWriteByte(uint8_t data) {
	busyWait();
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_WRITEBYTE);
	_wire->write(data);
	_wire->endTransmission();
}

uint8_t DS248X::wireReadByte() {
	busyWait();
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_READBYTE);
	_wire->endTransmission();
	busyWait();
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_SRP);
	_wire->write(DS248X_POINTER_DATA);
	_wire->endTransmission();

	_wire->requestFrom((uint8_t)_address, (uint8_t)1, (uint8_t) true);
	return _wire->read();
}

void DS248X::wireWriteBit(uint8_t v) {
	busyWait();
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_SINGLEBIT);
	_wire->write(v);
	_wire->endTransmission();
}

uint8_t DS248X::wireReadBit() {
	busyWait();
	return getSingleBitRead(wireReadStatus());
}

void DS248X::wireWriteTriplet(uint8_t v) {
	busyWait();
	_wire->beginTransmission(_address);
	_wire->write(DS248X_CMD_TRIPLET);
	_wire->write(v);
	_wire->endTransmission();
}

void DS248X::busyWait() {
	const uint8_t TIMEOUT_MS = 10;
	uint8_t status = wireReadStatus();
	unsigned long start = millis();

	while (getOneWireBusy(status) && (millis() - start) <= TIMEOUT_MS) {
		delay(1);
		status = wireReadStatus();
	}
}

bool DS248X::getOneWireBusy(uint8_t status) {
	return (status & DS248X_STATUS_BUSY) ? 1 : 0;
}

uint8_t DS248X::getSingleBitRead(uint8_t status) {
	return (status & DS248X_STATUS_SBR) ? 1 : 0;
}

uint8_t DS248X::getTripletSecondBit(uint8_t status) {
	return (status & DS248X_STATUS_TSB) ? 1 : 0;
}

uint8_t DS248X::getBranchDirectionTaken(uint8_t status) {
	return (status & DS248X_STATUS_DIR) ? 1 : 0;
}

uint8_t DS248X::getPresencePulseDetect(uint8_t status) {
	return (status & DS248X_STATUS_PPD) ? 1 : 0;
}