/*! ==============================================================
 * @file: M5AmMeterUnit.h
 * @author: M5Stack
 * @modified: takkaO
 * @license: MIT License
 * @reference: https://github.com/m5stack/M5-ProductExampleCodes/tree/1bc38736e61a362080d1dffc4020b3a2a1dff6ae/Unit/A_Meter_Unit/ammeter
 * ============================================================= */

#ifndef M5_AM_METER_UNIT_H
#define M5_AM_METER_UNIT_H

#include <Arduino.h>
#include <Wire.h>

#define ADS1115_PGA_6144 0x00
#define ADS1115_PGA_4096 0x01
#define ADS1115_PGA_2048 0x02 // default
#define ADS1115_PGA_1024 0x03
#define ADS1115_PGA_512 0x04
#define ADS1115_PGA_256 0x05

class M5AmMeterUnit {
public:
	M5AmMeterUnit(TwoWire *theWire = &Wire);
	void setGain(uint8_t gain);
	double calculateAmpere(int16_t ad_value);

private:
	TwoWire *_wire;
	double resolution;
	double calibration_factor;

	double getResolution(uint8_t gain);
	uint8_t getPGAEEEPROMAddr(uint8_t gain);
	bool readCalibrationFromEEPROM(uint8_t gain,
	                               int16_t *hope,
	                               int16_t *actual);
	bool i2cReadBytes(uint8_t addr,
	                  uint8_t reg_addr,
	                  uint8_t *buff,
	                  uint16_t len);
	bool i2cWriteBytes(uint8_t addr,
	                   uint8_t reg_addr,
	                   uint8_t *buff,
	                   uint16_t len);
	bool i2cReadU16(uint8_t addr, uint8_t reg_addr, uint16_t *value);
	bool i2cWriteU16(uint8_t addr, uint8_t reg_addr, uint16_t value);
};

#endif /* M5_AM_METER_UNIT_H */