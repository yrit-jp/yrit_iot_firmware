#include "user_display_default.h"

#ifdef USER_DISPLAY_DEFAULT_H
#ifdef DISPLAY_128X128

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
};
struct _Features g_features = {false, false, false, false};

struct _RemainData {
	int ftp;
	int http;
	int mqtt;
	int ambient;
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
		HAL.Display.fillRect(0, 41, HAL.Display.width(), 16, TFT_BLACK);
		render->setAlignment(Align::TopLeft);
		render->setFontSize(10);
		render->setCursor(0, 43);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		render->printf("[%d] ", g_current_sensor_number);

		uint8_t lines = 1;
		int i         = 0;
		while (sensor_name[i] != '\0') {
			render->printf("%c", sensor_name[i]);
			if (render->getCursorX() > 115) {
				render->printf("...");
				lines++;
			}
			i++;

			if (lines > 1) {
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
		render->setAlignment(Align::BottomRight);
		// Update sensor unit
		render->setFontSize(12);
		// Clear prev unit
		// render->setCursor(HAL.Display.width(), 93);
		// render->setFontColor(TFT_BLACK, TFT_BLACK);
		// render->printf("%s", _last_unit.c_str());

		HAL.Display.fillRect(0, 83, HAL.Display.width(), 15, TFT_BLACK);
		// Draw new unit
		render->setCursor(HAL.Display.width(), 93);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		render->printf("%s", sensor_unit);

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
		render->setAlignment(Align::TopLeft);
		render->setFontSize(11);
		render->setCursor(0, 2);
		render->setFontColor(MY_DARKGRAY, MY_DARKGRAY);
		render->printf("更新 : %s", _last_s_day.c_str());
		render->setCursor(0, 2);
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
		render->setAlignment(Align::TopLeft);
		render->setFontSize(11);
		render->setCursor(0, 2);
		render->setFontColor(MY_DARKGRAY, MY_DARKGRAY);
		render->printf("%s", _last_ip_addr.c_str());
		render->setCursor(0, 2);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		render->printf("%s", ip_addr.c_str());
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex();

		_last_ip_addr = ip_addr;
	}
}

void drawLeftPanelFrame() {
	uint16_t _panel_color = 0;
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Left panels
	HAL.Display.fillRect(0, 20, 128, 21, TFT_LIGHTGREY);
	// FTP
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(3, 22, 29, 17, _panel_color);
	// HTTP
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(34, 22, 29, 17, _panel_color);
	// MQTT
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(65, 22, 29, 17, _panel_color);
	// Ambient
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(96, 22, 29, 17, _panel_color);

	HAL.Display.giveSemaphoreMutex();
}

void drawFeatures2LeftPanel(std::unique_ptr<OpenFontRender> &render) {
	uint16_t _panel_color = 0;
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
	render->setAlignment(Align::MiddleCenter);

	// FTP
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(13);
	render->setCursor(17, 27);
	render->printf("FTP");
	// HTTP
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(10);
	render->setCursor(34 + 14, 27);
	render->printf("HTTP");
	// MQTT
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(10);
	render->setCursor(65 + 14, 28);
	render->printf("MQTT");
	// Ambient
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(12);
	render->setCursor(96 + 14, 26);
	render->printf("AMB");

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
		Icon_WiFiStatus(102, 16, _rssi);
		HAL.Display.giveSemaphoreMutex();

		break;
	}
	case DATALINK_LAYER::ETHERNET: {
		// _isConnected = HAL.Lan.isLinkUp();
		// _ip          = HAL.Lan.localIP();

		// HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// Icon_EthernetStatus(260, 5, _isConnected);
		// HAL.Display.giveSemaphoreMutex();

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
	struct _Rect rect     = {0, 21 + 8, HAL.Display.width(), 15};
	uint16_t x_water_mark = 0;

	std::vector<String> items = {
	    "種別",
	    "FW",
	    "送信",
	    "Wi-Fi",
	    // "FTPサーバ",
	    // "HTTPサーバ",
	    // "MQTTサーバ",
	};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// draw header
	render->setAlignment(Align::TopLeft);
	render->setFontSize(11);
	render->setCursor(0, 2);
	render->setFontColor(TFT_ORANGE, MY_DARKGRAY);
	render->printf("各種情報 1");

	// // draw title
	// render->setFontSize(20);
	// render->setCursor(10, 30);
	// render->setFontColor(TFT_ORANGE);
	// render->printf("各種情報");
	// render->setFontColor(TFT_WHITE);

	// draw items
	render->setAlignment(Align::TopLeft);
	render->setFontSize(12);
	for (int i = 0; i < items.size(); i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = (rect.height * i);

		HAL.Display.fillRect(rect.x, (rect.y + y_offset), rect.width, rect.height, bg); // draw table cell
		render->setFontColor(MY_DARKGRAY, bg);
		render->setCursor((rect.x + 2), (rect.y + y_offset - 1));
		render->printf(items[i].c_str());

		x_water_mark = (x_water_mark < render->getCursorX()) ? render->getCursorX() : x_water_mark;
	}

	// draw separate
	constexpr uint8_t LINE_WIDTH = 2;
	for (int i = 0; i < LINE_WIDTH; i++) {
		constexpr uint8_t OFFSET = 3;
		HAL.Display.drawFastVLine((x_water_mark + OFFSET + i), rect.y,
		                          rect.height * items.size(), TFT_DARKGREY);
	}

	// draw value
	const uint16_t VALUE_CELL_WIDTH = rect.width - x_water_mark;
	for (int i = 0; i < items.size(); i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = (rect.height * i) - 1;
		uint32_t x_offset = 10;

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
			// uint16_t h = HAL.Info.TX_INTERVAL_SEC / 3600;
			uint8_t m = (HAL.Info.TX_INTERVAL_SEC % 3600) / 60;
			// info       = String(h) + "時間 ";
			info = String(m) + "分 ";
			info += String(HAL.Info.TX_INTERVAL_SEC % 60) + "秒  毎";
			break;
		}
		case 3:
			info = HAL.Info.WIFI_SSID;
			break;
		case 4:
			info = HAL.Info.FTP_SERVER;
			break;
		case 5:
			info = HAL.Info.HTTP_SERVER;
			break;
		case 6:
			info = HAL.Info.MQTT_SERVER;
			break;
		default:
			break;
		}

		if (render->getTextWidth(info.c_str()) < VALUE_CELL_WIDTH) {
			// short strings
			render->setCursor((x_water_mark + x_offset), (rect.y) + y_offset);
			render->printf(info.c_str());
		} else {
			uint32_t size = 14;
			while (render->getTextWidth(info.c_str()) > VALUE_CELL_WIDTH and size > 1) {
				size--;
				render->setFontSize(size);
			}
			render->setCursor((x_water_mark + x_offset), (rect.y) + y_offset);
			render->printf(info.c_str());
		}
	}

	render->setFontColor(TFT_WHITE, TFT_BLACK);
	HAL.Display.giveSemaphoreMutex();
}

void drawInformation2(std::unique_ptr<OpenFontRender> &render) {
	constexpr struct _Rect rect    = {0, 21 + 8, 128, 15};
	constexpr uint8_t FONT_SIZE    = 12;
	constexpr uint8_t LABEL_COUNT  = 3;
	constexpr uint8_t SEP_GAP      = 3;
	constexpr uint8_t SEP_WIDTH    = 2;
	constexpr uint8_t VALUE_OFFSET = SEP_GAP + SEP_WIDTH + 5;
	const char *const labels[]     = {"FTP", "HTTP", "MQTT"};

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

	// draw header
	render->setAlignment(Align::TopLeft);
	render->setFontSize(11);
	render->setCursor(0, 2);
	render->setFontColor(TFT_ORANGE, MY_DARKGRAY);
	render->printf("各種情報 2");

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
	constexpr struct _Rect rect    = {0, 21 + 8, 128, 15};
	constexpr uint8_t FONT_SIZE    = 12;
	constexpr uint8_t LABEL_COUNT  = 4;
	constexpr uint8_t SEP_GAP      = 3;
	constexpr uint8_t SEP_WIDTH    = 2;
	constexpr uint8_t VALUE_OFFSET = SEP_GAP + SEP_WIDTH + 5;
	const char *const labels[]     = {"ID", "起動", "Heap", "Min"};

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

		// draw header
		render->setAlignment(Align::TopLeft);
		render->setFontSize(11);
		render->setCursor(0, 2);
		render->setFontColor(TFT_ORANGE, MY_DARKGRAY);
		render->printf("各種情報 3");

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
			if (info.length() > 10) {
				info = info.substring(0, 10) + "...";
			}
			break;
		case 1: {
			unsigned long ms = millis();
			unsigned long s  = ms / 1000;
			unsigned long m  = s / 60;
			unsigned long h  = m / 60;
			char buf[32];
			snprintf(buf, sizeof(buf), "%luh %02lum %02lus", h, m % 60, s % 60);
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
		render->setFontSize(11);
		render->printf("Bluetooth LE 接続名\n");
		render->printf("%s", HAL.getChipUUID().c_str());

		HAL.Display.pushImage(8, IMG_H,
		                      BluetoothIconWidth, BluetoothIconHeight,
		                      (uint16_t *)BluetoothIcon);
		HAL.Display.fillRect(144 - 83, 75 - 16, 5, 21, TFT_WHITE);
		HAL.Display.fillRect(136 - 83, 83 - 16, 21, 5, TFT_WHITE);
		HAL.Display.pushImage(80, IMG_H,
		                      DynamicspidIconWidth, DynamicspidIconHeight,
		                      (uint16_t *)DynamicspidIcon);

		render->setFontColor(TFT_YELLOW, TFT_BLACK);

		render->setFontSize(12);
		render->setCursor(128 / 2, 108);
		render->cprintf("短押で表示切替");

		render->setFontColor(TFT_WHITE, TFT_BLACK);
		break;
	}
	case CONFIG_PAGE_2: {
		HAL.Display.fillRect(0, 0, HAL.Display.width(), HAL.Display.height(), TFT_BLACK);

		render->setCursor(3, 3);
		render->setFontSize(11);
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

		render->setFontSize(12);
		render->setCursor(128 / 2, 108);
		render->cprintf("短押で表示切替");

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
	OpenFontRender::Cursor c = {115, 62};
	const struct _Rect limit = {0, 0, HAL.Display.width() - 10, 20};
	const struct _Rect clear = {0, 58, HAL.Display.width(), 25};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%d", value);

	render->setAlignment(Align::MiddleRight);
	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y);
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->printf("%d", value);

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, unit);
}

void updateSensorDisplay_Double(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, double value, const char *unit) {
	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	drawSensorName(render, sensor_name);

	// Update sensor value
	OpenFontRender::Cursor c = {115, 62};
	const struct _Rect limit = {0, 0, HAL.Display.width() - 10, 20};
	const struct _Rect clear = {0, 58, HAL.Display.width(), 25};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%.1f", value);

	render->setAlignment(Align::MiddleRight);
	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y);
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->printf("%.1f", value);

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, unit);
}

void updateSensorDisplay_String(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, const char *value, const char *unit) {
	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	drawSensorName(render, sensor_name);

	// Update sensor value
	OpenFontRender::Cursor c = {115, 62};
	const struct _Rect limit = {0, 0, HAL.Display.width() - 10, 20};
	const struct _Rect clear = {0, 58, HAL.Display.width(), 25};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%s", value);

	render->setAlignment(Align::MiddleRight);
	render->setFontSize(fit_size);
	render->setCursor(c.x, c.y);
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->printf("%s", value);

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
	OpenFontRender::Cursor c = {115, 62};
	const struct _Rect limit = {0, 0, HAL.Display.width() - 10, 20};
	const struct _Rect clear = {0, 58, HAL.Display.width(), 25};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSize(limit.width, limit.height, Layout::Horizontal, str);

	render->setAlignment(Align::MiddleCenter);
	render->setFontSize(fit_size);
	render->setCursor(HAL.Display.width() / 2, c.y);
	HAL.Display.fillRect(clear.x, clear.y, clear.width, clear.height, TFT_BLACK);
	render->setFontColor(TFT_YELLOW);
	render->printf(str);
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
		// 	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	Icon_TfCardStatus(295, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING), 0x0000);
		// 	HAL.Display.giveSemaphoreMutex();
		// }
		break;
	}
	case DisplayTiming::SS_READY: {
		drawConfigPage(render, CONFIG_PAGE_1);

		// Switch to config mode
		g_display_page = CONFIG_PAGE_1;
		break;
	}
	case DisplayTiming::SS_LOOP: {
		if (not HAL.BtnA_wasPressed()) {
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
		// Icon_TfCardStatus(300, 4, TfCardStatusT::UNKNOWN, 0xFFFF); // Reset status
		HAL.Display.fillScreen(TFT_BLACK);
		HAL.Display.fillRect(0, 0, HAL.Display.width(), 20, MY_DARKGRAY); // Top bar
		// Icon_BatteryStatus(290, 5, HAL.getBatteryLevel());
		HAL.Display.giveSemaphoreMutex();

		drawNetworkStatus(render);

		struct tm timeinfo = {0};
		drawLastUpdateDate(render, timeinfo);

		// Draw Left panel
		drawLeftPanelFrame();
		drawFeatures2LeftPanel(render);

		if (g_sensor_num == 0) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			render->setAlignment(Align::MiddleCenter);
			render->setFontSize(20);
			render->setCursor(HAL.Display.width() / 2, 73); // set right cursor
			render->setFontColor(TFT_WHITE, TFT_BLACK);
			render->printf("No Sensors");
			HAL.Display.giveSemaphoreMutex();
		}

		// Bottom bar
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setAlignment(Align::MiddleCenter);
		HAL.Display.fillRect(0, 98, HAL.Display.width(), 30, MY_DARKGRAY);
		render->setFontSize(12);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		render->setCursor(HAL.Display.width() / 2, 105);
		render->printf("短押 / センサ切替");
		render->setCursor(HAL.Display.width() / 2, 120);
		render->printf("長押 / 表示切替");
		HAL.Display.giveSemaphoreMutex();

		// if (HAL.Log.getCurrentLoggingState()) {
		// 	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	render->setCursor(130, 213);
		// 	render->printf("SD切替");
		// 	HAL.Display.giveSemaphoreMutex();
		// }

		// if (not HAL.Tf.ready()) {
		// 	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	Icon_TfCardStatus(240, 4, TfCardStatusT::UNMOUNTED);
		// 	HAL.Display.giveSemaphoreMutex();
		// } else {
		// 	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	Icon_TfCardStatus(240, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING));
		// 	HAL.Display.giveSemaphoreMutex();
		// }

		break;
	}
	case DisplayTiming::EVERY_ONE_SECONDS_EVENT: {
		if (heartbeat_circle_state) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillEllipse(120, 10, 3, 3, TFT_GREENYELLOW);
			HAL.Display.giveSemaphoreMutex();
		} else {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillEllipse(120, 10, 3, 3, MY_DARKGRAY);
			HAL.Display.giveSemaphoreMutex();
		}

		heartbeat_circle_state = not heartbeat_circle_state;

		if (g_display_page == MAIN_PAGE_INFO_3) {
			drawInformation3(render, true);
		}

		// WLAN manual disconnect option
		// if (HAL.BtnA_isPressed()) {
		// 	btnA_pressed_counter++;
		// 	if (btnA_pressed_counter == 8) {
		// 		HAL.Wlan.disconnectWlan(false, false);
		// 		HAL.println("[WARN] WLAN manual disconnected OK!");

		// 		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 		HAL.Display.fillEllipse(285, 40, 5, 5, TFT_YELLOW);
		// 		HAL.Display.giveSemaphoreMutex();
		// 	}
		// } else {
		// 	if (btnA_pressed_counter >= 8) {
		// 		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 		HAL.Display.fillEllipse(285, 40, 5, 5, TFT_BLACK);
		// 		HAL.Display.giveSemaphoreMutex();

		// 		drawNetworkStatus(render);
		// 	}
		// 	btnA_pressed_counter = 0;
		// }

		break;
	}
	case DisplayTiming::EVERY_HALF_MINUTES_EVENT: {
		// Update battery status
		// HAL.printf("Battery Level: %d %%\n", HAL.getBatteryLevel());

		// HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// Icon_BatteryStatus(290, 5, HAL.getBatteryLevel());
		// HAL.Display.giveSemaphoreMutex();

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
		// 	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	Icon_TfCardStatus(240, 4, TfCardStatusT::UNMOUNTED);
		// 	HAL.Display.giveSemaphoreMutex();
		// } else {
		// 	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		// 	Icon_TfCardStatus(240, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING));
		// 	HAL.Display.giveSemaphoreMutex();
		// }

		break;
	}
	case DisplayTiming::BTNC_PRESSED: {
		g_display_page = ((g_display_page + 1 == _MAIN_PAGE_SENTINEL) ? MAIN_PAGE_SENSOR_1 : static_cast<_DisplayPage>(g_display_page + 1));

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(0, 0, 92, 20, MY_DARKGRAY); // Clear top bar
		HAL.Display.giveSemaphoreMutex();

		const bool FORCE_UPDATE = true;

		switch (g_display_page) {
		case MAIN_PAGE_SENSOR_1: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 51, HAL.Display.width(), 47, TFT_BLACK); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawLeftPanelFrame();
			drawLastUpdateDate(render, g_last_timeinfo, FORCE_UPDATE);
			drawFeatures2LeftPanel(render);

			if (g_sensor_num == 0) {
				HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
				HAL.Display.fillRect(0, 41, HAL.Display.width(), 47, TFT_BLACK); // Clear center area
				render->setAlignment(Align::MiddleCenter);
				render->setFontSize(20);
				render->setCursor(HAL.Display.width() / 2, 73); // set right cursor
				render->setFontColor(TFT_WHITE, TFT_BLACK);
				render->printf("No Sensors");
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

			break;
		}
		case MAIN_PAGE_INFO_1: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 20, HAL.Display.width(), 78, TFT_ORANGE);
			HAL.Display.giveSemaphoreMutex();

			drawInformation(render);

			break;
		}
		case MAIN_PAGE_INFO_2: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 20, HAL.Display.width(), 78, TFT_ORANGE);
			HAL.Display.giveSemaphoreMutex();

			drawInformation2(render);

			break;
		}
		case MAIN_PAGE_INFO_3: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 20, HAL.Display.width(), 78, TFT_ORANGE);
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
	render->setAlignment(Align::MiddleCenter);

	if (_last_sec < 0) {
		render->setFontSize(13);
		render->setCursor(HAL.Display.width() / 2, 15);
		render->printf("Waiting 00 sec");
		fillArc(HAL.Display.width() / 2, 75, 30, 45, -90 + (6 * current_sec), 270, TFT_ORANGE);
	}
	// clear last value
	render->setFontColor(TFT_BLACK);
	render->setFontSize(25);
	render->setCursor(HAL.Display.width() / 2, 66);
	render->printf("%02d", 60 - current_sec + 1);

	render->setFontColor(TFT_WHITE);
	render->setCursor(HAL.Display.width() / 2, 66);
	render->printf("%02d", 60 - current_sec);

	fillArc(HAL.Display.width() / 2, 75, 30, 45, -90, -90 + (6 * current_sec), TFT_BLACK);
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
	//
	//  HAL.printf("Datetime: %s\n", _s_day);
	//
}

void cb_SamplingI2cSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
	//
	//  HAL.printf("Datetime: %s\n", _s_day);
	//
}

void cb_SamplingModbusSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
	//
	//  HAL.printf("Datetime: %s\n", _s_day);
	//
}

void cb_FtpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.ftp = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	render->setAlignment(Align::MiddleCenter);
	render->setFontSize(14);
	render->setCursor(17, 25);
	HAL.Display.fillRect(3, 22, 29, 17, _panel_color);
	render->setFontColor(TFT_WHITE, _panel_color);
	render->printf("%d", remain_data_quantity);
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

	render->setAlignment(Align::MiddleCenter);
	render->setFontSize(14);
	render->setCursor(34 + 14, 25);
	HAL.Display.fillRect(34, 22, 29, 17, _panel_color);
	render->setFontColor(TFT_WHITE, _panel_color);
	render->printf("%d", remain_data_quantity);
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

	render->setAlignment(Align::MiddleCenter);
	render->setFontSize(14);
	render->setCursor(65 + 14, 25);
	HAL.Display.fillRect(65, 22, 29, 17, _panel_color);
	render->setFontColor(TFT_WHITE, _panel_color);
	render->printf("%d", remain_data_quantity);
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

	render->setAlignment(Align::MiddleCenter);
	render->setFontSize(14);
	render->setCursor(96 + 14, 25);
	HAL.Display.fillRect(96, 22, 29, 17, _panel_color);
	render->setFontColor(TFT_WHITE, _panel_color);
	render->printf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}

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

const uint16_t BluetoothIconWidth  = 40;
const uint16_t BluetoothIconHeight = 40;

// Icon: Bluetooth icon (C) Terence Eden
//       https://icon-icons.com/icon/bluetooth/130975
// License: CC BY 4.0
// Convert by: https://lang-ship.com/tools/image2data/
// RGB565 Dump(little endian)
// clang-format off
const unsigned short BluetoothIcon[1600] PROGMEM = {
    0x0200, 0x0008, 0xE700, 0xD402, 0xBA03, 0xFE03, 0xFF03, 0x1E04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFE03, 0x3F04, 0xFD0B, 0x1F04, 0x1F04, 0xFE0B, 0xFF03, 0x1F0C, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1E04, 0x1F04, 0x1F04, 0xDA03, 0xB402, 0xE708, 0x0008, 0x0200, // row 0, 40 pixels
    0x0000, 0xCC01, 0x7A03, 0xFF03, 0x3F04, 0x3F04, 0x3F04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x1F04, 0x3F04, 0x1F04, 0xDF03, 0x7E04, 0x3F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x3F04, 0x3F04, 0x1F04, 0x1F04, 0x9A03, 0xCC01, 0x0008, // row 1, 80 pixels
    0xA600, 0x9A03, 0xBF04, 0x5F04, 0x3F04, 0x1F04, 0x1F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xDF03, 0x1D04, 0x3F14, 0xFF0B, 0x1F04, 0x1E04, 0x7F04, 0xDF0B, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xDF03, 0x1F04, 0x1F04, 0x3F04, 0x5F04, 0xBF04, 0x9B03, 0xC608, // row 2, 120 pixels
    0xB502, 0x1F04, 0x3F04, 0x1B04, 0x1E0C, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x5F96, 0x1F65, 0xDD03, 0x3F04, 0x5F04, 0xFC0B, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x1F04, 0xFF03, 0x1E04, 0xFB03, 0x1F04, 0x1F04, 0xD502, // row 3, 160 pixels
    0xFB03, 0x1F0C, 0xDF03, 0x1E0C, 0x1E14, 0xFE03, 0x5F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0xFE0B, 0x5F9E, 0xDEEF, 0x9F54, 0xFF03, 0xBF03, 0x3E04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x5F04, 0xDE03, 0xFE13, 0x1E04, 0xFF03, 0x3F04, 0xDB03, // row 4, 200 pixels
    0xFE03, 0x1F04, 0x1F04, 0x3F04, 0xFF0B, 0x1E04, 0x5F04, 0xDF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3E04, 0xFF03, 0x3F86, 0xFFFF, 0xFFEF, 0x7F4C, 0x1F04, 0x5F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x5F04, 0xFE03, 0xDE0B, 0x3F04, 0x3F04, 0x1F04, 0xFE03, // row 5, 240 pixels
    0xFF03, 0x3F04, 0x5F04, 0x1F04, 0x1F04, 0x3F04, 0x3F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1E04, 0x5F04, 0x1F8E, 0xDFF7, 0xDFFF, 0x9FE7, 0x7F34, 0xFC03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x3F04, 0x5F04, 0x1F04, 0x1F04, 0x3F04, 0x3F04, 0x1F04, // row 6, 280 pixels
    0x1E04, 0x1F04, 0xFF0B, 0xDE03, 0x3F04, 0x1F04, 0xDE03, 0x3D04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xBE03, 0x1FA6, 0xDFFF, 0xFEFF, 0xDDFF, 0x9FE7, 0x3F14, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x5E04, 0xDE03, 0x1F04, 0x3F04, 0xFF03, 0x1F04, 0x3F04, 0xFE03, // row 7, 320 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFD0B, 0x1E04, 0xDF03, 0x1F04, 0x3F04, 0x3F04, 0x1D14, 0xFF03, 0x1F0C, 0x3F04, 0xFE95, 0xDEFF, 0xDFFF, 0xFFF7, 0xDFFF, 0x5FEF, 0x3F24, 0x3D0C, 0xFF03, 0x3E04, 0x1F04, 0x1E04, 0xFF13, 0x1E04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 8, 360 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x1F04, 0x5F04, 0xDE0B, 0x1F04, 0x5F0C, 0x1F04, 0x1E04, 0xFD03, 0x5F04, 0xFF9D, 0xFDF7, 0xBFF7, 0x7ED7, 0xFFFF, 0xFFF7, 0x5FD7, 0x1F1C, 0x3E04, 0x1F04, 0xFF03, 0x3F04, 0x3E04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 9, 400 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F0C, 0x5F04, 0xFF03, 0x3E04, 0xDF03, 0xBE03, 0x1F04, 0x3E04, 0x1F04, 0xFF03, 0x1F96, 0xFFFF, 0xFEFF, 0x5E1C, 0x7FD7, 0xDFFF, 0xFFF7, 0x9FC6, 0x5E04, 0x1F04, 0x3D04, 0xFE03, 0x3F04, 0xDF0B, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 10, 440 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xDD03, 0x5F0C, 0x1C04, 0xDE0B, 0x3FBF, 0xBE4C, 0x1F04, 0x3F04, 0x3E04, 0x3F0C, 0xFC95, 0xFFFF, 0xDFF7, 0x9E14, 0xFF33, 0x9FDF, 0xFFFF, 0xFEFF, 0xBFC6, 0xDF03, 0x5F0C, 0x1E04, 0xFF0B, 0x5F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 11, 480 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xFE03, 0x1F04, 0xBFAE, 0xDFFF, 0xBEF7, 0xDE4C, 0xDF03, 0x3E04, 0xDE03, 0xFE95, 0xFEF7, 0xFFF7, 0x5D34, 0x1F04, 0x5E1C, 0xBFDF, 0xFFFF, 0xDEFF, 0xDFB6, 0x9E03, 0xFF03, 0xFE03, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 12, 520 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xFF03, 0xFF03, 0xFFAE, 0xBCFF, 0xDFFF, 0xBFEF, 0xDF4C, 0x1F04, 0x3F04, 0x5F8E, 0xBFFF, 0xFEFF, 0x9F2C, 0x3E04, 0xFF03, 0xBD85, 0xFFFF, 0xDFFF, 0xFFEF, 0xBF44, 0x3F04, 0x1F0C, 0xFE03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 13, 560 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x1F04, 0xFE03, 0x9FAE, 0xFFFF, 0xBFFF, 0xDFF7, 0x3F44, 0x3D04, 0xFF9D, 0xBFFF, 0xFFFF, 0x5F1C, 0x1F04, 0x5F7D, 0xFFFF, 0xFFFF, 0xFFFF, 0x9F85, 0xFE03, 0xDF03, 0x3F04, 0x5F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 14, 600 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1E04, 0x5F04, 0xFD03, 0x1F04, 0x1F04, 0x7FB6, 0xFEFF, 0xFEFF, 0xFEFF, 0x7F4C, 0x3D96, 0xFFF7, 0xFDFF, 0x5F14, 0x3F7D, 0xDFFF, 0xFFFF, 0xFFFF, 0x7E7D, 0x1E04, 0x5E0C, 0x3F04, 0x3F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 15, 640 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x1E04, 0x1F04, 0x3F0C, 0x1F04, 0xDE03, 0xBF9E, 0xDDFF, 0xDFFF, 0xFFFF, 0x3F8E, 0xFEFF, 0xDFFF, 0x5E75, 0xFFFF, 0xFFFF, 0xBFFF, 0xDF75, 0xFF03, 0xDE03, 0x3F04, 0x1F04, 0x1D04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 16, 680 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x1F04, 0x3F04, 0xDF03, 0x3F04, 0x1F04, 0xFC1B, 0x7F96, 0xFDFF, 0xFFFF, 0xDFFF, 0xFFF7, 0xFFF7, 0xDFFF, 0xFFFF, 0xBEF7, 0x9F75, 0x1E14, 0xDE03, 0x5F0C, 0x1F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 17, 720 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xFE03, 0xFF03, 0x5F0C, 0x1E04, 0x3F04, 0xFF03, 0x5F14, 0xBFA6, 0xFFFF, 0xDFFF, 0xFFFF, 0xFEFF, 0xFFFF, 0xBEF7, 0x1F86, 0x1F0C, 0xFF03, 0x3F04, 0x3F0C, 0xFF03, 0xFF03, 0xFF03, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 18, 760 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1D04, 0x3F04, 0xFF03, 0x1F04, 0x1F0C, 0xFF03, 0xFE03, 0xDF03, 0x7FA6, 0xDFFF, 0xFFFF, 0xFFFF, 0xBFFF, 0xDE8D, 0xDF03, 0xDE03, 0x3F04, 0xFF0B, 0xFF03, 0x5F04, 0x5F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 19, 800 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x3E04, 0x1F04, 0xDF03, 0x5F0C, 0xDF03, 0xFF03, 0x5F04, 0xFF0B, 0x1F65, 0xDDFF, 0xFFFF, 0xFFFF, 0x9EEF, 0x9F4C, 0x1F04, 0x5F04, 0x3F04, 0xBE03, 0xFF03, 0x3F04, 0x1F04, 0x1F0C, 0xFE03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 20, 840 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xFE03, 0xDF03, 0x5F0C, 0x1E04, 0x1F04, 0x3F04, 0xFF03, 0xFF54, 0xFFFF, 0xFFFF, 0xFFF7, 0xFFF7, 0xFEFF, 0xBEEF, 0x7F44, 0xFF03, 0x3F04, 0x3F04, 0x1E04, 0x1F04, 0xFF03, 0x1F04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 21, 880 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x1F04, 0x3F0C, 0xDE03, 0xFE03, 0x9F04, 0xBD03, 0xFE5C, 0xFFFF, 0xBFFF, 0xFFF7, 0xFFFF, 0xFFFF, 0xFEF7, 0xDFFF, 0xFFF7, 0x7C44, 0xFF03, 0x5F04, 0x3F04, 0x1F04, 0x3F04, 0x1E04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 22, 920 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x5F04, 0x1E04, 0x1E0C, 0x3F04, 0xFF03, 0xFE5C, 0xFFFF, 0xDFFF, 0xFFFF, 0x7FCF, 0xBFFF, 0x9FFF, 0xDFB6, 0xFFFF, 0xFEFF, 0xFFFF, 0x3D3C, 0xFF03, 0x5F04, 0xDF03, 0x3F04, 0x1D04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 23, 960 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xDF0B, 0x3F04, 0x1E04, 0x1F04, 0x5F65, 0xBFF7, 0xFFFF, 0xFFFF, 0xBF75, 0x1F96, 0xDFFF, 0xDFF7, 0xBF24, 0x7DAE, 0xDFFF, 0xDFFF, 0xBFEF, 0x9E2C, 0x3E04, 0x1F04, 0x3F04, 0x5F04, 0xDF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 24, 1000 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x3F04, 0xFE03, 0x1F0C, 0xFF6C, 0x9FFF, 0xFFFF, 0xFFF7, 0x9F95, 0x3F04, 0x5EA6, 0xFFFF, 0xFFF7, 0x1F24, 0x5F0C, 0xDFA6, 0xFFFF, 0xFFEF, 0xBFFF, 0x7F3C, 0xFF0B, 0xDF03, 0x5F04, 0x3D04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 25, 1040 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x5F0C, 0xFE03, 0x1F5D, 0xFEF7, 0xFCFF, 0xFFFF, 0xDFA5, 0xDE0B, 0x1F04, 0x1F96, 0xFFF7, 0xFFF7, 0x7F1C, 0x1F04, 0xFF03, 0x5FAE, 0xFEFF, 0xDDFF, 0x9EEF, 0x9F34, 0x3F04, 0xDE03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 26, 1080 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1E04, 0x1F04, 0xDF03, 0xBFDF, 0xDEFF, 0xFFFF, 0x1FA6, 0x3F0C, 0x5F04, 0xFF03, 0xFE9D, 0xFFFF, 0xFFFF, 0x9F2C, 0xFE03, 0xDF0B, 0xDFBE, 0xFFFF, 0xDEF7, 0xBFDF, 0x5F24, 0x1F04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 27, 1120 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3E04, 0x1F04, 0xFE03, 0x3E34, 0xFFE7, 0x3F9E, 0xFF03, 0xFE03, 0x3F04, 0x1D04, 0xFE9D, 0xFFFF, 0xDFFF, 0x1F1C, 0x1F1C, 0xBFAE, 0xFFFF, 0xFFF7, 0xBFE7, 0x3F14, 0x1F04, 0x1E04, 0x3F04, 0x1E04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 28, 1160 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xBE03, 0x5F04, 0x1F04, 0x1E0C, 0x5F0C, 0x3D04, 0xFF03, 0x1F14, 0xFF03, 0x3E0C, 0x1F86, 0xFFFF, 0xBFFF, 0xBF34, 0x7EA6, 0xFFF7, 0xDEFF, 0xFFF7, 0x1C24, 0x1F04, 0x1E04, 0x3D04, 0x3F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 29, 1200 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x7F04, 0x1F04, 0x9E03, 0x3F04, 0x1D04, 0xFF03, 0x5F04, 0xFF03, 0xFF95, 0xFFF7, 0xDFF7, 0xDEAE, 0xFEFF, 0xFFFF, 0xBFEF, 0x7C44, 0x3F04, 0xFF0B, 0x3F0C, 0x1F04, 0x3F04, 0x1F0C, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 30, 1240 pixels
    0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x5D04, 0xDF03, 0xFF0B, 0x3E04, 0xFF03, 0x1F04, 0x3F04, 0x3F04, 0xDE03, 0x5F04, 0xDE95, 0xFEFF, 0xDDFF, 0xFFFF, 0xDFFF, 0xDFFF, 0x9F4C, 0x3F04, 0x1F04, 0xFF03, 0xDF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, // row 31, 1280 pixels
    0xFE03, 0x3F04, 0x1F04, 0xFF03, 0x3F04, 0x1F04, 0xDE03, 0x3D04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xDF03, 0x3F96, 0xFEFF, 0xFFFF, 0xFFFF, 0xFFF7, 0x7F3C, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3D04, 0xDE03, 0x1F04, 0x3F04, 0xFF03, 0x1F04, 0x3F04, 0xFE03, // row 32, 1320 pixels
    0xFF03, 0x3F04, 0x3F04, 0x1F04, 0x3F04, 0x5F04, 0x3F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3E04, 0x3F04, 0xFF95, 0xFFEF, 0xFFF7, 0x9FF7, 0x9F4C, 0x3C04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x3F04, 0x5F04, 0x3F04, 0x1F04, 0x3F04, 0x3F04, 0xFF03, // row 33, 1360 pixels
    0xDE03, 0x1F04, 0x3F04, 0x3F04, 0xFF0B, 0xFE03, 0x5F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1E04, 0xFF03, 0x1F86, 0xFFFF, 0xFFEF, 0xDF54, 0x3E04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x5F04, 0xFE03, 0xFF0B, 0x3F04, 0x3F04, 0x1F04, 0xDE03, // row 34, 1400 pixels
    0xDB03, 0x3F04, 0xFF03, 0x1E04, 0xFE13, 0xDE03, 0x5F04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xDE03, 0x1F04, 0x7F9E, 0xBEFF, 0x9F64, 0x3E04, 0x1E04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x5F04, 0xDE03, 0xFE13, 0x1E04, 0xFF03, 0x3F04, 0xDB03, // row 35, 1440 pixels
    0xD502, 0x1F04, 0x1F04, 0xFB03, 0x1E04, 0xFF03, 0x1F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x5F04, 0x1F04, 0x5F96, 0x3F6D, 0xFC0B, 0x1F04, 0x5F04, 0xFE03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x1F04, 0xFF03, 0x1E04, 0xFB03, 0x1F04, 0x1F04, 0xD502, // row 36, 1480 pixels
    0xC600, 0x9A03, 0x9F04, 0x5F04, 0x3F04, 0x3F04, 0x1F04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFE03, 0x1F14, 0x3E04, 0x1F04, 0xDD03, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF03, 0x1F04, 0x3F04, 0x3F04, 0x5F04, 0x9F04, 0x9A03, 0xC600, // row 37, 1520 pixels
    0x0008, 0xCC01, 0x9A03, 0x1F04, 0x1F04, 0x3F04, 0x3F04, 0x3F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0xFF03, 0x5F04, 0x1F04, 0xFF03, 0x5F04, 0x3E04, 0xFF03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x3F04, 0x3F04, 0x3F04, 0x1F04, 0x1F04, 0x9A03, 0xCC01, 0x0008, // row 38, 1560 pixels
    0x0200, 0x0008, 0xE708, 0xD402, 0xDA03, 0x1F04, 0xFF03, 0xFE03, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFF0B, 0x3F04, 0x1C0C, 0x3F04, 0x1F04, 0x3E0C, 0x1E04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0x1F04, 0xFE03, 0xFF03, 0x1F04, 0xDA03, 0xD402, 0xE708, 0x0008, 0x0200, // row 39, 1600 pixels
};
// clang-format on

const uint16_t DynamicspidIconWidth  = 40;
const uint16_t DynamicspidIconHeight = 40;

// Convert by: https://lang-ship.com/tools/image2data/
// RGB565 Dump(little endian)
// clang-format off
const unsigned short DynamicspidIcon[1600] PROGMEM = {
    0x6031, 0xA0AC, 0xE1DD, 0x00DE, 0x00E6, 0xE0E5, 0x00E6, 0x20E6, 0x00E6, 0x00E6, 0x00E6, 0x00E6, 0x00E6, 0x00E6, 0x00E6, 0x00E6, 0x20E6, 0x20E6, 0x20E6, 0x20E6, 0x40E6, 0x40E6, 0x40E6, 0x40E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x40E6, 0xC0E6, 0x80E6, 0x60E6, 0xC0E6, 0x81DE, 0x20B5, 0xC039, // row 0, 40 pixels
    0x60AC, 0x20F6, 0xE1F5, 0xE0DD, 0x21EE, 0xE0E5, 0x00E6, 0x20EE, 0x40EE, 0x40EE, 0x40EE, 0x40EE, 0x40EE, 0x40EE, 0x40EE, 0x40EE, 0x40E6, 0x40E6, 0x40E6, 0x60EE, 0x60EE, 0x60EE, 0x60EE, 0x60EE, 0x80EE, 0x80EE, 0x80EE, 0x80EE, 0x80EE, 0x80EE, 0x80EE, 0x80EE, 0x60F6, 0xA0E6, 0x80E6, 0x61EE, 0x80EE, 0x81EE, 0xC0F6, 0xE0B4, // row 1, 80 pixels
    0xE0DD, 0x20F6, 0x63BC, 0x858B, 0xC68B, 0xA483, 0xA683, 0xA68B, 0x8683, 0x8683, 0x8683, 0x868B, 0x868B, 0x868B, 0x868B, 0x868B, 0xA68B, 0xA68B, 0xA68B, 0x8683, 0x8683, 0x8683, 0x8683, 0x8683, 0xA683, 0xA683, 0xA683, 0xA683, 0xA683, 0xA68B, 0xA68B, 0xA68B, 0x878B, 0xC583, 0xA583, 0xA68B, 0xC48B, 0xA4B4, 0xA0EE, 0x61E6, // row 2, 120 pixels
    0x00DE, 0xE0E5, 0x6583, 0xE739, 0xC831, 0xC631, 0xE831, 0xA839, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE739, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xE731, 0xA839, 0xE631, 0xE731, 0xA731, 0x053A, 0xA583, 0xA0DE, 0xC0DE, // row 3, 160 pixels
    0x00EE, 0x01EE, 0x868B, 0xC731, 0xA831, 0xC631, 0xE639, 0xC539, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xE739, 0xE739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xA739, 0xE739, 0xE839, 0x8831, 0xC739, 0xC783, 0xC1EE, 0xA1EE, // row 4, 200 pixels
    0x00EE, 0xE0ED, 0x6583, 0xE631, 0xC839, 0xC639, 0xE639, 0xC641, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC541, 0xC639, 0xC739, 0xA739, 0xC639, 0xA57B, 0xA0E6, 0x80EE, // row 5, 240 pixels
    0x20E6, 0x00E6, 0x647B, 0xE631, 0xE839, 0xC639, 0xC739, 0xC839, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xC839, 0xC831, 0xE739, 0xC739, 0xE739, 0xA67B, 0xC0E6, 0xA0EE, // row 6, 280 pixels
    0x00E6, 0x01E6, 0x6783, 0xC739, 0xA839, 0xC541, 0xE739, 0xC731, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xE739, 0xE631, 0xC539, 0xA639, 0xC839, 0xA883, 0xC0EE, 0x60EE, // row 7, 320 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x083A, 0xA531, 0x064A, 0x284A, 0x854A, 0xE739, 0xA731, 0xC841, 0x0832, 0xC731, 0xC731, 0x073A, 0xA631, 0xC739, 0xC739, 0xC631, 0xC731, 0x063A, 0xE639, 0xE739, 0xC631, 0xE739, 0xC739, 0xA839, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 8, 360 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xA731, 0x6762, 0xC2D5, 0x20EE, 0xC0DD, 0xC2D5, 0xA2CD, 0xA2AC, 0x6583, 0xC441, 0xC839, 0xA731, 0xE539, 0xE639, 0x8631, 0xE949, 0xE839, 0xA731, 0x8731, 0xE941, 0xE739, 0xA531, 0x083A, 0xC739, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 9, 400 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC639, 0xA762, 0x01D6, 0x20E6, 0x20EE, 0x40EE, 0x21EE, 0x20EE, 0x80E6, 0x42E6, 0xA5B4, 0xA762, 0xC839, 0xA639, 0x0742, 0x6629, 0xA531, 0xC941, 0xC941, 0x8639, 0xC739, 0xC639, 0xA531, 0xE839, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 10, 440 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC631, 0x8739, 0xC562, 0xC66A, 0x8683, 0x0494, 0xA4AC, 0xA4CD, 0x80E6, 0x00DE, 0x60F6, 0x40F6, 0x24BD, 0xC752, 0xA729, 0x2842, 0x0642, 0xA631, 0xA631, 0xE639, 0xE639, 0xE741, 0xA631, 0xC731, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 11, 480 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x273A, 0xA931, 0xC639, 0xC741, 0xA639, 0xC741, 0xE641, 0x8839, 0x8652, 0x64A4, 0xC2DD, 0x20E6, 0x60E6, 0x61E6, 0x2494, 0xA839, 0x6631, 0x073A, 0xC731, 0xC731, 0x0542, 0xA739, 0xC939, 0xE539, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 12, 520 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x8739, 0xE739, 0xC531, 0xE639, 0xE531, 0xE831, 0x073A, 0xE731, 0xA741, 0x083A, 0x263A, 0x0694, 0xA3DD, 0x80EE, 0x61E6, 0x42C5, 0x4652, 0x8731, 0x4A42, 0x8529, 0xC739, 0xA839, 0xA739, 0xE639, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 13, 560 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xA641, 0xC741, 0x053A, 0xE939, 0xC839, 0x8839, 0x8731, 0xC539, 0xA731, 0xC631, 0x0832, 0xA729, 0x855A, 0xE4BC, 0x40EE, 0x60E6, 0x65C5, 0xE351, 0xC739, 0x073A, 0xA839, 0xC839, 0xC639, 0xA731, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 14, 600 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC731, 0xC941, 0x639C, 0x44C5, 0xA4B4, 0x45A4, 0x667B, 0xC639, 0x063A, 0xA739, 0xA739, 0x063A, 0xC631, 0xC739, 0x869C, 0x60EE, 0x60EE, 0xC6CD, 0xE549, 0xA731, 0x053A, 0xC639, 0xA839, 0x073A, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 15, 640 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC639, 0xE76A, 0x80E6, 0x20EE, 0xE0ED, 0x41EE, 0x60E6, 0xE0D5, 0x86AC, 0x464A, 0xE931, 0xC841, 0x8639, 0xE831, 0xC539, 0x269C, 0x80E6, 0x40EE, 0x21C5, 0xE839, 0x273A, 0xA639, 0x8841, 0x0532, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 16, 680 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xE731, 0xC639, 0xA3A4, 0xE6C4, 0x65DD, 0x21EE, 0xE0DD, 0x60EE, 0x21EE, 0x20D6, 0x248C, 0xC649, 0xC641, 0xE631, 0xA731, 0xC739, 0x669C, 0x80EE, 0x83EE, 0x2394, 0xA629, 0x0742, 0xA631, 0xC839, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 17, 720 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC739, 0xC839, 0xC731, 0x0632, 0xC529, 0xC641, 0xA583, 0x04B5, 0x40E6, 0x80F6, 0x60EE, 0x42BD, 0xC541, 0xE839, 0xE739, 0xE831, 0xE841, 0x04B5, 0x80E6, 0x61E6, 0x0763, 0x8429, 0x273A, 0xA841, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 18, 760 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC741, 0x8739, 0xA739, 0xE639, 0xE731, 0xE731, 0x8831, 0xC739, 0xE762, 0x61C5, 0x20EE, 0x43EE, 0xC4BD, 0x074A, 0xE741, 0x8439, 0xA731, 0x865A, 0xC1D5, 0xA0EE, 0x44C5, 0xC631, 0xC631, 0xC739, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 19, 800 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x0732, 0xE739, 0xC841, 0xC641, 0xC639, 0x0742, 0xC839, 0xA631, 0x0832, 0xC541, 0xE3B4, 0x02E6, 0x61EE, 0x42BD, 0xA741, 0xA839, 0x073A, 0xA631, 0x0794, 0x40DE, 0x80EE, 0x2973, 0xC639, 0xE731, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 20, 840 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xA631, 0xE539, 0x276B, 0x0684, 0x466B, 0xA639, 0xE639, 0x063A, 0x8731, 0x073A, 0xA839, 0xC3AC, 0x40F6, 0x40EE, 0x4594, 0x293A, 0xA739, 0xE739, 0x4642, 0x01D6, 0x80EE, 0xE5BC, 0xA739, 0xE731, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 21, 880 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC739, 0x6662, 0x42E6, 0x20EE, 0x40F6, 0x02DE, 0x2594, 0x064A, 0xC741, 0xA631, 0x283A, 0xC639, 0x63C5, 0x60F6, 0x20D6, 0x654A, 0xC741, 0xA731, 0x0732, 0x65A4, 0x40DE, 0x61E6, 0xE549, 0xC631, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 22, 920 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xE831, 0xC541, 0x81BD, 0xA1DD, 0x20EE, 0x01EE, 0x61EE, 0x82D5, 0xA54A, 0xA841, 0xA739, 0x0632, 0xE762, 0x20E6, 0x60F6, 0x86AC, 0xC541, 0xA731, 0xA939, 0x6652, 0x80E6, 0x81DE, 0xE68B, 0xC739, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 23, 960 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC631, 0xA841, 0x8941, 0x063A, 0x8752, 0xE4AC, 0x60EE, 0x20EE, 0x81D5, 0x2852, 0xC531, 0xC639, 0xC841, 0xE4B4, 0x80F6, 0xE0D5, 0xA641, 0x074A, 0xA731, 0x8839, 0xE3CD, 0x60E6, 0x01AD, 0xE741, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 24, 1000 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xE639, 0xE731, 0xC731, 0xC439, 0xC739, 0xC741, 0x457B, 0x60E6, 0x61EE, 0x6594, 0xE539, 0xA839, 0xA831, 0x8483, 0x00E6, 0xA0EE, 0x867B, 0xA739, 0xE631, 0xE641, 0xC3A4, 0x81EE, 0xE2CD, 0xA631, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 25, 1040 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xC831, 0xE639, 0xA631, 0xC741, 0xE941, 0xA731, 0xE741, 0x03AD, 0x01E6, 0x41DE, 0xC641, 0xE841, 0xE739, 0xC639, 0x01E6, 0x40E6, 0xA4A4, 0xA731, 0x0832, 0xC841, 0x048C, 0x60EE, 0xE2DD, 0xE739, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 26, 1080 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0xA639, 0x469C, 0xC5AC, 0xA583, 0x8739, 0xC739, 0xE741, 0x684A, 0x40EE, 0x40EE, 0x6573, 0xA541, 0xC731, 0xE631, 0xA4DD, 0x40E6, 0x24BD, 0xC739, 0xE531, 0x8739, 0x867B, 0x81EE, 0x00E6, 0x8552, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 27, 1120 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x86AC, 0xE0DD, 0x40EE, 0x40DE, 0xC583, 0xE741, 0xE539, 0xC731, 0x00DE, 0x21F6, 0x268C, 0xA541, 0xA731, 0x0632, 0xE5BC, 0x40E6, 0x63C5, 0xA839, 0xE739, 0xA841, 0xA662, 0x80E6, 0xA0F6, 0xC85A, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 28, 1160 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x63D5, 0x40EE, 0x01F6, 0x40F6, 0xA6AC, 0xC739, 0xE839, 0xA941, 0x61BD, 0x60E6, 0x4773, 0xE941, 0xC739, 0xE839, 0xC3A4, 0xC0E6, 0xE5AC, 0xE641, 0xC639, 0xC841, 0xC55A, 0x61D6, 0x22D6, 0x454A, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 29, 1200 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x24BD, 0x60E6, 0x20EE, 0x01E6, 0x469C, 0xC731, 0xC731, 0xC741, 0xE741, 0x8662, 0x0642, 0xC731, 0x8639, 0xA839, 0xA741, 0x2773, 0xC841, 0xA639, 0xA739, 0xA931, 0x8731, 0xE762, 0xC76A, 0xC539, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 30, 1240 pixels
    0x00E6, 0x00EE, 0x6683, 0xC739, 0xE739, 0xC639, 0xC739, 0xC739, 0x675A, 0x04BD, 0x83DD, 0xC3AC, 0xE441, 0xA831, 0x0842, 0xC631, 0xC839, 0xA839, 0xC631, 0x2732, 0xA839, 0xC841, 0xE631, 0xC741, 0xE629, 0x0742, 0xA739, 0x273A, 0xE631, 0xC639, 0xA739, 0xA839, 0xC739, 0xC739, 0xC639, 0xC739, 0xC731, 0xA683, 0xA0EE, 0xA0E6, // row 31, 1280 pixels
    0x00E6, 0x01EE, 0x6783, 0xC739, 0xA739, 0xC539, 0xE739, 0xC831, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC731, 0xE641, 0xA641, 0xA839, 0x8883, 0xA0EE, 0x60EE, // row 32, 1320 pixels
    0x20EE, 0x00E6, 0x6483, 0xE631, 0xE839, 0xC539, 0xC739, 0xC731, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC731, 0xC729, 0xE731, 0xC739, 0x0632, 0xC573, 0xC0DE, 0xC0E6, // row 33, 1360 pixels
    0xE1ED, 0xE0ED, 0x6583, 0xE739, 0xC939, 0xC641, 0xC741, 0xA641, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC639, 0xC641, 0xE731, 0xE739, 0xC739, 0xE739, 0xA67B, 0xA0E6, 0x81E6, // row 34, 1400 pixels
    0x00E6, 0x01EE, 0x8683, 0xC731, 0xA831, 0xC639, 0xC739, 0xA639, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xC731, 0xC731, 0xC731, 0xC739, 0xC739, 0xC739, 0xC739, 0xC739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0x8639, 0xE731, 0xC739, 0x6831, 0xC739, 0xA783, 0xA0EE, 0x80EE, // row 35, 1440 pixels
    0x40DE, 0x20E6, 0x8483, 0x0632, 0xE731, 0xE631, 0xE731, 0xC739, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xE739, 0xE739, 0xE739, 0xE739, 0xE739, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xC731, 0xA839, 0x0732, 0x073A, 0xA739, 0x0542, 0xC583, 0xA0E6, 0xC0E6, // row 36, 1480 pixels
    0xC1DD, 0x00EE, 0x24AC, 0x457B, 0x8783, 0x447B, 0x467B, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x6683, 0x8683, 0x8683, 0x8683, 0x8683, 0x8683, 0x8683, 0x8683, 0x8683, 0x677B, 0xA573, 0xA67B, 0x8683, 0xA483, 0x84A4, 0xA0EE, 0x61DE, // row 37, 1520 pixels
    0xA0B4, 0x40FE, 0x00F6, 0x00E6, 0x20EE, 0x00E6, 0x20E6, 0x40EE, 0x20E6, 0x20E6, 0x20E6, 0x20E6, 0x20E6, 0x20E6, 0x40EE, 0x40EE, 0x40E6, 0x40E6, 0x40E6, 0x60EE, 0x60EE, 0x60EE, 0x60EE, 0x60EE, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60EE, 0x60EE, 0x60EE, 0x60EE, 0x80EE, 0xC0DE, 0x80E6, 0x60E6, 0xA0E6, 0xA0EE, 0xE0F6, 0x20B5, // row 38, 1560 pixels
    0x8039, 0xC0B4, 0xE1E5, 0x20DE, 0x00EE, 0x00EE, 0x21E6, 0x20EE, 0x20E6, 0x20E6, 0x20E6, 0x20E6, 0x40EE, 0x40EE, 0x40EE, 0x40EE, 0x40E6, 0x40E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x60E6, 0x80E6, 0x80E6, 0x80E6, 0x80E6, 0x80E6, 0x80E6, 0x80E6, 0x60F6, 0xC0EE, 0x61E6, 0x60EE, 0xC0EE, 0x82E6, 0x20BD, 0xC041, // row 39, 1600 pixels
};
// clang-format on

#endif /* DISPLAY_320X240 */
#endif /* USER_DISPLAY_DEFAULT_H */
