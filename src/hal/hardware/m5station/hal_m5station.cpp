#include "hal_m5station.h"
#ifdef HAL_API_M5STATION_H

HAL_API HAL;

#define TIM_0 (0)
#define TIM_1 (1)
#define TIM_2 (2)
#define TIM_3 (3)

// Variables (timer)
hw_timer_t *timer0 = NULL;
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;
hw_timer_t *timer3 = NULL;
volatile SemaphoreHandle_t timer0_Semaphore;
volatile SemaphoreHandle_t timer1_Semaphore;
volatile SemaphoreHandle_t timer2_Semaphore;
volatile SemaphoreHandle_t timer3_Semaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR onTimer0();
void IRAM_ATTR onTimer1();
void IRAM_ATTR onTimer2();
void IRAM_ATTR onTimer3();

struct Counter {
	uint32_t limit;
	uint32_t current;
};
std::unordered_map<std::string, struct Counter> sub_timer0;
std::unordered_map<std::string, struct Counter> sub_timer1;
std::unordered_map<std::string, struct Counter> sub_timer2;
std::unordered_map<std::string, struct Counter> sub_timer3;

// Variables (BLE)
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_API::begin() {
	auto cfg = M5.config();

	cfg.serial_baudrate = 115200;

	cfg.clear_display = true;  // default=true. clear the screen when begin.
	cfg.output_power  = true;  // default=true. use external port 5V output.
	cfg.internal_imu  = true;  // default=true. use internal IMU.
	cfg.internal_rtc  = true;  // default=true. use internal RTC.
	cfg.internal_spk  = false; // default=true. use internal speaker.
	cfg.internal_mic  = false; // default=true. use internal microphone.

	// begin M5Unified.
	M5.begin(cfg);
	// Wire.begin();
	M5.Display.init();

	HAL.printf("Board Manager Version: %d.%d.%d\n",
	           ESP_ARDUINO_VERSION_MAJOR,
	           ESP_ARDUINO_VERSION_MINOR,
	           ESP_ARDUINO_VERSION_PATCH);

	HAL.Info.DEVICE_NAME = "M5Station";
}

void HAL_API::update() {
	M5.update();
	if (M5.BtnPWR.wasClicked()) {
		reboot();
	}
}

uint8_t HAL_API::waitForButtonPressed(uint16_t wait_ms) {
	uint8_t ret           = (uint8_t)PRESSED_BUTTON::NONE;
	bool all_free_flag    = false;
	unsigned long long ts = millis();

	while ((millis() - ts) < wait_ms) {
		if (not all_free_flag) {
			HAL.update();
			if ((not M5.BtnA.isPressed()) and (not M5.BtnC.isPressed())) {
				all_free_flag = true;
			}
		} else {
			HAL.update();
			if (M5.BtnA.isPressed()) {
				ret |= (uint8_t)PRESSED_BUTTON::PRESSED_BTN1;
			}
			HAL.update();
			if (M5.BtnC.isPressed()) {
				ret |= (uint8_t)PRESSED_BUTTON::PRESSED_BTN2;
			}

			if (ret != (uint8_t)PRESSED_BUTTON::NONE) {
				return ret;
			}
		}

		delay(5);
	}
	return ret;
}

uint8_t HAL_API::BtnA_wasPressed() {
	return M5.BtnA.wasPressed();
}

uint8_t HAL_API::BtnB_wasPressed() {
	return M5.BtnB.wasPressed();
}

uint8_t HAL_API::BtnC_wasPressed() {
	return M5.BtnC.wasPressed();
}

uint8_t HAL_API::BtnA_isPressed() {
	return M5.BtnA.isPressed();
}

uint8_t HAL_API::BtnB_isPressed() {
	return M5.BtnB.isPressed();
}

uint8_t HAL_API::BtnC_isPressed() {
	return M5.BtnC.isPressed();
}

void HAL_API::endSetup() {
	// BT の停止とメモリ解放
	::esp_bt_controller_disable();
	::esp_bt_controller_deinit();
	::esp_bt_mem_release(ESP_BT_MODE_BTDM);

	// WatchDog Timer の再初期化
	constexpr uint16_t WDT_TIMEOUT_SEC = 120;
	esp_task_wdt_config_t wdt_config   = {
	      .timeout_ms     = WDT_TIMEOUT_SEC * 1000,
	      .idle_core_mask = 0, // Bitmask of cores
	      .trigger_panic  = true};
	::esp_task_wdt_deinit();
	::esp_task_wdt_init(&wdt_config);

	::enableLoopWDT(); // Loop タスクの WatchDog Timer を有効化
}

int8_t HAL_API::getBatteryLevel() {
	if (M5.Power.isCharging() == m5::Power_Class::is_charging) {
		return 100;
	}
	return M5.Power.getBatteryLevel(); // return 0% - 100%
}

uint32_t HAL_API::getFreeHeapSize() {
	return xPortGetFreeHeapSize();
}

uint32_t HAL_API::getMinimumFreeHeapSize() {
	return xPortGetMinimumEverFreeHeapSize();
}

uint32_t HAL_API::getRandomNumber(uint32_t minimum, uint32_t maximum) {
	constexpr uint32_t DR_REG_RNG_BASE = 0x3ff75144;
	if (minimum == maximum) {
		return minimum;
	}

	if (minimum > maximum) {
		std::swap(minimum, maximum);
	}

	return (READ_PERI_REG(DR_REG_RNG_BASE) % (maximum - minimum)) + minimum;
}

String HAL_API::getChipUUID(bool include_prefix) {
	String prefix = "";
	if (include_prefix) {
		prefix = "ESP32-";
	}

	uint64_t chipid  = ESP.getEfuseMac();
	char bt_name[20] = {0};
	snprintf(bt_name, 20, "%llX", chipid);
	return prefix + String(bt_name);
}

void HAL_API::delay(uint32_t ms) {
	//::esp_task_wdt_reset();
	::delay(ms);
}

void HAL_API::reboot() {
	ESP.restart();
}

bool HAL_API::initializeSemaphoreMutex(String key) {
	if (_semaphore_mutex.find(key) != _semaphore_mutex.end()) {
		// Already initialized
		return true;
	}

	_semaphore_mutex[key] = NULL;
	_semaphore_mutex[key] = xSemaphoreCreateMutex();
	//_semaphore_mutex[key] = xSemaphoreCreateRecursiveMutex();
	return ((_semaphore_mutex[key] == NULL) ? false : true);
}

bool HAL_API::takeSemaphoreMutex(String key, uint32_t wait_tick_time) {
	if (_semaphore_mutex.find(key) == _semaphore_mutex.end()) {
		// Not exist
		return true;
	}

	if (_semaphore_mutex[key] == NULL) {
		// Not initialized
		return true;
	}

	return ((xSemaphoreTake(_semaphore_mutex[key], wait_tick_time) == pdTRUE) ? true : false);
	// return ((xSemaphoreTakeRecursive(_semaphore_mutex[key], wait_tick_time) == pdTRUE) ? true : false);
}

bool HAL_API::giveSemaphoreMutex(String key) {
	if (_semaphore_mutex.find(key) == _semaphore_mutex.end()) {
		// Not exist
		return true;
	}

	if (_semaphore_mutex[key] == NULL) {
		// Not initialized
		return true;
	}

	return ((xSemaphoreGive(_semaphore_mutex[key]) == pdTRUE) ? true : false);
	// return ((xSemaphoreGiveRecursive(_semaphore_mutex[key]) == pdTRUE) ? true : false);
}

void HAL_API::periodicLoopTaskDebugMessage() {
	HAL.printf("Free Memory: %6d\n", xPortGetFreeHeapSize());
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_Timer_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void IRAM_ATTR onTimer0() {
	// Increment the counter and set the time of ISR
	portENTER_CRITICAL_ISR(&timerMux);

	for (auto &pair : sub_timer0) {
		pair.second.current++;
	}

	portEXIT_CRITICAL_ISR(&timerMux);
	// Give a semaphore that we can check in the loop
	xSemaphoreGiveFromISR(timer0_Semaphore, NULL);
	// It is safe to use digitalRead/Write here if you want to toggle an output
}

void IRAM_ATTR onTimer1() {
	// Increment the counter and set the time of ISR
	portENTER_CRITICAL_ISR(&timerMux);

	for (auto &pair : sub_timer1) {
		pair.second.current++;
	}

	portEXIT_CRITICAL_ISR(&timerMux);
	// Give a semaphore that we can check in the loop
	xSemaphoreGiveFromISR(timer1_Semaphore, NULL);
	// It is safe to use digitalRead/Write here if you want to toggle an output
}

void IRAM_ATTR onTimer2() {
	// Increment the counter and set the time of ISR
	portENTER_CRITICAL_ISR(&timerMux);

	for (auto &pair : sub_timer2) {
		pair.second.current++;
	}

	portEXIT_CRITICAL_ISR(&timerMux);
	// Give a semaphore that we can check in the loop
	xSemaphoreGiveFromISR(timer2_Semaphore, NULL);
	// It is safe to use digitalRead/Write here if you want to toggle an output
}

void IRAM_ATTR onTimer3() {
	// Increment the counter and set the time of ISR
	portENTER_CRITICAL_ISR(&timerMux);

	for (auto &pair : sub_timer3) {
		pair.second.current++;
	}

	portEXIT_CRITICAL_ISR(&timerMux);
	// Give a semaphore that we can check in the loop
	xSemaphoreGiveFromISR(timer3_Semaphore, NULL);
	// It is safe to use digitalRead/Write here if you want to toggle an output
}

void HAL_Timer_API::begin(uint32_t interval_ms) {
	_interval_ms = interval_ms;
	switch (_timer_num) {
	case TIM_0:
		// Create semaphore to inform us when the timer has fired
		timer0_Semaphore = xSemaphoreCreateBinary();
		// timer, count 1 MHz (1 micro sec), autoReload
		timer0 = timerBegin(1000000);
		// timer, interrupt function, edge trigger
		timerAttachInterrupt(timer0, onTimer0);
		// timer, interrupt interval, autoReload
		timerAlarm(timer0, interval_ms * 1000, true, 0);
		break;
	case TIM_1:
		// Create semaphore to inform us when the timer has fired
		timer1_Semaphore = xSemaphoreCreateBinary();
		// timer, count 1 MHz (1 micro sec), autoReload
		timer1 = timerBegin(1000000);
		// timer, interrupt function, edge trigger
		timerAttachInterrupt(timer1, onTimer1);
		// timer, interrupt interval, autoReload
		timerAlarm(timer1, interval_ms * 1000, true, 0);
		break;
	case TIM_2:
		// Create semaphore to inform us when the timer has fired
		timer2_Semaphore = xSemaphoreCreateBinary();
		// timer, count 1 MHz (1 micro sec), autoReload
		timer2 = timerBegin(1000000);
		// timer, interrupt function, edge trigger
		timerAttachInterrupt(timer2, onTimer2);
		// timer, interrupt interval, autoReload
		timerAlarm(timer2, interval_ms * 1000, true, 0);
		break;
	case TIM_3:
		// Create semaphore to inform us when the timer has fired
		timer3_Semaphore = xSemaphoreCreateBinary();
		// timer, count 1 MHz (1 micro sec), autoReload
		timer3 = timerBegin(1000000);
		// timer, interrupt function, edge trigger
		timerAttachInterrupt(timer3, onTimer3);
		// timer, interrupt interval, autoReload
		timerAlarm(timer3, interval_ms * 1000, true, 0);
		break;
	default:
		return;
	}
}

void HAL_Timer_API::addSubTimer(std::string key, uint32_t count_limit, uint32_t initial_count) {
	portENTER_CRITICAL_ISR(&timerMux);
	switch (_timer_num) {
	case TIM_0:
		sub_timer0[key].limit   = count_limit;
		sub_timer0[key].current = initial_count;
		break;
	case TIM_1:
		sub_timer1[key].limit   = count_limit;
		sub_timer1[key].current = initial_count;
		break;
	case TIM_2:
		sub_timer2[key].limit   = count_limit;
		sub_timer2[key].current = initial_count;
		break;
	case TIM_3:
		sub_timer3[key].limit   = count_limit;
		sub_timer3[key].current = initial_count;
		break;
	default:
		break;
	}
	portEXIT_CRITICAL_ISR(&timerMux);
}

uint32_t HAL_Timer_API::getSubTimerLimit(std::string key) {
	uint32_t limit = 0;

	portENTER_CRITICAL_ISR(&timerMux);
	switch (_timer_num) {
	case TIM_0:
		limit = sub_timer0[key].limit;
		break;
	case TIM_1:
		limit = sub_timer1[key].limit;
		break;
	case TIM_2:
		limit = sub_timer2[key].limit;
		break;
	case TIM_3:
		limit = sub_timer3[key].limit;
		break;
	default:
		break;
	}
	portEXIT_CRITICAL_ISR(&timerMux);
	return limit;
}

uint32_t HAL_Timer_API::getSubTimerLimitMillis(std::string key) {
	return getSubTimerLimit(key) * _interval_ms;
}

void HAL_Timer_API::removeSubTimer(std::string key) {
	portENTER_CRITICAL_ISR(&timerMux);
	switch (_timer_num) {
	case TIM_0:
		sub_timer0.erase(key);
		break;
	case TIM_1:
		sub_timer1.erase(key);
		break;
	case TIM_2:
		sub_timer2.erase(key);
		break;
	case TIM_3:
		sub_timer3.erase(key);
		break;
	default:
		break;
	}
	portEXIT_CRITICAL_ISR(&timerMux);
}

bool HAL_Timer_API::checkSubTimer(std::string key, bool reset) {
	bool result = false;

	portENTER_CRITICAL_ISR(&timerMux);
	switch (_timer_num) {
	case TIM_0: {
		auto itr = sub_timer0.find(key);
		if (itr != sub_timer0.end()) {
			result = (sub_timer0[key].limit <= sub_timer0[key].current) ? true : false;
			if (result == true) {
				sub_timer0[key].current = (reset == true) ? 0 : sub_timer0[key].current;
			}
		}
		break;
	}
	case TIM_1: {
		auto itr = sub_timer1.find(key);
		if (itr != sub_timer1.end()) {
			result = (sub_timer1[key].limit <= sub_timer1[key].current) ? true : false;
			if (result == true) {
				sub_timer1[key].current = (reset == true) ? 0 : sub_timer1[key].current;
			}
		}
		break;
	}
	case TIM_2: {
		auto itr = sub_timer2.find(key);
		if (itr != sub_timer2.end()) {
			result = (sub_timer2[key].limit <= sub_timer2[key].current) ? true : false;
			if (result == true) {
				sub_timer2[key].current = (reset == true) ? 0 : sub_timer2[key].current;
			}
		}
		break;
	}
	case TIM_3: {
		auto itr = sub_timer3.find(key);
		if (itr != sub_timer3.end()) {
			result = (sub_timer3[key].limit <= sub_timer3[key].current) ? true : false;
			if (result == true) {
				sub_timer3[key].current = (reset == true) ? 0 : sub_timer3[key].current;
			}
		}
		break;
	}
	default:
		break;
	}
	portEXIT_CRITICAL_ISR(&timerMux);

	return result;
}

void HAL_Timer_API::start(bool enable_first_interrupt) {
	switch (_timer_num) {
	case TIM_0:
		if (timer0 == nullptr) {
			return;
		}
		timerRestart(timer0);
		timerStart(timer0); // Enable timer
		if (enable_first_interrupt) {
			xSemaphoreGive(timer0_Semaphore); // for first time interrupt
		}
		break;
	case TIM_1:
		if (timer1 == nullptr) {
			return;
		}
		timerRestart(timer1);
		timerStart(timer1); // Enable timer
		if (enable_first_interrupt) {
			xSemaphoreGive(timer1_Semaphore); // for first time interrupt
		}
		break;
	case TIM_2:
		if (timer2 == nullptr) {
			return;
		}
		timerRestart(timer2);
		timerStart(timer2); // Enable timer
		if (enable_first_interrupt) {
			xSemaphoreGive(timer2_Semaphore); // for first time interrupt
		}
		break;
	case TIM_3:
		if (timer3 == nullptr) {
			return;
		}
		timerRestart(timer3);
		timerStart(timer3); // Enable timer
		if (enable_first_interrupt) {
			xSemaphoreGive(timer3_Semaphore); // for first time interrupt
		}
		break;
	default:
		break;
	}
}

bool HAL_Timer_API::interrupted() {
	switch (_timer_num) {
	case TIM_0:
		if (timer0 == nullptr) {
			return false;
		}
		return (xSemaphoreTake(timer0_Semaphore, 0) == pdTRUE);
	case TIM_1:
		if (timer1 == nullptr) {
			return false;
		}
		return (xSemaphoreTake(timer1_Semaphore, 0) == pdTRUE);
	case TIM_2:
		if (timer2 == nullptr) {
			return false;
		}
		return (xSemaphoreTake(timer2_Semaphore, 0) == pdTRUE);
	case TIM_3:
		if (timer3 == nullptr) {
			return false;
		}
		return (xSemaphoreTake(timer3_Semaphore, 0) == pdTRUE);
	default:
		break;
	}
	return false;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_WLAN_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

int HAL_WLAN_API::getStatus() {
	return WiFi.status();
}

int HAL_WLAN_API::getRSSI() {
	if (WiFi.status() != WL_CONNECTED) {
		return -255;
	}
	return WiFi.RSSI();
}

std::vector<uint8_t> HAL_WLAN_API::fetchMacAddress() {
	uint8_t mac[6] = {0};
	esp_read_mac(mac, ESP_MAC_WIFI_STA);

	// HAL.printf("[WLAN] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\n",
	//            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return std::vector<uint8_t>(mac, mac + (sizeof(mac) / sizeof(mac[0])));
}

MyIPAddress HAL_WLAN_API::localIP() {
	IPAddress ip = WiFi.localIP();
	return MyIPAddress(ip[0], ip[1], ip[2], ip[3]);
}

bool HAL_WLAN_API::config(String ip, String gateway, String subnet, String dns1, String dns2) {
	IPAddress _ip;
	IPAddress _gateway;
	IPAddress _subnet;
	IPAddress _dns1;
	IPAddress _dns2;

	if (_ip.fromString(ip) == false) {
		return false;
	}
	if (_gateway.fromString(gateway) == false) {
		return false;
	}
	if (_subnet.fromString(subnet) == false) {
		return false;
	}

	if (_dns1.fromString(dns1) == false and _dns2.fromString(dns2) == false) {
		// dns option mode
		return WiFi.config(_ip, _gateway, _subnet);
	} else {
		return WiFi.config(_ip, _gateway, _subnet, _dns1, _dns2);
	}
}

bool HAL_WLAN_API::connectWlan(unsigned long timeout_ms) {
	return HAL_WLAN_API::connectWlan(_ap_name, _pass, timeout_ms);
}

bool HAL_WLAN_API::connectWlan(String ap_name, String pass, unsigned long timeout_ms) {
	if (WiFi.status() == WL_CONNECTED) {
		return true;
	}

	if (not _ap_name.equals(ap_name)) {
		_ap_name = ap_name;
	}
	if (not _pass.equals(pass)) {
		_pass = pass;
	}

	disconnectWlan(true, false);
	WiFi.mode(WIFI_STA);
	wl_status_t result = WiFi.status();
	WiFi.begin(_ap_name.c_str(), _pass.c_str());

	unsigned long start = millis();
	while (result != WL_CONNECTED and (millis() - start) < (timeout_ms)) {
		result = WiFi.status();
		delay(500);
	}

	if (result != WL_CONNECTED) {
		return false;
	}
	return true;
}

void HAL_WLAN_API::disconnectWlan(bool exit_sta_mode, bool erase_ap_settings) {
	WiFi.disconnect(exit_sta_mode, erase_ap_settings);
}

bool HAL_WLAN_API::detectTargetAccessPoint(String ap_name, int32_t *rssi, bool *exist_same_name, bool print_result) {
	std::map<int, String> result = scanAccessPoint(print_result);
	*exist_same_name             = false;

	bool ret            = false;
	bool is_first_match = true;
	for (const auto &kv : result) {
		if (ap_name.equals(kv.second)) {
			if (is_first_match) {
				*rssi          = WiFi.RSSI(kv.first);
				ret            = true;
				is_first_match = false;
			} else {
				*exist_same_name = true;
			}
		}
	}
	return ret;
}

MyIPAddress HAL_WLAN_API::resolveMDNS(String host, uint8_t try_count) {
	IPAddress invalid_ip(0, 0, 0, 0);
	if (host.endsWith(".local") == false) {
		return MyIPAddress(invalid_ip[0], invalid_ip[1], invalid_ip[2], invalid_ip[3]);
	}

	HAL.Wlan.connectWlan();

	String mdns_name = HAL.getChipUUID();

	if (not MDNS.begin(mdns_name.c_str())) {
		// Failed to begin MDNS server
		HAL.println("[mDNS] (Error) Failed to initialize mDNS responder");
		return MyIPAddress(invalid_ip[0], invalid_ip[1], invalid_ip[2], invalid_ip[3]);
	}

	// TODO: I think it is not perfect resolution
	host.replace(".local", "");
	while (try_count != 0) {
		IPAddress ip = MDNS.queryHost(host);
		if (invalid_ip != ip) {
			MDNS.end();
			return MyIPAddress(ip[0], ip[1], ip[2], ip[3]);
		} else {
			delay(250);
			try_count -= 1;
		}
	}

	MDNS.end();
	return MyIPAddress(invalid_ip[0], invalid_ip[1], invalid_ip[2], invalid_ip[3]);
}

std::map<int, String> HAL_WLAN_API::scanAccessPoint(bool print_result) {
	std::map<int, String> detected_nws;

	if (print_result) {
		HAL.println("==================== Scan WiFi result ====================");
		HAL.println("--------------------------------------------------");
		HAL.printf("| %-40s| RSSI |\n", "SSID");
		HAL.println("|-----------------------------------------+------|");
	}

	int n = WiFi.scanNetworks();
	for (int i = 0; i < n; i++) {
		detected_nws[i] = WiFi.SSID(i);
		if (print_result) {
			HAL.printf("| %-40s| %4d |\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
		}
	}

	if (print_result) {
		HAL.println("|-----------------------------------------+------|");
	}

	return detected_nws;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_NTP_RTC_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_NTP_RTC_API::begin() {
	constexpr uint32_t TIMEOUT_MS = 10000;
	struct tm timeinfo;
	bool result = false;

	for (int i = 0; i < servers.size(); i++) {
		configTzTime(time_zone.c_str(), servers[i].c_str());
		// delay(1000);
		if (not getLocalTime(&timeinfo, TIMEOUT_MS)) {
			// Failed to fetch
			continue;
		}
		selected_ntp_server = servers[i];
		result              = true;
		_inited             = true;
		break;
	}
	return result;
}

bool HAL_NTP_RTC_API::ready() {
	return _inited;
}

bool HAL_NTP_RTC_API::update() {
	struct tm timeinfo;
	bool result = false;
	configTzTime(time_zone.c_str(), selected_ntp_server.c_str());
	if (not getLocalTime(&timeinfo)) {
		return begin();
	}
	return true;
}

bool HAL_NTP_RTC_API::getCurrentTime(struct tm *timeinfo) {
	if (not getLocalTime(timeinfo)) {
		return false;
	}
	return true;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_SETTING_FILE_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_SETTING_FILE_IO_API::saveSettings(String fpath, JsonDocument json) {
	if (not SPIFFS.begin(true)) {
		HAL.println("Failed to begin SPIFFS.");
		return false;
	}

	String tmpl = "";
	serializeJson(json, tmpl);

	File f = SPIFFS.open(fpath.c_str(), "w");
	if (not f) {
		HAL.println("Open error.");
		SPIFFS.end();
		return false;
	}
	f.print(tmpl);
	f.close();
	SPIFFS.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::loadSettings(String fpath, JsonDocument &json) {
	if (not SPIFFS.begin(true)) {
		HAL.println("Failed to begin SPIFFS.");
		return false;
	}

	if (not SPIFFS.exists(fpath.c_str())) {
		HAL.println("File not exists.");
		SPIFFS.end();
		return false;
	}

	File f = SPIFFS.open(fpath.c_str(), "r");
	if (not f) {
		HAL.println("Open error.");
		SPIFFS.end();
		return false;
	}

	String doc = "";
	for (unsigned int i = 0; i < f.size(); i++) {
		doc += (char)f.read();
	}
	f.close();
	deserializeJson(json, doc.c_str());

	SPIFFS.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::formatStorage() {
	return SPIFFS.format();
}

JsonDocument HAL_SETTING_FILE_IO_API::deviceUniqueSettings() {

	JsonDocument doc;

	JsonObject SAMPLING   = doc["SAMPLING"].to<JsonObject>();
	SAMPLING["GPIO_MS"]   = "3000";
	SAMPLING["MODBUS_MS"] = "3000";

	String modbus1        = HAL.Modbus.getModifier() + String(PORT_MODBUS::PORT_1);
	JsonObject MODBUS1    = doc[modbus1.c_str()].to<JsonObject>();
	MODBUS1["BAUDRATE"]   = "9600";
	MODBUS1["DATA_BIT"]   = "8";
	MODBUS1["PARITY"]     = "0";
	MODBUS1["TIMEOUT_MS"] = "200";
	MODBUS1["ID_MIN"]     = "1";
	MODBUS1["ID_MAX"]     = "5";

	String modbus2        = HAL.Modbus.getModifier() + String(PORT_MODBUS::PORT_2);
	JsonObject MODBUS2    = doc[modbus2.c_str()].to<JsonObject>();
	MODBUS2["BAUDRATE"]   = "9600";
	MODBUS2["DATA_BIT"]   = "8";
	MODBUS2["PARITY"]     = "0";
	MODBUS2["TIMEOUT_MS"] = "200";
	MODBUS2["ID_MIN"]     = "1";
	MODBUS2["ID_MAX"]     = "5";

	String g25              = HAL.Gpio.getModifier() + "25";
	JsonObject G25          = doc[g25.c_str()].to<JsonObject>();
	G25["ENABLE"]           = "false";
	G25["NAME"]             = "G25";
	G25["TX_ON_CHANGE"]     = "false";
	G25["TX_LINK_I2C"]      = "false";
	G25["TX_LINK_MODBUS"]   = "false";
	G25["PULL_UP"]          = "false";
	G25["PULSE_COUNT_RISE"] = "false";
	G25["PULSE_COUNT_FALL"] = "false";

	String g26              = HAL.Gpio.getModifier() + "26";
	JsonObject G26          = doc[g26.c_str()].to<JsonObject>();
	G26["ENABLE"]           = "false";
	G26["NAME"]             = "G26";
	G26["TX_ON_CHANGE"]     = "false";
	G26["TX_LINK_I2C"]      = "false";
	G26["TX_LINK_MODBUS"]   = "false";
	G26["PULL_UP"]          = "false";
	G26["PULSE_COUNT_RISE"] = "false";
	G26["PULSE_COUNT_FALL"] = "false";

	String g35              = HAL.Gpio.getModifier() + "35";
	JsonObject G35          = doc[g35.c_str()].to<JsonObject>();
	G35["ENABLE"]           = "false";
	G35["NAME"]             = "G35";
	G35["SHIFT_LEVEL"]      = "4";
	G35["TX_ON_CHANGE"]     = "false";
	G35["TX_LINK_I2C"]      = "false";
	G35["TX_LINK_MODBUS"]   = "false";
	G35["DIGITAL"]          = "false";
	G35["PULSE_COUNT_RISE"] = "false";
	G35["PULSE_COUNT_FALL"] = "false";

	String g36              = HAL.Gpio.getModifier() + "36";
	JsonObject G36          = doc[g36.c_str()].to<JsonObject>();
	G36["ENABLE"]           = "false";
	G36["NAME"]             = "G36";
	G36["SHIFT_LEVEL"]      = "4";
	G36["TX_ON_CHANGE"]     = "false";
	G36["TX_LINK_I2C"]      = "false";
	G36["TX_LINK_MODBUS"]   = "false";
	G36["DIGITAL"]          = "false";
	G36["PULSE_COUNT_RISE"] = "false";
	G36["PULSE_COUNT_FALL"] = "false";

	return doc;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_BLE_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
IBluetoothLowEnergyControl::LocalBuffer IBluetoothLowEnergyControl::rx_buffer = {0};
IBluetoothLowEnergyControl::LocalBuffer IBluetoothLowEnergyControl::tx_buffer = {0};

class MyServerCallbacks : public NimBLEServerCallbacks {
	void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {};

	void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
		NimBLEDevice::startAdvertising(); // restart advertising
	}
};

class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
	void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
		std::string rxValue = pCharacteristic->getValue();

		if (rxValue.length() > 0) {
			for (int i = 0; i < rxValue.length(); i++) {
				HAL_BLE_API::rx_buffer.data[HAL_BLE_API::rx_buffer.store_index] = rxValue[i];
				HAL_BLE_API::rx_buffer.length += 1;
				HAL_BLE_API::rx_buffer.store_index = ((HAL_BLE_API::rx_buffer.store_index + 1) == BLE_BUF_LENGTH)
				                                         ? 0
				                                         : (HAL_BLE_API::rx_buffer.store_index + 1);
			}
		}
	}

	void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
		if (HAL_BLE_API::tx_buffer.length > 0) {
			pCharacteristic->setValue(HAL_BLE_API::tx_buffer.data, HAL_BLE_API::tx_buffer.length);
			HAL_BLE_API::tx_buffer.length = 0;
		} else {
			pCharacteristic->setValue(HAL_BLE_API::tx_buffer.data, 0);
		}
	}
};

void HAL_BLE_API::begin(String local_name) {
	if (local_name.length() > BLE_MAX_LOCAL_NAME_LEN) {
		local_name = local_name.substring(0, BLE_MAX_LOCAL_NAME_LEN);
	}

	// Create the BLE Device
	NimBLEDevice::init(local_name.c_str());

	// Create the BLE Server
	pServer = NimBLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	NimBLEService *pService = pServer->createService(SERVICE_UUID);

	// Create a BLE Characteristic
	pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
	                                                   NIMBLE_PROPERTY::READ);
	pTxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

	NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,
	                                                                         NIMBLE_PROPERTY::WRITE);
	pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

	NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->setName(local_name.c_str());
	pAdvertising->enableScanResponse(false); // important to send LocalName

	// Start the service
	pService->start();
	// Start advertising
	pServer->getAdvertising()->start();
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_GPIO_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

std::vector<uint8_t> HAL_GPIO_API::getPins() {
	std::vector<uint8_t> pins = {25, 26, 35, 36};
	return pins;
}

String HAL_GPIO_API::getModifier() {
	return "G";
}

void HAL_GPIO_API::setUnused(uint8_t pin) {
	if (pin < 34) {
		::pinMode(pin, INPUT_PULLUP);
	} else {
		// GPIO 34 以上は入力専用のため内部プルアップはできない
		::pinMode(pin, INPUT);
	}
}

bool HAL_GPIO_API::initializePulseCounter(uint8_t pin, PULSE_EDGE edge) {
	constexpr int16_t PCNT_H_LIMIT_COUNT = INT16_MAX;
	constexpr int16_t PCNT_L_LIMIT_COUNT = 0;

	if (AVAILABLE_UNIT_NUMBER > PCNT_UNIT_MAX) {
		return false;
	}

	// ユニット番号の割り当て
	_pin_unit_map[pin] = static_cast<pcnt_unit_t>(AVAILABLE_UNIT_NUMBER);

	pcnt_config_t pcnt_config;                       // 設定用の構造体の宣言
	pcnt_config.pulse_gpio_num = pin;                // パルス入力 GPIO ピン番号
	pcnt_config.ctrl_gpio_num  = PCNT_PIN_NOT_USED;  // 制御信号入力 GPIO ピン番号
	pcnt_config.lctrl_mode     = PCNT_MODE_KEEP;     // 制御信号 LOW 時の動作モード
	pcnt_config.hctrl_mode     = PCNT_MODE_KEEP;     // 制御信号 HIGH 時の動作モード
	pcnt_config.channel        = PCNT_CHANNEL_0;     // PCNTユニットで使うチャネル番号
	pcnt_config.unit           = _pin_unit_map[pin]; // ユニット番号

	switch (edge) {
	case PULSE_EDGE::BOTH:
		pcnt_config.pos_mode = PCNT_COUNT_INC; // ポジティブエッジカウントモード
		pcnt_config.neg_mode = PCNT_COUNT_INC; // ネガティブエッジカウントモード
		break;
	case PULSE_EDGE::RISE:
		pcnt_config.pos_mode = PCNT_COUNT_INC; // ポジティブエッジカウントモード
		pcnt_config.neg_mode = PCNT_COUNT_DIS; // ネガティブエッジカウントモード
		break;
	case PULSE_EDGE::FALL:
		pcnt_config.pos_mode = PCNT_COUNT_DIS; // ポジティブエッジカウントモード
		pcnt_config.neg_mode = PCNT_COUNT_INC; // ネガティブエッジカウントモード
		break;
	default:
		pcnt_config.pos_mode = PCNT_COUNT_DIS; // ポジティブエッジカウントモード
		pcnt_config.neg_mode = PCNT_COUNT_DIS; // ネガティブエッジカウントモード
		break;
	}

	pcnt_config.counter_h_lim = PCNT_H_LIMIT_COUNT; // カウンタ最大値リミット
	pcnt_config.counter_l_lim = PCNT_L_LIMIT_COUNT; // カウンタ最小値リミット

	pcnt_unit_config(&pcnt_config);         // ユニット初期化
	pcnt_counter_pause(_pin_unit_map[pin]); // カウンタ一時停止
	pcnt_counter_clear(_pin_unit_map[pin]); // カウンタ初期化

	AVAILABLE_UNIT_NUMBER++;
	return true;
}

bool HAL_GPIO_API::stopPulseCounter(uint8_t pin) {
	if (_pin_unit_map.find(pin) == _pin_unit_map.end()) {
		return false; // ユニット番号が見つからない場合
	}

	if (pcnt_counter_pause(_pin_unit_map[pin]) != ESP_OK) {
		return false;
	}

	return true;
}

bool HAL_GPIO_API::startPulseCounter(uint8_t pin) {
	if (_pin_unit_map.find(pin) == _pin_unit_map.end()) {
		return false; // ユニット番号が見つからない場合
	}

	if (pcnt_counter_resume(_pin_unit_map[pin]) != ESP_OK) {
		return false;
	}
	return true;
}

bool HAL_GPIO_API::stopPulseCounterAll() {
	bool result = true;
	for (auto item : _pin_unit_map) {
		if (pcnt_counter_pause(item.second) != ESP_OK) {
			result = false;
		}
	}
	return result;
}

bool HAL_GPIO_API::startPulseCounterAll() {
	bool result = true;
	for (auto item : _pin_unit_map) {
		if (pcnt_counter_resume(item.second) != ESP_OK) {
			result = false;
		}
	}
	return result;
}

uint32_t HAL_GPIO_API::getMaxPulseCount() {
	return INT16_MAX;
}

bool HAL_GPIO_API::updatePulseCount(uint8_t pin) {
	if (_pin_unit_map.find(pin) == _pin_unit_map.end()) {
		return false; // ユニット番号が見つからない場合
	}

	int16_t count = 0;
	if (pcnt_get_counter_value(_pin_unit_map[pin], &count) != ESP_OK) {
		return false;
	}
	_count[pin] = count;

	return true;
}

void HAL_GPIO_API::resetPulseCount(uint8_t pin) {
	if (_pin_unit_map.find(pin) == _pin_unit_map.end()) {
		return; // ユニット番号が見つからない場合
	}

	pcnt_counter_clear(_pin_unit_map[pin]);
}

void HAL_GPIO_API::resetPulseCountAll() {
	for (auto item : _pin_unit_map) {
		pcnt_counter_clear(item.second);
	}
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_I2C_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_I2C_API::begin(uint8_t port) {
	uint8_t _sda;
	uint8_t _scl;

	switch (port) {
	case PORT_I2C::PORT_1:
		// Wire.end();
		_sda = HAL_I2C_API::getSdaPin(port);
		_scl = HAL_I2C_API::getSclPin(port);
		Wire.end();
		if (Wire.begin(_sda, _scl) == true) {
			// ONLY ESP32 i2c begin return bool result
			_wires[port] = &Wire;
		}
		return true;
	default:
		return false;
	}

	return false;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_MODBUS_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

std::vector<uint8_t> HAL_MODBUS_API::getPorts() {
	std::vector<uint8_t> ports = {PORT_MODBUS::PORT_1, PORT_MODBUS::PORT_2};
	return ports;
}

bool HAL_MODBUS_API::begin(uint8_t port, uint32_t baudrate, uint8_t data_bit, uint8_t parity, uint32_t timeout_ms) {
	uint32_t config;
	constexpr uint8_t PARITY_ODD  = 1;
	constexpr uint8_t PARITY_EVEN = 2;
	if (data_bit == 8) {
		switch (parity) {
		case PARITY_ODD:
			config = SERIAL_8O1;
			break;
		case PARITY_EVEN:
			config = SERIAL_8E1;
			break;
		default:
			config = SERIAL_8N1;
			break;
		}
	} else {
		switch (parity) {
		case PARITY_ODD:
			config = SERIAL_7O1;
			break;
		case PARITY_EVEN:
			config = SERIAL_7E1;
			break;
		default:
			config = SERIAL_7N1;
			break;
		}
	}

	uint8_t _TXD, _RXD;
	switch (port) {
	case PORT_MODBUS::PORT_1:
		_TXD = HAL_MODBUS_API::getTxPin(port);
		_RXD = HAL_MODBUS_API::getRxPin(port);

		portSerial0.end();
		portSerial0.begin(baudrate, config, _RXD, _TXD);
		_serials[port]    = &portSerial0;
		_timeout_ms[port] = timeout_ms;
		return true;
	case PORT_MODBUS::PORT_2:
		_TXD = HAL_MODBUS_API::getTxPin(port);
		_RXD = HAL_MODBUS_API::getRxPin(port);

		portSerial1.end();
		portSerial1.begin(baudrate, config, _RXD, _TXD);
		_serials[port]    = &portSerial1;
		_timeout_ms[port] = timeout_ms;
		return true;
	default:
		return false;
	}

	return false;
}

uint8_t HAL_MODBUS_API::getTxPin(uint8_t port) {
	switch (port) {
	case PORT_MODBUS::PORT_1:
		return 14;
	case PORT_MODBUS::PORT_2:
		return 17;
	default:
		return 0;
	}
}

uint8_t HAL_MODBUS_API::getRxPin(uint8_t port) {
	switch (port) {
	case PORT_MODBUS::PORT_1:
		return 13;
	case PORT_MODBUS::PORT_2:
		return 16;
	default:
		return 0;
	}
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_Display_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_Display_API::drawPixel(int32_t x, int32_t y, uint32_t color) {
	M5.Display.drawPixel(x, y, (uint16_t)color);
}

void HAL_Display_API::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size) {
	M5.Display.drawChar(x, y, c, (uint16_t)color, (uint16_t)bg, size);
}

void HAL_Display_API::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
	M5.Display.drawLine(x0, y0, x1, y1, (uint16_t)color);
}

void HAL_Display_API::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
	M5.Display.drawFastVLine(x, y, h, (uint16_t)color);
}

void HAL_Display_API::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) {
	M5.Display.drawFastHLine(x, y, w, (uint16_t)color);
}

void HAL_Display_API::fillScreen(uint32_t color) {
	M5.Display.fillScreen((uint16_t)color);
}

int16_t HAL_Display_API::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font) {
	// Ignore font
	return M5.Display.drawChar(uniCode, x, y, font);
}

int16_t HAL_Display_API::drawChar(uint16_t uniCode, int32_t x, int32_t y) {
	return M5.Display.drawChar(uniCode, x, y);
}

int16_t HAL_Display_API::height(void) {
	return M5.Display.height();
}

int16_t HAL_Display_API::width(void) {
	return M5.Display.width();
}

int16_t HAL_Display_API::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font) {
	// Ignore font
	return M5.Display.drawString(string, poX, poY);
}

int16_t HAL_Display_API::drawString(const char *string, int32_t poX, int32_t poY) {
	return M5.Display.drawString(string, poX, poY);
}

int16_t HAL_Display_API::color565(uint8_t red, uint8_t green, uint8_t blue) {
	return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

void HAL_Display_API::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	M5.Display.drawRect(x, y, w, h, (uint16_t)color);
}

void HAL_Display_API::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	M5.Display.fillRect(x, y, w, h, (uint16_t)color);
}

void HAL_Display_API::drawRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) {
	M5.Display.drawRoundRect(x0, y0, w, h, radius, (uint16_t)color);
}

void HAL_Display_API::fillRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) {
	M5.Display.fillRoundRect(x0, y0, w, h, radius, (uint16_t)color);
}

void HAL_Display_API::setRotation(uint8_t r) {
	M5.Display.setRotation(r);
}

void HAL_Display_API::startWrite() {
	M5.Display.startWrite();
}

void HAL_Display_API::endWrite() {
	M5.Display.endWrite();
}

void HAL_Display_API::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
	M5.Display.drawCircle(x0, y0, r, (uint16_t)color);
}

void HAL_Display_API::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
	M5.Display.fillCircle(x0, y0, r, (uint16_t)color);
}

void HAL_Display_API::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
	M5.Display.drawEllipse(x0, y0, rx, ry, color);
}

void HAL_Display_API::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
	M5.Display.fillEllipse(x0, y0, rx, ry, color);
}

void HAL_Display_API::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	M5.Display.drawTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

void HAL_Display_API::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	M5.Display.fillTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

void HAL_Display_API::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {
	M5.Display.pushImage(x, y, w, h, data);
}

bool HAL_Display_API::initializeSemaphoreMutex() {
	return HAL.initializeSemaphoreMutex("spi");
}

bool HAL_Display_API::takeSemaphoreMutex(portTickType wait_tick_time) {
	return HAL.takeSemaphoreMutex("spi", wait_tick_time);
}

bool HAL_Display_API::giveSemaphoreMutex() {
	return HAL.giveSemaphoreMutex("spi");
}

#endif /* HAL_API_M5STATION_H */