/*==========================================================================================
ESP32_SoftWire

ESP32 Arduino bit banged fast I2C library, drop in replacement for Wire.h

The library reaches up to 3 MHz I2C clock speed. Supports optional clock stretching with configurable timeout. Blocking only. Made for fast IMU sensor reading were an occasional missed read does not matter, but bus hangups do matter.

Limitation: pins 0-31 only

Tested on ESP32, might work on other ESP32 variants.

Background

As of December 2023 the official arduino-esp32 Wire library has some nasty bugs. In particular, there is a 1 second timeout which hangs the bus on a bad read, and this timeout can not be lowered without changing the source code, see espressif/esp-idf#4999 and espressif/arduino-esp32#5934. So occasionally (a couple times per minute) 1000 samples are missed when you are reading a sensor at 1 kHz.

MIT License

Copyright (c) 2023,2024 https://github.com/qqqlab

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
===========================================================================================*/

//-----------------------------------------------------------------------------------------
// I2C cheat sheet: M() master sends, S() slave sends
// Write: M(start) M(adr|1)S(ack) M(data)S(ack) M(data)S(nack/ack) M(stop)
// Read:  M(start) M(adr|0)S(ack) S(data)M(ack) S(data)M(nack) M(stop)
//-----------------------------------------------------------------------------------------

#include "ESP32_SoftWire.h"
#include <soc/gpio_struct.h>

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
  #define SDA_HI() GPIO.out_w1ts.val = _sda_mask // Do not drive SDA (set pin high-impedance)
  #define SDA_LO() GPIO.out_w1tc.val = _sda_mask // Actively drive SDA signal low
  #define SDA_IN() (GPIO.in.val & _sda_mask ? 1 : 0) // Return current level of SCL line, 0 or 1
  #define SCL_HI() GPIO.out_w1ts.val = _scl_mask // Do not drive SCL (set pin high-impedance)
  #define SCL_LO() GPIO.out_w1tc.val = _scl_mask // Actively drive SCL signal low
  #define SCL_IN() (GPIO.in.val & _scl_mask ? 1 : 0) // Return current level of SCL line, 0 or 1
#else
  #define SDA_HI() GPIO.out_w1ts = _sda_mask // Do not drive SDA (set pin high-impedance)
  #define SDA_LO() GPIO.out_w1tc = _sda_mask // Actively drive SDA signal low
  #define SDA_IN() (GPIO.in & _sda_mask ? 1 : 0) // Return current level of SCL line, 0 or 1
  #define SCL_HI() GPIO.out_w1ts = _scl_mask // Do not drive SCL (set pin high-impedance)
  #define SCL_LO() GPIO.out_w1tc = _scl_mask // Actively drive SCL signal low
  #define SCL_IN() (GPIO.in & _scl_mask ? 1 : 0) // Return current level of SCL line, 0 or 1
#endif

#define DELAY_OVERHEAD_CYCLES 26  //26 cycles overhead for PIN_CLR(); _delay();

SoftWire::SoftWire() {
  _sda = -1;
  _scl = -1;
  _scl_mask = 0;
  _sda_mask = 0;
  _started = false;
  _error = ESP32_SOFTWIRE_ERR_OK;
  _stretch_timeout_us = 0; // clock stretching disabled by default
  setClock(100000);
  _ll_stop_cond();
}

bool SoftWire::setPins(int sda, int scl) {
  if(sda<0 || sda>31 || scl<0 || scl>31) return false;
  pinMode(sda, OUTPUT_OPEN_DRAIN);
  pinMode(scl, OUTPUT_OPEN_DRAIN);
  _sda_mask = (1<<sda);
  _scl_mask = (1<<scl);
  _sda = sda;
  _scl = scl;
  return true;
}

bool SoftWire::begin(int sda, int scl, uint32_t frequency) {
  setPins(sda, scl);
  setClock(frequency);
  return true;
}

bool SoftWire::setClock(uint32_t frequency) {
  if(frequency == 0) return false;
  _delay_cycles = ESP.getCpuFreqMHz() * 500000 / frequency;
  if(_delay_cycles>DELAY_OVERHEAD_CYCLES) _delay_cycles -= DELAY_OVERHEAD_CYCLES; else _delay_cycles = 0;
  return true;
}

void SoftWire::_delay() {
  uint32_t t = ESP.getCycleCount();
  while(ESP.getCycleCount() - t < _delay_cycles);
}

void SoftWire::setClockStretchTimeout(uint32_t timeout_us) {
  _stretch_timeout_us = timeout_us;
}

uint32_t SoftWire::getClockStretchTimeout(void) const {
  return _stretch_timeout_us;
}

uint8_t SoftWire::getLastError(void) const {
  return _error;
}

// Wait for SCL to go HIGH (clock stretching support).
// Returns true if SCL is HIGH, false on timeout.
bool SoftWire::_clock_stretch_wait(void) {
  if (_stretch_timeout_us == 0) return true;  // disabled: skip check
  if (SCL_IN()) return true;                  // already HIGH: fast path

  uint32_t start = micros();
  while (!SCL_IN()) {
    if (micros() - start >= _stretch_timeout_us) {
      _error = ESP32_SOFTWIRE_ERR_TIMEOUT;
      return false;
    }
  }
  return true;
}

uint8_t SoftWire::beginTransmission(uint8_t address) {
  _error = ESP32_SOFTWIRE_ERR_OK;
  _ll_start_cond();
  if (_error) return _error;

  uint8_t nack = _ll_write_byte(address<<1);
  if (_error) return _error;
  if (nack) _error = ESP32_SOFTWIRE_ERR_NACK_ADDR;
  return _error;
}

size_t SoftWire::write(uint8_t data) {
  if (_error) return 0;
  uint8_t nack = _ll_write_byte(data);
  if (_error) return 0;
  if (nack) _error = ESP32_SOFTWIRE_ERR_NACK_DATA;
  return (nack ? 0 : 1);
}

uint8_t SoftWire::endTransmission(bool sendStop) {
  if(sendStop) {
    _ll_stop_cond();
  }
  return _error;
}

size_t SoftWire::requestFrom(uint8_t address, size_t len, bool stopBit) {
  _error = ESP32_SOFTWIRE_ERR_OK;
  _data_len = 0;
  _data_i = 0;

  if (len == 0) return 0;

  _ll_start_cond();
  if (_error) return 0;

  uint8_t nack = _ll_write_byte(address<<1 | 1);
  if (_error) return 0;
  if (nack) {
    _error = ESP32_SOFTWIRE_ERR_NACK_ADDR;
    if (stopBit) _ll_stop_cond();
    return 0;
  }

  size_t i;
  for(i=0; i<len-1; i++) {
    _data[i] = _ll_read_byte(true);
    if (_error) return 0;
  }
  _data[i++] = _ll_read_byte(false);
  if (_error) return 0;

  if(stopBit) _ll_stop_cond();
  _data_len = i;
  _data_i = 0;
  return i;
}

int SoftWire::available(void) {
  return _data_len - _data_i;
}

int SoftWire::read(void) {
  return _data[_data_i++];
}

int SoftWire::peek(void) {
  return _data[_data_i];
} 

void SoftWire::flush(void) { 
  _data_len = 0;
  _data_i = 0;
} 

size_t SoftWire::write(const uint8_t *data, size_t n) {
  size_t cnt = 0;
  for(size_t i=0;i<n;i++) {
    cnt += write(data[i]);
  }
  return cnt;
}

//========================================================================
// Low Level Bit-Bang I2C
//========================================================================

void SoftWire::_ll_start_cond(void) {
  if (_started) { 
    // if started, do a restart condition
    // set SDA to 1
    SDA_HI();
    _delay();
    SCL_HI();
    
    // Clock stretching: wait for SCL to go HIGH
    if (!_clock_stretch_wait()) return;

    // Repeated start setup time, minimum 4.7us
    _delay();
  }

  // SCL is high, set SDA from 1 to 0.
  SDA_LO();
  _delay();
  SCL_LO();
  _started = true;
}

void SoftWire::_ll_stop_cond(void) {
  // set SDA to 0
  SDA_LO();
  _delay();

  SCL_HI();
  
  // Clock stretching: wait for SCL to go HIGH
  if (!_clock_stretch_wait()) return;

  // Stop bit setup time, minimum 4us
  _delay();

  // SCL is high, set SDA from 0 to 1
  SDA_HI();
  _delay();

  _started = false;
}

// Write a bit to I2C bus
void SoftWire::_ll_write_bit(bool bit) {
  if (bit) {
    SDA_HI();
  } else {
    SDA_LO();
  }

  // SDA change propagation delay
  _delay();

  // Set SCL high to indicate a new valid SDA value is available
  SCL_HI();

  // Clock stretching: wait for SCL to go HIGH
  if (!_clock_stretch_wait()) return;

  // Wait for SDA value to be read by target, minimum of 4us for standard mode
  _delay();

  // Clear the SCL to low in preparation for next change
  SCL_LO();
}

// Read a bit from I2C bus
uint8_t SoftWire::_ll_read_bit(void) {
  uint8_t bit;

  // Let the target drive data
  SDA_HI();

  // Wait for SDA value to be written by target, minimum of 4us for standard mode
  _delay();

  // Set SCL high to indicate a new valid SDA value is available
  SCL_HI();

  // Clock stretching: wait for SCL to go HIGH
  if (!_clock_stretch_wait()) return 0;

  // Wait for SDA value to be written by target, minimum of 4us for standard mode
  _delay();

  // SCL is high, read out bit (0=sda low, 1=sda high)
  bit = (SDA_IN() ? 1 : 0);

  // Set SCL low in preparation for next operation
  SCL_LO();

  return bit;
}

// Write a byte to I2C bus. Return 0 if ack by the target, 1 if nack.
uint8_t SoftWire::_ll_write_byte(uint8_t byte) {
  for (uint8_t bit = 0; bit < 8; ++bit) {
    _ll_write_bit((byte & 0x80) != 0);
    byte <<= 1;
  }

  bool nack = _ll_read_bit(); //ack=sda low, nack=sda high

  return (nack ? 1 : 0);
}

// Read a byte from I2C bus
uint8_t SoftWire::_ll_read_byte(bool ack) {
  uint8_t byte = 0;
  uint8_t bit;

  for (bit = 0; bit < 8; ++bit) {
    byte = (byte << 1) | _ll_read_bit();
  }

  _ll_write_bit(!ack);

  return byte;
}
