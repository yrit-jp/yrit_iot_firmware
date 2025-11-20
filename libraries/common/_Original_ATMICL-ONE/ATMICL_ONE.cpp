/*!
 * ATMICL-ONE.cpp - Library for ATMICL-ONE current sensor
 *
 * Copyright (c) 2024 takkaO
 *
 * This software is released under the MIT.
 * see https://opensource.org/license/MIT
 */

#include "ATMICL_ONE.h"

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Public
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

/*!
 * @brief ATMICL-ONE センサの初期化を行う
 * @param[in] (address) 以降使用するI2Cスレーブアドレスを指定する
 * @return 初期化成功の場合はtrue, 失敗すればfalseを返す
 * @note I2Cモードであること。自動的にオフセットはゼロにされます。
 */
bool ATMICL_ONE::begin(uint8_t address) {
	DEVICE_ADDRESS = address;

	return adjustOffset(0);
}

/*!
 * @brief 交流電源の電流値（A）を取得する
 * @param[in] (*data) 電流値取得用のポインタ
 * @return 取得成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルはreadAC関数内で定義されています
 */
bool ATMICL_ONE::readAcCurrent(double *data) {
	if (!readAC()) {
		return false;
	}

	*data = (double)_ac.current / 100.0;

	return true;
}

/*!
 * @brief 交流電源の周波数（Hz）を取得する
 * @param[in] (*data) 周波数取得用のポインタ
 * @return 取得成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルはreadAC関数内で定義されています
 */
bool ATMICL_ONE::readAcFrequency(double *data) {
	if (!readAC()) {
		return false;
	}

	*data = (double)_ac.frequency / 100.0;

	return true;
}

/*!
 * @brief 直流電源のカウンタを取得する
 * @param[in] (*data) カウント値取得用のポインタ
 * @return 取得成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルはreadDC関数内で定義されています
 */
bool ATMICL_ONE::readDcCounter(uint32_t *data) {
	if (!readDC()) {
		return false;
	}

	*data = _dc.counter;

	return true;
}

/*!
 * @brief 直流電源の状態（ON/OFF）を取得する
 * @param[in] (*data) 状態値取得用のポインタ
 * @return 取得成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルはreadDC関数内で定義されています
 */
bool ATMICL_ONE::readDcState(uint16_t *data) {
	if (!readDC()) {
		return false;
	}

	*data = (uint16_t)_dc.state;

	return true;
}

/*!
 * @brief 直流電源のカウント周波数（Hz）を取得する
 * @param[in] (*data) カウント周波数取得用のポインタ
 * @return 取得成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルはreadDC関数内で定義されています
 */
bool ATMICL_ONE::readDcFrequency(uint16_t *data) {
	if (!readDC()) {
		return false;
	}

	*data = _dc.frequency;

	return true;
}

/*!
 * @brief センサのI2Cアドレス変更関数
 * @param[in] (address) 新しく設定するアドレス値
 * @return 成功の場合はtrue, 失敗すればfalseを返す
 */
bool ATMICL_ONE::changeAddress(uint8_t address) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xA0);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xAA);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xA5);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(address);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);
	DEVICE_ADDRESS = address;

	return true;
}

/*!
 * @brief センサ値のオフセットを指定します
 * @param[in] (adj) 指定するオフセット値（-100 ~ 100）
 * @return 成功の場合はtrue, 失敗すればfalseを返す
 */
bool ATMICL_ONE::adjustOffset(int8_t adj) {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xA0);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xF2);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(adj);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	return true;
}

/*!
 * @brief ゼロ点補正関数
 * @return 成功の場合はtrue, 失敗すればfalseを返す
 */
bool ATMICL_ONE::setZeroPoint() {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xA0);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xF1);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	return true;
}

/*!
 * @brief DCカウントリセット関数
 * @return 成功の場合はtrue, 失敗すればfalseを返す
 */
bool ATMICL_ONE::clearDcCounter() {
	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xA0);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	_wire->beginTransmission(DEVICE_ADDRESS);
	_wire->write(0x00);
	_wire->write(0xF9);
	if (_wire->endTransmission() != 0) {
		// Error
		return false;
	}

	delay(100);

	return true;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Private
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

/*!
 * @brief 交流電源の電流値と周波数を取得する関数
 * @return 成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルは1000msに設定されています。
 */
bool ATMICL_ONE::readAC() {
	const int READ_INTERVAL_MS = 1000;
	if ((millis() - _last_read_time) < READ_INTERVAL_MS) {
		return true;
	}

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)5, (uint8_t) true);
	if (num != 5) {
		return false;
	}

	_ac.current = _wire->read();                      // 0: trash data?
	_ac.current = _wire->read();                      // 1:
	_ac.current = (_ac.current << 8) + _wire->read(); // 2:

	_ac.frequency = _wire->read();                        // 3:
	_ac.frequency = (_ac.frequency << 8) + _wire->read(); // 4:

	_last_read_time = millis();
	return true;
}

/*!
 * @brief 直流電源のカウント値、状態、カウント周波数を取得する関数
 * @return 成功の場合はtrue, 失敗すればfalseを返す
 * @note 最小取得インターバルは1000msに設定されています。
 */
bool ATMICL_ONE::readDC() {
	const int READ_INTERVAL_MS = 1000;
	if ((millis() - _last_read_time) < READ_INTERVAL_MS) {
		return true;
	}

	uint8_t num = _wire->requestFrom(DEVICE_ADDRESS, (uint8_t)8, (uint8_t) true);
	if (num != 8) {
		return false;
	}

	_dc.state = _wire->read(); // 0: Dummy
	_dc.state = _wire->read(); // 1: ON/OFF

	_dc.counter = _wire->read();                      // 2:
	_dc.counter = (_dc.counter << 8) + _wire->read(); // 3:
	_dc.counter = (_dc.counter << 8) + _wire->read(); // 4:
	_dc.counter = (_dc.counter << 8) + _wire->read(); // 5:

	_dc.frequency = _wire->read();                        // 6:
	_dc.frequency = (_dc.frequency << 8) + _wire->read(); // 7:

	_last_read_time = millis();
	return true;
}