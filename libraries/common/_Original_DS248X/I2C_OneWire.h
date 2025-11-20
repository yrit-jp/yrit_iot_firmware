#ifndef I2C_ONEWIRE_H
#define I2C_ONEWIRE_H

#include <Arduino.h>
#include <Wire.h>

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
#endif

#if ONEWIRE_CRC
	static uint8_t crc8(const uint8_t *addr, uint8_t len);

#if ONEWIRE_CRC16
	static bool check_crc16(const uint8_t *input, uint16_t len, const uint8_t *inverted_crc, uint16_t crc = 0);
	static uint16_t crc16(const uint8_t *input, uint16_t len, uint16_t crc = 0);
#endif
#endif

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
};

#endif