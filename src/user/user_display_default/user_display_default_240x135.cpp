#include "user_display_default.h"

#ifdef USER_DISPLAY_DEFAULT_H
#ifdef DISPLAY_240X135

#include "icon.h"

const uint16_t MY_DARKGRAY = 12678;
const uint16_t MY_CRIMSON  = 55463;

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
	MAIN_PAGE_SENSOR_2,
	MAIN_PAGE_INFO_1,
	MAIN_PAGE_INFO_2,
	MAIN_PAGE_INFO_3,
	_MAIN_PAGE_SENTINEL,
	CONFIG_PAGE_1,
	CONFIG_PAGE_2,
	_CONFIG_PAGE_SENTINEL,
};
enum _DisplayPage g_display_page = MAIN_PAGE_SENSOR_1;

struct _Rect {
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;
};

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

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Helper Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void drawSensorName(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool force_update = false) {
	static String _last_sensor_name = "";
	if (force_update or not _last_sensor_name.equals(String(sensor_name))) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(100, 30, 140, 30, TFT_BLACK);
		render->setFontSize(12);
		render->setCursor(105, 30);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		render->printf("[%d/%d] ", g_current_sensor_number, g_sensor_num);

		uint8_t lines = 1;
		int i         = 0;
		while (sensor_name[i] != '\0') {
			render->printf("%c", sensor_name[i]);
			if (render->getCursorX() > 230) {
				render->printf("\n");
				render->setCursor(105, render->getCursorY());
				lines++;
			}
			i++;

			if (lines > 2) {
				// スペースが無いため、２行以上は切り捨てて描画しない
				break;
			}
		}
		HAL.Display.giveSemaphoreMutex();

		_last_sensor_name = String(sensor_name);
	}
}

void drawSensorUnit(std::unique_ptr<OpenFontRender> &render, const char *sensor_unit, bool force_update = false) {
	static String _last_unit = "";
	if (force_update or not _last_unit.equals(String(sensor_unit))) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// Update sensor unit
		render->setFontSize(12);
		// Clear prev unit
		render->setCursor(235, 91);
		render->setFontColor(TFT_BLACK, TFT_BLACK);
		render->rprintf("%s", _last_unit.c_str());
		// Draw new unit
		render->setCursor(235, 91);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		render->rprintf("%s", sensor_unit);

		HAL.Display.giveSemaphoreMutex();

		_last_unit = String(sensor_unit);
	}
}

void drawLastUpdateDate(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo, bool force_update = false) {
	static String _last_s_day = "";
	char _s_day[64]           = {0};

	if (timeinfo.tm_mday != 0) {
		// tm_mday usually ranges from 1 to 31.
		strftime(_s_day, 64, "%m/%d %H:%M", &timeinfo);
	} else {
		strftime(_s_day, 64, "- - - -", &timeinfo);
	}
	if (force_update or not _last_s_day.equals(String(_s_day))) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setFontSize(18);
		render->setCursor(2, 3);
		render->setFontColor(MY_DARKGRAY, MY_DARKGRAY);
		render->printf("更新 : %s", _last_s_day.c_str());
		render->setCursor(2, 3);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		render->printf("更新 : %s", _s_day);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex();

		_last_s_day = String(_s_day);
	}
}

void drawCurrentIpAddress(std::unique_ptr<OpenFontRender> &render, String ip_addr, bool force_update = false) {
	static String _last_ip_addr = "";

	if (force_update or not _last_ip_addr.equals(ip_addr)) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setFontSize(16);
		render->setCursor(2, 3);
		render->setFontColor(MY_DARKGRAY, MY_DARKGRAY);
		render->printf("IP : %s", _last_ip_addr.c_str());
		render->setCursor(2, 3);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		render->printf("IP : %s", ip_addr.c_str());
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex();

		_last_ip_addr = ip_addr;
	}
}

void drawLeftPanelFrame() {
	uint16_t _panel_color = 0;
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

#ifdef DISPLAY_INCLUDE_LORA
	// Left panels (2 columns, 3 rows)
	// Panel area: x=0..98, y=29..107 (w=98, h=78)
	// Columns: x=2 w=46, x=50 w=46 (gap=2px between cols)
	// Rows: y=31 h=24, y=56 h=24, y=81 h=24 (gap=1px between rows)
	HAL.Display.fillRect(0, 29, 98, 78, TFT_LIGHTGREY);
	// FTP (col=0, row=0)
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31, 46, 24, _panel_color);
	// HTTP (col=1, row=0)
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(50, 31, 46, 24, _panel_color);
	// MQTT (col=0, row=1)
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 56, 46, 24, _panel_color);
	// Ambient (col=1, row=1)
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(50, 56, 46, 24, _panel_color);
	// LoRa (col=0, row=2)
	_panel_color = g_features.lora_wan ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 81, 46, 24, _panel_color);
	// (Blank) (col=1, row=2)
	HAL.Display.fillRect(50, 81, 46, 24, MY_CRIMSON);
#else
	// Left panels (2 columns, 2 rows: "田" shape)
	HAL.Display.fillRect(0, 29, 98, 78, TFT_LIGHTGREY);
	// FTP
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31, 50 - 4, 36, _panel_color);
	// HTTP
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31 + 38, 50 - 4, 36, _panel_color);
	// MQTT
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(50, 31, 50 - 4, 36, _panel_color);
	// Ambient
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(50, 31 + 38, 50 - 4, 36, _panel_color);
#endif

	HAL.Display.giveSemaphoreMutex();
}

void drawFeatures2LeftPanel(std::unique_ptr<OpenFontRender> &render) {
	uint16_t _panel_color = 0;
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

#ifdef DISPLAY_INCLUDE_LORA
	// 2 columns, 3 rows layout
	// Col centers: x=25 (col=0), x=73 (col=1)
	// Row panels: y=31 h=24, y=56 h=24, y=81 h=24
	// FTP (col=0, row=0)
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(16);
	render->setCursor(25, 31);
	render->cprintf("FTP");
	// HTTP (col=1, row=0)
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(14);
	render->setCursor(73, 32);
	render->cprintf("HTTP");
	// MQTT (col=0, row=1)
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(14);
	render->setCursor(25, 57);
	render->cprintf("MQTT");
	// Ambient (col=1, row=1)
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(16);
	render->setCursor(73, 56);
	render->cprintf("AMB");
	// LoRa (col=0, row=2)
	_panel_color = g_features.lora_wan ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(14);
	render->setCursor(25, 82);
	render->cprintf("LoRa");
#else
	// FTP
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(22);
	render->setCursor(25, 35 - 3);
	render->cprintf("FTP");
	// HTTP
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(16);
	render->setCursor(24, 35 + 38 + 3);
	render->cprintf("HTTP");
	// MQTT
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(16);
	render->setCursor(24 + 49, 38 - 3);
	render->cprintf("MQTT");
	// Ambient
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(21);
	render->setCursor(25 + 49, 35 + 38 - 3);
	render->cprintf("AMB");
#endif

	render->setFontColor(TFT_WHITE, TFT_BLACK);
	HAL.Display.giveSemaphoreMutex();
}

void drawNetworkStatus(std::unique_ptr<OpenFontRender> &render) {
	bool _isConnected = false;
	MyIPAddress _ip;

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI: {
		static const char *const WIFI_STATUS_MESSAGES[] = {
		    "Idle status.",       // 0
		    "No SSID available.", // 1
		    "Scan Completed!",    // 2
		    "Connected!",         // 3
		    "Connect Failed.",    // 4
		    "Connection Lost.",   // 5
		    "Disconnected.",      // 6
		};

		int _status  = HAL.Wlan.getStatus();
		int _rssi    = HAL.Wlan.getRSSI();
		_ip          = HAL.Wlan.localIP();
		_isConnected = (_status == 3);

		const char *_reason = (_status >= 0 and _status <= 6)
		                          ? WIFI_STATUS_MESSAGES[_status]
		                          : "Disabled Wi-Fi module.";

		HAL.printf("WiFi status -> %s", _reason);
		if (_isConnected) {
			HAL.printf(" <RSSI Level: %d>\n", _rssi);
		} else {
			HAL.println("");
			_rssi = -200;
		}

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		Icon_WiFiStatus(175, 21, _rssi);
		HAL.Display.giveSemaphoreMutex();

		break;
	}
	case DATALINK_LAYER::ETHERNET: {
		_isConnected = HAL.Lan.isLinkUp();
		_ip          = HAL.Lan.localIP();

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		Icon_EthernetStatus(165, 5, _isConnected);
		HAL.Display.giveSemaphoreMutex();

		break;
	}
	default:
		break;
	}

	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	if (_isConnected) {
		drawCurrentIpAddress(render, _ip.toString());
	} else {
		drawCurrentIpAddress(render, "- - - -");
	}
}

void drawInformation(std::unique_ptr<OpenFontRender> &render) {
	constexpr struct _Rect rect    = {10, 30, 220, 19};
	constexpr uint8_t FONT_SIZE    = 14;
	constexpr uint8_t LABEL_COUNT  = 4;
	constexpr uint8_t SEP_GAP      = 3;
	constexpr uint8_t SEP_WIDTH    = 2;
	constexpr uint8_t VALUE_OFFSET = SEP_GAP + SEP_WIDTH + 5;
	const char *const labels[]     = {"デバイス種別", "FW バージョン", "送信間隔", "接続先 Wi-Fi"};

	// ラベル幅から water mark を算出
	render->setFontSize(FONT_SIZE);
	uint16_t x_water_mark = 0;
	for (auto label : labels) {
		uint16_t w = render->getTextWidth(label);
		if (w > x_water_mark)
			x_water_mark = w;
	}
	x_water_mark += rect.x + 2;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// draw device id
	render->setFontSize(18);
	render->setCursor(2, 1);
	render->setFontColor(TFT_ORANGE, MY_DARKGRAY);
	render->printf("各種情報 1");
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	// draw labels
	render->setFontSize(FONT_SIZE);
	for (int i = 0; i < LABEL_COUNT; i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = rect.height * i;

		HAL.Display.fillRect(rect.x, rect.y + y_offset, rect.width, rect.height, bg);
		render->setFontColor(MY_DARKGRAY, bg);
		render->setCursor(rect.x + 2, rect.y + y_offset - 1);
		render->printf(labels[i]);
	}

	// draw separator
	for (int i = 0; i < SEP_WIDTH; i++) {
		HAL.Display.drawFastVLine(x_water_mark + SEP_GAP + i, rect.y,
		                          rect.height * LABEL_COUNT, TFT_DARKGREY);
	}

	// draw values
	const uint16_t VALUE_CELL_WIDTH = rect.x + rect.width - x_water_mark - VALUE_OFFSET;
	const uint16_t text_x           = x_water_mark + VALUE_OFFSET;

	render->setFontSize(FONT_SIZE);
	for (int i = 0; i < LABEL_COUNT; i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = rect.height * i - 1;

		render->setFontColor(MY_DARKGRAY, bg);
		String info = "- - - -";

		switch (i) {
		case 0:
			info = HAL.Info.DEVICE_NAME;
			break;
		case 1:
			info = String("v") + String(HAL.Info.V_MAIN) + "." + String(HAL.Info.V_MINOR) + " " + String(HAL.Info.V_RELEASE_TAG);
			break;
		case 2: {
			uint16_t h = HAL.Info.TX_INTERVAL_SEC / 3600;
			uint8_t m  = (HAL.Info.TX_INTERVAL_SEC % 3600) / 60;
			info       = String(h) + "時間 ";
			info += String(m) + "分 ";
			info += String(HAL.Info.TX_INTERVAL_SEC % 60) + "秒  毎";
			break;
		}
		case 3:
			info = HAL.Info.WIFI_SSID;
			break;
		default:
			break;
		}

		if (render->getTextWidth(info.c_str()) < VALUE_CELL_WIDTH) {
			render->setCursor(text_x, rect.y + y_offset);
			render->printf(info.c_str());
		} else {
			uint32_t size = FONT_SIZE;
			while (render->getTextWidth(info.c_str()) > VALUE_CELL_WIDTH and size > 1) {
				size--;
				render->setFontSize(size);
			}
			render->setCursor(text_x, rect.y + y_offset);
			render->printf(info.c_str());
			render->setFontSize(FONT_SIZE);
		}
	}

	render->setFontColor(TFT_WHITE, TFT_BLACK);
	HAL.Display.giveSemaphoreMutex();
}

void drawInformation2(std::unique_ptr<OpenFontRender> &render) {
	constexpr struct _Rect rect    = {10, 30, 220, 19};
	constexpr uint8_t FONT_SIZE    = 14;
	constexpr uint8_t LABEL_COUNT  = 3;
	constexpr uint8_t SEP_GAP      = 3;
	constexpr uint8_t SEP_WIDTH    = 2;
	constexpr uint8_t VALUE_OFFSET = SEP_GAP + SEP_WIDTH + 5;
	const char *const labels[]     = {"FTPサーバ", "HTTPサーバ", "MQTTサーバ"};

	// ラベル幅から water mark を算出
	render->setFontSize(FONT_SIZE);
	uint16_t x_water_mark = 0;
	for (auto label : labels) {
		uint16_t w = render->getTextWidth(label);
		if (w > x_water_mark)
			x_water_mark = w;
	}
	x_water_mark += rect.x + 2;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// draw device id
	render->setFontSize(18);
	render->setCursor(2, 1);
	render->setFontColor(TFT_ORANGE, MY_DARKGRAY);
	render->printf("各種情報 2");
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	// draw labels
	render->setFontSize(FONT_SIZE);
	for (int i = 0; i < LABEL_COUNT; i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = rect.height * i;

		HAL.Display.fillRect(rect.x, rect.y + y_offset, rect.width, rect.height, bg);
		render->setFontColor(MY_DARKGRAY, bg);
		render->setCursor(rect.x + 2, rect.y + y_offset - 1);
		render->printf(labels[i]);
	}

	// draw separator
	for (int i = 0; i < SEP_WIDTH; i++) {
		HAL.Display.drawFastVLine(x_water_mark + SEP_GAP + i, rect.y,
		                          rect.height * LABEL_COUNT, TFT_DARKGREY);
	}

	// draw values
	const uint16_t VALUE_CELL_WIDTH = rect.x + rect.width - x_water_mark - VALUE_OFFSET;
	const uint16_t text_x           = x_water_mark + VALUE_OFFSET;

	render->setFontSize(FONT_SIZE);
	for (int i = 0; i < LABEL_COUNT; i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = rect.height * i - 1;

		render->setFontColor(MY_DARKGRAY, bg);
		String info = "- - - -";

		switch (i) {
		case 0:
			info = HAL.Info.FTP_SERVER;
			break;
		case 1:
			info = HAL.Info.HTTP_SERVER;
			break;
		case 2:
			info = HAL.Info.MQTT_SERVER;
			break;
		default:
			break;
		}

		if (render->getTextWidth(info.c_str()) < VALUE_CELL_WIDTH) {
			render->setCursor(text_x, rect.y + y_offset);
			render->printf(info.c_str());
		} else {
			uint32_t size = FONT_SIZE;
			while (render->getTextWidth(info.c_str()) > VALUE_CELL_WIDTH and size > 1) {
				size--;
				render->setFontSize(size);
			}
			render->setCursor(text_x, rect.y + y_offset);
			render->printf(info.c_str());
			render->setFontSize(FONT_SIZE);
		}
	}

	render->setFontColor(TFT_WHITE, TFT_BLACK);
	HAL.Display.giveSemaphoreMutex();
}

void drawInformation3(std::unique_ptr<OpenFontRender> &render, bool update_only = false) {
	constexpr struct _Rect rect    = {10, 30, 220, 19};
	constexpr uint8_t FONT_SIZE    = 14;
	constexpr uint8_t LABEL_COUNT  = 4;
	constexpr uint8_t SEP_GAP      = 3;
	constexpr uint8_t SEP_WIDTH    = 2;
	constexpr uint8_t VALUE_OFFSET = SEP_GAP + SEP_WIDTH + 5;
	const char *const labels[]     = {"デバイスID", "起動時間", "Free Heap", "Min Heap"};

	// ラベル幅から water mark を算出
	render->setFontSize(FONT_SIZE);
	uint16_t x_water_mark = 0;
	for (auto label : labels) {
		uint16_t w = render->getTextWidth(label);
		if (w > x_water_mark)
			x_water_mark = w;
	}
	x_water_mark += rect.x + 2;

	if (not update_only) {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

		// draw device id
		render->setFontSize(18);
		render->setCursor(2, 1);
		render->setFontColor(TFT_ORANGE, MY_DARKGRAY);
		render->printf("各種情報 3");
		render->setFontColor(TFT_WHITE, TFT_BLACK);

		// draw labels
		render->setFontSize(FONT_SIZE);
		for (int i = 0; i < LABEL_COUNT; i++) {
			uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
			uint32_t y_offset = rect.height * i;

			HAL.Display.fillRect(rect.x, rect.y + y_offset, rect.width, rect.height, bg);
			render->setFontColor(MY_DARKGRAY, bg);
			render->setCursor(rect.x + 2, rect.y + y_offset - 2);
			render->printf(labels[i]);
		}

		// draw separator
		for (int i = 0; i < SEP_WIDTH; i++) {
			HAL.Display.drawFastVLine(x_water_mark + SEP_GAP + i, rect.y,
			                          rect.height * LABEL_COUNT, TFT_DARKGREY);
		}

		HAL.Display.giveSemaphoreMutex();
	}

	// draw values
	const uint16_t text_x    = x_water_mark + VALUE_OFFSET;
	const int32_t right_edge = rect.x + rect.width;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	render->setFontSize(FONT_SIZE);
	auto prev_fill = render->getBackgroundFillMethod();
	render->setBackgroundFillMethod(BgFillMethod::Block);

	for (int i = 0; i < LABEL_COUNT; i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = rect.height * i;

		render->setFontColor(MY_DARKGRAY, bg);
		String info = "- - - -";

		switch (i) {
		case 0:
			if (update_only) {
				continue;
			}
			info = HAL.Info.DEVICE_ID;
			if (info.length() > 14) {
				info = info.substring(0, 14) + "...";
			}
			break;
		case 1: {
			unsigned long ms = millis();
			unsigned long s  = ms / 1000;
			unsigned long m  = s / 60;
			unsigned long h  = m / 60;
			unsigned long d  = h / 24;
			char buf[32];
			snprintf(buf, sizeof(buf), "%lu日 %lu時間 %lu分 %02lu秒", d, h % 24, m % 60, s % 60);
			info = buf;
			break;
		}
		case 2:
			info = String(HAL.getFreeHeapSize()) + " bytes";
			break;
		case 3:
			info = String(HAL.getMinimumFreeHeapSize()) + " bytes";
			break;
		default:
			break;
		}

		render->setCursor(text_x, rect.y + y_offset - 2);
		render->printf(info.c_str());

		// clear remaining area after text (prevents artifacts when text gets shorter)
		int32_t text_end_x = render->getCursorX();
		if (text_end_x > text_x and text_end_x < right_edge) {
			HAL.Display.fillRect(text_end_x, rect.y + y_offset, right_edge - text_end_x, rect.height, bg);
		}
	}

	render->setBackgroundFillMethod(prev_fill);
	render->setFontColor(TFT_WHITE, TFT_BLACK);
	HAL.Display.giveSemaphoreMutex();
}

void drawConfigPage(std::unique_ptr<OpenFontRender> &render, enum _DisplayPage page) {
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	switch (page) {
	case CONFIG_PAGE_1: {
		constexpr uint32_t IMG_H = 50;

		HAL.Display.fillRect(0, 0, HAL.Display.width(), HAL.Display.height(), TFT_BLACK);
		render->setCursor(3, 3);
		render->printf("Bluetooth LE 接続名\n");
		render->printf("%s", HAL.getChipUUID().c_str());

		drawRLEIcon(40, IMG_H, BluetoothIconWidth,
		            BluetoothIcon_Palette, BluetoothIcon_RLE_Packed,
		            BluetoothIcon_RLE_RunCount);
		HAL.Display.fillRect(153 - 40, 73 - 15, 10, 30, TFT_WHITE);
		HAL.Display.fillRect(143 - 40, 83 - 15, 30, 10, TFT_WHITE);
		drawRLEIcon(145, IMG_H, DynamicspidIconWidth,
		            DynamicspidIcon_Palette, DynamicspidIcon_RLE_Packed,
		            DynamicspidIcon_RLE_RunCount);

		render->setFontColor(TFT_YELLOW, TFT_BLACK);
#if defined(BTN_LAYOUT_STICK_C_PLUS)

		render->setCursor(115, 108);
		render->printf("↓ 表示切替");

#elif defined(BTN_LAYOUT_STACK_BASIC)

		render->setCursor(170, 108);
		render->printf("表示切替");

#endif
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		break;
	}
	case CONFIG_PAGE_2: {
		HAL.Display.fillRect(0, 0, HAL.Display.width(), HAL.Display.height(), TFT_BLACK);

		render->setCursor(3, 3);
		if (HAL.Wlan.isSupported()) {
			auto mac = HAL.Wlan.fetchMacAddress();
			render->printf("無線LAN MAC:\n    ");
			for (auto it = mac.begin(); it != mac.end(); it++) {
				render->printf("%02X", *it);
				if (it != std::prev(mac.end())) {
					render->printf("-");
				} else {
					render->printf("\n");
					render->setCursor(3, render->getCursorY());
				}
			}
		}

		if (HAL.Lan.isSupported()) {
			auto mac = HAL.Lan.fetchMacAddress();
			render->printf("有線LAN MAC:\n    ");
			for (auto it = mac.begin(); it != mac.end(); it++) {
				render->printf("%02X", *it);
				if (it != std::prev(mac.end())) {
					render->printf("-");
				} else {
					render->printf("\n");
					render->setCursor(3, render->getCursorY());
				}
			}
		}

		render->setFontColor(TFT_YELLOW, TFT_BLACK);
#if defined(BTN_LAYOUT_STICK_C_PLUS)

		render->setCursor(115, 108);
		render->printf("↓ 表示切替");

#elif defined(BTN_LAYOUT_STACK_BASIC)

		render->setCursor(170, 108);
		render->printf("表示切替");

#endif
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		break;
	}
	default:
		break;
	}

	HAL.Display.giveSemaphoreMutex();
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Sensor Value Update Template
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void updateSensorDisplay_Int(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit) {
	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	drawSensorName(render, sensor_name);

	// Update sensor value
	OpenFontRender::Cursor c = {220, 60};
	const struct _Rect limit = {0, 0, 115, 28};
	const struct _Rect clear = {99, 60, 133, 33};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%d", value);
	FT_BBox bbox          = render->calculateBoundingBoxFmt(c.x, c.y, fit_size, Align::Right, Layout::Horizontal, "%d", value);

	// Calculate offset for vertical centering
	c.y = clear.y + (((clear.height - (bbox.yMax - bbox.yMin)) / 2) - (bbox.yMin - clear.y));

	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y); // set right cursor
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->rprintf("%d", value);

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, unit);
}

void updateSensorDisplay_Double(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit) {
	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	drawSensorName(render, sensor_name);

	// Update sensor value
	OpenFontRender::Cursor c = {220, 60};
	const struct _Rect limit = {0, 0, 115, 28};
	const struct _Rect clear = {99, 60, 133, 33};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%.1f", value);
	FT_BBox bbox          = render->calculateBoundingBoxFmt(c.x, c.y, fit_size, Align::Right, Layout::Horizontal, "%.1f", value);

	// Calculate offset for vertical centering
	c.y = clear.y + (((clear.height - (bbox.yMax - bbox.yMin)) / 2) - (bbox.yMin - clear.y));

	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y); // set right cursor
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->rprintf("%.1f", value);

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, unit);
}

void updateSensorDisplay_String(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit) {
	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	drawSensorName(render, sensor_name);

	// Update sensor value
	OpenFontRender::Cursor c = {220, 60};
	const struct _Rect limit = {0, 0, 115, 28};
	const struct _Rect clear = {99, 60, 133, 33};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%s", value);
	FT_BBox bbox          = render->calculateBoundingBoxFmt(c.x, c.y, fit_size, Align::Right, Layout::Horizontal, "%s", value);

	// Calculate offset for vertical centering
	c.y = clear.y + (((clear.height - (bbox.yMax - bbox.yMin)) / 2) - (bbox.yMin - clear.y));

	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y); // set right cursor
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->rprintf("%s", value);

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, unit);
}

void updateSensorDisplay_Error(std::unique_ptr<OpenFontRender> &render, const char *sensor_name) {
	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	const char str[] = "Sensor Error";

	drawSensorName(render, sensor_name);

	// Update sensor value
	OpenFontRender::Cursor c = {220, 60};
	const struct _Rect limit = {0, 0, 115, 28};
	const struct _Rect clear = {99, 60, 133, 33};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSize(limit.width, limit.height, Layout::Horizontal, str);
	FT_BBox bbox          = render->calculateBoundingBox(c.x, c.y, fit_size, Align::Right, Layout::Horizontal, str);

	// Calculate offset for vertical centering
	c.y = clear.y + (((clear.height - (bbox.yMax - bbox.yMin)) / 2) - (bbox.yMin - clear.y));

	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y); // set right cursor
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->setFontColor(TFT_YELLOW);
	render->rprintf(str);
	render->setFontColor(TFT_WHITE);

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, "Failed to get value");
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
	static bool heartbeat_circle_state  = true;
	static uint8_t btnA_pressed_counter = 0;

	switch (dt) {
	case DisplayTiming::SYSTEM_INIT_BEGIN: {
		// if (HAL.Tf.ready()) {
		//	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	Icon_TfCardStatus(295, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING), 0x0000);
		//	HAL.Display.giveSemaphoreMutex();
		// }
		break;
	}
	case DisplayTiming::SS_READY: {
		// HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// Icon_Bluetooth(10, 10);
		// HAL.Display.fillRect(153 - 40, 73 - 40, 10, 30, TFT_WHITE);
		// HAL.Display.fillRect(143 - 40, 83 - 40, 30, 10, TFT_WHITE);
		// Icon_Dynamicspid(145, 8);
		// HAL.Display.giveSemaphoreMutex();

		drawConfigPage(render, CONFIG_PAGE_1);

		// Switch to config mode
		g_display_page = CONFIG_PAGE_1;
		break;
	}
	case DisplayTiming::SS_LOOP: {
		if (not HAL.BtnC_wasPressed()) {
			// Nothing to do
			break;
		}

		// 表示ページを移動
		g_display_page = (((g_display_page + 1) == _CONFIG_PAGE_SENTINEL) ? CONFIG_PAGE_1 : static_cast<_DisplayPage>(g_display_page + 1));
		drawConfigPage(render, g_display_page);

		break;
	}
	case DisplayTiming::SETUP_END: {
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillScreen(TFT_BLACK);
		HAL.Display.fillRect(0, 0, 240, 29, MY_DARKGRAY);
		Icon_BatteryStatus(195, 5, HAL.getBatteryLevel());
		HAL.Display.giveSemaphoreMutex();

		drawNetworkStatus(render);

		struct tm timeinfo = {0};
		drawLastUpdateDate(render, timeinfo);

		// Draw Left panel
		drawLeftPanelFrame();
		drawFeatures2LeftPanel(render);

		if (g_sensor_num == 0) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			render->setFontSize(25);
			render->setCursor(HAL.Display.width() - 5, 50); // set right cursor
			render->setFontColor(TFT_WHITE, TFT_BLACK);
			render->rprintf("No Sensors");
			HAL.Display.giveSemaphoreMutex();
		}

		// Bottom bar
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(0, 107, 240, 28, MY_DARKGRAY);
		render->setFontSize(16);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		HAL.Display.giveSemaphoreMutex();

#if defined(BTN_LAYOUT_STICK_C_PLUS)

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setCursor(3, 108);
		render->printf("← センサ切替");
		render->setCursor(115, 108);
		render->printf("↓ 表示切替");
		HAL.Display.giveSemaphoreMutex();

#elif defined(BTN_LAYOUT_STACK_BASIC)

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setCursor(3, 108);
		render->printf("センサ切替");
		render->setCursor(170, 108);
		render->printf("表示切替");
		HAL.Display.giveSemaphoreMutex();

#endif
		break;
	}
	case DisplayTiming::EVERY_ONE_SECONDS_EVENT: {

		if (heartbeat_circle_state) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillEllipse(230, 15, 5, 5, TFT_GREENYELLOW);
			HAL.Display.giveSemaphoreMutex();
		} else {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillEllipse(230, 15, 5, 5, MY_DARKGRAY);
			HAL.Display.giveSemaphoreMutex();
		}
		heartbeat_circle_state = not heartbeat_circle_state;

		// WLAN manual disconnect option
		if (HAL.BtnA_isPressed()) {
			btnA_pressed_counter++;
			if (btnA_pressed_counter == 8) {
				HAL.Wlan.disconnectWlan(false, false);
				HAL.println("[WARN] WLAN manual disconnected OK!");

				HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
				HAL.Display.fillEllipse(205, 40, 5, 5, TFT_YELLOW);
				HAL.Display.giveSemaphoreMutex();
			}
		} else {
			if (btnA_pressed_counter >= 8) {
				HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
				HAL.Display.fillEllipse(205, 40, 5, 5, TFT_BLACK);
				HAL.Display.giveSemaphoreMutex();
				drawNetworkStatus(render);
			}
			btnA_pressed_counter = 0;
		}

		if (g_display_page == MAIN_PAGE_INFO_3) {
			drawInformation3(render, true);
		}

		break;
	}
	case DisplayTiming::EVERY_HALF_MINUTES_EVENT: {
		// Update battery status
		HAL.printf("Battery Level: %d %%\n", HAL.getBatteryLevel());

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		Icon_BatteryStatus(195, 5, HAL.getBatteryLevel());
		HAL.Display.giveSemaphoreMutex();

		break;
	}
	case DisplayTiming::EVERY_ONE_MINUTES_EVENT: {

		drawNetworkStatus(render);

		break;
	}
	case DisplayTiming::BTNA_PRESSED: {

		g_current_sensor_number = (((g_current_sensor_number + 1) > g_sensor_num) ? 1 : g_current_sensor_number + 1);

		break;
	}
	case DisplayTiming::BTNB_PRESSED: {
		// if (not HAL.Tf.ready()) {
		// 	Icon_TfCardStatus(240, 4, TfCardStatusT::UNMOUNTED);
		// } else {
		// 	Icon_TfCardStatus(240, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING));
		// }
		break;
	}
	case DisplayTiming::BTNC_PRESSED: {
		g_display_page = ((g_display_page + 1 == _MAIN_PAGE_SENTINEL) ? MAIN_PAGE_SENSOR_1 : static_cast<_DisplayPage>(g_display_page + 1));

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(0, 0, 160, 29, MY_DARKGRAY); // Clear top bar
		HAL.Display.giveSemaphoreMutex();

		const bool FORCE_UPDATE = true;

		switch (g_display_page) {
		case MAIN_PAGE_SENSOR_1: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 29, 240, 78, TFT_BLACK); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawLeftPanelFrame();
			drawLastUpdateDate(render, g_last_timeinfo, FORCE_UPDATE);
			drawFeatures2LeftPanel(render);

			if (g_sensor_num == 0) {
				HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
				render->setFontSize(25);
				render->setCursor(HAL.Display.width() - 5, 50); // set right cursor
				render->setFontColor(TFT_WHITE, TFT_BLACK);
				render->rprintf("No Sensors");
				HAL.Display.giveSemaphoreMutex();
			} else {
				drawSensorName(render, ""); // For force update
				drawSensorUnit(render, ""); // For force update

				switch (g_display_memory.type) {
				case _SensorType::INTEGER:
					updateSensorDisplay_Int(render,
					                        g_display_memory.name.c_str(),
					                        g_display_memory.vint,
					                        g_display_memory.unit.c_str());
					break;
				case _SensorType::DOUBLE:
					updateSensorDisplay_Double(render,
					                           g_display_memory.name.c_str(),
					                           g_display_memory.vdouble,
					                           g_display_memory.unit.c_str());
					break;
				case _SensorType::STRING:
					updateSensorDisplay_String(render,
					                           g_display_memory.name.c_str(),
					                           g_display_memory.vstring.c_str(),
					                           g_display_memory.unit.c_str());
					break;
				case _SensorType::ERROR:
					updateSensorDisplay_Error(render,
					                          g_display_memory.name.c_str());
					break;
				default:
					break;
				}
			}
			break;
		}
		case MAIN_PAGE_SENSOR_2: {
			drawLeftPanelFrame();
			drawCurrentIpAddress(render, ""); // For force update
			drawNetworkStatus(render);

			cb_FtpBufferUpdate_Default(render, g_last_remain_data.ftp);
			cb_HttpBufferUpdate_Default(render, g_last_remain_data.http);
			cb_MqttBufferUpdate_Default(render, g_last_remain_data.mqtt);
			cb_AmbientBufferUpdate_Default(render, g_last_remain_data.ambient);
			cb_LoraBufferUpdate_Default(render, g_last_remain_data.lora_wan);

			break;
		}
		case MAIN_PAGE_INFO_1: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 29, 240, 78, TFT_ORANGE); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawInformation(render);

			break;
		}
		case MAIN_PAGE_INFO_2: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 29, 240, 78, TFT_ORANGE); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawInformation2(render);

			break;
		}
		case MAIN_PAGE_INFO_3: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 29, 240, 78, TFT_ORANGE); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawInformation3(render);

			break;
		}
		default:
			break;
		}

		break;
	}
	default:
		break;
	}
}

void cb_SystemReady_Default(std::unique_ptr<OpenFontRender> &render, const char *dev_id, int sensor_num) {
	/* If you want to change the font, please reset here */
	// render.reset(new OpenFontRender);

	/* If you use RFID Reader, recomend to change data read interval */
	// timerAlarmWrite(timer2, 100000, true);	// for change i2c timer interval -> each 100 ms

	g_sensor_num = sensor_num;
}

void cb_WaitTiming_Default(std::unique_ptr<OpenFontRender> &render, int current_sec) {
	static int _last_sec = -1;

	if (_last_sec == current_sec) {
		return;
	}

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	if (_last_sec < 0) {
		render->setFontSize(20);
		render->setCursor((HAL.Display.width() / 2), 10);
		render->cprintf("Waiting 00 sec");
		fillArc((HAL.Display.width() / 2), 85, 29, 41, -90 + (6 * current_sec), 270, TFT_ORANGE);
	}
	// clear last value
	render->setFontColor(TFT_BLACK);
	render->setFontSize(30);
	render->setCursor(105, 63);
	render->printf("%02d", 60 - current_sec + 1);

	render->setFontColor(TFT_WHITE);
	render->setCursor(105, 63);
	render->printf("%02d", 60 - current_sec);

	fillArc((HAL.Display.width() / 2), 85, 29, 41, -90, -90 + (6 * current_sec), TFT_BLACK);
	render->setFontColor(TFT_WHITE);

	HAL.Display.giveSemaphoreMutex();
	_last_sec = current_sec;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Sensor Value Update Callbacks
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void cb_UpdateGpioValue_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Int(render, sensor_name, value, unit);
	g_display_memory.name = String(sensor_name);
	g_display_memory.unit = String(unit);
	g_display_memory.vint = value;
	g_display_memory.type = _SensorType::INTEGER;
}

void cb_UpdateI2cValueInt_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Int(render, sensor_name, value, unit);
	g_display_memory.name = String(sensor_name);
	g_display_memory.unit = String(unit);
	g_display_memory.vint = value;
	g_display_memory.type = _SensorType::INTEGER;
}

void cb_UpdateI2cValueDouble_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Double(render, sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vdouble = value;
	g_display_memory.type    = _SensorType::DOUBLE;
}

void cb_UpdateI2cValueString_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_String(render, sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vstring = String(value);
	g_display_memory.type    = _SensorType::STRING;
}

void cb_UpdateI2cValueError_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Error(render, sensor_name);
	g_display_memory.name = String(sensor_name);
	g_display_memory.type = _SensorType::ERROR;
}

void cb_UpdateModbusValueInt_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, int32_t value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Int(render, sensor_name, value, unit);
	g_display_memory.name = String(sensor_name);
	g_display_memory.unit = String(unit);
	g_display_memory.vint = value;
	g_display_memory.type = _SensorType::INTEGER;
}

void cb_UpdateModbusValueDouble_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Double(render, sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vdouble = value;
	g_display_memory.type    = _SensorType::DOUBLE;
}

void cb_UpdateModbusValueString_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_String(render, sensor_name, value, unit);
	g_display_memory.name    = String(sensor_name);
	g_display_memory.unit    = String(unit);
	g_display_memory.vstring = String(value);
	g_display_memory.type    = _SensorType::STRING;
}

void cb_UpdateModbusValueError_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	if (selected == false) {
		return;
	}
	updateSensorDisplay_Error(render, sensor_name);
	g_display_memory.name = String(sensor_name);
	g_display_memory.type = _SensorType::ERROR;
}

void cb_SamplingGpioSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
	// HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore
	//  HAL.printf("Datetime: %s\n", _s_day);
	// HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
}

void cb_SamplingI2cSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
	// HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore
	//  HAL.printf("Datetime: %s\n", _s_day);
	// HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
}

void cb_SamplingModbusSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
	// HAL.Display.giveSemaphoreMutex(); // Give (release) mutex semaphore
	//  HAL.printf("Datetime: %s\n", _s_day);
	// HAL.Display.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
}

void cb_FtpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.ftp = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

#ifdef DISPLAY_INCLUDE_LORA
	render->setFontSize(18);
	render->setCursor(41, 30);
	HAL.Display.fillRect(2, 31, 46, 24, _panel_color);
#else
	render->setFontSize(22);
	render->setCursor(25 + 19, 35 - 3);
	HAL.Display.fillRect(2, 31, 50 - 4, 36, _panel_color);
#endif
	render->setFontColor(TFT_WHITE, _panel_color);
	render->rprintf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}

void cb_HttpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.http = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

#ifdef DISPLAY_INCLUDE_LORA
	render->setFontSize(18);
	render->setCursor(89, 30);
	HAL.Display.fillRect(50, 31, 46, 24, _panel_color);
#else
	render->setFontSize(22);
	render->setCursor(25 + 19, 35 + 38 - 3);
	HAL.Display.fillRect(2, 31 + 38, 50 - 4, 36, _panel_color);
#endif
	render->setFontColor(TFT_WHITE, _panel_color);
	render->rprintf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}

void cb_MqttBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.mqtt = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

#ifdef DISPLAY_INCLUDE_LORA
	render->setFontSize(18);
	render->setCursor(41, 55);
	HAL.Display.fillRect(2, 56, 46, 24, _panel_color);
#else
	render->setFontSize(22);
	render->setCursor(25 + 49 + 19, 35 - 3);
	HAL.Display.fillRect(50, 31, 50 - 4, 36, _panel_color);
#endif
	render->setFontColor(TFT_WHITE, _panel_color);
	render->rprintf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}

void cb_AmbientBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.ambient = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

#ifdef DISPLAY_INCLUDE_LORA
	render->setFontSize(18);
	render->setCursor(89, 55);
	HAL.Display.fillRect(50, 56, 46, 24, _panel_color);
#else
	render->setFontSize(22);
	render->setCursor(25 + 49 + 19, 35 + 38 - 3);
	HAL.Display.fillRect(50, 31 + 38, 50 - 4, 36, _panel_color);
#endif
	render->setFontColor(TFT_WHITE, _panel_color);
	render->rprintf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}

#ifdef DISPLAY_INCLUDE_LORA
void cb_LoraBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.lora_wan = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.lora_wan ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	render->setFontSize(18);
	render->setCursor(41, 80);
	HAL.Display.fillRect(2, 81, 46, 24, _panel_color);
	render->setFontColor(TFT_WHITE, _panel_color);
	render->rprintf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}
#endif

void cb_ReadMqttStatus_Default(std::unique_ptr<OpenFontRender> &render, int status) {
	static const char *const MQTT_STATUS_MESSAGES[] = {
	    "Connection Timeout.",              // -4
	    "Connection Lost.",                 // -3
	    "Connect Failed.",                  // -2
	    "Disconnected.",                    // -1
	    "Connected!",                       //  0
	    "Server Unsupported MQTT version.", // 1
	    "Bad Client-ID.",                   //  2
	    "Server Unavailable.",              //  3
	    "Bad Credential. (User/Pass)",      //  4
	    "Unauthorized.",                    //  5
	};

	static int count = 0;
	if (count > 10) {
		count           = 0;
		int index       = status + 4;
		const char *msg = (index >= 0 and index <= 9)
		                      ? MQTT_STATUS_MESSAGES[index]
		                      : "Unknown Error.";
		HAL.print("MQTT status -> ");
		HAL.println(msg);
	}
	count++;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Const global variables for icon image
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

const uint16_t BluetoothIconWidth         = 49;
const uint16_t BluetoothIconHeight        = 49;
const uint16_t BluetoothIcon_RLE_RunCount = 455;

// Icon: Bluetooth icon (C) Terence Eden
//       https://icon-icons.com/icon/bluetooth/130975
// License: CC BY 4.0
// clang-format off
const uint16_t BluetoothIcon_Palette[5] PROGMEM = {
    0x041F, 0x44FF, 0x7DFF, 0xF7DF, 0xC71F,
};

// パレット化・行RLE圧縮アイコンデータ。
// Palette: 使用色一覧(RGB565)。RLE_Packed: ニブルパッキングされたラン列。
// フォーマット詳細は drawRLEIcon() のコメント参照。ロスレス(元PNG基準)。
const uint8_t BluetoothIcon_RLE_Packed[518] PROGMEM = {
    0x80, 0x31, 0x85, 0x00, 0x26, 0x86, 0x83, 0x00, 0x2A, 0x84, 0x82, 0x00, 0x2C, 0x83, 0x82, 0x00, 0x14, 0x11, 0x00, 0x18,
    0x82, 0x81, 0x00, 0x15, 0x22, 0x00, 0x17, 0x82, 0x81, 0x00, 0x15, 0x21, 0x31, 0x21, 0x00, 0x16, 0x82, 0x81, 0x00, 0x15,
    0x21, 0x32, 0x21, 0x00, 0x15, 0x82, 0x81, 0x00, 0x15, 0x21, 0x33, 0x21, 0x00, 0x14, 0x82, 0x81, 0x00, 0x15, 0x21, 0x34,
    0x21, 0x00, 0x13, 0x82, 0x81, 0x00, 0x15, 0x21, 0x35, 0x21, 0x00, 0x12, 0x82, 0x81, 0x00, 0x15, 0x21, 0x36, 0x21, 0x00,
    0x11, 0x82, 0x81, 0x00, 0x15, 0x21, 0x32, 0x42, 0x32, 0x41, 0x11, 0x00, 0x10, 0x82, 0x81, 0x00, 0x15, 0x21, 0x32, 0x21,
    0x11, 0x41, 0x32, 0x41, 0x11, 0x0F, 0x82, 0x81, 0x0D, 0x11, 0x41, 0x21, 0x05, 0x21, 0x32, 0x21, 0x01, 0x11, 0x41, 0x32,
    0x41, 0x11, 0x0E, 0x82, 0x81, 0x0C, 0x11, 0x41, 0x32, 0x21, 0x04, 0x21, 0x32, 0x21, 0x02, 0x11, 0x41, 0x32, 0x41, 0x11,
    0x0D, 0x82, 0x81, 0x0C, 0x11, 0x41, 0x33, 0x21, 0x03, 0x21, 0x32, 0x21, 0x03, 0x21, 0x33, 0x21, 0x0D, 0x82, 0x81, 0x0D,
    0x11, 0x41, 0x33, 0x21, 0x02, 0x21, 0x32, 0x21, 0x02, 0x11, 0x41, 0x32, 0x41, 0x11, 0x0D, 0x82, 0x81, 0x0E, 0x11, 0x41,
    0x33, 0x21, 0x01, 0x21, 0x32, 0x21, 0x01, 0x11, 0x41, 0x32, 0x41, 0x11, 0x0E, 0x82, 0x81, 0x0F, 0x11, 0x41, 0x33, 0x22,
    0x32, 0x21, 0x11, 0x41, 0x32, 0x41, 0x11, 0x0F, 0x82, 0x81, 0x00, 0x10, 0x11, 0x41, 0x33, 0x41, 0x32, 0x42, 0x32, 0x41,
    0x11, 0x00, 0x10, 0x82, 0x81, 0x00, 0x11, 0x11, 0x41, 0x38, 0x41, 0x11, 0x00, 0x11, 0x82, 0x81, 0x00, 0x12, 0x11, 0x41,
    0x36, 0x41, 0x11, 0x00, 0x12, 0x82, 0x81, 0x00, 0x13, 0x11, 0x41, 0x34, 0x41, 0x11, 0x00, 0x13, 0x82, 0x81, 0x00, 0x14,
    0x21, 0x34, 0x21, 0x00, 0x14, 0x82, 0x81, 0x00, 0x13, 0x11, 0x41, 0x35, 0x21, 0x00, 0x13, 0x82, 0x81, 0x00, 0x12, 0x11,
    0x41, 0x37, 0x21, 0x00, 0x12, 0x82, 0x81, 0x00, 0x11, 0x11, 0x41, 0x39, 0x21, 0x00, 0x11, 0x82, 0x81, 0x00, 0x10, 0x11,
    0x41, 0x32, 0x42, 0x32, 0x42, 0x33, 0x11, 0x00, 0x10, 0x82, 0x81, 0x0F, 0x11, 0x41, 0x32, 0x41, 0x11, 0x21, 0x32, 0x21,
    0x11, 0x41, 0x33, 0x11, 0x0F, 0x82, 0x81, 0x0E, 0x11, 0x41, 0x32, 0x41, 0x11, 0x01, 0x21, 0x32, 0x21, 0x01, 0x11, 0x41,
    0x33, 0x21, 0x0E, 0x82, 0x81, 0x0D, 0x11, 0x41, 0x32, 0x41, 0x11, 0x02, 0x21, 0x32, 0x21, 0x02, 0x11, 0x41, 0x32, 0x41,
    0x11, 0x0D, 0x82, 0x81, 0x0C, 0x11, 0x41, 0x32, 0x41, 0x11, 0x03, 0x21, 0x32, 0x21, 0x03, 0x21, 0x33, 0x21, 0x0D, 0x82,
    0x81, 0x0D, 0x41, 0x31, 0x41, 0x11, 0x04, 0x21, 0x32, 0x21, 0x02, 0x21, 0x33, 0x41, 0x11, 0x0D, 0x82, 0x81, 0x0D, 0x11,
    0x21, 0x11, 0x05, 0x21, 0x32, 0x21, 0x01, 0x21, 0x33, 0x41, 0x11, 0x0E, 0x82, 0x81, 0x00, 0x15, 0x21, 0x32, 0x22, 0x33,
    0x41, 0x11, 0x0F, 0x82, 0x81, 0x00, 0x15, 0x21, 0x32, 0x41, 0x33, 0x41, 0x11, 0x00, 0x10, 0x82, 0x81, 0x00, 0x15, 0x21,
    0x35, 0x41, 0x11, 0x00, 0x11, 0x82, 0x81, 0x00, 0x15, 0x21, 0x34, 0x41, 0x11, 0x00, 0x12, 0x82, 0x81, 0x00, 0x15, 0x21,
    0x33, 0x41, 0x11, 0x00, 0x13, 0x82, 0x81, 0x00, 0x15, 0x21, 0x32, 0x41, 0x11, 0x00, 0x14, 0x82, 0x81, 0x00, 0x15, 0x21,
    0x31, 0x41, 0x11, 0x00, 0x15, 0x82, 0x81, 0x00, 0x15, 0x21, 0x41, 0x11, 0x00, 0x16, 0x82, 0x82, 0x00, 0x14, 0x12, 0x00,
    0x17, 0x82, 0x82, 0x00, 0x2C, 0x83, 0x83, 0x00, 0x2A, 0x84, 0x84, 0x00, 0x28, 0x85, 0x80, 0x31, 0x80, 0x31,
};
// clang-format on

const uint16_t DynamicspidIconWidth         = 49;
const uint16_t DynamicspidIconHeight        = 49;
const uint16_t DynamicspidIcon_RLE_RunCount = 566;

// clang-format off
const uint16_t DynamicspidIcon_Palette[5] PROGMEM = {
    0xE640, 0xB4E2, 0xD5C1, 0x39E7, 0x6B25,
};
 
// パレット化・行RLE圧縮アイコンデータ。
// Palette: 使用色一覧(RGB565)。RLE_Packed: ニブルパッキングされたラン列。
// フォーマット詳細は drawRLEIcon() のコメント参照。ロスレス(元PNG基準)。
const uint8_t DynamicspidIcon_RLE_Packed[601] PROGMEM = {
    0x80, 0x31, 0x80, 0x31, 0x85, 0x00, 0x27, 0x85, 0x83, 0x00, 0x2B, 0x83, 0x83, 0x02, 0x10, 0x26, 0x21, 0x02, 0x83, 0x82,
    0x02, 0x11, 0x30, 0x26, 0x41, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30,
    0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83,
    0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38,
    0x41, 0x14, 0x42, 0x30, 0x18, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x11, 0x06, 0x21, 0x11, 0x41, 0x30, 0x15, 0x21,
    0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x41, 0x13, 0x22, 0x04, 0x11, 0x41, 0x30, 0x13, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11,
    0x3D, 0x42, 0x11, 0x21, 0x03, 0x11, 0x41, 0x30, 0x11, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x11, 0x41, 0x11, 0x02,
    0x21, 0x11, 0x30, 0x10, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x13, 0x41, 0x21, 0x02, 0x11, 0x3F, 0x21, 0x01, 0x83,
    0x82, 0x02, 0x11, 0x30, 0x14, 0x41, 0x11, 0x02, 0x11, 0x3E, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x41, 0x13, 0x42,
    0x38, 0x11, 0x02, 0x11, 0x3D, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x11, 0x04, 0x21, 0x11, 0x41, 0x37, 0x11, 0x02,
    0x11, 0x3C, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x41, 0x12, 0x22, 0x03, 0x21, 0x41, 0x36, 0x11, 0x02, 0x41, 0x3B,
    0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x3C, 0x41, 0x11, 0x21, 0x03, 0x11, 0x36, 0x11, 0x01, 0x21, 0x41, 0x3A, 0x21, 0x01,
    0x83, 0x82, 0x02, 0x11, 0x3F, 0x41, 0x21, 0x02, 0x11, 0x35, 0x41, 0x21, 0x01, 0x11, 0x3A, 0x21, 0x01, 0x83, 0x82, 0x02,
    0x11, 0x30, 0x10, 0x41, 0x11, 0x02, 0x11, 0x35, 0x11, 0x02, 0x41, 0x39, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x12,
    0x11, 0x02, 0x11, 0x35, 0x21, 0x01, 0x11, 0x39, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x41, 0x13, 0x41, 0x36, 0x11,
    0x02, 0x41, 0x34, 0x11, 0x01, 0x21, 0x41, 0x38, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x11, 0x03, 0x21, 0x11, 0x41,
    0x34, 0x41, 0x21, 0x01, 0x11, 0x34, 0x41, 0x21, 0x01, 0x11, 0x38, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x38, 0x41, 0x12,
    0x21, 0x02, 0x21, 0x41, 0x34, 0x11, 0x02, 0x41, 0x34, 0x11, 0x01, 0x11, 0x38, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x3B,
    0x41, 0x11, 0x21, 0x01, 0x21, 0x41, 0x33, 0x41, 0x21, 0x01, 0x11, 0x34, 0x41, 0x01, 0x21, 0x41, 0x37, 0x21, 0x01, 0x83,
    0x82, 0x02, 0x11, 0x3D, 0x41, 0x02, 0x11, 0x34, 0x11, 0x01, 0x21, 0x41, 0x33, 0x41, 0x02, 0x41, 0x37, 0x21, 0x01, 0x83,
    0x82, 0x02, 0x11, 0x3E, 0x11, 0x01, 0x21, 0x41, 0x33, 0x41, 0x02, 0x41, 0x33, 0x41, 0x21, 0x01, 0x41, 0x37, 0x21, 0x01,
    0x83, 0x82, 0x02, 0x11, 0x37, 0x43, 0x34, 0x41, 0x21, 0x01, 0x41, 0x33, 0x41, 0x02, 0x41, 0x34, 0x21, 0x01, 0x11, 0x37,
    0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x36, 0x41, 0x21, 0x02, 0x11, 0x34, 0x21, 0x01, 0x11, 0x33, 0x41, 0x21, 0x01, 0x11,
    0x34, 0x21, 0x01, 0x11, 0x37, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x36, 0x11, 0x03, 0x21, 0x41, 0x33, 0x11, 0x01, 0x11,
    0x34, 0x21, 0x01, 0x41, 0x34, 0x11, 0x01, 0x11, 0x37, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x36, 0x11, 0x03, 0x21, 0x41,
    0x33, 0x42, 0x35, 0x41, 0x11, 0x35, 0x41, 0x11, 0x41, 0x37, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x36, 0x41, 0x21, 0x02,
    0x11, 0x30, 0x1C, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x37, 0x43, 0x30, 0x1D, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30,
    0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83,
    0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x11, 0x30, 0x27, 0x21, 0x01, 0x83, 0x82, 0x02, 0x21, 0x41,
    0x30, 0x25, 0x41, 0x02, 0x83, 0x83, 0x02, 0x20, 0x26, 0x03, 0x83, 0x83, 0x00, 0x2A, 0x84, 0x80, 0x31, 0x80, 0x31, 0x80,
    0x31,
};
// clang-format on

#endif /* DISPLAY_320X240 */
#endif /* USER_DISPLAY_DEFAULT_H */
