/*!
 * @auther: Seeed Technology Co.,Ltd.
 * @page: https://wiki.seeedstudio.com/Grove-Water-Level-Sensor/#software
 * @license: MIT License (https://wiki.seeedstudio.com/License/)
 */

/*!
 * @modified: takkaO
 * @license: MIT License
 */

#ifndef GROVE_WATER_LEVEL_SENSOR
#define GROVE_WATER_LEVEL_SENSOR

#include <Arduino.h>
#include <Wire.h>

class GroveWaterLevelSensor {
public:
	GroveWaterLevelSensor(TwoWire *theWire = &Wire) {
		_wire = theWire;
	}
	bool begin();
	int8_t fetchWaterLevel(uint8_t threthold = 100);

private:
	TwoWire *_wire;
	unsigned char low_data[8]   = {0};
	unsigned char high_data[12] = {0};

	bool getLow8SectionValue(void);
	bool getHigh8SectionValue(void);
};

#endif