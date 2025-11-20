/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Display Override
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

#include "../../hal.h"

#ifdef TARGET__ESP32_DEVKIT

#include "../../../display/display.h"
#include "../../../display/display_timing_type_def.h"
#include "../../../user/user_display_design.h"
#include <FaBoLCDmini_AQM0802A.h>

class ScrollableString {
public:
	ScrollableString(String str, uint8_t display_width);
	void setLcdWidth(uint8_t display_width);
	void setString(String str);
	void setOffset(uint8_t offset);
	void next();
	bool isChangedPosition();
	String getScrollableString();

private:
	String _str;
	uint8_t _display_width;
	String _scrollable_str;
	uint8_t _count;
	bool _called_next;
};

enum _DisplayPage {
	MAIN_PAGE_1,
	MAIN_PAGE_2,
	_MAIN_PAGE_SENTINEL,
};
uint8_t g_display_page = MAIN_PAGE_1;

enum class _SensorType {
	NONE,
	GPIO,
	I2C_INT,
	I2C_DOUBLE,
	I2C_STR,
	I2C_ERR,
	MODBUS_INT,
	MODBUS_DOUBLE,
	MODBUS_STR,
	MODBUS_ERR,
	CAMERA,
};

struct _SensorValue {
	_SensorType type;
	String name;
	String unit;
	int32_t vint;
	double vdouble;
	String vstring;
};
struct _SensorValue g_display_memory = {_SensorType::NONE};

FaBoLCDmini_AQM0802A lcd;
ScrollableString sens_name("", 8);

uint8_t _sensor_num = 0;
String _dev_id      = "";

enum DisplayTiming updateDisplayOverrideTop(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt, va_list ap) {
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	switch (dt) {
	case DisplayTiming::SYSTEM_INIT_BEGIN:
		lcd.begin();
		lcd.clear();
		break;
	case DisplayTiming::SS_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("SS begin");
		break;
	case DisplayTiming::SS_MODE_ENTRY:
		lcd.setCursor(0, 0);
		lcd.print("SS Mode ");
		break;
	case DisplayTiming::SS_CLEAR_ALL:
		lcd.setCursor(0, 1);
		lcd.print("ClearAll");
		break;
	case DisplayTiming::SS_FORMAT:
		lcd.setCursor(0, 1);
		lcd.print("Format..");
		break;
	case DisplayTiming::SS_READY: {
		String chipid = HAL.getChipUUID();
		String top    = chipid.substring(0, 8);
		String bottom = chipid.substring(8, 16);
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.printf("%s", top.c_str());
		lcd.setCursor(0, 1);
		lcd.printf("%s", bottom.c_str());
		break;
	}
	case DisplayTiming::SS_LOOP:
		break;
	case DisplayTiming::SS_INIT_DONE:
		break;
	case DisplayTiming::DISPLAY_FW_VERSION: {
		const char *dev_id      = va_arg(ap, const char *);
		int main_version        = va_arg(ap, int);
		int miner_version       = va_arg(ap, int);
		const char *release_tag = va_arg(ap, const char *);
		int hold_sec            = va_arg(ap, int);

		lcd.setCursor(0, 0);
		lcd.printf("ID:%s", dev_id);
		lcd.setCursor(0, 1);
		lcd.printf("FW%d.%d   ", main_version, miner_version); // 幅の関係上、リリースタグの表示は省略
		for (uint8_t c = hold_sec; c != 0; c--) {
			lcd.setCursor(7, 1);
			lcd.printf("%d", c);
			HAL.Display.giveSemaphoreMutex();
			HAL.delay(1000);
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		}
		break;
	}
	case DisplayTiming::GPIO_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Init PIN");
		break;
	case DisplayTiming::GPIO_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::PULSE_COUNTER_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Init P_C");
		break;
	case DisplayTiming::PULSE_COUNTER_INIT_TARGET: {
		uint8_t pin = va_arg(ap, int);
		lcd.setCursor(0, 1);
		lcd.printf("P_C IO%2d", pin);
		break;
	}
	case DisplayTiming::PULSE_COUNTER_INIT_END:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::I2C_INIT_DEVICE_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Init I2C");
		break;
	case DisplayTiming::I2C_INIT_DEVICE_TARGET: {
		uint8_t address = va_arg(ap, int);
		int progress    = va_arg(ap, int);
		int i2c_dev_num = va_arg(ap, int);
		lcd.setCursor(0, 1);
		lcd.printf("0x%02X %d/%d", address, progress, i2c_dev_num);
		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_FAILED: {
		uint8_t address = va_arg(ap, int);

		lcd.setCursor(0, 1);
		lcd.printf("Err %02X", address);
		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::DISPLAY_OFFLINE_MODE:
		lcd.setCursor(0, 0);
		lcd.print("OFFLINE ");
		lcd.setCursor(0, 1);
		lcd.print("Mode    ");
		break;
	case DisplayTiming::WIFI_TRY_CONNECT: {
		const char *target_ssid = va_arg(ap, const char *);

		lcd.setCursor(0, 0);
		lcd.print("WiFi    ");
		lcd.setCursor(0, 1);
		lcd.printf("%.8s", target_ssid);
		break;
	}
	case DisplayTiming::WIFI_SEARCH_TARGET:
		lcd.setCursor(0, 0);
		lcd.print("SSID    ");
		lcd.setCursor(0, 1);
		lcd.print("Check...");
		break;
	case DisplayTiming::WIFI_CONNECT_SUCCESS:
		lcd.setCursor(0, 0);
		lcd.print("WiFi OK!");
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::WIFI_CONNECT_FAILED:
		lcd.setCursor(0, 0);
		lcd.print("WiFi Err");
		lcd.setCursor(0, 1);
		lcd.print("Res 10s ");
		break;
	case DisplayTiming::WIFI_TARGET_DETECTED: {
		const char *target_ssid = va_arg(ap, const char *);

		lcd.setCursor(0, 0);
		lcd.print("SSID    ");
		lcd.setCursor(0, 1);
		lcd.print("Found!  ");
		break;
	}
	case DisplayTiming::WIFI_TARGET_NOT_FOUND: {
		const char *target_ssid = va_arg(ap, const char *);

		lcd.setCursor(0, 1);
		lcd.print("SSID NG!");
		break;
	} break;
	case DisplayTiming::NTP_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Sync Tim");
		break;
	case DisplayTiming::NTP_CONNECT_WIFI:
		lcd.setCursor(0, 1);
		lcd.print("NTP WiFi");
		break;
	case DisplayTiming::NTP_SYNC_START:
		lcd.setCursor(0, 1);
		lcd.print("Init NTP");
		break;
	case DisplayTiming::NTP_SYNC_FAILED:
		lcd.setCursor(0, 0);
		lcd.print("Err NTP ");
		lcd.setCursor(0, 1);
		lcd.print("Res 10s ");
		break;
	case DisplayTiming::NTP_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::HEARTBEAT_FAILED:
		lcd.setCursor(0, 0);
		lcd.print("HB error");
		lcd.setCursor(0, 1);
		lcd.print("Res 5s ");
		break;
	case DisplayTiming::AMBIENT_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Init AMB");
		break;
	case DisplayTiming::AMBIENT_CONNECT_WIFI:
		lcd.setCursor(0, 1);
		lcd.print("AMB WiFi");
		break;
	case DisplayTiming::AMBIENT_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		if (remain_data_quantity != 0) {
			HAL.Led_Warning.on();
		} else {
			HAL.Led_Warning.off();
		}

		break;
	}
	case DisplayTiming::AMBIENT_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::FTP_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Init FTP");
		break;
	case DisplayTiming::FTP_CONNECT_WIFI:
		lcd.setCursor(0, 1);
		lcd.print("FTP WiFi");
		break;
	case DisplayTiming::FTP_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		if (remain_data_quantity != 0) {
			HAL.Led_Warning.on();
		} else {
			HAL.Led_Warning.off();
		}

		break;
	}
	case DisplayTiming::FTP_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::HTTP_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("InitHTTP");
		break;
	case DisplayTiming::HTTP_CONNECT_WIFI:
		lcd.setCursor(0, 1);
		lcd.print("HTTPWiFi");
		break;
	case DisplayTiming::HTTP_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		if (remain_data_quantity != 0) {
			HAL.Led_Warning.on();
		} else {
			HAL.Led_Warning.off();
		}

		break;
	}
	case DisplayTiming::HTTP_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::MQTT_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("InitMqtt");
		break;
	case DisplayTiming::MQTT_CONNECT_WIFI:
		lcd.setCursor(0, 1);
		lcd.print("MQTTWiFi");
		break;
	case DisplayTiming::MQTT_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		if (remain_data_quantity != 0) {
			HAL.Led_Warning.on();
		} else {
			HAL.Led_Warning.off();
		}

		break;
	}
	case DisplayTiming::MQTT_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::TI_INIT_BEGIN:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		lcd.setCursor(0, 1);
		lcd.print("Init Tim");
		break;
	case DisplayTiming::TI_INIT_DONE:
		lcd.setCursor(0, 1);
		lcd.print("        ");
		break;
	case DisplayTiming::SYSTEM_INIT_DONE: {
		const char *dev_id = va_arg(ap, const char *);
		int sensor_num     = va_arg(ap, int);

		_dev_id     = String(dev_id);
		_sensor_num = sensor_num;

		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.printf("ID:%s", dev_id);
		lcd.setCursor(0, 1);
		lcd.print("Ready!  ");
		break;
	}
	case DisplayTiming::WAIT_START_TIMING: {
		int sec = va_arg(ap, int);

		lcd.setCursor(0, 0);
		lcd.printf("WAIT 00s");
		lcd.setCursor(0, 1);
		lcd.printf("%02d sec  ", 60 - sec);

		break;
	}
	case DisplayTiming::SETUP_END: {
		lcd.clear();
		lcd.setCursor(0, 0);
		// lcd.printf("ID:%s", _dev_id.c_str());
		if (_sensor_num == 0) {
			// lcd.setCursor(0, 1);
			lcd.printf("NoSensor");
		}

		break;
	}
	case DisplayTiming::BTNA_PRESSED: {
		if (g_display_page == MAIN_PAGE_1) {
			lcd.clear();
		}
		break;
	}
	case DisplayTiming::BTNC_PRESSED: {
		g_display_page = ((g_display_page + 1 == _MAIN_PAGE_SENTINEL) ? MAIN_PAGE_1 : static_cast<_DisplayPage>(g_display_page + 1));
		lcd.clear();

		if (g_display_page == MAIN_PAGE_1) {
			lcd.setCursor(0, 0);
			lcd.printf("%.8s:", sens_name.getScrollableString().c_str());

			switch (g_display_memory.type) {
			case _SensorType::GPIO:
			// Fallthrough
			case _SensorType::I2C_INT:
				lcd.setCursor(0, 1);
				lcd.printf("%4d", g_display_memory.vint);
				break;
			case _SensorType::I2C_DOUBLE:
				lcd.setCursor(0, 1);
				lcd.printf("%4.1f", g_display_memory.vdouble);
				break;
			case _SensorType::I2C_STR:
				lcd.setCursor(0, 1);
				lcd.printf("%4s", g_display_memory.vstring.c_str());
				break;
			}
			lcd.setCursor(5, 1);
			lcd.printf("%s", g_display_memory.unit.c_str());

		} else if (g_display_page == MAIN_PAGE_2) {
			lcd.setCursor(0, 0);
			lcd.printf("devID:");
			lcd.setCursor(0, 1);
			lcd.printf("%-8s", _dev_id.c_str());
		}
		break;
	}
	case DisplayTiming::SET_DISPLAY_SENSOR_NAME: {
		const char *new_sensor_name = va_arg(ap, const char *);
		sens_name.setString(new_sensor_name);
		break;
	}
	case DisplayTiming::SET_DISPLAY_WIDTH: {
		int width = va_arg(ap, int);
		sens_name.setLcdWidth(width);
		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_GPIO: {
		const char *sensor_name = va_arg(ap, const char *);
		(void)sensor_name; // For warning unused variable
		int32_t value    = va_arg(ap, int32_t);
		const char *unit = va_arg(ap, const char *);
		bool selected    = va_arg(ap, int);

		if (g_display_page == MAIN_PAGE_1) {
			// 画面表示を更新
			if (selected == true) {
				lcd.setCursor(0, 1);
				lcd.printf("        ");
				lcd.setCursor(0, 1);
				lcd.printf("%4d", value);
				lcd.setCursor(5, 1);
				lcd.printf("%s", unit);

				g_display_memory.unit = String(unit);
				g_display_memory.vint = value;
				g_display_memory.type = _SensorType::GPIO;
			}
		}
		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_INT: {
		const char *sensor_name = va_arg(ap, const char *);
		(void)sensor_name; // For warning unused variable
		int32_t value    = va_arg(ap, int32_t);
		const char *unit = va_arg(ap, const char *);
		bool selected    = va_arg(ap, int);

		if (g_display_page == MAIN_PAGE_1) {
			// 画面表示を更新
			if (selected == true) {
				lcd.setCursor(0, 1);
				lcd.printf("        ");
				lcd.setCursor(0, 1);
				lcd.printf("%4d", value);
				lcd.setCursor(5, 1);
				lcd.printf("%s", unit);

				g_display_memory.unit = String(unit);
				g_display_memory.vint = value;
				g_display_memory.type = _SensorType::I2C_INT;
			}
		}
		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_DOUBLE: {
		const char *sensor_name = va_arg(ap, const char *);
		(void)sensor_name; // For warning unused variable
		double value     = va_arg(ap, double);
		const char *unit = va_arg(ap, const char *);
		bool selected    = va_arg(ap, int);

		if (g_display_page == MAIN_PAGE_1) {
			// 画面表示を更新
			if (selected == true) {
				lcd.setCursor(0, 1);
				lcd.printf("        ");
				lcd.setCursor(0, 1);
				lcd.printf("%4.1f", value);
				lcd.setCursor(5, 1);
				lcd.printf("%s", unit);

				g_display_memory.unit    = String(unit);
				g_display_memory.vdouble = value;
				g_display_memory.type    = _SensorType::I2C_DOUBLE;
			}
		}
		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_STRING: {
		const char *sensor_name = va_arg(ap, const char *);
		(void)sensor_name; // For warning unused variable
		const char *value = va_arg(ap, const char *);
		const char *unit  = va_arg(ap, const char *);
		bool selected     = va_arg(ap, int);

		if (g_display_page == MAIN_PAGE_1) {
			// 画面表示を更新
			if (selected == true) {
				lcd.setCursor(0, 1);
				lcd.printf("        ");
				lcd.setCursor(0, 1);
				lcd.printf("%.4s", value);
				lcd.setCursor(5, 1);
				lcd.printf("%s", unit);

				g_display_memory.unit    = String(unit);
				g_display_memory.vstring = String(value);
				g_display_memory.type    = _SensorType::I2C_STR;
			}
		}
		break;
	}
	case DisplayTiming::EVERY_ONE_SECONDS_EVENT: {
		if (g_display_page == MAIN_PAGE_1) {
			if (_sensor_num == 0) {
				break;
			}
			// Scroll strings
			sens_name.next();
			lcd.setCursor(0, 0);
			lcd.printf("%.8s", sens_name.getScrollableString().c_str());
		}
		break;
	}
	case DisplayTiming::EVERY_HALF_MINUTES_EVENT: {
		break;
	}
	case DisplayTiming::EVERY_ONE_MINUTES_EVENT: {
		break;
	}
	default:
		break;
	}

	HAL.Display.giveSemaphoreMutex();
	return DisplayTiming::IGNORE_DEFAULT_PROCESS;
}

/* ******************************************************
 * Implementation ScrollableString class
 * *****************************************************/
ScrollableString::ScrollableString(String str, uint8_t display_width) {
	_str           = str;
	_display_width = display_width;
	if (str.length() > _display_width) {
		_scrollable_str = "";
		for (int i = 0; i < _display_width - 1; i++) {
			_scrollable_str += " ";
		}
		_scrollable_str += _str;
		for (int i = 0; i < _display_width - 1; i++) {
			_scrollable_str += " ";
		}
		_count       = _display_width - 2;
	} else {
		_scrollable_str = _str;
		_count = 0;
	}
	_called_next = false;
}

void ScrollableString::setLcdWidth(uint8_t display_width) {
	_display_width = display_width;
	if (_str.length() > _display_width) {
		_scrollable_str = "";
		for (int i = 0; i < _display_width - 1; i++) {
			_scrollable_str += " ";
		}
		_scrollable_str += _str;
		for (int i = 0; i < _display_width - 1; i++) {
			_scrollable_str += " ";
		}
		_count = _display_width - 2;
	} else {
		_scrollable_str = _str;
		_count = 0;
	}
}

void ScrollableString::setString(String str) {
	_str = str;
	if (_str.length() > _display_width) {
		_scrollable_str = "";
		for (int i = 0; i < _display_width - 1; i++) {
			_scrollable_str += " ";
		}
		_scrollable_str += _str;
		for (int i = 0; i < _display_width - 1; i++) {
			_scrollable_str += " ";
		}
		_count = _display_width - 2;
	} else {
		_scrollable_str = _str;
		_count = 0;
	}
}

void ScrollableString::setOffset(uint8_t offset) {
	_scrollable_str = "";
	for (int i = 0; i < offset; i++) {
		_scrollable_str += " ";
	}
	_scrollable_str += _str;
	for (int i = 0; i < offset; i++) {
		_scrollable_str += " ";
	}
}

void ScrollableString::next() {
	if (_str.length() > _display_width) {
		_count       = ((_count + 1) == (_scrollable_str.length() - _display_width + 1)) ? 0 : (_count + 1);
	}
}

bool ScrollableString::isChangedPosition() {
	bool tmp     = _called_next;
	_called_next = false;
	return tmp;
}

String ScrollableString::getScrollableString() {
	return _scrollable_str.substring(_count, _count + _display_width);
}

#endif /* TARGET__ESP32_DEVKIT */