/*
 * Seeed_#defineMCP9600.h
 * Driver for mcp9600
 *
 * Copyright (c) 2018 Seeed Technology Co., Ltd.
 * Website    : www.seeed.cc
 * Author     : downey
 * Create Time: May 2018
 * License    : MIT License (MIT)
 */

/*
 * Modified by : takkaO
 * Change Time : Jan 2023
 * License     : MIT License (MIT)
 */

#include "MCP9600.h"

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//   APIs (Public)
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

MCP9600::MCP9600(TwoWire *wire, uint8_t slave_address) {
	_wire    = wire;
	_address = slave_address;
}

/**@brief set type of thermocouple.read version.
 * @param therm_type
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::init(uint8_t therm_type) {
	MCP9600::err_t ret = MCP9600::NO_ERROR;
	uint16_t ver;

	ret = read_version(&ver);	// this is here because the first read doesn't seem to work, but the second does. No idea why :/
	ret = read_version(&ver);
	if (ret != MCP9600::NO_ERROR) {
		return ret;
	}

	constexpr uint8_t DEVICE_ID = 0x40;
	if ((ver >> 8) != DEVICE_ID) {
		return MCP9600::ERROR_OTHERS;
	}
	return set_therm_type(therm_type);
}

/**@brief read version.
 * @param ver.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_version(uint16_t *ver) {
	if (I2C_read_16bit(MCP9600::VERSION_ID_REG_ADDR, ver)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read hot-junction,the temperature result.
 * @param value: result.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_hot_junc(double *value) {
	*value              = 0;
	uint16_t read_value = 0;
	if (I2C_read_16bit(MCP9600::HOT_JUNCTION_REG_ADDR, &read_value)) {
		return MCP9600::ERROR_COMM;
	}
	// Serial.print("read hot junc value=");
	// Serial.println(read_value,HEX);
	if (read_value & 0x8000) {
		*value = (read_value >> 8) * 16.0 + (read_value & 0x00ff) / 16.0 - 4096.0;
	} else {
		*value = (read_value >> 8) * 16.0 + (read_value & 0x00ff) / 16.0;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read junction delta.
 * @param value: result.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_junc_temp_delta(double *value) {
	*value              = 0;
	uint16_t read_value = 0;
	if (I2C_read_16bit(MCP9600::JUNCTION_TEMP_DELTA_REG_ADDR, &read_value)) {
		return MCP9600::ERROR_COMM;
	}
	if (read_value & 0x8000) {
		*value = (read_value >> 8) * 16.0 + (read_value & 0x00ff) / 16.0 - 4096.0;
	} else {
		*value = (read_value >> 8) * 16.0 + (read_value & 0x00ff) / 16.0;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read cold-junction.
 * @param value: result.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_cold_junc(double *value) {
	*value              = 0;
	uint16_t read_value = 0;
	if (I2C_read_16bit(MCP9600::COLD_JUNCTION_TEMP_REG_ADDR, &read_value)) {
		return MCP9600::ERROR_COMM;
	}

	if (read_value & 0xf000) {
		*value = (read_value >> 8) * 16.0 + (read_value & 0x00ff) / 16.0 - 4096;
	} else {
		*value = (read_value >> 8) * 16.0 + (read_value & 0x00ff) / 16.0;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read raw ADC value.
 * @param data
 * @param data_len
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_ADC_data(uint8_t *data, uint32_t data_len) {
	if (I2C_read_bytes(MCP9600::RAW_ADC_DATA_REG_ADDR, data, data_len)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read sensor status.
 * @param byte: result.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_status(uint8_t *byte) {
	*byte = 0;
	if (I2C_read_byte(MCP9600::STAT_REG_ADDR, byte)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief set_therm_cfg
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_therm_cfg(uint8_t set_byte) {
	if (I2C_write_byte(MCP9600::THERM_SENS_CFG_REG_ADDR, set_byte)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read thermocouple status.
 * @param byte: result.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_therm_cfg(uint8_t *byte) {
	if (I2C_read_byte(MCP9600::THERM_SENS_CFG_REG_ADDR, byte)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief set_therm_type
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_therm_type(uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::THERM_SENS_CFG_REG_ADDR, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}

	byte_to_set = (therm_cfg_data & 0x8f) | set_byte;

	return I2C_write_byte(MCP9600::THERM_SENS_CFG_REG_ADDR, byte_to_set);
}

/**@brief set_filt_coefficients
 * The  content of filt-coefficients register are:code-junction-resolution/ADC measurement resolution/burst mode temp samples/sensor mode
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_filt_coefficients(uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::THERM_SENS_CFG_REG_ADDR, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}

	byte_to_set = (therm_cfg_data & 0xf8) | set_byte;

	return I2C_write_byte(MCP9600::THERM_SENS_CFG_REG_ADDR, byte_to_set);
}

/**@brief set_dev_cfg
 * The content of device configuration register are:interrupt clear/monitor TH or TC/inerrupt pin rise or FALL/
 *                                                  Active-high or low/compare mode or int mode /alert enable or not
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_dev_cfg(uint8_t set_byte) {
	if (I2C_write_byte(MCP9600::DEVICE_CFG_REG_ADDR, set_byte)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief read_dev_cfg
 * @param byte: the byte to be read in.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_dev_cfg(uint8_t *byte) {
	if (I2C_read_byte(MCP9600::DEVICE_CFG_REG_ADDR, byte)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief set_sensor_mode
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_sensor_mode(uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::DEVICE_CFG_REG_ADDR, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0xfc) | set_byte;
	return I2C_write_byte(MCP9600::DEVICE_CFG_REG_ADDR, byte_to_set);
}

/**@brief set_burst_mode_samp
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_burst_mode_samp(uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::DEVICE_CFG_REG_ADDR, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0xe3) | set_byte;
	return I2C_write_byte(MCP9600::DEVICE_CFG_REG_ADDR, byte_to_set);
}

/**@brief set_ADC_meas_resolution
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_ADC_meas_resolution(uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::DEVICE_CFG_REG_ADDR, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0x9f) | set_byte;
	return I2C_write_byte(MCP9600::DEVICE_CFG_REG_ADDR, byte_to_set);
}

/**@brief set_cold_junc_resolution
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_cold_junc_resolution(uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::DEVICE_CFG_REG_ADDR, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0x7f) | set_byte;
	return I2C_write_byte(MCP9600::DEVICE_CFG_REG_ADDR, byte_to_set);
}

/**@brief set_alert_limitation
 * @param alert num the channel of alert
 * @param value: the 16bit value to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_limit(uint8_t alert_num, uint16_t value) {
	if (I2C_write_16bit(MCP9600::TEMP_ALERT1_LIMIT_REG_ADDR + alert_num, value)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief set_alert_hysteresis
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set..
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_hys(uint8_t alert_num, uint16_t value) {
	if (I2C_write_byte(MCP9600::ALERT1_HYS_REG_ADDR + alert_num, value)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief set_alert_cfg
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_cfg(uint8_t alert_num, uint8_t set_byte) {
	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, set_byte);
}

/**@brief read_alert_cfg
 * @param alert num the channel of alert
 * @param byte: the byte to be read in.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_alert_cfg(uint8_t alert_num, uint8_t *byte) {
	*byte = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte)) {
		return MCP9600::ERROR_COMM;
	}
	return MCP9600::NO_ERROR;
}

/**@brief clear_int_flag
 * @param alert num the channel of alert
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::clear_int_flag(uint8_t alert_num) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data | 0x80);
	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte_to_set);
}

/**@brief set_alert_for_TH_or_TC
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_for_TH_or_TC(uint8_t alert_num, uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0xef) | set_byte;
	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte_to_set);
}

/**@brief set_alert_limit_direction
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_limit_direction(uint8_t alert_num, uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0xf7) | set_byte;
	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte_to_set);
}

/**@brief set_alert_bit
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_bit(uint8_t alert_num, uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}

	byte_to_set = (therm_cfg_data & 0xfb) | set_byte;

	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte_to_set);
}

/**@brief set_alert_mode_bit
 * set alert mode:comparator mode or INT mode.
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_mode_bit(uint8_t alert_num, uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0xfd) | set_byte;
	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte_to_set);
}

/**@brief set_alert_enable
 * Eable alert pin or not.
 * @param alert num the channel of alert
 * @param set_byte: the byte to be set.
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::set_alert_enable(uint8_t alert_num, uint8_t set_byte) {
	uint8_t therm_cfg_data = 0;
	uint8_t byte_to_set    = 0;
	if (I2C_read_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, &therm_cfg_data)) {
		return MCP9600::ERROR_COMM;
	}
	byte_to_set = (therm_cfg_data & 0xfe) | set_byte;
	return I2C_write_byte(MCP9600::ALERT1_CFG_REG_ADDR + alert_num, byte_to_set);
}

/**@brief check_data_update
 * check if data ready.
 * @param stat :indicate if data ready
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::check_data_update(bool *stat) {
	*stat        = 0;
	uint8_t byte = 0;

	MCP9600::err_t ret = read_status(&byte);
	if (ret != MCP9600::NO_ERROR) {
		return ret;
	}

	if (byte & 0x40) {
		*stat = true;
	} else {
		*stat = false;
	}
	return ret;
}

/**@brief read_INT_stat
 * check if any interruption is generated.
 * @param stat :indicate if any interruption is generated
 * @return 0 if successed.
 * */
MCP9600::err_t MCP9600::read_INT_stat(uint8_t *stat) {
	*stat        = 0;
	uint8_t byte = 0;

	MCP9600::err_t ret = read_status(&byte);
	if (ret != MCP9600::NO_ERROR) {
		return ret;
	}
	// for (int i = 0; i < 4; i++) {
	// 	if (byte & 1 << i) {
	// 		Serial.print("channel ");
	// 		Serial.print(i);
	// 		Serial.println("generate interruption!!!");
	// 	}
	// }
	*stat = byte;
	return ret;
}

uint16_t MCP9600::covert_temp_to_reg_form(double temp) {
	uint8_t negetive = 0;
	if (temp < 0) {
		negetive = 1;
	}
	temp               = abs(temp);
	uint16_t dest_temp = 0;
	uint8_t temp_H = 0, temp_L = 0;
	uint16_t interger = (uint16_t)temp;
	double decimal    = temp - interger;
	temp_H            = interger / 16;
	temp_L |= (interger % 16) << 4;
	temp_L |= (uint8_t)(decimal / 0.25) << 2;
	if (negetive) {
		temp_H |= 0x80;
	}
	dest_temp = (uint16_t)temp_H << 8 | temp_L;
	return dest_temp;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//   APIs (Private)
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

MCP9600::err_t MCP9600::I2C_write_byte(uint8_t reg, uint8_t byte) {
	uint32_t ret = 0;
	_wire->beginTransmission(_address);
	_wire->write(reg);
	_wire->write(byte);
	ret = _wire->endTransmission();
	return ((!ret) ? MCP9600::NO_ERROR : MCP9600::ERROR_COMM);
}

MCP9600::err_t MCP9600::I2C_write_16bit(uint8_t reg, uint16_t value) {
	uint32_t ret = 0;
	_wire->beginTransmission(_address);
	_wire->write(reg);

	_wire->write((uint8_t)(value >> 8));
	_wire->write((uint8_t)value);
	ret = _wire->endTransmission();
	return ((!ret) ? MCP9600::NO_ERROR : MCP9600::ERROR_COMM);
}

MCP9600::err_t MCP9600::I2C_read_byte(uint8_t reg, uint8_t *byte) {
	uint8_t time_out_count = 0;
	_wire->beginTransmission(_address);
	_wire->write(reg);
	_wire->endTransmission();

	_wire->requestFrom(_address, (uint8_t)1, (uint8_t) true);
	while (1 != _wire->available()) {
		time_out_count++;
		if (time_out_count > 10) {
			return MCP9600::ERROR_COMM;
		}
		delay(1);
	}
	*byte = _wire->read();
	return MCP9600::NO_ERROR;
}

MCP9600::err_t MCP9600::I2C_read_16bit(uint8_t start_reg, uint16_t *value) {
	uint8_t time_out_count = 0;
	uint8_t val            = 0;
	*value                 = 0;

	_wire->beginTransmission(_address);
	_wire->write(start_reg);
	_wire->endTransmission();

	_wire->requestFrom(_address, (uint8_t)2, (uint8_t) true);
	while (2 != _wire->available()) {
		time_out_count++;
		if (time_out_count > 10) {
			return MCP9600::ERROR_COMM;
		}
		delay(1);
	}

	val = _wire->read();
	*value |= (uint16_t)val << 8;
	val = _wire->read();
	*value |= val;
	return MCP9600::NO_ERROR;
}

MCP9600::err_t MCP9600::I2C_read_bytes(uint8_t start_reg, uint8_t *data, uint32_t data_len) {
	uint8_t time_out_count = 0;
	_wire->beginTransmission(_address);
	_wire->write(start_reg);
	_wire->endTransmission();

	_wire->requestFrom(_address, data_len);
	while (data_len != _wire->available()) {
		time_out_count++;
		if (time_out_count > 10) {
			return MCP9600::ERROR_COMM;
		}
		delay(1);
	}

	for (int i = 0; i < data_len; i++) {
		data[i] = _wire->read();
	}
	return MCP9600::NO_ERROR;
}