#ifndef OW_DS18XXX_H
#define OW_DS18XXX_H

/********************************************************
テンプレートクラス wire に要求される API

uint8_t reset();
void select(const uint8_t rom[8]);
void skip();
void write(uint8_t v, uint8_t power = 0);
uint8_t read();
uint8_t read_bit();
uint8_t crc8(const uint8_t *addr, uint8_t len);
********************************************************/

// #define ENABLE_DS18XXX_DEBUG_MODE

#ifdef ENABLE_DS18XXX_DEBUG_MODE
#include <Arduino.h>
#endif

// #include <map>
#include <vector>

class DS18XXX {
public:
	DS18XXX(const uint8_t *_romId, bool _isParasiteMode) {
		for (int i = 0; i < 8; i++) {
			RomId[i] = _romId[i];
		}
		IsParasiteMode = _isParasiteMode;
	}

	bool isMatchRomId(const uint8_t *_romId) const {
		for (int i = 0; i < 8; i++) {
			if (_romId[i] != RomId[i]) {
				return false;
			}
		}
		return true;
	}

	void debug() {
#ifdef ENABLE_DS18XXX_DEBUG_MODE
		Serial.printf("\n====================\n");
		for (int i = 0; i < 8; i++) {
			Serial.printf("%02X ", RomId[i]);
		}
		Serial.printf(" : %d\n", IsParasiteMode);
		Serial.printf("====================\n");
#endif
	}

	uint8_t RomId[8];
	bool IsParasiteMode;
};

template <class T>
class OW_DS18XXX {
public:
	OW_DS18XXX() {};
	OW_DS18XXX(T *one_wire) {
		setOneWire(one_wire);
	}

	void setOneWire(T *one_wire) {
		_one_wire = one_wire;
	}

	bool begin() {
		std::vector<DS18XXX>().swap(_oneWireDevices); // clear
		uint8_t addr[8]    = {0};
		_targetDeviceIndex = -1;

		_one_wire->reset();
		_one_wire->reset_search();

		while (_one_wire->search(addr)) {
			if (!validAddress(addr)) {
				// Serial.printf("Addr error\n");
				continue;
			}

			if (!validFamily(addr)) {
				// Serial.printf("Family error\n");
				continue;
			}

			if (!readScratchpad(addr)) {
				// Serial.printf("ScratchPad error\n");
				continue;
			}

			_oneWireDevices.emplace_back(addr, checkParasiteMode(addr));
		}
		if (_oneWireDevices.size() == 0) {
#ifdef ENABLE_DS18XXX_DEBUG_MODE
			Serial.printf("Device Num 0 error\n");
#endif
			return false;
		}

#ifdef ENABLE_DS18XXX_DEBUG_MODE
		for (int i = 0; i < _oneWireDevices.size(); i++) {
			_oneWireDevices[i].debug();
		}
#endif

		_one_wire->reset();
		return true;
	}

	bool setTargetDevice(int16_t index) {
		if (_oneWireDevices.size() <= index) {
			_targetDeviceIndex = -1;
		} else {
			_targetDeviceIndex = index;
		}

		return (_targetDeviceIndex < 0) ? false : true;
	}

	bool setTargetDevice(const uint8_t *addr) {
		return setTargetDevice(addr2index(addr));
	}

	void getTargetDeviceAddress(uint8_t *addr) {
		for (int i = 0; i < 8; i++) {
			addr[i] = _oneWireDevices[_targetDeviceIndex].RomId[i];
		}
	}

	bool isParasiteMode() {
		if (_targetDeviceIndex < 0) {
			return true;
		}
		return _oneWireDevices[_targetDeviceIndex].IsParasiteMode;
	}

	uint16_t getOneWireDeviceNum() {
		return _oneWireDevices.size();
	}

	uint8_t getDeviceFamily(uint16_t index) {
		if (_targetDeviceIndex < 0) {
			return 0x00;
		}
		return _oneWireDevices[_targetDeviceIndex].RomId[0];
	}

	String getFamilyName(uint8_t family_code) {
		const FamilyInfo *info = findFamily(family_code);
		return info ? String(info->name) : String("Unknown");
	}

	uint8_t getResolution() {
		if (!readScratchpad(_oneWireDevices[_targetDeviceIndex].RomId)) {
			return 0;
		}

		switch (_scratchPad.BYTE.CONFIG) {
		case 0b00011111:
			return 9;
		case 0b00111111:
			return 10;
		case 0b01011111:
			return 11;
		case 0b01111111:
			return 12;
		default:
			return 0;
		}
	}

	bool setResolution(uint8_t res) {
		switch (res) {
		case 9:
			_scratchPad.BYTE.CONFIG = 0b00011111;
			break;
		case 10:
			_scratchPad.BYTE.CONFIG = 0b00111111;
			break;
		case 11:
			_scratchPad.BYTE.CONFIG = 0b01011111;
			break;
		case 12:
			_scratchPad.BYTE.CONFIG = 0b01111111;
			break;
		default:
			return false;
		}

		return writeScratchpad(_oneWireDevices[_targetDeviceIndex].RomId);
	}

	void waitForConversion(uint16_t TIMEOUT_MS = 850) {
		uint8_t count       = 0;
		uint8_t count_limit = TIMEOUT_MS / 10;
		if (_oneWireDevices[_targetDeviceIndex].IsParasiteMode) {
			while (count <= count_limit) {
				delay(10);
				count++;
			}
		} else {
			while (!_one_wire->read_bit() && count <= count_limit) {
				delay(10);
				count++;
			}
		}
	}

	bool getLastMeasurementResult() {
		return _isLastConversionSuccess;
	}

	double getTempC() {
		const uint8_t CONVERT_T  = 0x44;
		_isLastConversionSuccess = false;

		if (!_one_wire->reset()) {
			return -999;
		}

		_one_wire->select(_oneWireDevices[_targetDeviceIndex].RomId);
		_one_wire->write(CONVERT_T);
		waitForConversion(_waitTimeMs);

		if (readScratchpad(_oneWireDevices[_targetDeviceIndex].RomId) == false) {
			return -999;
		}

		// if (_scratchPad.array[6] == 0x0C) {
		// 	// Error: read before measurement compleat
		// 	return -999;
		// }

		uint8_t lsb = _scratchPad.BYTE.TMP_LSB;
		uint8_t msb = _scratchPad.BYTE.TMP_MSB;
		uint8_t res = _scratchPad.BYTE.CONFIG;

		switch (res) {
		case 0b00011111:
			// 9
			lsb &= 0xF8;
			break;
		case 0b00111111:
			// 10
			lsb &= 0xFC;
			break;
		case 0b01011111:
			// 11
			lsb &= 0xFE;
			break;
		case 0b01111111:
			break;
		default:
			return 0;
		}

		uint8_t sign = msb & 0x80;
		int16_t temp = (msb << 8) + lsb;

		if (sign) {
			temp = ((temp ^ 0xffff) + 1) * -1;
		}

		_isLastConversionSuccess = true;
		return temp / 16.0;
	}

private:
	T *_one_wire;
	std::vector<DS18XXX> _oneWireDevices;
	int16_t _targetDeviceIndex    = -1;
	bool _isLastConversionSuccess = false;
	uint16_t _waitTimeMs          = 760; // 750ms + 10ms(buffer)
	union Scratchpad {
		uint8_t array[9];
		struct {
			uint8_t TMP_LSB;
			uint8_t TMP_MSB;
			uint8_t TH_REG;
			uint8_t TL_REG;
			uint8_t CONFIG;
			uint8_t RESERVED1;
			uint8_t RESERVED2;
			uint8_t RESERVED3;
			uint8_t CRC;
		} BYTE;
	};
	union Scratchpad _scratchPad;

	struct FamilyInfo {
		uint8_t code;
		const char *name;
	};

	// 共通のヘルパー関数
	static const FamilyInfo *findFamily(uint8_t family_code) {
		static const FamilyInfo VALID_FAMILIES[] = {
		    {0x10, "DS18S20"},
		    {0x28, "DS18B20"},
		    {0x22, "DS1822"},
		    {0x42, "DS28EA00"},
		};
		static const size_t FAMILY_COUNT = sizeof(VALID_FAMILIES) / sizeof(VALID_FAMILIES[0]);

		for (size_t i = 0; i < FAMILY_COUNT; i++) {
			if (family_code == VALID_FAMILIES[i].code) {
				return &VALID_FAMILIES[i];
			}
		}
		return nullptr;
	}

	int16_t addr2index(const uint8_t *addr) {
		for (int i = 0; i < _oneWireDevices.size(); i++) {
			if (_oneWireDevices[i].isMatchRomId(addr) == true) {
				return i;
			}
		}
		return -1;
	}

	bool checkParasiteMode() {
		const uint8_t READ_POWER_SUPPLY = 0xB4;

		if (!_one_wire->reset()) {
			return true;
		}

		//_one_wire->reset_search();
		_one_wire->skip();
		//_one_wire->select(addr);
		delay(3);
		_one_wire->write(READ_POWER_SUPPLY);
		uint8_t result = _one_wire->read();
		// Serial.printf("--- %d --- ", (bool)result);
		return !((bool)result);
	}

	bool checkParasiteMode(const uint8_t *addr) {
		const uint8_t READ_POWER_SUPPLY = 0xB4;

		if (!_one_wire->reset()) {
			return true;
		}

		//_one_wire->reset_search();
		// _one_wire->skip();
		_one_wire->select(addr);
		delay(3);
		_one_wire->write(READ_POWER_SUPPLY);
		uint8_t result = _one_wire->read();
		// Serial.printf("--- %d --- ", (bool)result);
		return !((bool)result);
	}

	bool validAddress(const uint8_t *addr) {
		return (_one_wire->crc8(addr, 7) == addr[7]);
	}

	bool validFamily(const uint8_t *addr) {
		return findFamily(addr[0]) != nullptr;
	}

	bool readScratchpad(const uint8_t *addr) {
		const uint8_t READ_SCRATCHPAD = 0xBE;

		if (!_one_wire->reset()) {
			return false;
		}

		_one_wire->select(addr);
		_one_wire->write(READ_SCRATCHPAD);
		_one_wire->read_bytes(_scratchPad.array, 9);

		// for (int i = 0; i < 9; i++) {
		// 	//_scratchPad.array[i] = _one_wire->read();
		// 	Serial.printf("0x%02X ", _scratchPad.array[i]);
		// }
		// Serial.println("");

		return (_one_wire->crc8(_scratchPad.array, 8) == _scratchPad.BYTE.CRC);
	}

	bool writeScratchpad(const uint8_t *addr) {
		const uint8_t WRITE_SCRATCHPAD = 0x4E;
		const uint8_t COPY_SCRATCHPAD  = 0x48;

		if (!_one_wire->reset()) {
			return false;
		}

		_one_wire->select(addr);
		_one_wire->write(WRITE_SCRATCHPAD);
		_one_wire->write(_scratchPad.BYTE.TH_REG);
		_one_wire->write(_scratchPad.BYTE.TL_REG);
		_one_wire->write(_scratchPad.BYTE.CONFIG);

		if (!_one_wire->reset()) {
			return false;
		}
		_one_wire->select(addr);
		_one_wire->write(COPY_SCRATCHPAD);

		return true;
	}
};

#endif