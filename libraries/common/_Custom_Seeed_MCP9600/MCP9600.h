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

#ifndef MCP9600_H
#define MCP9600_H

#include <Arduino.h>
#include <Wire.h>

class MCP9600 {
public:
	static constexpr uint8_t HOT_JUNCTION_REG_ADDR        = 0x0;
	static constexpr uint8_t JUNCTION_TEMP_DELTA_REG_ADDR = 0x1;
	static constexpr uint8_t COLD_JUNCTION_TEMP_REG_ADDR  = 0x2;
	static constexpr uint8_t RAW_ADC_DATA_REG_ADDR        = 0x3;
	static constexpr uint8_t STAT_REG_ADDR                = 0x4;
	static constexpr uint8_t THERM_SENS_CFG_REG_ADDR      = 0x5;
	static constexpr uint8_t DEVICE_CFG_REG_ADDR          = 0x6;

	static constexpr uint8_t ALERT1_CFG_REG_ADDR = 0x8;
	static constexpr uint8_t ALERT2_CFG_REG_ADDR = 0x9;
	static constexpr uint8_t ALERT3_CFG_REG_ADDR = 0xA;
	static constexpr uint8_t ALERT4_CFG_REG_ADDR = 0xB;
	static constexpr uint8_t ALERT1_HYS_REG_ADDR = 0xC;
	static constexpr uint8_t ALERT2_HYS_REG_ADDR = 0xD;
	static constexpr uint8_t ALERT3_HYS_REG_ADDR = 0xE;
	static constexpr uint8_t ALERT4_HYS_REG_ADDR = 0xF;

	static constexpr uint8_t TEMP_ALERT1_LIMIT_REG_ADDR = 0x10;
	static constexpr uint8_t TEMP_ALERT2_LIMIT_REG_ADDR = 0x11;
	static constexpr uint8_t TEMP_ALERT3_LIMIT_REG_ADDR = 0x12;
	static constexpr uint8_t TEMP_ALERT4_LIMIT_REG_ADDR = 0x13;

	static constexpr uint8_t VERSION_ID_REG_ADDR = 0x20;

	static constexpr uint8_t DEFAULT_IIC_ADDR = 0x60;

	static constexpr uint8_t RESOLUTION_0_5_DEGREE    = 0;
	static constexpr uint8_t RESOLUTION_0_25_DEGREE   = 0x01;
	static constexpr uint8_t RESOLUTION_0_125_DEGREE  = 0x02;
	static constexpr uint8_t RESOLUTION_0_0625_DEGREE = 0x03;

	static constexpr uint8_t THER_TYPE_K = 0x0 << 4;
	static constexpr uint8_t THER_TYPE_J = 0x1 << 4;
	static constexpr uint8_t THER_TYPE_T = 0x2 << 4;
	static constexpr uint8_t THER_TYPE_N = 0x3 << 4;
	static constexpr uint8_t THER_TYPE_S = 0x4 << 4;
	static constexpr uint8_t THER_TYPE_E = 0x5 << 4;
	static constexpr uint8_t THER_TYPE_B = 0x6 << 4;
	static constexpr uint8_t THER_TYPE_R = 0x7 << 4;

	static constexpr uint8_t ALERT_NUN_1 = 1;
	static constexpr uint8_t ALERT_NUN_2 = 2;
	static constexpr uint8_t ALERT_NUN_3 = 3;
	static constexpr uint8_t ALERT_NUN_4 = 4;

	static constexpr uint8_t FILT_OFF = 0;
	static constexpr uint8_t FILT_MIN = 1;
	static constexpr uint8_t FILT_MID = 4;
	static constexpr uint8_t FILT_MAX = 7;

	static constexpr bool ENABLE  = true;
	static constexpr bool DISABLE = false;

	static constexpr uint8_t COLD_JUNC_RESOLUTION_0_625 = 0 << 7;
	static constexpr uint8_t COLD_JUNC_RESOLUTION_0_25  = 1 << 7;

	static constexpr uint8_t ADC_18BIT_RESOLUTION = 0 << 5;
	static constexpr uint8_t ADC_16BIT_RESOLUTION = 1 << 5;
	static constexpr uint8_t ADC_14BIT_RESOLUTION = 2 << 5;
	static constexpr uint8_t ADC_12BIT_RESOLUTION = 3 << 5;

	static constexpr uint8_t BURST_1_SAMPLE   = 0 << 2;
	static constexpr uint8_t BURST_2_SAMPLE   = 1 << 2;
	static constexpr uint8_t BURST_4_SAMPLE   = 2 << 2;
	static constexpr uint8_t BURST_8_SAMPLE   = 3 << 2;
	static constexpr uint8_t BURST_16_SAMPLE  = 4 << 2;
	static constexpr uint8_t BURST_32_SAMPLE  = 5 << 2;
	static constexpr uint8_t BURST_64_SAMPLE  = 6 << 2;
	static constexpr uint8_t BURST_128_SAMPLE = 7 << 2;

	static constexpr uint8_t NORMAL_OPERATION = 0;
	static constexpr uint8_t SHUTDOWN_MODE    = 1;
	static constexpr uint8_t BURST_MODE       = 2;

	static constexpr uint8_t ACTIVE_LOW  = 0 << 2;
	static constexpr uint8_t ACTIVE_HIGH = 1 << 2;

	static constexpr uint8_t INT_MODE     = 1 << 1;
	static constexpr uint8_t COMPARE_MODE = 0 << 1;

	static constexpr uint8_t UPDATE_FLAG = 1 << 6;

	typedef enum {
		NO_ERROR     = 0,
		ERROR_PARAM  = -1,
		ERROR_COMM   = -2,
		ERROR_OTHERS = -128,
	} err_t;

	MCP9600(TwoWire *wire, uint8_t slave_address = 0x60);
	MCP9600::err_t init(uint8_t therm_type);
	MCP9600::err_t read_version(uint16_t *ver);

	MCP9600::err_t read_hot_junc(double *value);
	MCP9600::err_t read_junc_temp_delta(double *value);
	MCP9600::err_t read_cold_junc(double *value);
	MCP9600::err_t read_ADC_data(uint8_t *data, uint32_t data_len);
	MCP9600::err_t read_status(uint8_t *byte);

	MCP9600::err_t set_therm_cfg(uint8_t set_byte);
	MCP9600::err_t read_therm_cfg(uint8_t *byte);
	MCP9600::err_t set_therm_type(uint8_t set_byte);
	MCP9600::err_t set_filt_coefficients(uint8_t set_byte);

	MCP9600::err_t set_dev_cfg(uint8_t set_byte);
	MCP9600::err_t read_dev_cfg(uint8_t *byte);
	MCP9600::err_t set_sensor_mode(uint8_t set_byte);
	MCP9600::err_t set_burst_mode_samp(uint8_t set_byte);
	MCP9600::err_t set_ADC_meas_resolution(uint8_t set_byte);
	MCP9600::err_t set_cold_junc_resolution(uint8_t set_byte);

	MCP9600::err_t set_alert_limit(uint8_t alert_num, uint16_t value);
	MCP9600::err_t set_alert_hys(uint8_t alert_num, uint16_t value);

	MCP9600::err_t set_alert_cfg(uint8_t alert_num, uint8_t set_byte);
	MCP9600::err_t read_alert_cfg(uint8_t alert_num, uint8_t *byte);

	MCP9600::err_t clear_int_flag(uint8_t alert_num);
	MCP9600::err_t set_alert_for_TH_or_TC(uint8_t alert_num, uint8_t set_byte);
	MCP9600::err_t set_alert_limit_direction(uint8_t alert_num, uint8_t set_byte);
	MCP9600::err_t set_alert_bit(uint8_t alert_num, uint8_t set_byte);
	MCP9600::err_t set_alert_mode_bit(uint8_t alert_num, uint8_t set_byte);
	MCP9600::err_t set_alert_enable(uint8_t alert_num, uint8_t set_byte);

	uint16_t covert_temp_to_reg_form(double temp);

	MCP9600::err_t check_data_update(bool *stat);
	MCP9600::err_t read_INT_stat(uint8_t *stat);

private:
	TwoWire *_wire;
	uint8_t _address;

	MCP9600::err_t I2C_write_byte(uint8_t reg, uint8_t byte);
	MCP9600::err_t I2C_write_16bit(uint8_t reg, uint16_t value);
	MCP9600::err_t I2C_read_byte(uint8_t reg, uint8_t *byte);
	MCP9600::err_t I2C_read_16bit(uint8_t start_reg, uint16_t *value);
	MCP9600::err_t I2C_read_bytes(uint8_t start_reg, uint8_t *data, uint32_t data_len);
};

#endif /* MCP9600_H */