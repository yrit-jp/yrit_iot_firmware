/*!
 * @modified: takkaO
 * @license: MIT License
 * @reference: https://github.com/m5stack/M5Unit-Sonic/tree/master
 */

#include "RCWL9620.h"

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Public
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

/*!
 * @brief センサの初期化を行う
 * @param[in] (address) 以降使用するI2Cスレーブアドレスを指定する
 * @return 初期化成功の場合はtrue, 失敗すればfalseを返す
 */
bool RCWL9620::begin(uint8_t address) {
	DEVICE_ADDRESS = address;

	double tmp;
	return getDistance(&tmp);
}

/*!
 * @brief 距離を取得する
 * @param[out] (*distance) 計測した距離。ただし、上限値は4500に切り捨てられる
 * @return 取得成功の場合はtrue, 失敗すればfalseを返す
 */
bool RCWL9620::getDistance(double *distance) {
	uint32_t data = 0;
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x01);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(120);

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)3, (uint8_t)true);
	if (num != 3) {
		return false;
	}

	data = _wire->read();
	data <<= 8;
	data |= _wire->read();
	data <<= 8;
	data |= _wire->read();

	*distance = data / 1000.0;
	if (*distance > 4500.00) {
		*distance = 4500.00;
	}

	return true;
}