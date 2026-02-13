#ifndef I2C_ONEWIRE_H
#define I2C_ONEWIRE_H

#include <Arduino.h>
#include <Wire.h>
#include <array>
#include <set>
#include <vector>

// you can exclude onewire_search by defining that to 0
#ifndef ONEWIRE_SEARCH
#define ONEWIRE_SEARCH 1
#endif

#ifndef ONEWIRE_CRC
#define ONEWIRE_CRC 1
#endif

#ifndef ONEWIRE_CRC16
#define ONEWIRE_CRC16 1
#endif

class I2C_OneWire {
public:
	// Compatible APIs
	I2C_OneWire();
	I2C_OneWire(TwoWire *wire, uint8_t address);
	I2C_OneWire(uint8_t pin);
	virtual void begin(TwoWire *wire, uint8_t address);
	virtual void begin(uint8_t pin);
	virtual uint8_t reset();
	virtual void select(const uint8_t rom[8]);
	virtual void skip();
	virtual void write(uint8_t v, uint8_t power = 0);
	virtual void write_bytes(const uint8_t *buf, uint16_t count, bool power = 0);
	virtual uint8_t read();
	virtual void read_bytes(uint8_t *buf, uint16_t count);
	virtual void write_bit(uint8_t v);
	virtual uint8_t read_bit();
	virtual void depower();

#if ONEWIRE_SEARCH
	virtual void reset_search();
	virtual void target_search(uint8_t family_code);
	virtual bool search(uint8_t *newAddr, bool search_mode = true);
#endif // ONEWIRE_SEARCH

#if ONEWIRE_CRC
	static uint8_t crc8(const uint8_t *addr, uint8_t len);

#if ONEWIRE_CRC16
	static bool check_crc16(const uint8_t *input, uint16_t len, const uint8_t *inverted_crc, uint16_t crc = 0);
	static uint16_t crc16(const uint8_t *input, uint16_t len, uint16_t crc = 0);
#endif // ONEWIRE_CRC16
#endif // ONEWIRE_CRC

	void pushSensor(const uint8_t *address, uint8_t value_num) {
		std::array<uint8_t, 8> addr_array;
		std::copy(address, address + 8, addr_array.begin());

		for (uint8_t k = 0; k < value_num; k++) {
			_devices.push_back({addr_array, k});
		}
	}

	uint16_t getSensorValueCount() const {
		return _devices.size();
	}

	size_t getDeviceCount() const {
		std::set<std::array<uint8_t, 8>> unique_addresses;
		for (const auto &device : _devices) {
			unique_addresses.insert(device.address);
		}
		return unique_addresses.size();
	}

	const uint8_t* getDeviceAddress(uint16_t index) const {
		if (index < _devices.size()) {
			return _devices[index].address.data();
		}
		return nullptr;
	}

	uint8_t getDeviceValueId(uint16_t index) const {
		if (index < _devices.size()) {
			return _devices[index].value_id;
		}
		return 0xFF;
	}

protected:
	TwoWire *_wire;
	uint8_t _address;

	uint8_t _searchLastDiscrepancy = 0;
	uint8_t _searchLastDeviceFlag  = 0;
	uint8_t _searchAddress[8]      = {0};

	virtual uint8_t wireReset()              = 0;
	virtual uint8_t wireReadStatus()         = 0;
	virtual void wireWriteByte(uint8_t data) = 0;
	virtual uint8_t wireReadByte()           = 0;
	virtual void wireWriteBit(uint8_t v)     = 0;
	virtual uint8_t wireReadBit()            = 0;
	virtual void wireWriteTriplet(uint8_t v) = 0;
	virtual void busyWait()                  = 0;

	virtual bool getOneWireBusy(uint8_t status)             = 0;
	virtual uint8_t getSingleBitRead(uint8_t status)        = 0;
	virtual uint8_t getTripletSecondBit(uint8_t status)     = 0;
	virtual uint8_t getBranchDirectionTaken(uint8_t status) = 0;
	virtual uint8_t getPresencePulseDetect(uint8_t status)  = 0;

	struct Device {
		std::array<uint8_t, 8> address; // 1-wire アドレス
		uint8_t value_id;               // センサ値識別番号
	};
	std::vector<Device> _devices;
};

#endif