#if defined(ARDUINO_ARDUINO_NESSO_N1)

#ifndef HAL_API_ARDUINO_NESSO_N1_H
#define HAL_API_ARDUINO_NESSO_N1_H

#include "../hal_interface.h"

#include <DFRobot_RTU.h>
#include <ESPmDNS.h>
#include <M5Unified.h>
#include <NimBLEDevice.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiType.h>
#include <Wire.h>
#include <esp_mac.h>
#include <esp_task_wdt.h>
#include <mbedtls/sha256.h>
#include <soc/wdev_reg.h>

#include <driver/gpio.h>
#include <driver/pcnt.h>
#include <soc/pcnt_struct.h>

// LoRaWAN
#include <Preferences.h> // for save lorawan nonce
#include <RadioLib.h>
#include <nvs_flash.h> // for save lorawan nonce

class HAL_Display_API : public IDisplayControl {
public:
	void drawPixel(int32_t x, int32_t y, uint32_t color) override;
	void drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size) override;
	void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) override;
	void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) override;
	void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) override;
	void fillScreen(uint32_t color) override;

	int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font) override;
	int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y) override;
	int16_t height(void) override;
	int16_t width(void) override;
	int16_t drawString(const char *string, int32_t poX, int32_t poY, uint8_t font) override;
	int16_t drawString(const char *string, int32_t poX, int32_t poY) override;
	int16_t color565(uint8_t red, uint8_t green, uint8_t blue) override;

	void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;
	void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;
	void drawRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) override;
	void fillRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) override;

	void setRotation(uint8_t r) override;
	void startWrite() override;
	void endWrite() override;

	void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) override;
	void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) override;
	void drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) override;
	void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) override;
	void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) override;
	void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) override;

	void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) override;

	void setCursor(int32_t x, int32_t y) override;
	int32_t getCursorX() override;
	int32_t getCursorY() override;
	void setTextColor(uint16_t color) override;
	void setTextColor(uint16_t fgcolor, uint16_t bgcolor) override;
	void print(const char *string) override;
	void println(const char *string) override;
	void printf(const char *fmt, ...) override;

	bool initializeSemaphoreMutex() override;
	bool takeSemaphoreMutex(uint32_t wait_tick_time) override;
	bool giveSemaphoreMutex() override;
};

class HAL_Timer_API : public ITimerControl {
public:
	using ITimerControl::ITimerControl;
	void begin(uint32_t interval_ms) override;
	void addSubTimer(std::string key,
	                 uint32_t count_limit,
	                 uint32_t initial_count = 0) override;
	uint32_t getSubTimerLimit(std::string key) override;
	uint32_t getSubTimerLimitMillis(std::string key) override;
	void removeSubTimer(std::string key) override;
	bool checkSubTimer(std::string key, bool reset = true) override;
	void start(bool enable_first_interrupt = false) override;
	bool interrupted() override;

private:
	uint32_t _interval_ms;
};

class HAL_WLAN_API : public IWirelessLanControl<WiFiClient> {
public:
	int getStatus() override;
	int getRSSI() override;
	std::vector<uint8_t> fetchMacAddress() override;
	MyIPAddress localIP() override;
	bool config(String ip, String gateway, String subnet, String dns1, String dns2) override;
	bool connectWlan(unsigned long timeout_ms = 0) override;
	bool connectWlan(String ap_name, String pass, unsigned long timeout_ms = 0) override;
	void disconnectWlan(bool exit_sta_mode = true, bool erase_ap_settings = true) override;
	bool detectTargetAccessPoint(String ap_name, int32_t *rssi, bool *exist_same_name, bool print_result = true) override;
	MyIPAddress resolveMDNS(String host, uint8_t try_count = 2) override;
	std::map<int, String> scanAccessPoint(bool print_result = true) override;
};

class HAL_NTP_RTC_API : public INtpRtcControl {
public:
	bool begin() override;
	bool ready() override;
	bool update() override;
	bool getCurrentTime(struct tm *timeinfo) override;

private:
	String selected_ntp_server = "";
	bool _inited               = false;
};

class HAL_LoRaWAN_API : public ILoRaWanControl {
public:
	bool beginOTAA(String dev_eui, String join_eui, String app_key) override;
	bool beginABP(String dev_addr, String nwk_s_key, String app_s_key) override;
	bool join(unsigned long timeout_ms = 60000) override;
	bool isJoined() override;
	bool send(const uint8_t *data, size_t size, uint8_t fport) override;
	bool waitTxDone(uint32_t timeout_ms = 30000) override;
	bool isSupported() override;
	String getDevEui() override;
	String getJoinEui() override;
	String getLastError() override;

	void saveSession();
	void sleep();
	void wakeup();

private:
	static bool parseHexString(const String &hex_str, uint64_t &value) {
		uint8_t buf[8];
		if (not ILoRaWanControl::parseHexString(hex_str, buf, 8)) {
			return false;
		}
		value = 0;
		for (int i = 0; i < 8; i++) {
			value = (value << 8) | buf[i];
		}
		return true;
	}

	bool _initRadio();

	bool _initialized  = false;
	bool _is_abp       = false;
	SX1262 *_radio     = nullptr;
	LoRaWANNode *_node = nullptr;
	String _last_error = "";

	Preferences store;

	// OTAA 用
	uint64_t _lorawan_dev_eui    = 0;
	uint64_t _lorawan_join_eui   = 0;
	uint8_t _lorawan_app_key[16] = {0};

	// ABP 用
	uint32_t _lorawan_dev_addr     = 0;
	uint8_t _lorawan_nwk_s_key[16] = {0};
	uint8_t _lorawan_app_s_key[16] = {0};
};

class HAL_SETTING_FILE_IO_API : public ISettingFileInputOutputControl {
public:
	bool saveSettings(String fpath, JsonDocument json) override;
	bool loadSettings(String fpath, JsonDocument &json) override;
	bool formatStorage() override;
	JsonDocument deviceUniqueSettings() override;
};

class HAL_BLE_API : public IBluetoothLowEnergyControl {
public:
	void begin(String local_name) override;
};

class HAL_GPIO_API : public IGpioControl {
public:
	std::vector<uint8_t> getPins() override;
	String getModifier() override;
	void setUnused(uint8_t pin) override;
	bool initializePulseCounter(uint8_t pin, PULSE_EDGE edge) override;
	bool stopPulseCounter(uint8_t pin) override;
	bool startPulseCounter(uint8_t pin) override;
	bool stopPulseCounterAll() override;
	bool startPulseCounterAll() override;
	uint32_t getMaxPulseCount() override;
	bool updatePulseCount(uint8_t pin) override;
	void resetPulseCount(uint8_t pin) override;
	void resetPulseCountAll() override;

private:
	uint8_t AVAILABLE_UNIT_NUMBER = 0;
	std::map<uint8_t, pcnt_unit_t> _pin_unit_map;
};

class HAL_I2C_API : public IWireI2cControl<TwoWire> {
public:
	bool begin(uint8_t port) override;
	uint8_t getSdaPin(uint8_t port) override;
	uint8_t getSclPin(uint8_t port) override;
};

class HAL_MODBUS_API : public IModbusControl {
public:
	std::vector<uint8_t> getPorts() override;
	bool begin(uint8_t port, uint32_t baudrate, uint8_t data_bit, uint8_t parity, uint32_t timeout_ms) override;
	uint8_t getTxPin(uint8_t port) override;
	uint8_t getRxPin(uint8_t port) override;
};

class HAL_API : public IHardwareAbstractionLayer<SemaphoreHandle_t> {
public:
	HAL_API() : Timer0(0), Timer1(1), Timer2(3), Log() {};

	void begin() override;
	void update() override;
	uint8_t waitForButtonPressed(uint16_t wait_ms) override;
	uint8_t BtnA_wasPressed() override;
	uint8_t BtnB_wasPressed() override;
	uint8_t BtnC_wasPressed() override;
	uint8_t BtnA_isPressed() override;
	uint8_t BtnB_isPressed() override;
	uint8_t BtnC_isPressed() override;
	String getChipUUID(bool include_prefix = true) override;

	void delay(uint32_t ms) override;
	void reboot() override;
	void system_sleep(uint32_t ms) override;
	bool isWakeFromSleep() override;
	void setPowerMode(bool wifi_required) override;

	void endSetup() override;
	int8_t getBatteryLevel() override;
	uint32_t getFreeHeapSize() override;
	uint32_t getMinimumFreeHeapSize() override;
	uint32_t getRandomNumber(uint32_t minimum = 0, uint32_t maximum = UINT32_MAX) override;
	void periodicLoopTaskDebugMessage() override;

	void printf(const char *fmt, ...) override;
	void println(const char *s) override;
	void print(const char *s) override;

	bool initializeSemaphoreMutex(String key) override;
	bool takeSemaphoreMutex(String key, uint32_t wait_tick_time) override;
	bool giveSemaphoreMutex(String key) override;

	HAL_Display_API Display;

	HAL_Timer_API Timer0;
	HAL_Timer_API Timer1;
	HAL_Timer_API Timer2;

	HAL_LoRaWAN_API LoRaWan;
	HAL_WLAN_API Wlan;
	IWiredLanControl<DummyClient> Lan;
	HAL_NTP_RTC_API NtpRtc;

	ILedControl Led_Status;
	ILedControl Led_Warning;

	HAL_SETTING_FILE_IO_API FileIO;
	HAL_BLE_API Ble;
	ITfCardControl Tf;
	IDebugLogControl Log;

	HAL_GPIO_API Gpio;
	HAL_I2C_API I2c;
	HAL_MODBUS_API Modbus;
	ICameraControl Camera;

	IMetadataControl Info;
};

extern HAL_API HAL;

#endif /* HAL_API_ARDUINO_NESSO_N1_H */
#endif /* ARDUINO_ARDUINO_NESSO_N1 */