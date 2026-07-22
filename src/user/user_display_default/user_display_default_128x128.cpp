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

		drawRLEIcon(8, IMG_H, BluetoothIconWidth,
		            BluetoothIcon_Palette, BluetoothIcon_RLE_Packed,
		            BluetoothIcon_RLE_RunCount);
		HAL.Display.fillRect(145 - 83, 75 - 16, 5, 21, TFT_WHITE);
		HAL.Display.fillRect(137 - 83, 83 - 16, 21, 5, TFT_WHITE);
		drawRLEIcon(80, IMG_H, DynamicspidIconWidth,
		            DynamicspidIcon_Palette, DynamicspidIcon_RLE_Packed,
		            DynamicspidIcon_RLE_RunCount);

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

const uint16_t BluetoothIconWidth         = 40;
const uint16_t BluetoothIconHeight        = 40;
const uint16_t BluetoothIcon_RLE_RunCount = 365;

// Icon: Bluetooth icon (C) Terence Eden
//       https://icon-icons.com/icon/bluetooth/130975
// License: CC BY 4.0
// clang-format off
const uint16_t BluetoothIcon_Palette[5] PROGMEM = {
    0x041F, 0x2CBF, 0x8E3F, 0xFFDF, 0xBEFF,
};

// パレット化・行RLE圧縮アイコンデータ。
// Palette: 使用色一覧(RGB565)。RLE_Packed: ニブルパッキングされたラン列。
// フォーマット詳細は drawRLEIcon() のコメント参照。ロスレス(元PNG基準)。
const uint8_t BluetoothIcon_RLE_Packed[400] PROGMEM = {
    0x80, 0x28, 0x84, 0x00, 0x20, 0x84, 0x82, 0x00, 0x23, 0x83, 0x82, 0x00, 0x10, 0x11, 0x00, 0x13, 0x82, 0x81, 0x00, 0x11,
    0x22, 0x00, 0x12, 0x82, 0x81, 0x00, 0x11, 0x21, 0x31, 0x21, 0x00, 0x12, 0x81, 0x81, 0x00, 0x11, 0x21, 0x32, 0x11, 0x00,
    0x11, 0x81, 0x81, 0x00, 0x11, 0x21, 0x33, 0x11, 0x00, 0x10, 0x81, 0x81, 0x00, 0x11, 0x21, 0x33, 0x41, 0x11, 0x0F, 0x81,
    0x81, 0x00, 0x11, 0x21, 0x34, 0x41, 0x11, 0x0E, 0x81, 0x81, 0x00, 0x11, 0x21, 0x31, 0x41, 0x21, 0x32, 0x41, 0x11, 0x0D,
    0x81, 0x81, 0x0B, 0x12, 0x04, 0x21, 0x31, 0x41, 0x11, 0x21, 0x32, 0x41, 0x11, 0x0C, 0x81, 0x81, 0x0A, 0x21, 0x31, 0x41,
    0x11, 0x03, 0x21, 0x31, 0x41, 0x02, 0x21, 0x32, 0x41, 0x11, 0x0B, 0x81, 0x81, 0x0A, 0x21, 0x32, 0x41, 0x11, 0x02, 0x21,
    0x31, 0x41, 0x02, 0x11, 0x41, 0x32, 0x21, 0x0B, 0x81, 0x81, 0x0B, 0x21, 0x32, 0x41, 0x11, 0x01, 0x21, 0x31, 0x41, 0x01,
    0x11, 0x21, 0x32, 0x21, 0x11, 0x0B, 0x81, 0x81, 0x0C, 0x21, 0x32, 0x41, 0x11, 0x21, 0x31, 0x41, 0x11, 0x21, 0x32, 0x21,
    0x11, 0x0C, 0x81, 0x81, 0x0D, 0x21, 0x32, 0x42, 0x31, 0x42, 0x32, 0x21, 0x11, 0x0D, 0x81, 0x81, 0x0E, 0x21, 0x37, 0x41,
    0x11, 0x0E, 0x81, 0x81, 0x0F, 0x21, 0x35, 0x41, 0x11, 0x0F, 0x81, 0x81, 0x00, 0x10, 0x21, 0x33, 0x41, 0x11, 0x00, 0x10,
    0x81, 0x81, 0x00, 0x10, 0x21, 0x33, 0x41, 0x11, 0x00, 0x10, 0x81, 0x81, 0x0F, 0x21, 0x35, 0x41, 0x11, 0x0F, 0x81, 0x81,
    0x0E, 0x21, 0x37, 0x41, 0x11, 0x0E, 0x81, 0x81, 0x0D, 0x21, 0x32, 0x42, 0x31, 0x41, 0x21, 0x32, 0x41, 0x11, 0x0D, 0x81,
    0x81, 0x0C, 0x21, 0x32, 0x41, 0x11, 0x21, 0x31, 0x41, 0x11, 0x21, 0x32, 0x41, 0x11, 0x0C, 0x81, 0x81, 0x0B, 0x21, 0x32,
    0x41, 0x11, 0x01, 0x21, 0x31, 0x41, 0x02, 0x21, 0x32, 0x41, 0x11, 0x0B, 0x81, 0x81, 0x0A, 0x21, 0x32, 0x41, 0x11, 0x02,
    0x21, 0x31, 0x41, 0x02, 0x11, 0x41, 0x32, 0x21, 0x0B, 0x81, 0x81, 0x0A, 0x11, 0x42, 0x11, 0x03, 0x21, 0x31, 0x41, 0x01,
    0x11, 0x41, 0x32, 0x21, 0x0C, 0x81, 0x81, 0x0B, 0x12, 0x04, 0x21, 0x31, 0x41, 0x11, 0x41, 0x32, 0x21, 0x11, 0x0C, 0x81,
    0x81, 0x00, 0x11, 0x21, 0x31, 0x42, 0x32, 0x21, 0x11, 0x0D, 0x81, 0x81, 0x00, 0x11, 0x21, 0x34, 0x21, 0x11, 0x0E, 0x81,
    0x81, 0x00, 0x11, 0x21, 0x33, 0x41, 0x11, 0x0F, 0x81, 0x81, 0x00, 0x11, 0x21, 0x32, 0x41, 0x11, 0x00, 0x10, 0x81, 0x81,
    0x00, 0x11, 0x21, 0x31, 0x41, 0x11, 0x00, 0x11, 0x81, 0x81, 0x00, 0x11, 0x21, 0x41, 0x11, 0x00, 0x11, 0x82, 0x81, 0x00,
    0x11, 0x21, 0x11, 0x00, 0x12, 0x82, 0x82, 0x00, 0x24, 0x82, 0x83, 0x00, 0x22, 0x83, 0x85, 0x00, 0x1D, 0x86, 0x80, 0x28,
};
// clang-format on

const uint16_t DynamicspidIconWidth         = 40;
const uint16_t DynamicspidIconHeight        = 40;
const uint16_t DynamicspidIcon_RLE_RunCount = 465;

// Convert by: https://lang-ship.com/tools/image2data/
// clang-format off
const uint16_t DynamicspidIcon_Palette[5] PROGMEM = {
    0xE620, 0xC561, 0x5A86, 0x39C7, 0x8BE3,
};

// パレット化・行RLE圧縮アイコンデータ。
// Palette: 使用色一覧(RGB565)。RLE_Packed: ニブルパッキングされたラン列。
// フォーマット詳細は drawRLEIcon() のコメント参照。ロスレス(元PNG基準)。
const uint8_t DynamicspidIcon_RLE_Packed[486] PROGMEM = {
    0x80, 0x28, 0x80, 0x28, 0x83, 0x00, 0x22, 0x83, 0x82, 0x02, 0x10, 0x1F, 0x03, 0x82, 0x82, 0x01, 0x11, 0x20, 0x1C, 0x33,
    0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82,
    0x01, 0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x37, 0x23,
    0x30, 0x15, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x36, 0x21, 0x14, 0x42, 0x21, 0x30, 0x11, 0x21, 0x02, 0x82, 0x82, 0x01,
    0x11, 0x36, 0x21, 0x13, 0x04, 0x11, 0x41, 0x3F, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x38, 0x23, 0x41, 0x12, 0x02, 0x11,
    0x21, 0x3D, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x3D, 0x21, 0x41, 0x11, 0x02, 0x41, 0x3C, 0x21, 0x02, 0x82, 0x82, 0x01,
    0x11, 0x3F, 0x21, 0x41, 0x02, 0x41, 0x21, 0x3A, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x37, 0x24, 0x36, 0x41, 0x11, 0x01,
    0x11, 0x21, 0x39, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x36, 0x21, 0x11, 0x02, 0x12, 0x41, 0x21, 0x34, 0x21, 0x11, 0x01,
    0x41, 0x39, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x36, 0x21, 0x41, 0x12, 0x03, 0x11, 0x41, 0x34, 0x21, 0x11, 0x01, 0x41,
    0x38, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x39, 0x22, 0x41, 0x11, 0x02, 0x41, 0x34, 0x41, 0x02, 0x21, 0x37, 0x21, 0x02,
    0x82, 0x82, 0x01, 0x11, 0x3C, 0x21, 0x41, 0x02, 0x41, 0x34, 0x41, 0x01, 0x11, 0x37, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11,
    0x3E, 0x41, 0x02, 0x41, 0x33, 0x21, 0x11, 0x01, 0x21, 0x36, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x37, 0x43, 0x21, 0x34,
    0x41, 0x02, 0x21, 0x33, 0x41, 0x01, 0x41, 0x36, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x36, 0x21, 0x11, 0x02, 0x11, 0x41,
    0x34, 0x41, 0x01, 0x11, 0x33, 0x21, 0x01, 0x11, 0x21, 0x35, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x37, 0x21, 0x41, 0x11,
    0x02, 0x41, 0x33, 0x21, 0x11, 0x01, 0x21, 0x33, 0x11, 0x01, 0x41, 0x35, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x3A, 0x41,
    0x11, 0x01, 0x41, 0x33, 0x41, 0x01, 0x41, 0x33, 0x41, 0x01, 0x41, 0x35, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x3B, 0x41,
    0x01, 0x11, 0x21, 0x32, 0x41, 0x01, 0x11, 0x33, 0x41, 0x01, 0x11, 0x35, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x36, 0x21,
    0x41, 0x21, 0x33, 0x11, 0x01, 0x21, 0x32, 0x21, 0x01, 0x11, 0x21, 0x32, 0x21, 0x01, 0x11, 0x35, 0x21, 0x02, 0x82, 0x82,
    0x01, 0x11, 0x35, 0x41, 0x02, 0x11, 0x21, 0x32, 0x41, 0x01, 0x41, 0x32, 0x21, 0x11, 0x01, 0x21, 0x32, 0x21, 0x01, 0x11,
    0x35, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x35, 0x11, 0x03, 0x21, 0x32, 0x41, 0x11, 0x21, 0x33, 0x12, 0x21, 0x32, 0x21,
    0x12, 0x35, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x35, 0x41, 0x02, 0x11, 0x21, 0x33, 0x21, 0x34, 0x22, 0x34, 0x21, 0x36,
    0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x36, 0x42, 0x21, 0x30, 0x16, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x30, 0x1F, 0x21,
    0x02, 0x82, 0x82, 0x01, 0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82, 0x01,
    0x11, 0x30, 0x1F, 0x21, 0x02, 0x82, 0x82, 0x01, 0x11, 0x20, 0x1F, 0x41, 0x02, 0x82, 0x82, 0x00, 0x24, 0x82, 0x83, 0x00,
    0x22, 0x83, 0x80, 0x28, 0x80, 0x28,
};
// clang-format on

#endif /* DISPLAY_320X240 */
#endif /* USER_DISPLAY_DEFAULT_H */
