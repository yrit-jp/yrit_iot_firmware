#include "hal_lilygo_t_display_s3_pro.h"
#ifdef HAL_API_LILYGO_T_DISPLAY_PRO_H

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

// Battery
PowersSY6970 PMU;

// Touch
TouchDrvCSTXXX touch;
TouchButtonGeneric_Class button;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_API::begin() {
	Serial.begin(115200);

	// Initialize power management unit
	PMU.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, SY6970_SLAVE_ADDRESS);
	// Disable LED
	PMU.disableStatLed();
	// Set the charging target voltage, Range:3840 ~ 4608mV ,step:16 mV
	PMU.setChargeTargetVoltage(4096);
	// Set the precharge current , Range: 64mA ~ 1024mA ,step:64mA
	PMU.setPrechargeCurr(64);
	// Set the charging current , Range:0~5056mA ,step:64mA
	PMU.setChargerConstantCurr(320);
	// To obtain voltage data, the ADC must be enabled first
	PMU.enableADCMeasure();

	// Touch センサのリセット
#if BOARD_TOUCH_RST != -1
	pinMode(BOARD_TOUCH_RST, OUTPUT);
	digitalWrite(BOARD_TOUCH_RST, LOW);
	delay(30);
	digitalWrite(BOARD_TOUCH_RST, HIGH);
	delay(50);
#endif
	touch.setPins(BOARD_TOUCH_RST, BOARD_SENSOR_IRQ);
	touch.begin(Wire, CST226SE_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL);

	button.begin();

	Display.begin();
	// Display.setRotation(3);
	Display.fillScreen(TFT_BLACK);

	HAL.printf("Board Manager Version: %d.%d.%d\n",
	           ESP_ARDUINO_VERSION_MAJOR,
	           ESP_ARDUINO_VERSION_MINOR,
	           ESP_ARDUINO_VERSION_PATCH);

	// K233 と K255 を判別
	Wire.beginTransmission(0x51); // PCF85063
	int8_t error = Wire.endTransmission(true);
	// Wire.end();	// touch 等が動いているため、 end してはいけない
	if (error != 0) {
		// RTC を検出しなかった
		HAL.Info.DEVICE_NAME = "LilyGo T-Display S3 Pro (K233)";
		HAL.println("K233");
	} else {
		// もっと厳密に判定するべき
		HAL.Info.DEVICE_NAME = "LilyGo T-Display S3 Pro (K255)";
		HAL.println("K255");
	}
}

void HAL_API::update() {
	int16_t x[5], y[5];

	HAL.takeSemaphoreMutex("i2c", portMAX_DELAY);
	uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
	HAL.giveSemaphoreMutex("i2c");

	button.update(millis(), x, y, touched);
}

uint8_t HAL_API::waitForButtonPressed(uint16_t wait_ms) {
	uint8_t ret           = (uint8_t)PRESSED_BUTTON::NONE;
	bool all_free_flag    = false;
	unsigned long long ts = millis();

	while ((millis() - ts) < wait_ms) {
		if (not all_free_flag) {
			HAL.update();
			if ((not HAL.BtnA_isPressed()) and (not HAL.BtnC_isPressed())) {
				all_free_flag = true;
			}
		} else {
			HAL.update();
			if (HAL.BtnA_isPressed()) {
				ret |= (uint8_t)PRESSED_BUTTON::PRESSED_BTN1;
			}
			HAL.update();
			if (HAL.BtnC_isPressed()) {
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
	auto pt = button.getDetail();
	if (pt.wasPressed() == false) {
		return false;
	}

	if (10 <= pt.x and pt.x <= 108 and 424 <= pt.y and pt.y <= 474) {
		return true;
	}

	return false;
}

uint8_t HAL_API::BtnB_wasPressed() {
	return false;
}

uint8_t HAL_API::BtnC_wasPressed() {
	auto pt = button.getDetail();
	if (pt.wasPressed() == false) {
		return false;
	}

	if (113 <= pt.x and pt.x <= 218 and 424 <= pt.y and pt.y <= 474) {
		return true;
	}

	return false;
}

uint8_t HAL_API::BtnA_isPressed() {
	auto pt = button.getDetail();
	if (pt.isPressed() == false) {
		return false;
	}

	if (10 <= pt.x and pt.x <= 108 and 424 <= pt.y and pt.y <= 474) {
		return true;
	}

	return false;
}

uint8_t HAL_API::BtnB_isPressed() {
	return false;
}

uint8_t HAL_API::BtnC_isPressed() {
	auto pt = button.getDetail();
	if (pt.isPressed() == false) {
		return false;
	}

	if (113 <= pt.x and pt.x <= 218 and 424 <= pt.y and pt.y <= 474) {
		return true;
	}

	return false;
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
	double bp = (static_cast<double>(PMU.getBattVoltage()) / PMU.getChargeTargetVoltage()) * 100.0;
	return (bp > 100) ? 100 : static_cast<int8_t>(bp);
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
	WiFi.persistent(false);
	WiFi.setSleep(false);
	WiFi.begin(_ap_name.c_str(), _pass.c_str());

	unsigned long start = millis();
	wl_status_t result;
	do {
		result = WiFi.status();
		delay(500);
	} while (result != WL_CONNECTED and (millis() - start) < (timeout_ms));

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

	HAL.Wlan.connectWlan(15000);

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
	if (not FFat.begin(true)) {
		HAL.println("Failed to begin FFat.");
		return false;
	}

	String tmpl = "";
	serializeJson(json, tmpl);

	File f = FFat.open(fpath.c_str(), "w");
	if (not f) {
		HAL.println("Open error.");
		FFat.end();
		return false;
	}
	f.print(tmpl);
	f.close();
	FFat.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::loadSettings(String fpath, JsonDocument &json) {
	if (not FFat.begin(true)) {
		HAL.println("Failed to begin FFat.");
		return false;
	}

	if (not FFat.exists(fpath.c_str())) {
		HAL.println("File not exists.");
		FFat.end();
		return false;
	}

	File f = FFat.open(fpath.c_str(), "r");
	if (not f) {
		HAL.println("Open error.");
		FFat.end();
		return false;
	}

	String doc = "";
	for (unsigned int i = 0; i < f.size(); i++) {
		doc += (char)f.read();
	}
	f.close();
	deserializeJson(json, doc.c_str());

	FFat.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::formatStorage() {
	return FFat.format();
}

JsonDocument HAL_SETTING_FILE_IO_API::deviceUniqueSettings() {
	JsonDocument doc;

	JsonObject SAMPLING   = doc["SAMPLING"].to<JsonObject>();
	SAMPLING["MODBUS_MS"] = "3000";

	String modbus1        = HAL.Modbus.getModifier() + String(PORT_MODBUS::PORT_1);
	JsonObject MODBUS1    = doc[modbus1.c_str()].to<JsonObject>();
	MODBUS1["BAUDRATE"]   = "9600";
	MODBUS1["DATA_BIT"]   = "8";
	MODBUS1["PARITY"]     = "0";
	MODBUS1["TIMEOUT_MS"] = "200";
	MODBUS1["ID_MIN"]     = "1";
	MODBUS1["ID_MAX"]     = "5";

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
//  MARK: HAL_MODBUS_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

std::vector<uint8_t> HAL_MODBUS_API::getPorts() {
	std::vector<uint8_t> ports = {PORT_MODBUS::PORT_1};
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
	default:
		return false;
	}

	return false;
}

uint8_t HAL_MODBUS_API::getTxPin(uint8_t port) {
	switch (port) {
	case PORT_MODBUS::PORT_1:
		return 43;
	default:
		return 0;
	}
}

uint8_t HAL_MODBUS_API::getRxPin(uint8_t port) {
	switch (port) {
	case PORT_MODBUS::PORT_1:
		return 44;
	default:
		return 0;
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

uint8_t HAL_I2C_API::getSdaPin(uint8_t port) {
	return BOARD_I2C_SDA;
}

uint8_t HAL_I2C_API::getSclPin(uint8_t port) {
	return BOARD_I2C_SCL;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_Display_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_Display_API::initializeSemaphoreMutex() {
	return HAL.initializeSemaphoreMutex("spi");
}

bool HAL_Display_API::takeSemaphoreMutex(portTickType wait_tick_time) {
	return HAL.takeSemaphoreMutex("spi", wait_tick_time);
}

bool HAL_Display_API::giveSemaphoreMutex() {
	return HAL.giveSemaphoreMutex("spi");
}

#endif /* HAL_API_ESP32_DEVKIT_H */