#include "user_display_default.h"

#ifdef USER_DISPLAY_DEFAULT_H
#ifdef DISPLAY_64X48_MONO

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: State
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

static int g_pending_ftp      = 0;
static int g_pending_http     = 0;
static int g_pending_mqtt     = 0;
static int g_pending_ambient  = 0;
static int g_pending_lora_wan = 0;

int g_sensor_num            = 0;
int g_current_sensor_number = 1;
struct tm g_last_timeinfo   = {0};

struct _Features {
	bool ftp;
	bool http;
	bool mqtt;
	bool ambient;
	bool lora_wan;
};
struct _Features g_features = {false, false, false, false, false};

struct _RemainData {
	int ftp;
	int http;
	int mqtt;
	int ambient;
	int lora_wan;
};
struct _RemainData g_last_remain_data = {0};

enum _DisplayPage {
	MAIN_PAGE_SENSOR_1,
	MAIN_PAGE_INFO_1,
	// MAIN_PAGE_INFO_2,
	// MAIN_PAGE_INFO_3,
	_MAIN_PAGE_SENTINEL,
	CONFIG_PAGE_1,
	CONFIG_PAGE_2,
	_CONFIG_PAGE_SENTINEL,
};
enum _DisplayPage g_display_page = MAIN_PAGE_SENSOR_1;

enum class _SensorType {
	NONE,
	INTEGER,
	DOUBLE,
	STRING,
	ERROR,
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

constexpr uint8_t HEADER_FIELD_HEIGHT = 1;
constexpr uint8_t MAIN_FIELD_POS_Y    = 5;
constexpr uint8_t FONT_HEIGHT         = 8;
constexpr uint8_t FONT_WIDTH          = 8;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Helper Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

class DisplayMutexGuard {
public:
	DisplayMutexGuard() {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
	}
	~DisplayMutexGuard() {
		HAL.Display.giveSemaphoreMutex();
	}

	// コピーコンストラクタと代入演算子を削除
	DisplayMutexGuard(const DisplayMutexGuard &)            = delete;
	DisplayMutexGuard &operator=(const DisplayMutexGuard &) = delete;
};

void drawSensorName(const char *sensor_name, bool force_update = false) {
	static String _last_sensor_name = "";
	if (force_update or not _last_sensor_name.equals(String(sensor_name))) {

		const int32_t WIDTH          = HAL.Display.width();
		const int32_t LINE1_Y        = MAIN_FIELD_POS_Y;
		const int32_t LINE2_Y        = MAIN_FIELD_POS_Y + FONT_HEIGHT;
		const int32_t ELLIPSIS_WIDTH = 8; // 2+1+2+1+2 px

		DisplayMutexGuard _;
		// 2×2 ドットを3つ（8px幅）、フォントセル下部に配置
		auto drawDots = [](int32_t x, int32_t y) {
			HAL.Display.fillRect(x + 0, y + 5, 2, 2, TFT_WHITE);
			HAL.Display.fillRect(x + 3, y + 5, 2, 2, TFT_WHITE);
			HAL.Display.fillRect(x + 6, y + 5, 2, 2, TFT_WHITE);
		};
		// 描画領域を初期化
		HAL.Display.fillRect(0, LINE1_Y, WIDTH, FONT_HEIGHT * 2, TFT_BLACK);

		HAL.Display.setCursor(0, LINE1_Y);
		HAL.Display.printf("%d:", g_current_sensor_number);

		uint8_t line = 1;
		int i        = 0;
		while (sensor_name[i] != '\0') {
			int32_t cur_x = HAL.Display.getCursorX();

			if (line == 1 && cur_x + FONT_WIDTH > WIDTH) {
				// 1行目が埋まったら2行目へ
				HAL.Display.setCursor(0, LINE2_Y);
				line  = 2;
				cur_x = 0;
			}

			if (line == 2) {
				bool is_last = (sensor_name[i + 1] == '\0');
				if (!is_last && cur_x + FONT_WIDTH + ELLIPSIS_WIDTH > WIDTH) {
					// 次の文字 + 省略ドットが収まらない → ドット描画して終了
					drawDots(cur_x, LINE2_Y);
					break;
				}
				if (cur_x + FONT_WIDTH > WIDTH) {
					// この文字自体が収まらない
					drawDots(cur_x, LINE2_Y);
					break;
				}
			}

			HAL.Display.printf("%c", sensor_name[i]);
			i++;
		}
		_last_sensor_name = String(sensor_name);
	}
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Sensor Value Update Template
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void updateSensorDisplay_Int(const char *name, int32_t value, const char *unit) {
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}

	drawSensorName(name);

	DisplayMutexGuard _;

	// センサ値描画
	HAL.Display.setCursor(0, 28);
	HAL.Display.printf("%10d", value);
	// センサ単位描画
	HAL.Display.setCursor(4, 40);
	HAL.Display.printf("%10s", unit);
}

void updateSensorDisplay_Double(const char *name, double value, const char *unit) {
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}

	drawSensorName(name);

	DisplayMutexGuard _;

	// センサ値描画
	HAL.Display.setCursor(0, 28);
	HAL.Display.printf("%10.2lf", value);
	// センサ単位描画
	HAL.Display.setCursor(4, 40);
	HAL.Display.printf("%10s", unit);
}

void updateSensorDisplay_String(const char *name, const char *value, const char *unit) {
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}

	drawSensorName(name);

	DisplayMutexGuard _;

	// センサ値描画
	HAL.Display.setCursor(0, 28);
	HAL.Display.printf("%10s", value);
	// センサ単位描画
	HAL.Display.setCursor(4, 40);
	HAL.Display.printf("%10s", unit);
}

void updateSensorDisplay_Error(const char *name) {
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}

	drawSensorName(name);

	DisplayMutexGuard _;

	// センサ値描画
	HAL.Display.setCursor(0, 28);
	HAL.Display.print("   Error  ");
	// センサ単位描画
	HAL.Display.setCursor(4, 40);
	HAL.Display.printf("%10s", "No Value");
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Callback Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void cb_BeforeSystemDisplayUpdate_Default(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
	switch (dt) {
	case DisplayTiming::FTP_INIT_DONE: {
		g_features.ftp = true;
		break;
	}
	case DisplayTiming::HTTP_INIT_DONE: {
		g_features.http = true;
		break;
	}
	case DisplayTiming::MQTT_INIT_DONE: {
		g_features.mqtt = true;
		break;
	}
	case DisplayTiming::AMBIENT_INIT_DONE: {
		g_features.ambient = true;
		break;
	}
	case DisplayTiming::LORA_WAN_INIT_DONE: {
		g_features.lora_wan = true;
		break;
	}
	default:
		break;
	}
}

void cb_AfterSystemDisplayUpdate_Default(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
	static bool heartbeat_circle_state = true;

	switch (dt) {
	case DisplayTiming::SS_READY: {
		g_display_page = CONFIG_PAGE_1;
		break;
	}
	case DisplayTiming::SS_LOOP: {
		if (not HAL.BtnA_wasPressed()) {
			// Nothing to do
			return;
		}

		// 表示ページを移動
		g_display_page = (((g_display_page + 1) == _CONFIG_PAGE_SENTINEL) ? CONFIG_PAGE_1 : static_cast<_DisplayPage>(g_display_page + 1));

		switch (g_display_page) {
		case CONFIG_PAGE_1: {
			DisplayMutexGuard _;
			HAL.Display.setCursor(0, 0);
			HAL.Display.println(" BLE Name");
			HAL.Display.println("----------");
			HAL.Display.printf("%s", HAL.getChipUUID().c_str());
			break;
		}

		case CONFIG_PAGE_2: {
			DisplayMutexGuard _;
			HAL.Display.setCursor(0, 0);
			HAL.Display.println(" WiFi MAC ");
			HAL.Display.println("----------");
			auto mac = HAL.Wlan.fetchMacAddress();
			for (auto it = mac.begin(); it != mac.end(); it++) {
				HAL.Display.printf("%02X", *it);
			}
			break;
		}
		default:
			break;
		}

		break;
	}
	case DisplayTiming::SETUP_END: {
		DisplayMutexGuard _;
		// HAL.Display.fillRect(0, 0, HAL.Display.width(), HEADER_FIELD_HEIGHT, TFT_WHITE);
		if (g_sensor_num == 0) {
			HAL.Display.setCursor(2, 20);
			HAL.Display.print("No Sensors");
		}
		break;
	}
	case DisplayTiming::EVERY_ONE_SECONDS_EVENT: {
		if (heartbeat_circle_state) {
			DisplayMutexGuard _;
			HAL.Display.drawFastHLine(0, 0, HAL.Display.width(), TFT_BLACK);
		} else {
			DisplayMutexGuard _;
			HAL.Display.drawFastHLine(0, 0, HAL.Display.width(), TFT_WHITE);
		}

		heartbeat_circle_state = not heartbeat_circle_state;
		break;
	}
	case DisplayTiming::BTNA_PRESSED: {
		g_current_sensor_number = (((g_current_sensor_number + 1) > g_sensor_num) ? 1 : g_current_sensor_number + 1);
		break;
	}
	case DisplayTiming::BTNC_PRESSED: {
		g_display_page = ((g_display_page + 1 == _MAIN_PAGE_SENTINEL) ? MAIN_PAGE_SENSOR_1 : static_cast<_DisplayPage>(g_display_page + 1));
		{
			// Clear center area
			DisplayMutexGuard _;
			HAL.Display.fillRect(0, MAIN_FIELD_POS_Y, HAL.Display.width(), HAL.Display.height() - MAIN_FIELD_POS_Y, TFT_BLACK);
		}

		switch (g_display_page) {
		case MAIN_PAGE_SENSOR_1: {
			if (g_sensor_num == 0) {
				DisplayMutexGuard _;
				HAL.Display.setCursor(2, 20);
				HAL.Display.print("No Sensors");
			} else {
				drawSensorName(""); // for force update

				switch (g_display_memory.type) {
				case _SensorType::INTEGER:
					updateSensorDisplay_Int(g_display_memory.name.c_str(),
					                        g_display_memory.vint,
					                        g_display_memory.unit.c_str());
					break;
				case _SensorType::DOUBLE:
					updateSensorDisplay_Double(g_display_memory.name.c_str(),
					                           g_display_memory.vdouble,
					                           g_display_memory.unit.c_str());
					break;
				case _SensorType::STRING:
					updateSensorDisplay_String(g_display_memory.name.c_str(),
					                           g_display_memory.vstring.c_str(),
					                           g_display_memory.unit.c_str());
					break;
				case _SensorType::ERROR:
					updateSensorDisplay_Error(g_display_memory.name.c_str());
					break;
				default:
					break;
				}
			}
			break;
		}
		case MAIN_PAGE_INFO_1: {
			cb_FtpBufferUpdate_Default(render, g_last_remain_data.ftp);
			cb_HttpBufferUpdate_Default(render, g_last_remain_data.http);
			cb_MqttBufferUpdate_Default(render, g_last_remain_data.mqtt);
			cb_AmbientBufferUpdate_Default(render, g_last_remain_data.ambient);
			cb_LoraBufferUpdate_Default(render, g_last_remain_data.lora_wan);
			break;
		}
		}
		break;
	}
	default:
		break;
	}
}

void cb_SystemReady_Default(std::unique_ptr<OpenFontRender> &render, const char *dev_id, int sensor_num) {
	g_sensor_num = sensor_num;
}

void cb_WaitTiming_Default(std::unique_ptr<OpenFontRender> &render, int current_sec) {
}

void cb_UpdateGpioValue_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Int(sensor_name, value, unit);
	g_display_memory.name = String(sensor_name);
	g_display_memory.unit = String(unit);
	g_display_memory.vint = value;
	g_display_memory.type = _SensorType::INTEGER;
}

void cb_UpdateI2cValueInt_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Int(sensor_name, value, unit);
	g_display_memory.name = String(sensor_name);
	g_display_memory.unit = String(unit);
	g_display_memory.vint = value;
	g_display_memory.type = _SensorType::INTEGER;
}

void cb_UpdateI2cValueDouble_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Double(sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vdouble = value;
	g_display_memory.type    = _SensorType::DOUBLE;
}

void cb_UpdateI2cValueString_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_String(sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vstring = String(value);
	g_display_memory.type    = _SensorType::STRING;
}

void cb_UpdateI2cValueError_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Error(sensor_name);
	g_display_memory.name = String(sensor_name);
	g_display_memory.type = _SensorType::ERROR;
}

void cb_UpdateModbusValueInt_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Int(sensor_name, value, unit);
	g_display_memory.name = String(sensor_name);
	g_display_memory.unit = String(unit);
	g_display_memory.vint = value;
	g_display_memory.type = _SensorType::INTEGER;
}

void cb_UpdateModbusValueDouble_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Double(sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vdouble = value;
	g_display_memory.type    = _SensorType::DOUBLE;
}

void cb_UpdateModbusValueString_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_String(sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vstring = String(value);
	g_display_memory.type    = _SensorType::STRING;
}

void cb_UpdateModbusValueError_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Error(sensor_name);
	g_display_memory.name = String(sensor_name);
	g_display_memory.type = _SensorType::ERROR;
}

void cb_SamplingGpioSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

void cb_SamplingI2cSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

void cb_SamplingModbusSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

void cb_SamplingCameraSensor_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
}

void cb_AmbientBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_pending_ambient = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_INFO_1) {
		return;
	}

	DisplayMutexGuard _;
	HAL.Display.setCursor(0, MAIN_FIELD_POS_Y + (FONT_HEIGHT * 0) + 0);
	HAL.Display.printf("AMB  %c %3d", g_features.ambient ? 'o' : 'x', g_pending_ambient);
}

void cb_FtpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_pending_ftp = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_INFO_1) {
		return;
	}

	DisplayMutexGuard _;
	HAL.Display.setCursor(0, MAIN_FIELD_POS_Y + (FONT_HEIGHT * 1) + 1);
	HAL.Display.printf("FTP  %c %3d", g_features.ftp ? 'o' : 'x', g_pending_ftp);
}

void cb_HttpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_pending_http = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_INFO_1) {
		return;
	}

	DisplayMutexGuard _;
	HAL.Display.setCursor(0, MAIN_FIELD_POS_Y + (FONT_HEIGHT * 2) + 2);
	HAL.Display.printf("HTTP %c %3d", g_features.http ? 'o' : 'x', g_pending_http);
}

void cb_MqttBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_pending_mqtt = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_INFO_1) {
		return;
	}

	DisplayMutexGuard _;
	HAL.Display.setCursor(0, MAIN_FIELD_POS_Y + (FONT_HEIGHT * 3) + 3);
	HAL.Display.printf("MQTT %c %3d", g_features.mqtt ? 'o' : 'x', g_pending_mqtt);
}

void cb_LoraBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_pending_lora_wan = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_INFO_1) {
		return;
	}

	DisplayMutexGuard _;
	HAL.Display.setCursor(0, MAIN_FIELD_POS_Y + (FONT_HEIGHT * 4) + 4);
	HAL.Display.printf("LoRa %c %3d", g_features.lora_wan ? 'o' : 'x', g_pending_lora_wan);
}

void cb_ReadMqttStatus_Default(std::unique_ptr<OpenFontRender> &render, int status) {
}

#endif /* DISPLAY_64X48_MONO */
#endif /* USER_DISPLAY_DEFAULT_H */
