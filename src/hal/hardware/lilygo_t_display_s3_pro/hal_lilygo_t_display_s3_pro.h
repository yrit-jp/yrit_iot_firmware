#if defined(ARDUINO_ESP32S3_DEV) && defined(ARDUINO_PARTITION_app3M_fat9M_16MB)

#ifndef HAL_API_LILYGO_T_DISPLAY_PRO_H
#define HAL_API_LILYGO_T_DISPLAY_PRO_H

#include "../hal_interface.h"
#include "utility/pin_define.h"
#include "utility/tft_color_define.h"
#include <ESPmDNS.h>
#include <FFat.h>
#include <FS.h>
#include <NimBLEDevice.h>
#include <TFT_eSPI.h>
#include <TouchButtonGeneric_Class.hpp>
#include <TouchDrvCSTXXX.hpp>
#include <WiFi.h>
#include <WiFiType.h>
#include <XPowersLib.h>
#include <esp_task_wdt.h>

class HAL_Display_API : public TFT_eSPI {
public:
	bool initializeSemaphoreMutex();
	bool takeSemaphoreMutex(uint32_t wait_tick_time);
	bool giveSemaphoreMutex();
};

class HAL_Timer_API : public ITimerControl {
public:
	using ITimerControl::ITimerControl;
	void begin(uint32_t interval_ms);
	void addSubTimer(std::string key,
	                 uint32_t count_limit,
	                 uint32_t initial_count = 0);
	uint32_t getSubTimerLimit(std::string key);
	uint32_t getSubTimerLimitMillis(std::string key);
	void removeSubTimer(std::string key);
	bool checkSubTimer(std::string key, bool reset = true);
	void start(bool enable_first_interrupt = false);
	bool interrupted();

private:
	uint32_t _interval_ms;
};

class HAL_WLAN_API : public IWirelessLanControl<WiFiClient> {
public:
	int getStatus();
	int getRSSI();
	std::vector<uint8_t> fetchMacAddress() override;
	MyIPAddress localIP();
	bool config(String ip, String gateway, String subnet, String dns1, String dns2);
	bool connectWlan(unsigned long timeout_ms = 0);
	bool connectWlan(String ap_name, String pass, unsigned long timeout_ms = 0);
	void disconnectWlan(bool exit_sta_mode = true, bool erase_ap_settings = true);
	bool detectTargetAccessPoint(String ap_name, int32_t *rssi, bool *exist_same_name, bool print_result = true);
	MyIPAddress resolveMDNS(String host, uint8_t try_count = 2);
	std::map<int, String> scanAccessPoint(bool print_result = true);
};

class HAL_NTP_RTC_API : public INtpRtcControl {
public:
	bool begin();
	bool ready();
	bool update() override;
	bool getCurrentTime(struct tm *timeinfo);

private:
	String selected_ntp_server = "";
	bool _inited               = false;
};

class HAL_SETTING_FILE_IO_API : public ISettingFileInputOutputControl {
public:
	bool saveSettings(String fpath, JsonDocument json);
	bool loadSettings(String fpath, JsonDocument &json);
	bool formatStorage();

	JsonDocument deviceUniqueSettings() override;
};

class HAL_BLE_API : public IBluetoothLowEnergyControl {
public:
	void begin(String local_name) override;
};

class HAL_I2C_API : public IWireI2cControl<TwoWire> {
public:
	bool begin(uint8_t port) override;
	uint8_t getSdaPin(uint8_t port) override;
	uint8_t getSclPin(uint8_t port) override;
};

class HAL_MODBUS_API : public IModbusControl {
public:
	HAL_MODBUS_API() : portSerial0(1) {};
	std::vector<uint8_t> getPorts() override;
	bool begin(uint8_t port, uint32_t baudrate, uint8_t data_bit, uint8_t parity, uint32_t timeout_ms) override;
	uint8_t getTxPin(uint8_t port) override;
	uint8_t getRxPin(uint8_t port) override;

protected:
	HardwareSerial portSerial0;
};

class HAL_API : public IHardwareAbstractionLayer<SemaphoreHandle_t> {
public:
	HAL_API() : Timer0(0), Timer1(1), Timer2(3) {};

	void begin();
	void update();
	uint8_t waitForButtonPressed(uint16_t wait_ms);
	uint8_t BtnA_wasPressed();
	uint8_t BtnB_wasPressed();
	uint8_t BtnC_wasPressed();
	uint8_t BtnA_isPressed();
	uint8_t BtnB_isPressed();
	uint8_t BtnC_isPressed();
	String getChipUUID(bool include_prefix = true);

	void delay(uint32_t ms);
	void reboot();

	void endSetup() override;
	int8_t getBatteryLevel() override;
	uint32_t getRandomNumber(uint32_t minimum = 0, uint32_t maximum = UINT32_MAX) override;
	void periodicLoopTaskDebugMessage() override;

	bool initializeSemaphoreMutex(String key) override;
	bool takeSemaphoreMutex(String key, uint32_t wait_tick_time) override;
	bool giveSemaphoreMutex(String key) override;

	HAL_Display_API Display;

	HAL_Timer_API Timer0;
	HAL_Timer_API Timer1;
	HAL_Timer_API Timer2;

	HAL_WLAN_API Wlan;
	IWiredLanControl<DummyClient> Lan;
	HAL_NTP_RTC_API NtpRtc;

	ILedControl Led_Status;
	ILedControl Led_Warning;

	HAL_SETTING_FILE_IO_API FileIO;
	HAL_BLE_API Ble;

	ITfCardControl Tf;
	IDebugLogControl Log;

	IGpioControl Gpio;
	HAL_I2C_API I2c;
	HAL_MODBUS_API Modbus;
	ICameraControl Camera;

	IMetadataControl Info;
};

extern HAL_API HAL;

#endif /* HAL_API_LILYGO_T_DISPLAY_PRO_H */
#endif /* ARDUINO_ESP32_DEV */