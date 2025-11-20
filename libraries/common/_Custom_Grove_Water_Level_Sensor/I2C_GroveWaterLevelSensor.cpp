/*!
 * @auther: Seeed Technology Co.,Ltd.
 * @page: https://wiki.seeedstudio.com/Grove-Water-Level-Sensor/#software
 * @license: MIT License (https://wiki.seeedstudio.com/License/)
 */

/*!
 * @modified: takkaO
 * @license: MIT License
 */


#include "I2C_GroveWaterLevelSensor.h"

bool GroveWaterLevelSensor::begin() {
	return getLow8SectionValue() && getHigh8SectionValue();
}

int8_t GroveWaterLevelSensor::fetchWaterLevel(uint8_t threthold) {
	uint32_t touch_val = 0;
	uint8_t trig_section = 0;

	if (getLow8SectionValue() == false){
		return -1;
	}
	if (getHigh8SectionValue() == false){
		return -1;
	}

	for (int i = 0; i < 8; i++) {
		if (low_data[i] > threthold) {
			touch_val |= 1 << i;
		}
	}
	for (int i = 0; i < 12; i++) {
		if (high_data[i] > threthold) {
			touch_val |= (uint32_t)1 << (8 + i);
		}
	}

	while (touch_val & 0x01) {
		trig_section++;
		touch_val >>= 1;
	}
	return trig_section * 5;
}

bool GroveWaterLevelSensor::getLow8SectionValue() {
	unsigned long _start = millis();
	const uint8_t ATTINY2_LOW_ADDR  = 0x77;

	memset(low_data, 0, sizeof(low_data));
	_wire->requestFrom(ATTINY2_LOW_ADDR, (uint8_t)8);
	while (8 != _wire->available() && (millis() - _start) <= 100) {
	}

	if ((millis() - _start) > 1000) {
		return false;
	}

	for (int i = 0; i < 8; i++) {
		low_data[i] = _wire->read(); // receive a byte as character
	}
	return true;
}

bool GroveWaterLevelSensor::getHigh8SectionValue() {
	unsigned long _start = millis();
	const uint8_t ATTINY1_HIGH_ADDR = 0x78;

	memset(high_data, 0, sizeof(high_data));
	_wire->requestFrom(ATTINY1_HIGH_ADDR, (uint8_t)12);
	while (12 != _wire->available() && (millis() - _start) <= 100) {
	}

	if ((millis() - _start) > 1000) {
		return false;
	}

	for (int i = 0; i < 12; i++) {
		high_data[i] = _wire->read();
	}
	return true;
}