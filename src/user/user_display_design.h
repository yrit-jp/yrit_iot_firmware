#include "../hal/hal.h"

#ifndef USER_DISPLAY_DESIGN_H
#define USER_DISPLAY_DESIGN_H

#include "./user_display_default/user_display_default.h"

void cb_BeforeSystemDisplayUpdate(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt);
void cb_AfterSystemDisplayUpdate(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt);

void cb_SystemReady(std::unique_ptr<OpenFontRender> &render, const char *dev_id, int sensor_num);
void cb_WaitTiming(std::unique_ptr<OpenFontRender> &render, int current_sec);

// Update sensor value
// gpio
void cb_UpdateGpioValue(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected);
// i2c
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected);
void cb_UpdateI2cValueDouble(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected);
void cb_UpdateI2cValueString(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected);
void cb_UpdateI2cValueError(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected);
// modbus
void cb_UpdateModbusValueInt(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected);
void cb_UpdateModbusValueDouble(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected);
void cb_UpdateModbusValueString(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected);
void cb_UpdateModbusValueError(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected);
// camera
void cb_UpdateCameraValue(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected);

// Complete sampling sensor
void cb_SamplingGpioSensors(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo);
void cb_SamplingI2cSensors(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo);
void cb_SamplingModbusSensors(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo);
void cb_SamplingCameraSensor(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo);

void cb_AmbientBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity);
void cb_FtpBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity);
void cb_HttpBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity);
void cb_MqttBufferUpdate(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity);

void cb_ReadMqttStatus(std::unique_ptr<OpenFontRender> &render, int status);

#endif /* USER_DISPLAY_DESIGN_H */
