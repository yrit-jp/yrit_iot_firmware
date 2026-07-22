#include "user_display_default.h"

#ifdef USER_DISPLAY_DEFAULT_H
#ifdef DISPLAY_320X240

#include "icon.h"

void drawRLEIcon(int32_t x, int32_t y, int32_t w, const uint16_t *palette, const uint8_t *packed, uint16_t runCount);

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
		HAL.Display.fillRect(75, 30, 245, 40, TFT_BLACK);
		render->setFontSize(16);
		render->setCursor(80, 29);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		render->printf("[%d/%d] ", g_current_sensor_number, g_sensor_num);

		uint8_t lines = 1;
		int i         = 0;
		while (sensor_name[i] != '\0') {
			render->printf("%c", sensor_name[i]);
			if (render->getCursorX() > 300) {
				render->printf("\n");
				render->setCursor(80, render->getCursorY());
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
		FT_BBox bbox;

		// Update sensor unit
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(75, 160, 245, 51, TFT_BLACK);
		render->setFontSize(25);

		// Draw new unit
		bbox = render->calculateBoundingBoxFmt(300, 160, 25,
		                                       Align::TopRight, Layout::Horizontal,
		                                       "%s", sensor_unit);

		render->setCursor(bbox.xMin, bbox.yMin);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		render->printf("%s", sensor_unit);

		// Fill remain area
		HAL.Display.giveSemaphoreMutex();

		_last_unit = String(sensor_unit);
	}
}

void drawLastUpdateDate(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo, bool force_update = false) {
	static String _last_s_day = "";
	char _s_day[64]           = {0};

	if (timeinfo.tm_mday != 0) {
		// tm_mday usually ranges from 1 to 31.
		strftime(_s_day, 64, "%Y/%m/%d %H:%M", &timeinfo);
	} else {
		strftime(_s_day, 64, "- - - -", &timeinfo);
	}
	if (force_update or not _last_s_day.equals(String(_s_day))) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setFontSize(16);
		render->setCursor(2, 3);
		render->setFontColor(MY_DARKGRAY, MY_DARKGRAY);
		render->printf("最終更新 : %s", _last_s_day.c_str());
		render->setCursor(2, 3);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		render->printf("最終更新 : %s", _s_day);
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		HAL.Display.giveSemaphoreMutex();

		_last_s_day = String(_s_day);
	}
}

void drawCurrentIpAddress(std::unique_ptr<OpenFontRender> &render, String ip_addr, bool force_update = false) {
	static String _last_ip_addr = "";

	if (force_update or not _last_ip_addr.equals(ip_addr)) {

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		render->setFontSize(18);
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

	// Left panels
	HAL.Display.fillRect(0, 29, 70, 182, TFT_LIGHTGREY);
	// FTP
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31, 70 - 4, 43, _panel_color);
	// HTTP
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31 + 45, 70 - 4, 43, _panel_color);
	// MQTT
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31 + 90, 70 - 4, 43, _panel_color);
	// Ambient
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	HAL.Display.fillRect(2, 31 + 135, 70 - 4, 43, _panel_color);

	HAL.Display.giveSemaphoreMutex();
}

void drawFeatures2LeftPanel(std::unique_ptr<OpenFontRender> &render) {
	uint16_t _panel_color = 0;
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// FTP
	_panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(25);
	render->setCursor(36, 30 + 5);
	render->cprintf("FTP");
	// HTTP
	_panel_color = g_features.http ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(22);
	render->setCursor(35, 30 + 46 + 5);
	render->cprintf("HTTP");
	// MQTT
	_panel_color = g_features.mqtt ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(22);
	render->setCursor(36, 30 + 90 + 7);
	render->cprintf("MQTT");
	// Ambient
	_panel_color = g_features.ambient ? TFT_DARKGREEN : MY_CRIMSON;
	render->setFontColor(TFT_WHITE, _panel_color);
	render->setFontSize(25);
	render->setCursor(34, 30 + 135 + 4);
	render->cprintf("AMB");

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
		Icon_WiFiStatus(255, 21, _rssi);
		HAL.Display.giveSemaphoreMutex();

		break;
	}
	case DATALINK_LAYER::ETHERNET: {
		_isConnected = HAL.Lan.isLinkUp();
		_ip          = HAL.Lan.localIP();

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		Icon_EthernetStatus(245, 5, _isConnected);
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
	struct _Rect rect     = {10, 60, 300, 20};
	uint16_t x_water_mark = 0;

	std::vector<String> items = {
	    "デバイス種別",
	    "FW バージョン",
	    "送信間隔",
	    "接続先 Wi-Fi",
	    "FTPサーバ",
	    "HTTPサーバ",
	    "MQTTサーバ",
	};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// draw device id
	// render->setFontSize(18);
	// render->setCursor(2, 3);
	// render->setFontColor(TFT_WHITE, MY_DARKGRAY);
	// render->printf("devID: %.10s", HAL.Info.DEVICE_ID.c_str());
	// if (HAL.Info.DEVICE_ID.length() > 10) {
	// 	render->printf("...");
	// }
	// render->setFontColor(TFT_WHITE, TFT_BLACK);

	// draw title
	render->setFontSize(20);
	render->setCursor(10, 30);
	render->setFontColor(TFT_ORANGE);
	render->printf("各種情報1");
	render->setFontColor(TFT_WHITE);

	// draw items
	render->setFontSize(14);
	for (int i = 0; i < items.size(); i++) {
		uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
		uint32_t y_offset = (rect.height * i);

		HAL.Display.fillRect(rect.x, (rect.y + y_offset), rect.width, rect.height, bg); // draw table cell
		render->setFontColor(MY_DARKGRAY, bg);
		render->setCursor((rect.x + 2), (rect.y + y_offset));
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

void drawInformation2(std::unique_ptr<OpenFontRender> &render, bool update_only = false) {
	constexpr struct _Rect rect    = {10, 60, 300, 20};
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

		// draw title
		render->setFontSize(20);
		render->setCursor(10, 30);
		render->setFontColor(TFT_ORANGE);
		render->printf("各種情報2");
		render->setFontColor(TFT_WHITE);

		// draw labels
		render->setFontSize(FONT_SIZE);
		for (int i = 0; i < LABEL_COUNT; i++) {
			uint16_t bg       = (i % 2 == 0) ? TFT_LIGHTGREY : TFT_WHITE;
			uint32_t y_offset = rect.height * i;

			HAL.Display.fillRect(rect.x, rect.y + y_offset, rect.width, rect.height, bg);
			render->setFontColor(MY_DARKGRAY, bg);
			render->setCursor(rect.x + 2, rect.y + y_offset);
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
			if (info.length() > 24) {
				info = info.substring(0, 24) + "...";
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

		render->setCursor(text_x, rect.y + y_offset - 1);
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
		constexpr uint32_t IMG_H = 79;

		HAL.Display.fillRect(0, 0, HAL.Display.width(), HAL.Display.height(), TFT_BLACK);
		render->setCursor(3, 3);
		render->printf("Bluetooth LE 接続名\n");
		render->printf("%s", HAL.getChipUUID().c_str());

		drawRLEIcon(50, IMG_H, BluetoothIconWidth,
		            BluetoothIcon_Palette, BluetoothIcon_RLE_Packed,
		            BluetoothIcon_RLE_RunCount);
		HAL.Display.fillRect(158, 103, 10, 30, TFT_WHITE);
		HAL.Display.fillRect(148, 113, 30, 10, TFT_WHITE);
		drawRLEIcon(195, IMG_H, DynamicspidIconWidth,
		            DynamicspidIcon_Palette, DynamicspidIcon_RLE_Packed,
		            DynamicspidIcon_RLE_RunCount);

		render->setCursor(225, 213);
		render->setFontColor(TFT_YELLOW, TFT_BLACK);
		render->printf("表示切替");
		render->setFontColor(TFT_WHITE, TFT_BLACK);
		break;
	}
	case CONFIG_PAGE_2: {
		HAL.Display.fillRect(0, 0, HAL.Display.width(), HAL.Display.height(), TFT_BLACK);

		render->setCursor(3, 3);
		if (HAL.Wlan.isSupported()) {
			auto mac = HAL.Wlan.fetchMacAddress();
			render->printf("無線LAN MAC: ");
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
			render->printf("有線LAN MAC: ");
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

		render->setCursor(225, 213);
		render->setFontColor(TFT_YELLOW, TFT_BLACK);
		render->printf("表示切替");
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
	OpenFontRender::Cursor c = {300, 90};
	const struct _Rect limit = {0, 0, 210, 50};
	const struct _Rect clear = {75, 70, 240, 90};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%d", value);
	FT_BBox bbox          = render->calculateBoundingBoxFmt(c.x, c.y, fit_size, Align::MiddleRight, Layout::Horizontal, "%d", value);

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
	OpenFontRender::Cursor c = {300, 90};
	const struct _Rect limit = {0, 0, 210, 50};
	const struct _Rect clear = {75, 70, 240, 90};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%.1f", value);
	FT_BBox bbox          = render->calculateBoundingBoxFmt(c.x, c.y, fit_size, Align::MiddleRight, Layout::Horizontal, "%.1f", value);

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
	OpenFontRender::Cursor c = {300, 90};
	const struct _Rect limit = {0, 0, 210, 50};
	const struct _Rect clear = {75, 70, 240, 90};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSizeFmt(limit.width, limit.height, Layout::Horizontal, "%s", value);
	FT_BBox bbox          = render->calculateBoundingBoxFmt(c.x, c.y, fit_size, Align::MiddleRight, Layout::Horizontal, "%s", value);

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
	OpenFontRender::Cursor c = {300, 90};
	const struct _Rect limit = {0, 0, 210, 50};
	const struct _Rect clear = {75, 70, 240, 90};

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	// Calculate appropriate font size
	unsigned int fit_size = render->calculateFitFontSize(limit.width, limit.height, Layout::Horizontal, str);
	FT_BBox bbox          = render->calculateBoundingBox(c.x, c.y, fit_size, Align::MiddleRight, Layout::Horizontal, str);

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
	default:
		break;
	}
}

void cb_AfterSystemDisplayUpdate_Default(std::unique_ptr<OpenFontRender> &render, enum DisplayTiming dt) {
	static bool heartbeat_circle_state  = true;
	static uint8_t btnA_pressed_counter = 0;

	switch (dt) {
	case DisplayTiming::SS_READY: {
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
		Icon_TfCardStatus(300, 4, TfCardStatusT::UNKNOWN, 0xFFFF); // Reset status
		HAL.Display.fillScreen(TFT_BLACK);
		HAL.Display.fillRect(0, 0, 320, 29, MY_DARKGRAY);
		Icon_BatteryStatus(275, 5, HAL.getBatteryLevel());
		HAL.Display.giveSemaphoreMutex();

		drawNetworkStatus(render);

		struct tm timeinfo = {0};
		drawLastUpdateDate(render, timeinfo);

		// Draw Left panel
		drawLeftPanelFrame();
		drawFeatures2LeftPanel(render);

		if (g_sensor_num == 0) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			render->setFontSize(40);
			render->setCursor(300, 90); // set right cursor
			render->setFontColor(TFT_WHITE, TFT_BLACK);
			render->rprintf("No Sensors");
			HAL.Display.giveSemaphoreMutex();
		}

		// Bottom bar
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(0, 211, 320, 29, MY_DARKGRAY);
		render->setFontSize(18);
		render->setFontColor(TFT_WHITE, MY_DARKGRAY);
		render->setCursor(8, 213);
		render->printf("センサ切替");
		render->setCursor(227, 213);
		render->printf("表示切替"); // TODO: Implement
		HAL.Display.giveSemaphoreMutex();

		if (HAL.Log.getCurrentLoggingState()) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			render->setCursor(130, 213);
			render->printf("TF切替");
			HAL.Display.giveSemaphoreMutex();
		}

		if (not HAL.Tf.ready()) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			Icon_TfCardStatus(225, 4, TfCardStatusT::UNMOUNTED);
			HAL.Display.giveSemaphoreMutex();
		} else {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			Icon_TfCardStatus(225, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING));
			HAL.Display.giveSemaphoreMutex();
		}
		break;
	}
	case DisplayTiming::EVERY_ONE_SECONDS_EVENT: {
		if (heartbeat_circle_state) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillEllipse(311, 14, 5, 5, TFT_GREENYELLOW);
			HAL.Display.giveSemaphoreMutex();
		} else {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillEllipse(311, 14, 5, 5, MY_DARKGRAY);
			HAL.Display.giveSemaphoreMutex();
		}

		heartbeat_circle_state = not heartbeat_circle_state;

		// WLAN manual disconnect option
		if (HAL.BtnA_isPressed()) {
			btnA_pressed_counter++;
			if (btnA_pressed_counter == 8) {
				HAL.Wlan.disconnectWlan(false, false);
				HAL.println("[WARN] WLAN manual disconnected OK!");

				HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
				HAL.Display.fillEllipse(285, 40, 5, 5, TFT_YELLOW);
				HAL.Display.giveSemaphoreMutex();
			}
		} else {
			if (btnA_pressed_counter >= 8) {
				HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
				HAL.Display.fillEllipse(285, 40, 5, 5, TFT_BLACK);
				HAL.Display.giveSemaphoreMutex();

				drawNetworkStatus(render);
			}
			btnA_pressed_counter = 0;
		}

		if (g_display_page == MAIN_PAGE_INFO_2) {
			drawInformation2(render, true);
		}

		break;
	}
	case DisplayTiming::EVERY_HALF_MINUTES_EVENT: {
		// Update battery status
		HAL.printf("Battery Level: %d %%\n", HAL.getBatteryLevel());

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		Icon_BatteryStatus(275, 5, HAL.getBatteryLevel());
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

		if (not HAL.Tf.ready()) {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			Icon_TfCardStatus(225, 4, TfCardStatusT::UNMOUNTED);
			HAL.Display.giveSemaphoreMutex();
		} else {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			Icon_TfCardStatus(225, 4, (HAL.Log.getCurrentLoggingState() ? TfCardStatusT::LOGGING : TfCardStatusT::NO_LOGGING));
			HAL.Display.giveSemaphoreMutex();
		}

		break;
	}
	case DisplayTiming::BTNC_PRESSED: {
		g_display_page = ((g_display_page + 1 == _MAIN_PAGE_SENTINEL) ? MAIN_PAGE_SENSOR_1 : static_cast<_DisplayPage>(g_display_page + 1));

		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		HAL.Display.fillRect(0, 0, 220, 29, MY_DARKGRAY); // Clear top bar
		HAL.Display.giveSemaphoreMutex();

		const bool FORCE_UPDATE = true;

		switch (g_display_page) {
		case MAIN_PAGE_SENSOR_1: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 29, 320, 182, TFT_BLACK); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawLeftPanelFrame();
			drawLastUpdateDate(render, g_last_timeinfo, FORCE_UPDATE);
			drawFeatures2LeftPanel(render);

			if (g_sensor_num == 0) {
				HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
				render->setFontSize(40);
				render->setCursor(300, 90); // set right cursor
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
				case _SensorType::ERROR:
					updateSensorDisplay_Error(render,
					                          g_display_memory.name.c_str());
					break;
				case _SensorType::CAMERA:
					cb_UpdateCameraValue_Default(render,
					                             g_display_memory.name.c_str(),
					                             true);
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
			HAL.Display.fillRect(0, 29, 320, 182, TFT_BLACK); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawInformation(render);

			break;
		}
		case MAIN_PAGE_INFO_2: {
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
			HAL.Display.fillRect(0, 29, 320, 182, TFT_BLACK); // Clear center area
			HAL.Display.giveSemaphoreMutex();

			drawInformation2(render);

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
		render->setCursor(HAL.Display.width() / 2, 30);
		render->cprintf("Waiting 00 sec");
		fillArc(160, 140, 50, 67, -90 + (6 * current_sec), 270, TFT_ORANGE);
	}
	// clear last value
	render->setFontColor(TFT_BLACK);
	render->setFontSize(40);
	render->setCursor(138, 110);
	render->printf("%02d", 60 - current_sec + 1);

	render->setFontColor(TFT_WHITE);
	render->setCursor(138, 110);
	render->printf("%02d", 60 - current_sec);

	fillArc(160, 140, 50, 67, -90, -90 + (6 * current_sec), TFT_BLACK);
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

void cb_UpdateCameraValue_Default(std::unique_ptr<OpenFontRender> &render, const char *sensor_name, bool selected) {
	if (selected == false) {
		return;
	}

	g_display_memory.name = String(sensor_name);
	g_display_memory.type = _SensorType::CAMERA;

	if (g_display_page >= MAIN_PAGE_INFO_1) {
		return;
	}

	drawSensorName(render, sensor_name);

	// Resize image
	HAL.takeSemaphoreMutex("camera", portMAX_DELAY);
	HAL.Camera.resizeCapturedImage(230, 135);

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	uint32_t scaledWidth  = HAL.Camera.scaledWidth();
	uint32_t scaledHeight = HAL.Camera.scaledHeight();

	// Draw image to Display
	HAL.Display.pushImage(105, 70,
	                      scaledWidth,
	                      scaledHeight,
	                      (uint16_t *)(&HAL.Camera.scaledImage[0]));

	HAL.giveSemaphoreMutex("camera");

	// Fill both side
	HAL.Display.fillRect(105 + scaledWidth,
	                     70,
	                     HAL.Display.width() - (105 + scaledWidth),
	                     scaledHeight,
	                     TFT_BLACK);

	HAL.Display.fillRect(70,
	                     70,
	                     105 - 70,
	                     scaledHeight,
	                     TFT_BLACK);

	// HAL.Display.pushImage(75, 70,
	//                   HAL.Camera.capturedWidth(),
	//                   HAL.Camera.capturedHeight(),
	//                   (uint16_t *)(&HAL.Camera.capturedImage[0]));

	HAL.Display.giveSemaphoreMutex();

	drawSensorUnit(render, "   ");
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Sampling Update Callbacks
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void cb_SamplingGpioSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
}

void cb_SamplingI2cSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
}

void cb_SamplingModbusSensors_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
}

void cb_SamplingCameraSensor_Default(std::unique_ptr<OpenFontRender> &render, struct tm timeinfo) {
	g_last_timeinfo = timeinfo;
	if (g_display_page != MAIN_PAGE_SENSOR_1) {
		return;
	}
	drawLastUpdateDate(render, timeinfo);
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Buffer Update Callbacks
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void cb_FtpBufferUpdate_Default(std::unique_ptr<OpenFontRender> &render, int remain_data_quantity) {
	g_last_remain_data.ftp = remain_data_quantity;
	if (g_display_page != MAIN_PAGE_SENSOR_2) {
		return;
	}

	uint16_t _panel_color = g_features.ftp ? TFT_DARKGREEN : MY_CRIMSON;

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	render->setFontSize(25);
	render->setCursor(60, 30 + 5);
	HAL.Display.fillRect(2, 31, 70 - 4, 43, _panel_color);
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

	render->setFontSize(25);
	render->setCursor(60, 30 + 45 + 5);
	HAL.Display.fillRect(2, 31 + 45, 70 - 4, 43, _panel_color);
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

	render->setFontSize(25);
	render->setCursor(60, 30 + 90 + 5);
	HAL.Display.fillRect(2, 31 + 90, 70 - 4, 43, _panel_color);
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

	render->setFontSize(25);
	render->setCursor(60, 30 + 135 + 5);
	HAL.Display.fillRect(2, 31 + 135, 70 - 4, 43, _panel_color);
	render->setFontColor(TFT_WHITE, _panel_color);
	render->rprintf("%d", remain_data_quantity);
	render->setFontColor(TFT_WHITE, TFT_BLACK);

	HAL.Display.giveSemaphoreMutex();
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Status Update Callbacks
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

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
// Const global variables for icon image
// MARK: Icons
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

const uint16_t BluetoothIconWidth         = 78;
const uint16_t BluetoothIconHeight        = 78;
const uint16_t BluetoothIcon_RLE_RunCount = 640;

// Icon: Bluetooth icon (C) Terence Eden
//       https://icon-icons.com/icon/bluetooth/130975
// License: CC BY 4.0
// clang-format off
const uint16_t BluetoothIcon_Palette[5] PROGMEM = {
    0x041F, 0x557F, 0xD75F, 0xA69F, 0xFFFF,
};

// パレット化・行RLE圧縮アイコンデータ。
// Palette: 使用色一覧(RGB565)。RLE_Packed: ニブルパッキングされたラン列。
// フォーマット詳細は drawRLEIcon() のコメント参照。ロスレス(元PNG基準)。
const uint8_t BluetoothIcon_RLE_Packed[783] PROGMEM = {
    0x80, 0x4E, 0x80, 0x4E, 0x89, 0x00, 0x3C, 0x89, 0x87, 0x00, 0x40, 0x87, 0x86, 0x00, 0x42, 0x86, 0x85, 0x00, 0x44, 0x85,
    0x84, 0x00, 0x46, 0x84, 0x83, 0x00, 0x21, 0x11, 0x00, 0x26, 0x83, 0x83, 0x00, 0x21, 0x21, 0x31, 0x00, 0x25, 0x83, 0x82,
    0x00, 0x22, 0x31, 0x41, 0x11, 0x00, 0x25, 0x82, 0x82, 0x00, 0x22, 0x31, 0x42, 0x11, 0x00, 0x24, 0x82, 0x82, 0x00, 0x22,
    0x31, 0x43, 0x11, 0x00, 0x23, 0x82, 0x82, 0x00, 0x22, 0x31, 0x44, 0x11, 0x00, 0x22, 0x82, 0x82, 0x00, 0x22, 0x31, 0x45,
    0x11, 0x00, 0x21, 0x82, 0x82, 0x00, 0x22, 0x31, 0x46, 0x11, 0x00, 0x20, 0x82, 0x82, 0x00, 0x22, 0x31, 0x47, 0x00, 0x20,
    0x82, 0x82, 0x00, 0x22, 0x31, 0x47, 0x21, 0x00, 0x1F, 0x82, 0x82, 0x00, 0x22, 0x31, 0x48, 0x21, 0x00, 0x1E, 0x82, 0x82,
    0x00, 0x22, 0x31, 0x49, 0x21, 0x00, 0x1D, 0x82, 0x82, 0x00, 0x22, 0x31, 0x44, 0x21, 0x45, 0x21, 0x00, 0x1C, 0x82, 0x82,
    0x00, 0x22, 0x31, 0x44, 0x01, 0x31, 0x45, 0x31, 0x00, 0x1B, 0x82, 0x82, 0x00, 0x22, 0x31, 0x44, 0x02, 0x21, 0x45, 0x31,
    0x00, 0x1A, 0x82, 0x82, 0x00, 0x17, 0x11, 0x0A, 0x31, 0x44, 0x03, 0x21, 0x45, 0x31, 0x00, 0x19, 0x82, 0x82, 0x00, 0x16,
    0x11, 0x41, 0x11, 0x09, 0x31, 0x44, 0x04, 0x21, 0x45, 0x31, 0x00, 0x18, 0x82, 0x82, 0x00, 0x15, 0x11, 0x43, 0x11, 0x08,
    0x31, 0x44, 0x05, 0x46, 0x11, 0x00, 0x17, 0x82, 0x82, 0x00, 0x14, 0x11, 0x45, 0x11, 0x07, 0x31, 0x44, 0x05, 0x11, 0x46,
    0x11, 0x00, 0x16, 0x82, 0x82, 0x00, 0x14, 0x11, 0x46, 0x11, 0x06, 0x31, 0x44, 0x06, 0x11, 0x46, 0x11, 0x00, 0x15, 0x82,
    0x82, 0x00, 0x15, 0x11, 0x46, 0x11, 0x05, 0x31, 0x44, 0x06, 0x31, 0x45, 0x21, 0x00, 0x16, 0x82, 0x82, 0x00, 0x16, 0x11,
    0x46, 0x11, 0x04, 0x31, 0x44, 0x05, 0x21, 0x45, 0x21, 0x00, 0x17, 0x82, 0x82, 0x00, 0x17, 0x11, 0x46, 0x11, 0x03, 0x31,
    0x44, 0x04, 0x21, 0x45, 0x21, 0x00, 0x18, 0x82, 0x82, 0x00, 0x18, 0x11, 0x46, 0x11, 0x02, 0x31, 0x44, 0x03, 0x21, 0x45,
    0x21, 0x00, 0x19, 0x82, 0x82, 0x00, 0x19, 0x11, 0x46, 0x11, 0x01, 0x31, 0x44, 0x02, 0x21, 0x45, 0x21, 0x00, 0x1A, 0x82,
    0x82, 0x00, 0x1A, 0x11, 0x46, 0x11, 0x31, 0x44, 0x01, 0x31, 0x45, 0x21, 0x00, 0x1B, 0x82, 0x82, 0x00, 0x1B, 0x11, 0x4B,
    0x21, 0x45, 0x21, 0x00, 0x1C, 0x82, 0x82, 0x00, 0x1C, 0x11, 0x4F, 0x21, 0x00, 0x1D, 0x82, 0x82, 0x00, 0x1D, 0x11, 0x4D,
    0x21, 0x00, 0x1E, 0x82, 0x82, 0x00, 0x1E, 0x11, 0x4B, 0x21, 0x00, 0x1F, 0x82, 0x82, 0x00, 0x1F, 0x11, 0x49, 0x21, 0x00,
    0x20, 0x82, 0x82, 0x00, 0x20, 0x11, 0x47, 0x21, 0x00, 0x21, 0x82, 0x82, 0x00, 0x21, 0x21, 0x46, 0x11, 0x00, 0x21, 0x82,
    0x82, 0x00, 0x20, 0x31, 0x48, 0x11, 0x00, 0x20, 0x82, 0x82, 0x00, 0x1F, 0x31, 0x4A, 0x11, 0x00, 0x1F, 0x82, 0x82, 0x00,
    0x1E, 0x31, 0x4C, 0x11, 0x00, 0x1E, 0x82, 0x82, 0x00, 0x1D, 0x31, 0x4E, 0x11, 0x00, 0x1D, 0x82, 0x82, 0x00, 0x1C, 0x31,
    0x40, 0x10, 0x11, 0x00, 0x1C, 0x82, 0x82, 0x00, 0x1B, 0x31, 0x45, 0x22, 0x44, 0x31, 0x46, 0x11, 0x00, 0x1B, 0x82, 0x82,
    0x00, 0x1A, 0x31, 0x45, 0x21, 0x01, 0x31, 0x44, 0x01, 0x11, 0x46, 0x11, 0x00, 0x1A, 0x82, 0x82, 0x00, 0x19, 0x31, 0x46,
    0x02, 0x31, 0x44, 0x02, 0x11, 0x46, 0x11, 0x00, 0x19, 0x82, 0x82, 0x00, 0x18, 0x31, 0x46, 0x11, 0x02, 0x31, 0x44, 0x03,
    0x11, 0x46, 0x11, 0x00, 0x18, 0x82, 0x82, 0x00, 0x17, 0x31, 0x46, 0x11, 0x03, 0x31, 0x44, 0x04, 0x11, 0x46, 0x11, 0x00,
    0x17, 0x82, 0x82, 0x00, 0x16, 0x31, 0x46, 0x11, 0x04, 0x31, 0x44, 0x05, 0x11, 0x46, 0x11, 0x00, 0x16, 0x82, 0x82, 0x00,
    0x15, 0x31, 0x46, 0x11, 0x05, 0x31, 0x44, 0x06, 0x11, 0x46, 0x11, 0x00, 0x15, 0x82, 0x82, 0x00, 0x14, 0x31, 0x46, 0x11,
    0x06, 0x31, 0x44, 0x06, 0x31, 0x45, 0x21, 0x11, 0x00, 0x15, 0x82, 0x82, 0x00, 0x14, 0x11, 0x45, 0x11, 0x07, 0x31, 0x44,
    0x05, 0x31, 0x45, 0x21, 0x00, 0x17, 0x82, 0x82, 0x00, 0x15, 0x11, 0x43, 0x11, 0x08, 0x31, 0x44, 0x04, 0x31, 0x45, 0x21,
    0x00, 0x18, 0x82, 0x82, 0x00, 0x16, 0x11, 0x21, 0x11, 0x09, 0x31, 0x44, 0x03, 0x31, 0x46, 0x11, 0x00, 0x18, 0x82, 0x82,
    0x00, 0x22, 0x31, 0x44, 0x02, 0x31, 0x46, 0x11, 0x00, 0x19, 0x82, 0x82, 0x00, 0x22, 0x31, 0x44, 0x01, 0x11, 0x46, 0x11,
    0x00, 0x1A, 0x82, 0x82, 0x00, 0x22, 0x31, 0x44, 0x31, 0x46, 0x11, 0x00, 0x1B, 0x82, 0x82, 0x00, 0x22, 0x31, 0x4A, 0x11,
    0x00, 0x1C, 0x82, 0x82, 0x00, 0x22, 0x31, 0x49, 0x11, 0x00, 0x1D, 0x82, 0x82, 0x00, 0x22, 0x31, 0x48, 0x11, 0x00, 0x1E,
    0x82, 0x82, 0x00, 0x22, 0x31, 0x47, 0x11, 0x00, 0x1F, 0x82, 0x82, 0x00, 0x22, 0x31, 0x46, 0x11, 0x00, 0x20, 0x82, 0x82,
    0x00, 0x22, 0x31, 0x45, 0x11, 0x00, 0x21, 0x82, 0x82, 0x00, 0x22, 0x31, 0x44, 0x11, 0x00, 0x22, 0x82, 0x82, 0x00, 0x22,
    0x31, 0x43, 0x11, 0x00, 0x23, 0x82, 0x82, 0x00, 0x22, 0x31, 0x42, 0x31, 0x00, 0x24, 0x82, 0x82, 0x00, 0x22, 0x31, 0x41,
    0x31, 0x00, 0x25, 0x82, 0x83, 0x00, 0x21, 0x21, 0x31, 0x00, 0x25, 0x83, 0x83, 0x00, 0x21, 0x11, 0x00, 0x26, 0x83, 0x84,
    0x00, 0x46, 0x84, 0x85, 0x00, 0x44, 0x85, 0x86, 0x00, 0x42, 0x86, 0x87, 0x00, 0x40, 0x87, 0x89, 0x00, 0x3C, 0x89, 0x80,
    0x4E, 0x80, 0x4E,
};
// clang-format on

const uint16_t DynamicspidIconWidth         = 78;
const uint16_t DynamicspidIconHeight        = 78;
const uint16_t DynamicspidIcon_RLE_RunCount = 904;

// clang-format off
const uint16_t DynamicspidIcon_Palette[5] PROGMEM = {
    0xE640, 0xA463, 0xCDA1, 0x39C7, 0x6B05,
};

// パレット化・行RLE圧縮アイコンデータ。
// Palette: 使用色一覧(RGB565)。RLE_Packed: ニブルパッキングされたラン列。
// フォーマット詳細は drawRLEIcon() のコメント参照。ロスレス(元PNG基準)。
const uint8_t DynamicspidIcon_RLE_Packed[980] PROGMEM = {
    0x80, 0x4E, 0x80, 0x4E, 0x80, 0x4E, 0x80, 0x4E, 0x86, 0x00, 0x42, 0x86, 0x85, 0x00, 0x44, 0x85, 0x84, 0x00, 0x45, 0x85,
    0x84, 0x04, 0x10, 0x3D, 0x21, 0x04, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3D, 0x41, 0x04, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E,
    0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84,
    0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E,
    0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84,
    0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x41,
    0x14, 0x43, 0x30, 0x29, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x21, 0x06, 0x22, 0x12, 0x41, 0x30, 0x25, 0x21, 0x03,
    0x84, 0x84, 0x03, 0x11, 0x3C, 0x41, 0x21, 0x0B, 0x21, 0x11, 0x41, 0x30, 0x22, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D,
    0x11, 0x0E, 0x21, 0x11, 0x41, 0x30, 0x1F, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3E, 0x43, 0x13, 0x22, 0x08, 0x21, 0x11,
    0x30, 0x1E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x15, 0x42, 0x11, 0x22, 0x06, 0x21, 0x41, 0x30, 0x1C, 0x21, 0x03,
    0x84, 0x84, 0x03, 0x11, 0x30, 0x19, 0x41, 0x11, 0x21, 0x06, 0x11, 0x41, 0x30, 0x1A, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11,
    0x30, 0x1B, 0x41, 0x11, 0x21, 0x05, 0x21, 0x41, 0x30, 0x19, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x1E, 0x11, 0x21,
    0x04, 0x21, 0x41, 0x30, 0x18, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x1F, 0x41, 0x11, 0x05, 0x11, 0x30, 0x17, 0x21,
    0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x21, 0x11, 0x21, 0x04, 0x11, 0x30, 0x16, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3E,
    0x45, 0x3F, 0x41, 0x21, 0x04, 0x11, 0x30, 0x15, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x11, 0x04, 0x22, 0x12, 0x41,
    0x3C, 0x41, 0x21, 0x04, 0x11, 0x30, 0x14, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3C, 0x41, 0x21, 0x08, 0x21, 0x11, 0x41,
    0x3B, 0x41, 0x21, 0x04, 0x41, 0x30, 0x13, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x21, 0x0B, 0x21, 0x41, 0x3A, 0x41,
    0x21, 0x03, 0x21, 0x41, 0x30, 0x12, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x41, 0x13, 0x22, 0x08, 0x11, 0x41, 0x39,
    0x41, 0x21, 0x03, 0x21, 0x30, 0x12, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x12, 0x42, 0x11, 0x22, 0x05, 0x21, 0x11,
    0x39, 0x41, 0x04, 0x11, 0x30, 0x11, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x16, 0x41, 0x11, 0x21, 0x05, 0x11, 0x39,
    0x11, 0x03, 0x21, 0x41, 0x30, 0x10, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x18, 0x41, 0x11, 0x05, 0x11, 0x39, 0x21,
    0x03, 0x11, 0x30, 0x10, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x1A, 0x11, 0x21, 0x04, 0x11, 0x38, 0x41, 0x04, 0x41,
    0x3F, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x1B, 0x41, 0x21, 0x04, 0x11, 0x38, 0x11, 0x03, 0x11, 0x3F, 0x21, 0x03,
    0x84, 0x84, 0x03, 0x11, 0x30, 0x1C, 0x41, 0x21, 0x04, 0x41, 0x37, 0x41, 0x21, 0x03, 0x41, 0x3E, 0x21, 0x03, 0x84, 0x84,
    0x03, 0x11, 0x3E, 0x44, 0x3B, 0x41, 0x21, 0x03, 0x21, 0x41, 0x37, 0x11, 0x03, 0x11, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03,
    0x11, 0x3D, 0x11, 0x21, 0x02, 0x22, 0x11, 0x41, 0x39, 0x41, 0x21, 0x03, 0x11, 0x37, 0x41, 0x03, 0x21, 0x41, 0x3D, 0x21,
    0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x21, 0x06, 0x21, 0x11, 0x39, 0x11, 0x04, 0x41, 0x37, 0x11, 0x03, 0x11, 0x3D, 0x21,
    0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x11, 0x08, 0x21, 0x41, 0x38, 0x21, 0x03, 0x11, 0x37, 0x41, 0x03, 0x11, 0x3D, 0x21,
    0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x41, 0x13, 0x21, 0x05, 0x21, 0x41, 0x37, 0x41, 0x03, 0x21, 0x41, 0x36, 0x41, 0x21,
    0x02, 0x21, 0x41, 0x3C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x12, 0x41, 0x21, 0x04, 0x21, 0x41, 0x37, 0x21, 0x03,
    0x11, 0x37, 0x11, 0x03, 0x41, 0x3C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x14, 0x11, 0x21, 0x03, 0x11, 0x37, 0x41,
    0x03, 0x21, 0x37, 0x11, 0x03, 0x11, 0x3C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x15, 0x41, 0x04, 0x41, 0x36, 0x41,
    0x21, 0x03, 0x41, 0x36, 0x41, 0x03, 0x11, 0x3C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x16, 0x11, 0x03, 0x21, 0x37,
    0x11, 0x03, 0x11, 0x36, 0x41, 0x21, 0x02, 0x21, 0x3C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x16, 0x41, 0x21, 0x03,
    0x41, 0x36, 0x11, 0x03, 0x11, 0x37, 0x21, 0x02, 0x21, 0x41, 0x3B, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3D, 0x42, 0x38,
    0x11, 0x03, 0x11, 0x36, 0x41, 0x03, 0x21, 0x37, 0x21, 0x03, 0x41, 0x3B, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3B, 0x41,
    0x23, 0x11, 0x41, 0x36, 0x41, 0x03, 0x11, 0x36, 0x41, 0x03, 0x21, 0x37, 0x11, 0x03, 0x41, 0x3B, 0x21, 0x03, 0x84, 0x84,
    0x03, 0x11, 0x3A, 0x41, 0x21, 0x04, 0x21, 0x41, 0x35, 0x41, 0x21, 0x02, 0x21, 0x36, 0x41, 0x21, 0x02, 0x21, 0x37, 0x11,
    0x03, 0x41, 0x3B, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3A, 0x11, 0x06, 0x11, 0x36, 0x21, 0x02, 0x21, 0x36, 0x41, 0x21,
    0x02, 0x21, 0x37, 0x11, 0x03, 0x41, 0x3B, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3A, 0x21, 0x06, 0x11, 0x36, 0x11, 0x02,
    0x11, 0x37, 0x21, 0x02, 0x11, 0x37, 0x11, 0x02, 0x21, 0x41, 0x3B, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3A, 0x21, 0x06,
    0x11, 0x36, 0x41, 0x12, 0x41, 0x37, 0x41, 0x12, 0x41, 0x38, 0x12, 0x41, 0x3C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3A,
    0x11, 0x06, 0x41, 0x30, 0x2C, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x3B, 0x11, 0x04, 0x11, 0x30, 0x2D, 0x21, 0x03, 0x84,
    0x84, 0x03, 0x11, 0x3C, 0x41, 0x12, 0x41, 0x30, 0x2E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84,
    0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30,
    0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84,
    0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30, 0x3E, 0x21, 0x03, 0x84, 0x84, 0x03, 0x11, 0x30,
    0x3D, 0x41, 0x21, 0x03, 0x84, 0x84, 0x03, 0x21, 0x41, 0x30, 0x3C, 0x41, 0x04, 0x84, 0x84, 0x04, 0x20, 0x3D, 0x05, 0x84,
    0x85, 0x00, 0x44, 0x85, 0x85, 0x00, 0x43, 0x86, 0x87, 0x00, 0x40, 0x87, 0x80, 0x4E, 0x80, 0x4E, 0x80, 0x4E, 0x80, 0x4E,
};
// clang-format on

#endif /* DISPLAY_320X240 */
#endif /* USER_DISPLAY_DEFAULT_H */
