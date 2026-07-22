#ifdef ARDUINO_WIO_TRACKER_1110

#ifndef HAL_API_WIO_TRACKER_1110_H
#define HAL_API_WIO_TRACKER_1110_H

#include <Arduino.h>

#ifndef SDA
#define SDA (PIN_WIRE_SDA)
#endif

#ifndef SCL
#define SCL (PIN_WIRE_SCL)
#endif

#include "../hal_interface.h"
#include "utility/button.h"
#include <DFRobot_RTU.h>
#include <Wire.h>

#include <LbmWm1110.hpp>        // LoRaWAN Stack
#include <Lbm_Modem_Common.hpp> // LoRaWAN Common define
#include <Lbmx.hpp>             // LoRaWAN Stack Rapper

// #include <Adafruit_LittleFS.h>
#include "ExternalFileSystem.h" // for external flash storage (custom library)

#include <Adafruit_TinyUSB.h> // for Serial
#include <bluefruit.h>        // for Bluetooth
#include <malloc.h>
#include <nrf.h> // for timer

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

	bool _interrupted = false;

private:
	bool _inited = false;
	uint32_t _interval_ms;
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
	void doWork() override;

private:
	LbmWm1110 *_lbmWm1110 = nullptr;
	bool _initialized     = false;
	String _last_error    = "";
};

class HAL_LED_API : public ILedControl {
public:
	void on() override;
	void off() override;
	void toggle() override;
};

class HAL_SETTING_FILE_IO_API : public ISettingFileInputOutputControl {
public:
	bool saveSettings(String fpath, JsonDocument json) override;
	bool loadSettings(String fpath, JsonDocument &json) override;
	bool formatStorage() override;

	JsonDocument deviceUniqueSettings() override;

	bool usePresetWiFi() override {
		return false;
	}

	bool usePresetAmbient() override {
		return false;
	}

	bool usePresetFtp() override {
		return false;
	}

	bool usePresetHttp() override {
		return false;
	}

	bool usePresetMqtt() override {
		return false;
	}
};

class HAL_BLE_API : public IBluetoothLowEnergyControl {
public:
	void begin(String local_name) override;
};

class HAL_GPIO_API : public IGpioControl {
public:
	std::vector<uint8_t> getPins() override;
	String getModifier() override;
	uint8_t getAdcBits() override;
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
	HAL_API() : Timer0(0), Timer1(1), Timer2(2) {};

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
	void reboot() override;
	void system_sleep(uint32_t ms) override;
	void endSetup() override;

	// int8_t getBatteryLevel() override;
	uint32_t getFreeHeapSize() override;
	uint32_t getMinimumFreeHeapSize() override;

	bool initializeSemaphoreMutex(String key) override;
	bool takeSemaphoreMutex(String key, uint32_t wait_tick_time) override;
	bool giveSemaphoreMutex(String key) override;

	void periodicLoopTaskDebugMessage() override;

	void printf(const char *fmt, ...) override;
	void println(const char *s) override;
	void print(const char *s) override;

	HAL_Display_API Display;

	HAL_Timer_API Timer0;
	HAL_Timer_API Timer1;
	HAL_Timer_API Timer2;

	HAL_LoRaWAN_API LoRaWan;
	IWirelessLanControl<DummyClient> Wlan;
	IWiredLanControl<DummyClient> Lan;
	INtpRtcControl NtpRtc;

	HAL_LED_API Led_Status;
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

extern TaskHandle_t TransmitTaskHandle;
extern TaskHandle_t UserTaskHandle;
extern HAL_API HAL;

#endif /* HAL_API_WIO_TRACKER_1110_H */
#endif /* WIO_TRACKER_1110 */