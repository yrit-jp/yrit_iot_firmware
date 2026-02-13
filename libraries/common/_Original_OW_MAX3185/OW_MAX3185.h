#ifndef OW_MAX3185_H
#define OW_MAX3185_H

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

// #define ENABLE_MAX3185_DEBUG_MODE

#ifdef ENABLE_MAX3185_DEBUG_MODE
#include <Arduino.h>
#endif

#include <vector>

class MAX3185 {
public:
	enum CircuitState {
		CIRCUIT_OK,
		CIRCUIT_SHORT_TO_VDD,
		CIRCUIT_SHORT_TO_GND,
		CIRCUIT_OPEN
	};

	MAX3185(const uint8_t *_romId, bool _isParasiteMode) {
		for (int i = 0; i < 8; i++) {
			RomId[i] = _romId[i];
		}
		IsParasiteMode = _isParasiteMode;
	}

	bool isMatchRomId(const uint8_t *_romId) {
		for (int i = 0; i < 8; i++) {
			if (_romId[i] != RomId[i]) {
				return false;
			}
		}
		return true;
	}

	void debug() {
#ifdef ENABLE_MAX3185_DEBUG_MODE
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
class OW_MAX3185 {
public:
	OW_MAX3185() {};
	OW_MAX3185(T *one_wire) {
		setOneWire(one_wire);
	}

	void setOneWire(T *one_wire) {
		_one_wire = one_wire;
	}

	bool begin() {
		std::vector<MAX3185>().swap(_oneWireDevices); // clear
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
#ifdef ENABLE_MAX3185_DEBUG_MODE
			Serial.printf("Device Num 0 error\n");
#endif
			return false;
		}

#ifdef ENABLE_MAX3185_DEBUG_MODE
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

	bool convertT() {
		const uint8_t CONVERT_T = 0x44;
		if (!_one_wire->reset()) {
			return false;
		}
		_one_wire->select(_oneWireDevices[_targetDeviceIndex].RomId);
		_one_wire->write(CONVERT_T);

		delay(_waitTimeMs);

		if (readScratchpad(_oneWireDevices[_targetDeviceIndex].RomId) == false) {
			return false;
		}

		// Fault bit のチェック
		if ((_scratchPad.BYTE.CJ_COMP_THERMOCOUPLE_TEMP_LSB & 0x01) == 0) {
			_temperature_updated      = true;
			_cold_temperature_updated = true;
			return true;
		}
		return false;
	}

	enum MAX3185::CircuitState getCircuitState() {
		uint8_t status = _scratchPad.BYTE.INTERNAL_TEMPERATURE_LSB;
		if (status & 0x01) {
			return MAX3185::CIRCUIT_OPEN;
		}
		if (status & 0x02) {
			return MAX3185::CIRCUIT_SHORT_TO_GND;
		}
		if (status & 0x04) {
			return MAX3185::CIRCUIT_SHORT_TO_VDD;
		}
		return MAX3185::CIRCUIT_OK;
	}

	bool checkTemperatureUpdated() {
		return _temperature_updated;
	}

	bool checkColdTemperatureUpdated() {
		return _cold_temperature_updated;
	}

	double getTempC() {
		uint8_t lsb = _scratchPad.BYTE.CJ_COMP_THERMOCOUPLE_TEMP_LSB;
		uint8_t msb = _scratchPad.BYTE.CJ_COMP_THERMOCOUPLE_TEMP_MSB;
		int16_t val = (msb << 8) + lsb;

		constexpr uint8_t MSB_NUMBER = 14;   // 最小を0番目とカウントしたときの最大有効ビット
		constexpr uint8_t LSB_NUMBER = 2;    // 最小を0番目とカウントしたときの最小有効ビット
		constexpr double MIN_VALUE   = 0.25; // とりうる値の中で 0 を除く最小値

		bool isNegative = (val & 0x8000) != 0; // 符号ビット (MSB) のチェック
		double result   = 0.0;
		double weight   = 1024.0; // 2^10 = 1024（最上位の有効ビット）

		uint16_t absVal = isNegative ? (~val & 0x7FFF) : val; // 2の補数を求める

		for (int i = MSB_NUMBER; i >= LSB_NUMBER; --i) { // 14ビット目から2ビット目まで処理
			if (absVal & (1 << i)) {
				result += weight;
			}
			weight /= 2.0; // 次のビットの重みを半分に
		}

		_temperature_updated = false;
		return isNegative ? -(result + MIN_VALUE) : result;
	}

	double getColdTempC() {
		uint8_t lsb = _scratchPad.BYTE.INTERNAL_TEMPERATURE_LSB;
		uint8_t msb = _scratchPad.BYTE.INTERNAL_TEMPERATURE_MSB;
		int16_t val = (msb << 8) + lsb;

		constexpr uint8_t MSB_NUMBER = 14;     // 最小を0番目とカウントしたときの最大有効ビット
		constexpr uint8_t LSB_NUMBER = 4;      // 最小を0番目とカウントしたときの最小有効ビット
		constexpr double MIN_VALUE   = 0.0625; // とりうる値の中で 0 を除く最小値

		bool isNegative = (val & 0x8000) != 0; // 符号ビット (MSB) のチェック
		double result   = 0.0;
		double weight   = 64.0; // 2^6 = 64（最上位の有効ビット）

		uint16_t absVal = isNegative ? (~val & 0x7FFF) : val; // 2の補数を求める

		for (int i = MSB_NUMBER; i >= LSB_NUMBER; --i) { // 14ビット目から2ビット目まで処理
			if (absVal & (1 << i)) {
				result += weight;
			}
			weight /= 2.0; // 次のビットの重みを半分に
		}

		_cold_temperature_updated = false;
		return isNegative ? -(result + MIN_VALUE) : result;
	}

private:
	T *_one_wire;
	std::vector<MAX3185> _oneWireDevices;
	int16_t _targetDeviceIndex     = -1;
	uint16_t _waitTimeMs           = 100;
	bool _temperature_updated      = false; // 更新監視用フラグ
	bool _cold_temperature_updated = false; // 更新監視用フラグ
	union Scratchpad {
		uint8_t array[9];
		struct {
			uint8_t CJ_COMP_THERMOCOUPLE_TEMP_LSB;
			uint8_t CJ_COMP_THERMOCOUPLE_TEMP_MSB;
			uint8_t INTERNAL_TEMPERATURE_LSB;
			uint8_t INTERNAL_TEMPERATURE_MSB;
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
		    {0x3B, "MAX3185"},
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
		// const uint8_t WRITE_SCRATCHPAD = 0x4E;
		// const uint8_t COPY_SCRATCHPAD  = 0x48;

		// if (!_one_wire->reset()) {
		// 	return false;
		// }

		// _one_wire->select(addr);
		// _one_wire->write(WRITE_SCRATCHPAD);
		// _one_wire->write(_scratchPad.BYTE.TH_REG);
		// _one_wire->write(_scratchPad.BYTE.TL_REG);
		// _one_wire->write(_scratchPad.BYTE.CONFIG);

		// if (!_one_wire->reset()) {
		// 	return false;
		// }
		// _one_wire->select(addr);
		// _one_wire->write(COPY_SCRATCHPAD);

		return true;
	}
};

#endif