#include "I2C_OneWire.h"

I2C_OneWire::I2C_OneWire() {
}

I2C_OneWire::I2C_OneWire(TwoWire *wire, uint8_t address) {
	begin(wire, address);
}

void I2C_OneWire::begin(TwoWire *wire, uint8_t address) {
	_wire    = wire;
	_address = address;
}

I2C_OneWire::I2C_OneWire(uint8_t pin) {
	begin(pin);
}

void I2C_OneWire::begin(uint8_t pin) {
	// Nothing todo
}

uint8_t I2C_OneWire::reset() {
	return wireReset();
}

void I2C_OneWire::write_bit(uint8_t v) {
	wireWriteBit(v);
}

uint8_t I2C_OneWire::read_bit() {
	return wireReadBit();
}

void I2C_OneWire::write(uint8_t v, uint8_t power /* =0 */) {
	wireWriteByte(v);
}

void I2C_OneWire::write_bytes(const uint8_t *buf, uint16_t count, bool power /* =0 */) {
	for (uint16_t i = 0; i < count; i++) {
		write(buf[i]);
	}
}

uint8_t I2C_OneWire::read() {
	return wireReadByte();
}

void I2C_OneWire::read_bytes(uint8_t *buf, uint16_t count) {
	for (uint16_t i = 0; i < count; i++) {
		buf[i] = read();
	}
}

void I2C_OneWire::select(const uint8_t rom[8]) {
	write(0x55);
	for (int i = 0; i < 8; i++) {
		write(rom[i]);
	}
}

void I2C_OneWire::skip() {
	write(0xCC);
}

void I2C_OneWire::depower() {
	// Nothing todo
}

#if ONEWIRE_SEARCH

void I2C_OneWire::reset_search() {
	_searchLastDeviceFlag  = 0;
	_searchLastDiscrepancy = 0; // 0 means no discrepancy yet (using 1-based indexing)
	for (int i = 0; i < 8; i++) {
		_searchAddress[i] = 0;
	}
}

void I2C_OneWire::target_search(uint8_t family_code) {
	// Nothing todo
}

bool I2C_OneWire::search(uint8_t *newAddr, bool search_mode /* =true */) {
	uint8_t direction;
	uint8_t last_zero = 0;

	if (_searchLastDeviceFlag)
		return 0;

	if (!reset()) {
		return 0;
	}

	wireWriteByte((search_mode) ? 0xF0 : 0xEC);

	for (uint8_t i = 1; i <= 64; i++) {
		int searchByte = (i - 1) / 8;
		int searchBit  = 1 << ((i - 1) % 8);

		if (i < _searchLastDiscrepancy)
			direction = _searchAddress[searchByte] & searchBit;
		else
			direction = (i == _searchLastDiscrepancy);

		wireWriteTriplet(direction ? 0x80 : 0x00);
		busyWait();

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

#endif

#if ONEWIRE_CRC

uint8_t I2C_OneWire::crc8(const uint8_t *addr, uint8_t len) {
	uint8_t crc = 0;

	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix)
				crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}

#if ONEWIRE_CRC16

bool I2C_OneWire::check_crc16(const uint8_t *input, uint16_t len, const uint8_t *inverted_crc, uint16_t crc) {
	crc = ~crc16(input, len, crc);
	return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

uint16_t I2C_OneWire::crc16(const uint8_t *input, uint16_t len, uint16_t crc) {
	static const uint8_t oddparity[16] =
	    {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

	for (uint16_t i = 0; i < len; i++) {
		// Even though we're just copying a byte from the input,
		// we'll be doing 16-bit computation with it.
		uint16_t cdata = input[i];
		cdata          = (cdata ^ crc) & 0xff;
		crc >>= 8;

		if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
			crc ^= 0xC001;

		cdata <<= 6;
		crc ^= cdata;
		cdata <<= 1;
		crc ^= cdata;
	}
	return crc;
}

#endif
#endif