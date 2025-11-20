#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "firmware_version.h"

#include "hal/hal.h"

#include "./config/setting_parameters.h"
#include "./display/display.h"
#include "./i2c/i2c_manager.h"
#include "./modbus/modbus_manager.h"
#include "./transmit/iot_tx.h"
#include "./transmit/iot_tx_ambient.h"
#include "./transmit/iot_tx_ftp.h"
#include "./transmit/iot_tx_http.h"
#include "./transmit/iot_tx_mqtt.h"
#include "./transmit/tx_timing_helper.h"
#include "sensor_type.h"
#include "zigbee_api.h"

struct Features {
	bool wake_bat_enable;
	bool wait_enable;
	bool tx_random_wait_enable;
	bool camera_enable;
	bool ambient_enable;
	bool ftp_enable;
	bool http_enable;
	bool mqtt_enable;
	bool batch_send_enable;
	bool offline_mode;
};

struct TxEvent_t {
	bool transmit;           // 送信フラグ
	bool on_change_transmit; // 割り込み送信フラグ
	bool link_i2c;
	bool link_modbus;
};

extern std::vector<struct Sensor> sensors;
extern ParamManager param_manager;
extern std::unique_ptr<I2C_Manager> i2c_device;
extern std::unique_ptr<Modbus_Manager> modbus_device;
extern std::unique_ptr<SettingBT> sbt;
extern std::unique_ptr<IoTTxAmbient> tx_ambient;
extern std::unique_ptr<IoTTxFTP> tx_ftp;
extern std::unique_ptr<IoTTxHTTP> tx_http;
extern std::unique_ptr<IoTTxMQTT> tx_mqtt;

void InitializeLog(const char *save_dir);
void InitializeSettingParameter();
void InitializeLed();
void InitializeGPIO();
void InitializePulseCounter();
void InitializeI2c();
void InitializeI2cDevice();
void InitializeCameraDevice();
void InitializeModbus();
void InitializeModbusDevice();
void InitializeTimerInterrupt();
void InitializeWiredLan();
void InitializeWirelessLan();
void InitializeNTP();
void InitializeAmbient();
void InitializeFTP();
void InitializeHTTP();
void InitializeMQTT();

struct Features ReadFeatures();
void WaitStartTiming();
void UpdateSettings();

void updateValue(uint16_t type, int target_index = -1);
void printSensorData(bool force_update = false, uint16_t target = TYPE_NONE);

#endif /* INITIALIZE_H */