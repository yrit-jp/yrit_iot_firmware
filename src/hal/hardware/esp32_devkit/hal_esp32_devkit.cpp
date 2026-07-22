#include "hal_esp32_devkit.h"
#ifdef HAL_API_ESP32_DEVKIT_H

HAL_API HAL;

#define TIM_0 (0)
#define TIM_1 (1)
#define TIM_2 (2)
#define TIM_3 (3)

#define DEBOUNCE_MS (10)
Button BtnA = Button(PSW1, true, DEBOUNCE_MS);
Button BtnC = Button(PSW2, true, DEBOUNCE_MS);

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
	Serial.begin(115200);
	// Wire.begin();
	// IO initialize
	pinMode(PIN_IO0, INPUT);
	pinMode(PIN_IO1, INPUT);
	pinMode(PIN_IO2, INPUT);
	pinMode(PIN_IO3, INPUT);
	pinMode(PIN_IO4, INPUT);
	pinMode(PIN_IO5, INPUT);
	// initialize voltage monitoring pin
	pinMode(PIN_VOB, INPUT);

	pinMode(PIN_WAKE, OUTPUT);
	digitalWrite(PIN_WAKE, HIGH);

	HAL.printf("Board Manager Version: %d.%d.%d\n",
	           ESP_ARDUINO_VERSION_MAJOR,
	           ESP_ARDUINO_VERSION_MINOR,
	           ESP_ARDUINO_VERSION_PATCH);

	HAL.Info.DEVICE_NAME = "ESP32 Devkit";
}

void HAL_API::update() {
	BtnA.read();
	BtnC.read();
}

uint8_t HAL_API::waitForButtonPressed(uint16_t wait_ms) {
	uint8_t ret           = (uint8_t)PRESSED_BUTTON::NONE;
	unsigned long long ts = millis();

	while ((millis() - ts) < wait_ms) {
		HAL.update();
		if (BtnA.isPressed()) {
			ret |= (uint8_t)PRESSED_BUTTON::PRESSED_BTN1;
		}
		HAL.update();
		if (BtnC.isPressed()) {
			ret |= (uint8_t)PRESSED_BUTTON::PRESSED_BTN2;
		}

		if (ret != (uint8_t)PRESSED_BUTTON::NONE) {
			return ret;
		}

		delay(1);
	}
	return ret;
}

uint8_t HAL_API::BtnA_wasPressed() {
	return BtnA.wasPressed();
}

uint8_t HAL_API::BtnB_wasPressed() {
	return 0;
}

uint8_t HAL_API::BtnC_wasPressed() {
	return BtnC.wasPressed();
}

uint8_t HAL_API::BtnA_isPressed() {
	return BtnA.isPressed();
}

uint8_t HAL_API::BtnB_isPressed() {
	return 0;
}

uint8_t HAL_API::BtnC_isPressed() {
	return BtnC.isPressed();
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
	double voltage   = 3.3 * (::analogRead(PIN_VOB) / 4095.0);
	int8_t bat_level = std::max((voltage - 2.4) / (3.3 - 2.4), 0.0);
	bat_level        = (bat_level > 100) ? 100 : bat_level;
	return bat_level;
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

void HAL_API::preventBatterySleep(bool on_off) {
	::digitalWrite(PIN_WAKE, (on_off ? LOW : HIGH));
}

String HAL_API::getChipUUID(bool include_prefix) {
	String prefix = "";
	if (include_prefix) {
		prefix = "ESP32-";
	}

	uint64_t chipid  = ESP.getEfuseMac();
	char bt_name[20] = {0};
	snprintf(bt_name, 20, "%012llX", chipid);
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
		// timer frequency 1MHz (1 micro sec)
		timer0 = timerBegin(1000000);
		// timer, interrupt function
		timerAttachInterrupt(timer0, onTimer0);
		// timer, interrupt interval, autoReload, reload count
		timerAlarm(timer0, interval_ms * 1000, true, 0);
		break;
	case TIM_1:
		// Create semaphore to inform us when the timer has fired
		timer1_Semaphore = xSemaphoreCreateBinary();
		// timer frequency 1MHz (1 micro sec)
		timer1 = timerBegin(1000000);
		// timer, interrupt function
		timerAttachInterrupt(timer1, onTimer1);
		// timer, interrupt interval, autoReload, reload count
		timerAlarm(timer1, interval_ms * 1000, true, 0);
		break;
	case TIM_2:
		// Create semaphore to inform us when the timer has fired
		timer2_Semaphore = xSemaphoreCreateBinary();
		// timer frequency 1MHz (1 micro sec)
		timer2 = timerBegin(1000000);
		// timer, interrupt function
		timerAttachInterrupt(timer2, onTimer2);
		// timer, interrupt interval, autoReload, reload count
		timerAlarm(timer2, interval_ms * 1000, true, 0);
		break;
	case TIM_3:
		// Create semaphore to inform us when the timer has fired
		timer3_Semaphore = xSemaphoreCreateBinary();
		// timer frequency 1MHz (1 micro sec)
		timer3 = timerBegin(1000000);
		// timer, interrupt function
		timerAttachInterrupt(timer3, onTimer3);
		// timer, interrupt interval, autoReload, reload count
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
		timerStart(timer0); // Enable timer interrupt
		if (enable_first_interrupt) {
			xSemaphoreGive(timer0_Semaphore); // for first time interrupt
		}
		break;
	case TIM_1:
		if (timer1 == nullptr) {
			return;
		}
		timerRestart(timer1);
		timerStart(timer1); // Enable timer interrupt
		if (enable_first_interrupt) {
			xSemaphoreGive(timer1_Semaphore); // for first time interrupt
		}
		break;
	case TIM_2:
		if (timer2 == nullptr) {
			return;
		}
		timerRestart(timer2);
		timerStart(timer2); // Enable timer interrupt
		if (enable_first_interrupt) {
			xSemaphoreGive(timer2_Semaphore); // for first time interrupt
		}
		break;
	case TIM_3:
		if (timer3 == nullptr) {
			return;
		}
		timerRestart(timer3);
		timerStart(timer3); // Enable timer interrupt
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
//  MARK: HAL_LED_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

HAL_LED_API::HAL_LED_API(uint8_t pin, uint8_t channel, uint16_t freq, uint8_t resolution) {
	_pin  = pin;
	_ch   = channel;
	_freq = freq;
	_res  = resolution;
	if (_res > 16) {
		_res = 16;
	}
}

void HAL_LED_API::begin() {
	_brightness.limit = (0xFFFF >> (16 - _res)) + 1;
	_brightness.max   = 0; // light when pin L
	_brightness.min   = _brightness.limit;

	ledcAttachChannel(_pin, _freq, _res, _ch);
	ledcWrite(_pin, _brightness.min);
	// ledcSetup(_ch, _freq, _res);
	// ledcAttachPin(_pin, _ch);
	// ledcWrite(_ch, _brightness.min);
	_current_led_state = false;
}

void HAL_LED_API::setBrightnessLevel(double percentage) {
	percentage      = (percentage >= 100) ? 100 : percentage;
	_brightness.max = _brightness.limit * ((100 - percentage) / 100.0);
	_brightness.max = ((_brightness.max == _brightness.limit) ? _brightness.limit - 1 : _brightness.max);
	HAL.delay(100);
}

void HAL_LED_API::on() {
	ledcWrite(_pin, _brightness.max);
	_current_led_state = true;
}

void HAL_LED_API::off() {
	ledcWrite(_pin, _brightness.min);
	_current_led_state = false;
}

void HAL_LED_API::toggle() {
	if (_current_led_state) {
		off();
	} else {
		on();
	}
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

	JsonObject PARAM            = doc["PARAM"].to<JsonObject>();
	PARAM["WAKE_BAT"]           = "false";
	PARAM["LED_BRIGHTNESS_PCT"] = "10";

	JsonObject SAMPLING = doc["SAMPLING"].to<JsonObject>();
	SAMPLING["GPIO_MS"] = "3000";

	String p33              = HAL.Gpio.getModifier() + "33";
	JsonObject P33          = doc[p33.c_str()].to<JsonObject>();
	P33["ENABLE"]           = "false";
	P33["NAME"]             = "P33";
	P33["SHIFT_LEVEL"]      = "4";
	P33["TX_ON_CHANGE"]     = "false";
	P33["TX_LINK_I2C"]      = "false";
	P33["TX_LINK_MODBUS"]   = "false";
	P33["DIGITAL"]          = "false";
	P33["PULL_UP"]          = "false";
	P33["PULSE_COUNT_RISE"] = "false";
	P33["PULSE_COUNT_FALL"] = "false";

	String p32              = HAL.Gpio.getModifier() + "32";
	JsonObject P32          = doc[p32.c_str()].to<JsonObject>();
	P32["ENABLE"]           = "false";
	P32["NAME"]             = "P32";
	P32["SHIFT_LEVEL"]      = "4";
	P32["TX_ON_CHANGE"]     = "false";
	P32["TX_LINK_I2C"]      = "false";
	P32["TX_LINK_MODBUS"]   = "false";
	P32["DIGITAL"]          = "false";
	P32["PULL_UP"]          = "false";
	P32["PULSE_COUNT_RISE"] = "false";
	P32["PULSE_COUNT_FALL"] = "false";

	String p26              = HAL.Gpio.getModifier() + "26";
	JsonObject P26          = doc[p26.c_str()].to<JsonObject>();
	P26["ENABLE"]           = "false";
	P26["NAME"]             = "P26";
	P26["TX_ON_CHANGE"]     = "false";
	P26["TX_LINK_I2C"]      = "false";
	P26["TX_LINK_MODBUS"]   = "false";
	P26["PULL_UP"]          = "false";
	P26["PULSE_COUNT_RISE"] = "false";
	P26["PULSE_COUNT_FALL"] = "false";

	String p25              = HAL.Gpio.getModifier() + "25";
	JsonObject P25          = doc[p25.c_str()].to<JsonObject>();
	P25["ENABLE"]           = "false";
	P25["NAME"]             = "P25";
	P25["TX_ON_CHANGE"]     = "false";
	P25["TX_LINK_I2C"]      = "false";
	P25["TX_LINK_MODBUS"]   = "false";
	P25["PULL_UP"]          = "false";
	P25["PULSE_COUNT_RISE"] = "false";
	P25["PULSE_COUNT_FALL"] = "false";

	String p14              = HAL.Gpio.getModifier() + "14";
	JsonObject P14          = doc[p14.c_str()].to<JsonObject>();
	P14["ENABLE"]           = "false";
	P14["NAME"]             = "P14";
	P14["TX_ON_CHANGE"]     = "false";
	P14["TX_LINK_I2C"]      = "false";
	P14["TX_LINK_MODBUS"]   = "false";
	P14["PULL_UP"]          = "false";
	P14["PULSE_COUNT_RISE"] = "false";
	P14["PULSE_COUNT_FALL"] = "false";

	String p27              = HAL.Gpio.getModifier() + "27";
	JsonObject P27          = doc[p27.c_str()].to<JsonObject>();
	P27["ENABLE"]           = "false";
	P27["NAME"]             = "P27";
	P27["TX_ON_CHANGE"]     = "false";
	P27["TX_LINK_I2C"]      = "false";
	P27["TX_LINK_MODBUS"]   = "false";
	P27["PULL_UP"]          = "false";
	P27["PULSE_COUNT_RISE"] = "false";
	P27["PULSE_COUNT_FALL"] = "false";

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
	std::vector<uint8_t> pins = {33, 32, 26, 25, 14, 27};
	return pins;
}

String HAL_GPIO_API::getModifier() {
	return "P";
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
//  MARK: HAL_Display_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_Display_API::initializeSemaphoreMutex() {
	return HAL.initializeSemaphoreMutex("i2c");
}

bool HAL_Display_API::takeSemaphoreMutex(portTickType wait_tick_time) {
	return HAL.takeSemaphoreMutex("i2c", wait_tick_time);
}

bool HAL_Display_API::giveSemaphoreMutex() {
	return HAL.giveSemaphoreMutex("i2c");
}

#endif /* HAL_API_ESP32_DEVKIT_H */