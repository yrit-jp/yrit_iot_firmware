#include "hal_wio_terminal.h"
#ifdef HAL_API_WIO_H

HAL_API HAL;

// Button API
#define DEBOUNCE_MS (10)
Button BtnA = Button(WIO_KEY_C, true, DEBOUNCE_MS);
Button BtnB = Button(WIO_KEY_B, true, DEBOUNCE_MS);
Button BtnC = Button(WIO_KEY_A, true, DEBOUNCE_MS);

// Variables (timer)
#define TIM_0 (0)
#define TIM_1 (1)
#define TIM_2 (2)

SAMD51TC Timer0_TC(TIM_0);
SAMD51TC Timer1_TC(TIM_1);
SAMD51TC Timer2_TC(TIM_2);
void onTimer0();
void onTimer1();
void onTimer2();

struct Counter {
	uint32_t limit;
	uint32_t current;
};
std::unordered_map<std::string, struct Counter> sub_timer0;
std::unordered_map<std::string, struct Counter> sub_timer1;
std::unordered_map<std::string, struct Counter> sub_timer2;

// Variables (BLE)
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
TaskHandle_t TransmitTaskHandle;
TaskHandle_t UserTaskHandle;

bool IsBatteryConnected = false;
bool IsFirstLoop        = true;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_API::begin() {
	Serial.begin(115200);
	Serial.flush();
	Serial1.begin(115200);
	Serial1.flush();
	delay(100);

	// HAL.printf("Free Heap: %06d\n", xPortGetFreeHeapSize());

	// Initialize TRNG
	MCLK->APBCMASK.bit.TRNG_   = 1; // Enable Main Clock (MCLK) for TRNG
	TRNG->CTRLA.bit.ENABLE     = 1; // Enable TRNG
	TRNG->INTENSET.bit.DATARDY = 1; // Enable TRNG interrupt when data ready
	HAL.delay(10);                  // Short delay to allow generation of new random number

	SAMCrashMonitor::begin();
	SAMCrashMonitor::disableWatchdog(); // Make sure it is turned off during init.

	Display.begin();
	Display.setRotation(3);
	Display.fillScreen(TFT_BLACK);
	digitalWrite(LCD_BACKLIGHT, HIGH);

	pinMode(SDCARD_DET_PIN, INPUT);

	// Reset RTL8720D
	pinMode(RTL8720D_CHIP_PU, OUTPUT);
	digitalWrite(RTL8720D_CHIP_PU, LOW);
	HAL.delay(100);
	digitalWrite(RTL8720D_CHIP_PU, HIGH);

	if (lipo.begin()) {
		// Successful battery connected
		IsBatteryConnected                  = true;
		const unsigned int BATTERY_CAPACITY = 650; // Set Wio Terminal Battery's Capacity
		lipo.setCapacity(BATTERY_CAPACITY);
	}

	HAL.Info.DEVICE_NAME = "Wio Terminal";

	// SD と LCD が独立しているっぽい
	// HAL.initializeSemaphoreMutex("log");
}

void HAL_API::update() {
	BtnA.read();
	BtnB.read();
	BtnC.read();

	SAMCrashMonitor::iAmAlive(); // Reset WDT
}

uint8_t HAL_API::waitForButtonPressed(uint16_t wait_ms) {
	uint8_t ret           = (uint8_t)PRESSED_BUTTON::NONE;
	bool all_free_flag    = false;
	unsigned long long ts = millis();

	while ((millis() - ts) < wait_ms) {
		if (not all_free_flag) {
			HAL.update();
			if ((not BtnA.isPressed()) and (not BtnC.isPressed())) {
				all_free_flag = true;
			}
		} else {
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
		}

		delay(5);
	}
	return ret;
}

uint8_t HAL_API::BtnA_wasPressed() {
	return BtnA.wasPressed();
}

uint8_t HAL_API::BtnB_wasPressed() {
	return BtnB.wasPressed();
}

uint8_t HAL_API::BtnC_wasPressed() {
	return BtnC.wasPressed();
}

uint8_t HAL_API::BtnA_isPressed() {
	return BtnA.isPressed();
}

uint8_t HAL_API::BtnB_isPressed() {
	return BtnB.isPressed();
}

uint8_t HAL_API::BtnC_isPressed() {
	return BtnC.isPressed();
}

String HAL_API::getChipUUID(bool include_prefix) {
#define SERIAL_NUMBER_WORD_0 *(volatile uint32_t *)(0x008061FC)
#define SERIAL_NUMBER_WORD_1 *(volatile uint32_t *)(0x00806010)
#define SERIAL_NUMBER_WORD_2 *(volatile uint32_t *)(0x00806014)
#define SERIAL_NUMBER_WORD_3 *(volatile uint32_t *)(0x00806018)

	String prefix = "";
	if (include_prefix) {
		prefix = "Wio-";
	}

	uint32_t pdwUniqueID[4];
	pdwUniqueID[0] = SERIAL_NUMBER_WORD_0;
	pdwUniqueID[1] = SERIAL_NUMBER_WORD_1;
	pdwUniqueID[2] = SERIAL_NUMBER_WORD_2;
	pdwUniqueID[3] = SERIAL_NUMBER_WORD_3;

	char bt_name[40] = {0};
	snprintf(bt_name, 40, "%08lX%08lX%08lX%08lX",
	         pdwUniqueID[0], pdwUniqueID[1], pdwUniqueID[2], pdwUniqueID[3]);
	return prefix + String(bt_name);
}

int8_t HAL_API::getBatteryLevel() {
	if (not IsBatteryConnected) {
		return -1;
	}

	return lipo.soc();
}

uint32_t HAL_API::getFreeHeapSize() {
	return xPortGetFreeHeapSize();
}

uint32_t HAL_API::getMinimumFreeHeapSize() {
	return xPortGetMinimumEverFreeHeapSize();
}

uint32_t HAL_API::getRandomNumber(uint32_t minimum, uint32_t maximum) {
	if (minimum == maximum) {
		return minimum;
	}

	if (minimum > maximum) {
		std::swap(minimum, maximum);
	}

	return ((TRNG->DATA.reg) % (maximum - minimum)) + minimum;
}

void HAL_API::delay(uint32_t ms) {
	// vTaskDelay(ms);
	::delay(ms);
}

void HAL_API::reboot() {
	NVIC_SystemReset();
}

void HAL_API::printf(const char *fmt, ...) {
	char s[256] = {0};
	va_list args;

	::va_start(args, fmt);
	::vsnprintf(s, 256, fmt, args);
	::va_end(args);

	Serial1.print(s);
	// if (HAL.Log.getCurrentLoggingState()) {
	// 	HAL.takeSemaphoreMutex("log", portMAX_DELAY);
	// 	HAL.Log.buffering(s);
	// 	HAL.giveSemaphoreMutex("log");
	// }
}

void HAL_API::println(const char *s) {
	Serial1.println(s);
	// if (HAL.Log.getCurrentLoggingState()) {
	// 	HAL.takeSemaphoreMutex("log", portMAX_DELAY);
	// 	HAL.Log.buffering(s);
	// 	HAL.Log.buffering("\n");
	// 	HAL.giveSemaphoreMutex("log");
	// }
}

void HAL_API::print(const char *s) {
	Serial1.print(s);
	// if (HAL.Log.getCurrentLoggingState()) {
	// 	HAL.takeSemaphoreMutex("log", portMAX_DELAY);
	// 	HAL.Log.buffering(s);
	// 	HAL.giveSemaphoreMutex("log");
	// }
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
	if (IsFirstLoop) {
		IsFirstLoop = false;
		int timeout = SAMCrashMonitor::enableWatchdog(8000);
		// HAL.println("WDT enabled!");
		// HAL.printf("WDT timeout %d ms.\n", timeout);
		HAL.printf("Reset reason %d: %s\n",
		           SAMCrashMonitor::getResetCause(),
		           SAMCrashMonitor::getResetDescription().c_str());
	}

	// HAL.printf("Free Heap: %06d\n", xPortGetFreeHeapSize());

	/* 起動してからの最小スタックサイズをロギング */
	// HAL.printf("Loop stack WM: %d\n", uxTaskGetStackHighWaterMark(NULL));
	// HAL.printf("Tx stack WM  : %d\n", uxTaskGetStackHighWaterMark(TransmitTaskHandle));
	// HAL.printf("User stack WM: %d\n", uxTaskGetStackHighWaterMark(UserTaskHandle));
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_Timer_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_Timer_API::begin(uint32_t interval_ms) {
	_interval_ms = interval_ms;
	switch (_timer_num) {
	case TIM_0:
		Timer0_TC.begin(interval_ms * 1000);
		Timer0_TC.attachInterrupt(onTimer0);
		break;
	case TIM_1:
		Timer1_TC.begin(interval_ms * 1000);
		Timer1_TC.attachInterrupt(onTimer1);
		break;
	case TIM_2:
		Timer2_TC.begin(interval_ms * 1000);
		Timer2_TC.attachInterrupt(onTimer2);
		break;
	default:
		return;
	}
	this->_inited = true;
}

void HAL_Timer_API::addSubTimer(std::string key, uint32_t count_limit, uint32_t initial_count) {
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
	default:
		break;
	}
}

uint32_t HAL_Timer_API::getSubTimerLimit(std::string key) {
	uint32_t limit = 0;

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
	default:
		break;
	}

	return limit;
}

uint32_t HAL_Timer_API::getSubTimerLimitMillis(std::string key) {
	return getSubTimerLimit(key) * _interval_ms;
}

void HAL_Timer_API::removeSubTimer(std::string key) {
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
	default:
		break;
	}
}

bool HAL_Timer_API::checkSubTimer(std::string key, bool reset) {
	bool result = false;

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
	default:
		break;
	}

	return result;
}

void HAL_Timer_API::start(bool enable_first_interrupt) {
	if (this->_inited == false) {
		return;
	}

	switch (_timer_num) {
	case TIM_0:
		Timer0_TC.start();
		break;
	case TIM_1:
		Timer1_TC.start();
		break;
	case TIM_2:
		Timer2_TC.start();
		break;
	default:
		return;
	}

	if (enable_first_interrupt) {
		this->_interrupted = true;
	}
}

bool HAL_Timer_API::interrupted() {
	if (this->_interrupted) {
		this->_interrupted = false;
		return true;
	}
	return false;
}

void onTimer0() {
	for (auto &pair : sub_timer0) {
		pair.second.current++;
	}
	HAL.Timer0._interrupted = true;
}

void onTimer1() {
	for (auto &pair : sub_timer1) {
		pair.second.current++;
	}
	HAL.Timer1._interrupted = true;
}

void onTimer2() {
	for (auto &pair : sub_timer2) {
		pair.second.current++;
	}
	HAL.Timer2._interrupted = true;
}

void TC0_Handler() {
	// まずハンドラが呼ばれて、そこからコールバック関数が呼ばれる
	Timer0_TC.processIsr();
}

void TC1_Handler() {
	// まずハンドラが呼ばれて、そこからコールバック関数が呼ばれる
	Timer1_TC.processIsr();
}

void TC2_Handler() {
	// まずハンドラが呼ばれて、そこからコールバック関数が呼ばれる
	Timer2_TC.processIsr();
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
	WiFi.macAddress(mac);

	// HAL.printf("[WLAN] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\n",
	//            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return std::vector<uint8_t>(mac, mac + (sizeof(mac) / sizeof(mac[0])));
}

MyIPAddress HAL_WLAN_API::localIP() {
	IPAddress ip = WiFi.localIP();
	return MyIPAddress(ip[0], ip[1], ip[2], ip[3]);
}

bool HAL_WLAN_API::config(String ip, String gateway, String subnet, String dns1, String dns2) {
	_use_dhcp = true;
	_set_dns  = true;

	if (_ip.fromString(ip) == false) {
		return false;
	}
	if (_gateway.fromString(gateway) == false) {
		return false;
	}
	if (_subnet.fromString(subnet) == false) {
		return false;
	}

	_use_dhcp = false;
	if (_dns1.fromString(dns1) == false and _dns2.fromString(dns2) == false) {
		// dns option mode
		_set_dns = false;
	}
	return true;
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

	WiFi.mode(WIFI_STA);
	disconnectWlan(false, false);
	wl_status_t result = WiFi.status();

	unsigned long start = millis();
	while (result != WL_CONNECTED and (millis() - start) < (timeout_ms)) {
		if (_pass.equals("")) {
			result = WiFi.begin(_ap_name.c_str());
		} else {
			result = WiFi.begin(_ap_name.c_str(), _pass.c_str());
		}

		if (result != WL_CONNECTED) {
			disconnectWlan(false, false);
		}
		delay(500);
	}

	if (result != WL_CONNECTED) {
		return false;
	}

	if (_use_dhcp) {
		return true;
	}

	if (_set_dns) {
		return WiFi.config(_ip, _gateway, _subnet, _dns1, _dns2);
	} else {
		return WiFi.config(_ip, _gateway, _subnet);
	}
}

void HAL_WLAN_API::disconnectWlan(bool exit_sta_mode, bool erase_ap_settings) {
	WiFi.disconnect(exit_sta_mode, erase_ap_settings);
}

bool HAL_WLAN_API::detectTargetAccessPoint(String ap_name, int32_t *rssi, bool *exist_same_name, bool print_result) {
	std::map<int, String> result = scanAccessPoint(print_result);
	*exist_same_name             = false;

	bool ret            = false;
	bool is_first_match = true;
	for (const auto &[key, value] : result) {
		if (ap_name.equals(value)) {
			if (is_first_match) {
				*rssi          = WiFi.RSSI(key);
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
	return MyIPAddress(0, 0, 0, 0);
}

std::map<int, String> HAL_WLAN_API::scanAccessPoint(bool print_result) {
	std::map<int, String> detected_nws;

	if (print_result) {
		HAL.printf("RTL8720 FW Version: %s\n", rpc_system_version());
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
	if (not this->_rtc.begin()) {
		HAL.println("RTC Error");
		return false;
	}

	const int OVER_WRITE = 1;
	// Set timezone
	setenv("TZ", time_zone.c_str(), OVER_WRITE);
	tzset();

	unsigned long devicetime = getNTPtime();
	if (devicetime == 0) {
		return false;
	}

	this->_rtc.adjust(DateTime(devicetime));
	this->_rtc.now().unixtime(); // If remove, first call getCurrentTime is little shift
	_inited = true;
	return true;
}

bool HAL_NTP_RTC_API::ready() {
	return _inited;
}

bool HAL_NTP_RTC_API::update() {
	return begin();
}

bool HAL_NTP_RTC_API::getCurrentTime(struct tm *timeinfo) {
	time_t tim     = this->_rtc.now().unixtime();
	struct tm *tmp = localtime(&tim);

	if (tmp != NULL) {
		memcpy(timeinfo, tmp, sizeof(*timeinfo));
		return true;
	}
	return false;
}

unsigned long HAL_NTP_RTC_API::getNTPtime() {
	if (WiFi.status() != WL_CONNECTED) {
		HAL.println("WiFi Status Error");
		return 0;
	}

	WiFiUDP _udp;                  // The udp library class
	unsigned int localPort = 2390; // local port to listen for UDP packets

	for (int i = 0; i < servers.size(); i++) {
		// initializes the UDP state
		// This initializes the transfer buffer
		_udp.begin(WiFi.localIP(), localPort);

		makeNTPpacket();
		// all NTP fields have been given values, now
		// you can send a packet requesting a timestamp:
		_udp.beginPacket(servers[i].c_str(), 123); // NTP requests are to port 123
		_udp.write(_packetBuffer, NTP_PACKET_SIZE);
		_udp.endPacket();
		delay(2000);

		if (not _udp.parsePacket()) {
			// HAL.println("UDP Parse packet");
			_udp.stop();
			continue;
		}

		_udp.read(_packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
		unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
		unsigned long lowWord  = word(_packetBuffer[42], _packetBuffer[43]);
		// combine the four bytes (two words) into a long integer
		// this is NTP time (seconds since Jan 1 1900):
		unsigned long secsSince1900 = highWord << 16 | lowWord;
		// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
		const unsigned long seventyYears = 2208988800UL;
		// subtract seventy years:
		unsigned long epoch = secsSince1900 - seventyYears;

		// adjust time for timezone offset in secs +/- from UTC
		// WA time offset from UTC is +9 hours (32400 secs)
		// + East of GMT
		// - West of GMT
		// long tzOffset = 32400UL;

		_udp.stop();

		// WA local time
		// unsigned long adjustedTime;
		// return adjustedTime = epoch + tzOffset;

		return epoch;
	}
	return 0;
}

void HAL_NTP_RTC_API::makeNTPpacket() {
	// set all bytes in the buffer to 0
	for (int i = 0; i < NTP_PACKET_SIZE; ++i) {
		_packetBuffer[i] = 0;
	}
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	_packetBuffer[0] = 0b11100011; // LI, Version, Mode
	_packetBuffer[1] = 0;          // Stratum, or type of clock
	_packetBuffer[2] = 6;          // Polling Interval
	_packetBuffer[3] = 0xEC;       // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	_packetBuffer[12] = 49;
	_packetBuffer[13] = 0x4E;
	_packetBuffer[14] = 49;
	_packetBuffer[15] = 52;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_SETTING_FILE_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_SETTING_FILE_IO_API::saveSettings(String fpath, JsonDocument json) {
	if (not fatfs.begin(&flash)) {
		HAL.println("Err FATFS.");
		return false;
	}

	String tmpl = "";
	serializeJson(json, tmpl);

	File32 f = fatfs.open((fpath).c_str(), (O_WRONLY | O_CREAT | O_TRUNC));
	if (f) {
		f.println(tmpl.c_str());
		f.close();
	} else {
		HAL.println("Open error.");
		fatfs.end();
		return false;
	}

	auto info = getStorageSpaceInfo();
	HAL.printf("Used Bytes:%lu / Total Bytes:%lu (%f %%)\n",
	           info.usedBytes,
	           info.totalBytes,
	           info.usageRate * 100);

	fatfs.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::loadSettings(String fpath, JsonDocument &json) {
	if (not fatfs.begin(&flash)) {
		HAL.println("Err FATFS.");
		return false;
	}

	if (fatfs.exists(fpath.c_str()) == false) {
		HAL.println("File not exists.");
		fatfs.end();
		return false;
	}

	File32 f = fatfs.open((fpath).c_str(), (O_RDONLY));
	if (f) {
		String doc = "";
		while (f.available()) {
			doc += (char)f.read();
		}
		f.close();
		deserializeJson(json, doc.c_str());
	} else {
		HAL.println("Open error.");
		fatfs.end();
		return false;
	}

	fatfs.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::formatStorage() {
	/* FAT12 format. */

	uint8_t workbuf[4096] = {}; // Working buffer for f_mkfs.
	FATFS elmchamFatfs;         // Elm Cham's fatfs objects

	// Make filesystem.
	FRESULT r = ::f_mkfs("", FM_FAT, 0, workbuf, sizeof(workbuf));
	if (r != FR_OK) {
		// f_mkfs failed
		return false;
	}

	// mount to set disk label
	r = ::f_mount(&elmchamFatfs, _drv, 1);
	if (r != FR_OK) {
		// f_mount failed
		return false;
	}

	// Setting label
	constexpr char DISK_LABEL[] = "EXT FLASH";

	r = ::f_setlabel(DISK_LABEL);
	if (r != FR_OK) {
		// f_setlabel failed
		return false;
	}

	// unmount
	::f_unmount(_drv);

	// sync to make sure all data is written to flash
	flash.syncBlocks();
	return true;
}

HAL_SETTING_FILE_IO_API::StorageSpaceInfo HAL_SETTING_FILE_IO_API::getStorageSpaceInfo() {
	StorageSpaceInfo info = {0, 0, 0};
	FATFS *fsinfo;
	DWORD free_clust;
	FATFS elmchamFatfs; // Elm Cham's fatfs objects

	FRESULT ret = ::f_mount(&elmchamFatfs, _drv, 1);
	if (ret != FR_OK) {
		HAL.printf("f_mount failed: %d\n", ret);
		return info;
	}

	ret = ::f_getfree(_drv, &free_clust, &fsinfo);
	if (ret != FR_OK) {
		HAL.printf("f_getfree failed: %d\n", ret);
		::f_unmount(_drv);
		return info;
	}
	::f_unmount(_drv);

	uint16_t ssize = 512; // sector size
#if defined(_MAX_SS) && _MAX_SS != 512
	ssize = (fsinfo->ssize);
#endif

	info.usedBytes  = (((uint64_t)(fsinfo->csize)) * ((fsinfo->n_fatent - 2) - (fsinfo->free_clst)) * ssize);
	info.totalBytes = (((uint64_t)(fsinfo->csize)) * (fsinfo->n_fatent - 2) * ssize);
	info.usageRate  = (double)info.usedBytes / (double)info.totalBytes;

	return info;
}

DSTATUS HAL_SETTING_FILE_IO_API::disk_status(BYTE pdrv) {
	(void)pdrv;
	return 0;
}

DSTATUS HAL_SETTING_FILE_IO_API::disk_initialize(BYTE pdrv) {
	(void)pdrv;
	return 0;
}

DRESULT HAL_SETTING_FILE_IO_API::disk_read(BYTE pdrv,
                                           BYTE *buff,
                                           DWORD sector,
                                           UINT count) {
	(void)pdrv;
	return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT HAL_SETTING_FILE_IO_API::disk_write(BYTE pdrv,
                                            const BYTE *buff,
                                            DWORD sector,
                                            UINT count) {
	(void)pdrv;
	return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT HAL_SETTING_FILE_IO_API::disk_ioctl(BYTE pdrv,
                                            BYTE cmd,
                                            void *buff) {
	(void)pdrv;

	switch (cmd) {
	case CTRL_SYNC:
		flash.syncBlocks();
		return RES_OK;

	case GET_SECTOR_COUNT:
		*((DWORD *)buff) = flash.size() / 512;
		return RES_OK;

	case GET_SECTOR_SIZE:
		*((WORD *)buff) = 512;
		return RES_OK;

	case GET_BLOCK_SIZE:
		*((DWORD *)buff) = 8; // erase block size in units of sector size
		return RES_OK;

	default:
		return RES_PARERR;
	}
}

JsonDocument HAL_SETTING_FILE_IO_API::deviceUniqueSettings() {

	JsonDocument doc;

	JsonObject SAMPLING = doc["SAMPLING"].to<JsonObject>();
	SAMPLING["GPIO_MS"] = "3000";

	String io0              = HAL.Gpio.getModifier() + "0";
	JsonObject IO0          = doc[io0.c_str()].to<JsonObject>();
	IO0["ENABLE"]           = "false";
	IO0["NAME"]             = "IO0";
	IO0["SHIFT_LEVEL"]      = "4";
	IO0["TX_ON_CHANGE"]     = "false";
	IO0["TX_LINK_I2C"]      = "false";
	IO0["TX_LINK_MODBUS"]   = "false";
	IO0["DIGITAL"]          = "false";
	IO0["PULL_UP"]          = "false";
	IO0["PULSE_COUNT_RISE"] = "false";
	IO0["PULSE_COUNT_FALL"] = "false";

	String io1              = HAL.Gpio.getModifier() + "1";
	JsonObject IO1          = doc[io1.c_str()].to<JsonObject>();
	IO1["ENABLE"]           = "false";
	IO1["NAME"]             = "IO1";
	IO1["SHIFT_LEVEL"]      = "4";
	IO1["TX_ON_CHANGE"]     = "false";
	IO1["TX_LINK_I2C"]      = "false";
	IO1["TX_LINK_MODBUS"]   = "false";
	IO1["DIGITAL"]          = "false";
	IO1["PULL_UP"]          = "false";
	IO1["PULSE_COUNT_RISE"] = "false";
	IO1["PULSE_COUNT_FALL"] = "false";

	return doc;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_BLE_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
IBluetoothLowEnergyControl::LocalBuffer IBluetoothLowEnergyControl::rx_buffer = {0};
IBluetoothLowEnergyControl::LocalBuffer IBluetoothLowEnergyControl::tx_buffer = {0};

class MyServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer *pServer) {};

	void onDisconnect(BLEServer *pServer) {
		pServer->startAdvertising(); // restart advertising
	}
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		std::string rxValue = pCharacteristic->getValue();

		if (rxValue.length() > 0) {
			for (int i = 0; i < rxValue.length(); i++) {
				HAL_BLE_API::rx_buffer.data[HAL_BLE_API::rx_buffer.store_index] = rxValue[i];
				HAL_BLE_API::rx_buffer.length += 1;
				HAL_BLE_API::rx_buffer.store_index = ((HAL_BLE_API::rx_buffer.store_index + 1) == BLE_BUF_LENGTH) ? 0 : (HAL_BLE_API::rx_buffer.store_index + 1);
			}
		}
	}

	void onRead(BLECharacteristic *pCharacteristic) {
		if (HAL_BLE_API::tx_buffer.length > 0) {
			pCharacteristic->setValue(HAL_BLE_API::tx_buffer.data, HAL_BLE_API::tx_buffer.length);
			HAL_BLE_API::tx_buffer.length = 0;
		} else {
			pCharacteristic->setValue(NULL, 0);
		}
	}
};

void HAL_BLE_API::begin(String local_name) {
	if (local_name.length() > BLE_MAX_LOCAL_NAME_LEN) {
		local_name = local_name.substring(0, BLE_MAX_LOCAL_NAME_LEN);
	}

	// Create the BLE Device
	BLEDevice::init(local_name.c_str());

	// Create the BLE Server
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(SERVICE_UUID);

	// Create a BLE Characteristic
	pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
	                                                   BLECharacteristic::PROPERTY_READ);
	// pTxCharacteristic->setAccessPermissions(GATT_PERM_READ);
	// pTxCharacteristic->addDescriptor(new BLE2902());
	pTxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

	BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,
	                                                                      BLECharacteristic::PROPERTY_WRITE);
	// pRxCharacteristic->setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);
	pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

	BLEAdvertising *pAdvertising = pServer->getAdvertising();
	// pAdvertising->setScanResponse(false); // important to send LocalName

	// Start the service
	pService->start();
	// Start advertising
	pServer->getAdvertising()->start();
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_TF_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// bool HAL_TF_API::begin() {
// 	_tf_available = false;
// 	if (SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
// 		_tf_available = true;
// 	} else {
// 		SD.end();
// 	}

// 	return _tf_available;
// }

// bool HAL_TF_API::exist(String fpath) {
// 	if (not _tf_available) {
// 		return false;
// 	}
// 	fpath = SD.getDriveLetter() + fpath;
// 	return SD.exists(fpath.c_str());
// }

// bool HAL_TF_API::makeDirRecursive(String absolute_path) {
// 	if (not _tf_available) {
// 		return false;
// 	}

// 	String exist_dir_path = SD.getDriveLetter();
// 	while (1) {
// 		int sp = absolute_path.indexOf('/', 1);

// 		String d;
// 		if (sp < 0) {
// 			d = absolute_path.substring(1, absolute_path.length());
// 			if (d.length() == 0) {
// 				break;
// 			}
// 		} else {
// 			d = absolute_path.substring(1, sp);
// 		}
// 		exist_dir_path += "/" + d;
// 		if (not SD.exists(exist_dir_path.c_str())) {
// 			if (not SD.mkdir(exist_dir_path.c_str())) {
// 				return false;
// 			}
// 		}
// 		absolute_path = absolute_path.substring(sp, absolute_path.length());
// 	}

// 	return true;
// }

// bool HAL_TF_API::print(const char *fpath, const char *mode, const char *str) {
// 	if (not _tf_available) {
// 		return false;
// 	}

// 	String filepath = SD.getDriveLetter() + String(fpath);
// 	File f          = SD.open(filepath.c_str(), mode);
// 	if (not f) {
// 		return false;
// 	}
// 	f.print(str);
// 	f.close();

// 	return true;
// }

// bool HAL_TF_API::printf(const char *fpath, const char *mode, const char *fmt, ...) {
// 	if (not _tf_available) {
// 		return false;
// 	}

// 	char s[256] = {0};
// 	va_list args;

// 	::va_start(args, fmt);
// 	::vsnprintf(s, 256, fmt, args);
// 	::va_end(args);

// 	return HAL.Tf.print(fpath, mode, (const char *)s);
// }

// bool HAL_TF_API::initializeSemaphoreMutex() {
// 	return HAL.initializeSemaphoreMutex("spi");
// }

// bool HAL_TF_API::takeSemaphoreMutex(portTickType wait_tick_time) {
// 	return HAL.takeSemaphoreMutex("spi", wait_tick_time);
// }

// bool HAL_TF_API::giveSemaphoreMutex() {
// 	return HAL.giveSemaphoreMutex("spi");
// }

// /*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
// //
// //  MARK: HAL_Debug_Log_API
// //
// /*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// void HAL_DEBUG_LOG_API::buffering(const char *str) {
// 	static bool insert_time_flag = true;

// 	if (not (_inited and _logging)) {
// 		return;
// 	}

// 	if (_current_linage >= _limit_linage) {
// 		_fnumber += 1;
// 		_current_linage = 0;
// 	}

// 	int i = 0;
// 	while (str[i] != '\0') {
// 		if (str[i] == '\n') {
// 			_current_linage += 1;
// 		}
// 		i++;
// 	}

// 	if (HAL.NtpRtc.ready()) {
// 		if (insert_time_flag) {
// 			struct tm timeinfo = {0};
// 			char s_day[64]     = {0};
// 			HAL.NtpRtc.getCurrentTime(&timeinfo);
// 			strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &timeinfo);
// 			_buffer += String(s_day) + " -> " + String(str);
// 			insert_time_flag = false;
// 		} else {
// 			_buffer += String(str);
// 		}
// 	} else {
// 		_buffer += String(str);
// 	}

// 	if (_buffer.endsWith("\n")) {
// 		insert_time_flag = true;
// 	}
// }

// bool HAL_DEBUG_LOG_API::begin() {
// 	if (not HAL.Tf.ready()) {
// 		if (not HAL.Tf.begin()) {
// 			Serial1.println("    Failed to mount SD card");
// 			return false;
// 		}
// 	}

// 	if (not HAL.Tf.exist(_dpath)) {
// 		if (not HAL.Tf.makeDirRecursive(_dpath)) {
// 			Serial1.println("    Failed to create directory");
// 			return false;
// 		}
// 	}

// 	// Binary Tree Search for file name decision
// 	// unsigned long long start = millis();
// 	uint16_t left  = 0;
// 	uint16_t right = UINT16_MAX / 32; // Because in most cases, the number of files is less than 2000
// 	if (HAL.Tf.exist(HAL.Tf.joinPath(_dpath, _fname + String(UINT16_MAX / 32) + _fext))) {
// 		if (HAL.Tf.exist(HAL.Tf.joinPath(_dpath, _fname + String(UINT16_MAX / 6) + _fext))) {
// 			left  = UINT16_MAX / 6; // about 10000
// 			right = UINT16_MAX;
// 		} else {
// 			left  = UINT16_MAX / 32; // about 2000
// 			right = UINT16_MAX / 6;  // about 10000
// 		}
// 	}
// 	uint16_t middle = 0;

// 	while (right - left > 1) {
// 		middle = left + ((right - left) / 2);
// 		if (HAL.Tf.exist(HAL.Tf.joinPath(_dpath, _fname + String(middle) + _fext))) {
// 			left = middle;
// 		} else {
// 			right = middle;
// 		}
// 	}
// 	_fnumber = ((left > right) ? left : right); // Select max index
// 	// HAL.printf("%lu ms\n", millis() - start);

// 	_buffer = "";
// 	_inited = true;
// 	return true;
// }

// bool HAL_DEBUG_LOG_API::print(const char *str) {
// 	if (not (_inited and _logging)) {
// 		return false;
// 	}

// 	buffering(str);

// 	String log_fpath = HAL.Tf.joinPath(_dpath, _fname + String(_fnumber) + _fext);
// 	if (not HAL.Tf.exist(_dpath)) {
// 		if (not HAL.Tf.makeDirRecursive(_dpath)) {
// 			return false;
// 		}
// 	}

// 	bool result = HAL.Tf.print(log_fpath.c_str(), "a", _buffer.c_str());
// 	if (result) {
// 		_buffer = "";
// 	}

// 	return result;
// }

// bool HAL_DEBUG_LOG_API::printf(const char *fmt, ...) {
// 	if (not (_inited and _logging)) {
// 		return false;
// 	}

// 	char s[256] = {0};
// 	va_list args;

// 	::va_start(args, fmt);
// 	::vsnprintf(s, 256, fmt, args);
// 	::va_end(args);

// 	return HAL.Log.print(s);
// }

// void HAL_DEBUG_LOG_API::flush() {
// 	if (not (_inited and _logging)) {
// 		return;
// 	}

// 	if (_buffer.length() == 0) {
// 		return;
// 	}

// 	String log_fpath = HAL.Tf.joinPath(_dpath, _fname + String(_fnumber) + _fext);
// 	if (not HAL.Tf.exist(_dpath)) {
// 		if (not HAL.Tf.makeDirRecursive(_dpath)) {
// 			return;
// 		}
// 	}

// 	bool result = HAL.Tf.print(log_fpath.c_str(), "a", _buffer.c_str());
// 	if (result) {
// 		_buffer = "";
// 	}
// }

// void HAL_DEBUG_LOG_API::loggingStart() {
// 	if (not _inited) {
// 		_logging = false;
// 	}

// 	if (digitalRead(SDCARD_DET_PIN) == 1) {
// 		_logging = false;
// 	}

// 	_logging = true;
// }

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_GPIO_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

std::vector<uint8_t> HAL_GPIO_API::getPins() {
	std::vector<uint8_t> pins = {0, 1};
	return pins;
}

String HAL_GPIO_API::getModifier() {
	return "IO";
}

bool HAL_GPIO_API::initializePulseCounter(uint8_t pin, PULSE_EDGE edge) {
	auto pins = getPins();
	if (std::count(pins.begin(), pins.end(), pin) == 0) {
		// GPIO のピンとして使用されていない
		return false;
	}

	uint8_t ulPort = g_APinDescription[pin].ulPort;
	uint8_t ulPin  = g_APinDescription[pin].ulPin;

	// ピン機能の有効化
	PORT->Group[ulPort].PINCFG[ulPin].bit.PMUXEN = 1;       // ピン機能を有効化（周辺マルチプレクサの有効化）
	PORT->Group[ulPort].PMUX[ulPin].reg |= (ulPin % 2 == 0) // Peripheral Multiplexing
	                                           ? PORT_PMUX_PMUXE(ulPort + 1)
	                                           : PORT_PMUX_PMUXO(ulPort + 1);

	EIC->CTRLA.bit.ENABLE = 0; // 外部割り込みコントローラを無効化
	while (EIC->SYNCBUSY.bit.ENABLE) {
		// 無効化待ち
	}

	uint8_t bank = (ulPin < 8) ? 0 : 1;
	switch (edge) {
	case PULSE_EDGE::RISE:
		EIC->CONFIG[bank].reg |= (EIC_CONFIG_SENSE0_RISE_Val << ((ulPin % 8) * 4));
		break;
	case PULSE_EDGE::FALL:
		EIC->CONFIG[bank].reg |= (EIC_CONFIG_SENSE0_FALL_Val << ((ulPin % 8) * 4));
		break;
	case PULSE_EDGE::BOTH:
		EIC->CONFIG[bank].reg |= (EIC_CONFIG_SENSE0_BOTH_Val << ((ulPin % 8) * 4));
		break;
	default:
		return false; // 不適切なパラメータ
	}

	EIC->EVCTRL.reg |= 1 << ulPin;   // 割り込みイベントを設定
	EIC->INTENCLR.reg |= 1 << ulPin; // 割り込みを無効化
	EIC->ASYNCH.reg |= 1 << ulPin;   // 非同期モードを有効化
	EIC->CTRLA.bit.ENABLE = 1;       // 外部割り込みコントローラを有効化
	while (EIC->SYNCBUSY.bit.ENABLE) {
		// 有効化待ち
	}

	MCLK->APBBMASK.reg |= MCLK_APBBMASK_EVSYS; // イベントシステムを有効化

	_TimerNumber[ulPin] = _TIMER;
	_TIMER++;

	Tc *TC;
	switch (_TimerNumber[ulPin]) {
	case 0:
		MCLK->APBAMASK.bit.TC0_                = 1;
		GCLK->PCHCTRL[TC0_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC0_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC0;
		break;
	case 1:
		MCLK->APBAMASK.bit.TC1_                = 1;
		GCLK->PCHCTRL[TC1_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC1_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC1;
		break;
	case 2:
		MCLK->APBBMASK.bit.TC2_                = 1;
		GCLK->PCHCTRL[TC2_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC2_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC2;
		break;
	case 3:
		MCLK->APBBMASK.bit.TC3_                = 1;
		GCLK->PCHCTRL[TC3_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC3_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC3;
		break;
	case 4:
		MCLK->APBCMASK.bit.TC4_                = 1;
		GCLK->PCHCTRL[TC4_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC4_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC4;
		break;
	case 5:
		MCLK->APBCMASK.bit.TC5_                = 1;
		GCLK->PCHCTRL[TC5_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC5_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC5;
		break;
	case 6:
		MCLK->APBDMASK.bit.TC6_                = 1;
		GCLK->PCHCTRL[TC6_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC6_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC6;
		break;
	case 7:
		MCLK->APBDMASK.bit.TC7_                = 1;
		GCLK->PCHCTRL[TC7_GCLK_ID].reg         = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
		EVSYS->USER[EVSYS_ID_USER_TC7_EVU].reg = EVSYS_USER_CHANNEL(_USER_CHANNEL);
		TC                                     = TC7;
		break;
	default:
		return false; // 不適切なパラメータ
	}

	// 外部割り込みイベントを設定（EVSYS_CHANNEL_EVGEN は将来的に変更の可能性あり）
	EVSYS->Channel[_USER_CHANNEL - 1].CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |
	                                                EVSYS_CHANNEL_PATH_ASYNCHRONOUS |
	                                                EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_0 + ulPin);

	// 最大値は 65535
	TC->COUNT16.CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16;                  // 16ビットカウンタモードに設定
	TC->COUNT16.EVCTRL.reg     = TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT; // カウントイベントを設定

	// TC->COUNT16.CTRLA.bit.ENABLE = 1;  // 有効化
	// while (TC->COUNT16.SYNCBUSY.bit.ENABLE) {
	// 	// 有効化待ち
	// }

	_USER_CHANNEL++;

	return true;
}

bool HAL_GPIO_API::stopPulseCounter(uint8_t pin) {
	uint8_t ulPin = g_APinDescription[pin].ulPin;
	if (_TimerNumber.find(ulPin) == _TimerNumber.end()) {
		return false; // タイマー番号が見つからない場合、0を返す
	}

	Tc *TC = getTimerObj(_TimerNumber[ulPin]);
	if (TC == nullptr) {
		return false;
	}

	TC->COUNT16.CTRLA.bit.ENABLE = 0;
	while (TC->COUNT16.SYNCBUSY.bit.ENABLE) {
		// 設定が反映されるのを待つ
	}
	return true;
}

bool HAL_GPIO_API::startPulseCounter(uint8_t pin) {
	uint8_t ulPin = g_APinDescription[pin].ulPin;
	if (_TimerNumber.find(ulPin) == _TimerNumber.end()) {
		return false; // タイマー番号が見つからない場合、0を返す
	}

	Tc *TC = getTimerObj(_TimerNumber[ulPin]);
	if (TC == nullptr) {
		return false;
	}

	TC->COUNT16.CTRLA.bit.ENABLE = 1;
	while (TC->COUNT16.SYNCBUSY.bit.ENABLE) {
		// 設定が反映されるのを待つ
	}
	return true;
}

bool HAL_GPIO_API::stopPulseCounterAll() {
	for (auto item : _TimerNumber) {
		Tc *TC = getTimerObj(item.second);
		if (TC == nullptr) {
			continue;
		}

		TC->COUNT16.CTRLA.bit.ENABLE = 0;
		while (TC->COUNT16.SYNCBUSY.bit.ENABLE) {
			// 設定が反映されるのを待つ
		}
	}
	return true;
}

bool HAL_GPIO_API::startPulseCounterAll() {
	for (auto item : _TimerNumber) {
		Tc *TC = getTimerObj(item.second);
		if (TC == nullptr) {
			continue;
		}

		TC->COUNT16.CTRLA.bit.ENABLE = 1;
		while (TC->COUNT16.SYNCBUSY.bit.ENABLE) {
			// 設定が反映されるのを待つ
		}
	}

	return true;
}

uint32_t HAL_GPIO_API::getMaxPulseCount() {
	return UINT16_MAX;
}

bool HAL_GPIO_API::updatePulseCount(uint8_t pin) {
	uint8_t ulPin = g_APinDescription[pin].ulPin;
	if (_TimerNumber.find(ulPin) == _TimerNumber.end()) {
		return false; // タイマー番号が見つからない場合、0を返す
	}

	Tc *TC = getTimerObj(_TimerNumber[ulPin]);
	if (TC == nullptr) {
		return false;
	}

	// カウンタ値の同期読み取りを有効化
	TC->COUNT16.CTRLBSET.reg = TC_CTRLBCLR_CMD_READSYNC;
	while (TC->COUNT16.SYNCBUSY.bit.CTRLB) {
		// 同期待ち
	}
	while (TC->COUNT16.CTRLBSET.bit.CMD) {
		// 同期待ち
	}
	while (TC->COUNT16.SYNCBUSY.bit.COUNT) {
		// 同期待ち
	}

	// 同期は1回のみ、読み取り後は自動的に非同期になるハズ
	_count[pin] = TC->COUNT16.COUNT.reg;
	return true;
}

void HAL_GPIO_API::resetPulseCount(uint8_t pin) {
	uint8_t ulPin = g_APinDescription[pin].ulPin;
	if (_TimerNumber.find(ulPin) == _TimerNumber.end()) {
		return; // タイマー番号が見つからない場合、0を返す
	}

	Tc *TC = getTimerObj(_TimerNumber[ulPin]);
	if (TC == nullptr) {
		return;
	}

	TC->COUNT16.CTRLBSET.reg = TC_CTRLBCLR_CMD_RETRIGGER; // カウンタをリトリガー
	while (TC->COUNT16.SYNCBUSY.bit.CTRLB) {
		// 同期待ち
	}
	// while (TC->COUNT16.CTRLBSET.bit.CMD) {
	// 	// 同期待ち
	// }
}

void HAL_GPIO_API::resetPulseCountAll() {
	for (auto item : _TimerNumber) {
		Tc *TC = getTimerObj(item.second);
		if (TC == nullptr) {
			continue;
		}

		TC->COUNT16.CTRLBSET.reg = TC_CTRLBCLR_CMD_RETRIGGER; // カウンタをリトリガー
		while (TC->COUNT16.SYNCBUSY.bit.CTRLB) {
			// 同期待ち
		}
		// while (TC->COUNT16.CTRLBSET.bit.CMD) {
		// 	// 同期待ち
		// }
	}
}

Tc *HAL_GPIO_API::getTimerObj(uint8_t num) {
	switch (num) {
	case 0:
		return TC0;
	case 1:
		return TC1;
	case 2:
		return TC2;
	case 3:
		return TC3;
	case 4:
		return TC4;
	case 5:
		return TC5;
	case 6:
		return TC6;
	case 7:
		return TC7;
	default:
		return nullptr; // 不適切なパラメータ
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

		// 未接続時の誤検出防止用
		pinMode(SDA, OUTPUT);
		pinMode(SCL, INPUT);
		digitalWrite(SDA, LOW);

		Wire.begin(_sda, _scl);
		_wires[port] = &Wire;

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
	return HAL.initializeSemaphoreMutex("spi");
}

bool HAL_Display_API::takeSemaphoreMutex(portTickType wait_tick_time) {
	return HAL.takeSemaphoreMutex("spi", wait_tick_time);
}

bool HAL_Display_API::giveSemaphoreMutex() {
	return HAL.giveSemaphoreMutex("spi");
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: fatfs diskio
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

extern "C" {
DSTATUS disk_status(BYTE pdrv) {
	return HAL.FileIO.disk_status(pdrv);
}

DSTATUS disk_initialize(BYTE pdrv) {
	return HAL.FileIO.disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv,
                  BYTE *buff,
                  DWORD sector,
                  UINT count) {
	return HAL.FileIO.disk_read(pdrv, buff, sector, count);
}

DRESULT disk_write(BYTE pdrv,
                   const BYTE *buff,
                   DWORD sector,
                   UINT count) {
	return HAL.FileIO.disk_write(pdrv, buff, sector, count);
}

DRESULT disk_ioctl(BYTE pdrv,
                   BYTE cmd,
                   void *buff) {
	return HAL.FileIO.disk_ioctl(pdrv, cmd, buff);
}
}

#endif /* HAL_API_WIO_H */