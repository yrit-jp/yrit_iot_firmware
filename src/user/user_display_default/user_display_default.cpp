#include "user_display_default.h"
#ifdef USER_DISPLAY_DEFAULT_H

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Callback Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

__attribute__((weak)) void cb_BeforeSystemDisplayUpdate_Default(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
}

__attribute__((weak)) void cb_AfterSystemDisplayUpdate_Default(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
}

__attribute__((weak)) void cb_SystemReady_Default(std::unique_ptr<OpenFontRender> &render, const char *dev_id, int sensor_num) {
}

__attribute__((weak)) void cb_WaitTiming_Default(std::unique_ptr<OpenFontRender> &render, int current_sec) {
}

__attribute__((weak)) void cb_UpdateGpioValue_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateI2cValueInt_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateI2cValueDouble_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateI2cValueString_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateI2cValueError_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
}

__attribute__((weak)) void cb_UpdateModbusValueInt_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateModbusValueDouble_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateModbusValueString_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
}

__attribute__((weak)) void cb_UpdateModbusValueError_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
}

__attribute__((weak)) void cb_UpdateCameraValue_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
}

__attribute__((weak)) void cb_SamplingGpioSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

__attribute__((weak)) void cb_SamplingI2cSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

__attribute__((weak)) void cb_SamplingModbusSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

__attribute__((weak)) void cb_SamplingCameraSensor_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

__attribute__((weak)) void cb_FtpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
}

__attribute__((weak)) void cb_HttpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
}

__attribute__((weak)) void cb_MqttBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
}

__attribute__((weak)) void cb_AmbientBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
}

__attribute__((weak)) void cb_LoraBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
}

__attribute__((weak)) void cb_ReadMqttStatus_Default(std::unique_ptr<OpenFontRender> &render, int status) {
}

#endif
