#include "./user_display_design.h"
#ifdef USER_DISPLAY_DESIGN_H

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  User Defines
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Callback Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void cb_BeforeSystemDisplayUpdate(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
	cb_BeforeSystemDisplayUpdate_Default(render, dt);
}

void cb_AfterSystemDisplayUpdate(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
	cb_AfterSystemDisplayUpdate_Default(render, dt);
}

void cb_SystemReady(std::unique_ptr<OpenFontRender> &render, const char *dev_id, int sensor_num) {
	/* If you want to change the font, please reset here */
	// render.reset(new OpenFontRender);

	cb_SystemReady_Default(render, dev_id, sensor_num);
}

void cb_WaitTiming(std::unique_ptr<OpenFontRender> &render, int current_sec) {
	cb_WaitTiming_Default(render, current_sec);
}

void cb_UpdateGpioValue(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	cb_UpdateGpioValue_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	cb_UpdateI2cValueInt_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateI2cValueDouble(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
	cb_UpdateI2cValueDouble_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateI2cValueString(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
	cb_UpdateI2cValueString_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateI2cValueError(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	cb_UpdateI2cValueError_Default(render, sensor_name, selected);
}

void cb_UpdateModbusValueInt(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	cb_UpdateModbusValueInt_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateModbusValueDouble(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
	cb_UpdateModbusValueDouble_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateModbusValueString(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
	cb_UpdateModbusValueString_Default(render, sensor_name, value, unit, selected);
}

void cb_UpdateModbusValueError(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	cb_UpdateModbusValueError_Default(render, sensor_name, selected);
}

void cb_UpdateCameraValue(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	cb_UpdateCameraValue_Default(render, sensor_name, selected);
}

void cb_SamplingGpioSensors(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	cb_SamplingGpioSensors_Default(render, timeinfo);
}

void cb_SamplingI2cSensors(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	cb_SamplingI2cSensors_Default(render, timeinfo);
}

void cb_SamplingModbusSensors(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	cb_SamplingModbusSensors_Default(render, timeinfo);
}

void cb_SamplingCameraSensor(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	cb_SamplingCameraSensor_Default(render, timeinfo);
}

void cb_FtpBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	cb_FtpBufferUpdate_Default(render, remain_data_quantity);
}

void cb_HttpBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	cb_HttpBufferUpdate_Default(render, remain_data_quantity);
}

void cb_MqttBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	cb_MqttBufferUpdate_Default(render, remain_data_quantity);
}

void cb_AmbientBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	cb_AmbientBufferUpdate_Default(render, remain_data_quantity);
}

void cb_ReadMqttStatus(std::unique_ptr<OpenFontRender> &render, int status) {
	cb_ReadMqttStatus_Default(render, status);
}

#endif /* USER_DISPLAY_DESIGN_H */