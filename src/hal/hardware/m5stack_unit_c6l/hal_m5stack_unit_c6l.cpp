#include "hal_m5stack_unit_c6l.h"
#ifdef HAL_API_M5STACK_UNIT_C6L_H

HAL_API HAL;

// Button API
#define DEBOUNCE_MS (10)

#define TIM_0 (0)
#define TIM_1 (1)
#define TIM_2 (2)
#define TIM_3 (3)

void IRAM_ATTR onTimer0();
void IRAM_ATTR onTimer1();
void IRAM_ATTR onTimer2();
void IRAM_ATTR onTimer3();

namespace {
	M5GFX GFX;

	Button BtnA = Button(0, true, DEBOUNCE_MS);

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

	static String bytesToHex(const uint8_t *data, uint8_t len);
	String getFactoryDevEUI();
	String getFactoryAppKey();

	RTC_DATA_ATTR static uint8_t rtcLoRaWanSession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_API::begin() {
	if (HAL.isWakeFromSleep()) {
		::gpio_hold_dis(GPIO_NUM_23); // LORA_CS  (HIGH 固定: SX1262 非アクティブ)
		::gpio_hold_dis(GPIO_NUM_20); // SPI_SCK  (フローティング防止)
		::gpio_hold_dis(GPIO_NUM_21); // SPI_MOSI (フローティング防止)
		::gpio_hold_dis(GPIO_NUM_11); // BEEP

		::rtc_gpio_hold_dis(GPIO_NUM_6); // LCD_CS   (HIGH 固定: OLCD 非アクティブ)

		::rtc_gpio_hold_dis(GPIO_NUM_0); // 未使用
		::rtc_gpio_hold_dis(GPIO_NUM_1); // 未使用
		::rtc_gpio_hold_dis(GPIO_NUM_2); // RGB_LED_DAT
		::rtc_gpio_hold_dis(GPIO_NUM_3); // IO Expander IRQ
		::rtc_gpio_hold_dis(GPIO_NUM_4); // Grove
		::rtc_gpio_hold_dis(GPIO_NUM_5); // Grove
		::rtc_gpio_hold_dis(GPIO_NUM_7); // SX1262 IRQ
	}

	Serial.begin(115200);
	if (not HAL.isWakeFromSleep()) {
		HAL.delay(1000);
	}

	GFX.init();
	Wire.begin(IN_SDA, IN_SCL, 100000);

	constexpr uint8_t LED_PIN = 2;
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	HAL.initializeSemaphoreMutex("in_i2c"); // for PI4IOE5V
	HAL.takeSemaphoreMutex("in_i2c", portMAX_DELAY);

	// IOエキスパンダを初期化
	if (IO_Multiplexer::initialize(Wire) == false) {
		HAL.giveSemaphoreMutex("in_i2c");
		Serial.println("Error: IO Expander init failed.");
		HAL.delay(5000);
		HAL.reboot();
	}

	HAL.giveSemaphoreMutex("in_i2c");

	if (HAL.isWakeFromSleep()) {
		GFX.setBrightness(0);      // Mutex で囲った方がイイかも
		GFX.fillScreen(TFT_BLACK); // Mutex で囲った方がイイかも
		GFX.sleep();               // Mutex で囲った方がイイかも
	}

	// HAL.printf("Freq: %d\n", getCpuFrequencyMhz());

	HAL.printf("Board Manager Version: %d.%d.%d\n",
	           ESP_ARDUINO_VERSION_MAJOR,
	           ESP_ARDUINO_VERSION_MINOR,
	           ESP_ARDUINO_VERSION_PATCH);

	HAL.Info.DEVICE_NAME = "M5Stack Unit C6L";
}

void HAL_API::update() {
	HAL.takeSemaphoreMutex("in_i2c", portMAX_DELAY);
	BtnA.read();
	HAL.giveSemaphoreMutex("in_i2c");
}

uint8_t HAL_API::waitForButtonPressed(uint16_t wait_ms) {
	uint8_t ret           = (uint8_t)PRESSED_BUTTON::NONE;
	bool all_free_flag    = false;
	unsigned long long ts = millis();

	while ((millis() - ts) < wait_ms) {
		if (not all_free_flag) {
			HAL.update();
			if (not BtnA.isPressed()) {
				all_free_flag = true;
			}
		} else {
			HAL.update();
			if (BtnA.isPressed()) {
				ret |= (uint8_t)PRESSED_BUTTON::PRESSED_BTN1;
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
	static bool button_pressed   = false;
	static unsigned long long ts = 0;

	if (BtnA.isPressed() and not button_pressed) {
		button_pressed = true;
		ts             = millis();
	} else if (BtnA.isReleased() and button_pressed) {
		button_pressed = false;
		if ((millis() - ts) < 1500) {
			return HIGH;
		}
	}
	return LOW;
}

uint8_t HAL_API::BtnB_wasPressed() {
	return LOW;
}

uint8_t HAL_API::BtnC_wasPressed() {
	static bool is_released = true;
	uint8_t result;
	if (is_released == false and BtnA.isPressed() == true) {
		return LOW;
	}
	result = BtnA.pressedFor(1500);
	// HAL.printf("%d -- %d\n", result, M5.BtnA.pressedFor(1500));
	is_released = (result == HIGH) ? false : true;
	return result;
}

uint8_t HAL_API::BtnA_isPressed() {
	return BtnA.isPressed();
}

uint8_t HAL_API::BtnB_isPressed() {
	return LOW;
}

uint8_t HAL_API::BtnC_isPressed() {
	return LOW;
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

void HAL_API::printf(const char *fmt, ...) {
	// if (HAL.isWakeFromSleep()) {
	// 	return;
	// }

	char s[256] = {0};
	va_list args;
	::va_start(args, fmt);
	::vsnprintf(s, 256, fmt, args);
	::va_end(args);
	Serial.print(s);
	Serial.flush();
}

void HAL_API::println(const char *s) {
	// if (HAL.isWakeFromSleep()) {
	// 	return;
	// }

	Serial.println(s);
	Serial.flush();
}

void HAL_API::print(const char *s) {
	// if (HAL.isWakeFromSleep()) {
	// 	return;
	// }

	Serial.print(s);
	Serial.flush();
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
	return (esp_random() % (maximum - minimum)) + minimum;
}

void HAL_API::delay(uint32_t ms) {
	//::esp_task_wdt_reset();
	::delay(ms);
}

void HAL_API::reboot() {
	ESP.restart();
}

void HAL_API::system_sleep(uint32_t ms) {
	constexpr uint32_t COLD_BOOT_OFFSET_MS = 20000;
	constexpr uint32_t MANUAL_OFFSET_MS    = 1500;
	if (HAL.LoRaWan.isJoined()) {
		LoRaWan.saveSession();
	}
	HAL.LoRaWan.sleep(); // モジュールをスリープに

	// ディスプレイを省電力モードに
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
	GFX.setBrightness(0);
	GFX.fillScreen(TFT_BLACK);
	GFX.sleep();
	HAL.Display.giveSemaphoreMutex();

	::gpio_hold_en(GPIO_NUM_23); // LORA_CS  (HIGH 固定: SX1262 非アクティブ)
	::gpio_hold_en(GPIO_NUM_20); // SPI_SCK  (フローティング防止)
	::gpio_hold_en(GPIO_NUM_21); // SPI_MOSI (フローティング防止)
	::gpio_hold_en(GPIO_NUM_11); // BEEP

	::rtc_gpio_hold_en(GPIO_NUM_6); // LCD_CS   (HIGH 固定: OLCD 非アクティブ)

	IO_Multiplexer::switchRF(false);
	IO_Multiplexer::enableLNA(false);
	// for (int i = 0; i < 7; i++) {
	// 	IO_Multiplexer::writePin(i, false);
	// }

	::esp_bt_controller_disable();
	::esp_bt_controller_deinit();
	::esp_wifi_stop();

	// 全ての wakeup トリガを無効化
	::esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

	// Flash のパワーダウン（Deep Sleep なら不要かも）
	//::esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);

	// LP_IO ピン: isolate（GPIO6 以外すべて）
	::rtc_gpio_isolate(GPIO_NUM_0); // 未使用
	::rtc_gpio_isolate(GPIO_NUM_1); // 未使用
	::rtc_gpio_isolate(GPIO_NUM_2); // RGB_LED_DAT
	::rtc_gpio_isolate(GPIO_NUM_3); // IO Expander IRQ
	::rtc_gpio_isolate(GPIO_NUM_4); // Grove
	::rtc_gpio_isolate(GPIO_NUM_5); // Grove
	::rtc_gpio_isolate(GPIO_NUM_7); // SX1262 IRQ

	// 起動にかかる時間を考慮してスリープ時間を調整
	uint32_t offset_ms = HAL.isWakeFromSleep() ? millis() - MANUAL_OFFSET_MS : COLD_BOOT_OFFSET_MS;
	uint32_t sleep_ms  = (ms > offset_ms) ? (ms - offset_ms) : ms;

	// HAL.printf("Sleep: %d\n", sleep_ms);
	// HAL.delay(1000);

	::esp_sleep_enable_timer_wakeup(sleep_ms * 1000UL); // function uses uS
	::esp_deep_sleep_start();
}

bool HAL_API::isWakeFromSleep() {
	return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
}

void HAL_API::setPowerMode(bool wifi_required) {
	// HAL.printf("Freq: %d\n", getCpuFrequencyMhz());

	if (wifi_required) {
		// 動作周波数を変更
		::setCpuFrequencyMhz(80);
		// HAL.printf("Freq: %d\n", getCpuFrequencyMhz());
		return;
	}

	// 動作周波数を変更
	::setCpuFrequencyMhz(40);
	// HAL.printf("Freq: %d\n", getCpuFrequencyMhz());
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
//  MARK: HAL_LoRaWAN_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

namespace {
	constexpr uint8_t LORA_MOSI       = 21;
	constexpr uint8_t LORA_MISO       = 22;
	constexpr uint8_t LORA_SCK        = 20;
	constexpr uint8_t LORA_CS         = 23;
	constexpr uint8_t LORA_BUSY       = 19;
	constexpr uint8_t LORA_IRQ        = 7;
	constexpr uint16_t E0_LORA_ANT_SW = 906;
	constexpr uint16_t E0_LORA_LNA_EN = 905;
	constexpr uint16_t E0_LORA_NRST   = 907;

	class C6L_RadioHal : public ArduinoHal {
	public:
		using ArduinoHal::ArduinoHal;

		void digitalWrite(uint32_t pin, uint32_t value) override {
			switch (pin) {
			case E0_LORA_NRST: {
				HAL.takeSemaphoreMutex("in_i2c", portMAX_DELAY);
				IO_Multiplexer::setNRST(value);
				HAL.giveSemaphoreMutex("in_i2c");
				break;
			}
			case E0_LORA_LNA_EN: {
				HAL.takeSemaphoreMutex("in_i2c", portMAX_DELAY);
				IO_Multiplexer::enableLNA(value);
				HAL.giveSemaphoreMutex("in_i2c");
				break;
			}
			case E0_LORA_ANT_SW: {
				HAL.takeSemaphoreMutex("in_i2c", portMAX_DELAY);
				IO_Multiplexer::switchRF(value);
				HAL.giveSemaphoreMutex("in_i2c");
				break;
			}
			default:
				ArduinoHal::digitalWrite(pin, value);
				break;
			}
		}
	};

	class LoRaMutexGuard {
	public:
		LoRaMutexGuard() {
			HAL.takeSemaphoreMutex("spi", portMAX_DELAY);
		}
		~LoRaMutexGuard() {
			HAL.giveSemaphoreMutex("spi");
		}

		// コピーコンストラクタと代入演算子を削除
		LoRaMutexGuard(const LoRaMutexGuard &)            = delete;
		LoRaMutexGuard &operator=(const LoRaMutexGuard &) = delete;
	};
}

bool HAL_LoRaWAN_API::_initRadio() {
	int result = RADIOLIB_ERR_NONE;

	HAL.initializeSemaphoreMutex("spi"); // for SX1262

	// RadioLib 内部で使用される乱数を生成するためにランダムシードを設定
	randomSeed(HAL.getRandomNumber());

	{
		LoRaMutexGuard _;
		static C6L_RadioHal radioHal(SPI);
		_radio = new SX1262(new Module(&radioHal, LORA_CS, LORA_IRQ, E0_LORA_NRST, LORA_BUSY));
		result = _radio->begin();
	}
	if (not(result == RADIOLIB_ERR_NONE)) {
		HAL.println("\n        Radio initialize error.");
		HAL.printf("        result: %d\n", result);
		_last_error = "Err: " + String(result);
		return false;
	}

	// nonce 値保存用の nvs 領域を初期化（OTAA で使用）
	bool nvs_ok = false;
	for (int i = 0; i < 2; i++) {
		if (store.begin("lorawan")) {
			nvs_ok = true;
			break;
		}
		nvs_flash_erase();
		nvs_flash_init();
	}
	if (not nvs_ok) {
		HAL.println("\n        NVS initialize error.");
		_last_error = "NVS error";
		return false;
	}

	_initialized = true;
	return true;
}

bool HAL_LoRaWAN_API::beginOTAA(String dev_eui, String join_eui, String app_key) {
	if (_initialized) {
		return true;
	}

	if (not parseHexString(dev_eui, _lorawan_dev_eui)) {
		return false;
	}
	if (not parseHexString(join_eui, _lorawan_join_eui)) {
		return false;
	}
	if (not ILoRaWanControl::parseHexString(app_key, _lorawan_app_key, 16)) {
		return false;
	}

	_is_abp = false;
	return _initRadio();
}

bool HAL_LoRaWAN_API::beginABP(String dev_addr, String nwk_s_key, String app_s_key) {
	if (_initialized) {
		return true;
	}

	// DEV_ADDR: 8桁 hex → uint32_t
	uint8_t addr_buf[4] = {0};
	if (not ILoRaWanControl::parseHexString(dev_addr, addr_buf, 4)) {
		return false;
	}
	_lorawan_dev_addr = ((uint32_t)addr_buf[0] << 24) | ((uint32_t)addr_buf[1] << 16) | ((uint32_t)addr_buf[2] << 8) | addr_buf[3];

	if (not ILoRaWanControl::parseHexString(nwk_s_key, _lorawan_nwk_s_key, 16)) {
		return false;
	}
	if (not ILoRaWanControl::parseHexString(app_s_key, _lorawan_app_s_key, 16)) {
		return false;
	}

	_is_abp = true;
	return _initRadio();
}

bool HAL_LoRaWAN_API::join(unsigned long timeout_ms) {
	if (not _initialized) {
		return false;
	}

	int result = RADIOLIB_ERR_NONE;

	if (_is_abp) {
		// ABP: サーバーとのネットワーク交換なし、即座に完了する
		{
			LoRaMutexGuard _;
			_node = new LoRaWANNode(_radio, &AS923);
			// LoRaWAN 1.0: fNwkSIntKey = sNwkSIntKey = nwkSEncKey = NwkSKey
			result = _node->beginABP(_lorawan_dev_addr,
			                         _lorawan_nwk_s_key, _lorawan_nwk_s_key, _lorawan_nwk_s_key,
			                         _lorawan_app_s_key);
		}
		if (not(result == RADIOLIB_ERR_NONE)) {
			HAL.println("\n        ABP initialize error.");
			HAL.printf("        result: %d\n", result);
			_last_error = "Err: " + String(result);
			return false;
		}

		{
			LoRaMutexGuard _;
			_node->setADR(true);
			_node->setTxPower(AS923.powerMax);
			result = _node->setDatarate(3);
		}
		if (not(result == RADIOLIB_ERR_NONE)) {
			HAL.println("\n        Datarate initialize error.");
			HAL.printf("        result: %d\n", result);
			_last_error = "Err: " + String(result);
			return false;
		}

		{
			LoRaMutexGuard _;
			result = _node->activateABP();
		}
		if (not(result == RADIOLIB_ERR_NONE)) {
			HAL.println("\n        Activate ABP error.");
			HAL.printf("        result: %d\n", result);
			_last_error = "Err: " + String(result);
			return false;
		}

		return true;
	}

	// OTAA
	{
		LoRaMutexGuard _;
		_node = new LoRaWANNode(_radio, &AS923);
		// LoRaWAN 1.0 系のみサポート
		result = _node->beginOTAA(_lorawan_join_eui, _lorawan_dev_eui, NULL, _lorawan_app_key);
	}
	if (not(result == RADIOLIB_ERR_NONE)) {
		HAL.println("\n        OTAA initialize error.");
		HAL.printf("        result: %d\n", result);
		_last_error = "Err: " + String(result);
		return false;
	}

	{
		LoRaMutexGuard _;
		_node->setADR(true);
		_node->setTxPower(AS923.powerMax);
		_node->setCSMA(true, 6, 6, 2); // キャリアセンスの有効化
		result = _node->setDatarate(3);
	}
	if (not(result == RADIOLIB_ERR_NONE)) {
		HAL.println("\n        Datarate initialize error.");
		HAL.printf("        result: %d\n", result);
		_last_error = "Err: " + String(result);
		return false;
	}

	// 保存済み nonces を復元（DevNonce のインクリメントを維持するため）
	if (store.isKey("nonces")) {
		uint8_t buf[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
		store.getBytes("nonces", buf, sizeof(buf));
		_node->setBufferNonces(buf);
	}

	// JoinRequest はアップリンクなので届きやすいが、JoinAccept はダウンリンクなので
	// 弱電界では受信失敗しやすい。失敗時はリトライを行う。
	constexpr int MAX_JOIN_RETRIES    = 3;
	constexpr uint32_t RETRY_DELAY_MS = 5000; // デューティサイクル違反を避けるため間隔を空ける

	bool join_ok = false;
	for (int attempt = 0; attempt < MAX_JOIN_RETRIES; attempt++) {
		if (attempt > 0) {
			HAL.printf("        Retry %d/%d...\n", attempt, MAX_JOIN_RETRIES - 1);
			HAL.delay(RETRY_DELAY_MS);
		}
		{
			LoRaMutexGuard _;
			// セッション復元は初回のみ。2回目以降は新規 JoinRequest として送信する
			if (attempt == 0 and HAL.isWakeFromSleep()) {
				// RTC に保存済みのセッションを復元（ディープスリープ復帰時の再 Join を省略するため）
				_node->setBufferSession(rtcLoRaWanSession);
			}
			result = _node->activateOTAA();
			// Join Request 送信時点で DevNonce がインクリメントされるため、
			// Join Accept 受信失敗時でも次回以降の Join で重複しないよう保存する
			store.putBytes("nonces", _node->getBufferNonces(), RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
		}
		if (result == RADIOLIB_LORAWAN_NEW_SESSION or result == RADIOLIB_LORAWAN_SESSION_RESTORED) {
			join_ok = true;
			break;
		}
	}

	if (not join_ok) {
		HAL.println("\n        Activate OTAA error.");
		HAL.printf("        result: %d\n", result);
		_last_error = "Err: " + String(result);
		return false;
	}

	return true;
}

bool HAL_LoRaWAN_API::isJoined() {
	if (_node == nullptr) {
		return false;
	}
	LoRaMutexGuard _;
	return _node->isActivated();
}

bool HAL_LoRaWAN_API::send(const uint8_t *data, size_t size, uint8_t fport) {
	if (not isJoined()) {
		return false;
	}

	int result;

	// エラッタ解消のために１時間ごとにAGCをリセットする
	// https://github.com/jgromes/RadioLib/wiki/Radio-Errata-Notes
	constexpr uint32_t AGC_RESET_INTERVAL_MS = 3600000UL;
	static uint32_t last_agc_reset_ms        = 0;
	if ((millis() - last_agc_reset_ms) > AGC_RESET_INTERVAL_MS) {
		{
			LoRaMutexGuard _;
			_radio->resetAGC();
		}
		last_agc_reset_ms = millis();
	}

	{
		LoRaMutexGuard _;
		result = _node->sendReceive(data, size, fport);
	}

	if (result < RADIOLIB_ERR_NONE) {
		_last_error = "Err: " + String(result);
	}
	return result >= RADIOLIB_ERR_NONE;
}

bool HAL_LoRaWAN_API::isSupported() {
	return true;
}

String HAL_LoRaWAN_API::getDevEui() {
	uint8_t buf[8];
	for (int i = 0; i < 8; i++) {
		buf[i] = (_lorawan_dev_eui >> (56 - i * 8)) & 0xFF;
	}
	return toHexString(buf, 8);
}

String HAL_LoRaWAN_API::getJoinEui() {
	uint8_t buf[8];
	for (int i = 0; i < 8; i++) {
		buf[i] = (_lorawan_join_eui >> (56 - i * 8)) & 0xFF;
	}
	return toHexString(buf, 8);
}

String HAL_LoRaWAN_API::getLastError() {
	return _last_error;
}

bool HAL_LoRaWAN_API::waitTxDone(uint32_t timeout_ms) {
	if (not _initialized) {
		return false;
	}

	return true;
}

void HAL_LoRaWAN_API::saveSession() {
	if (_node == nullptr) {
		return;
	}
	LoRaMutexGuard _;
	memcpy(rtcLoRaWanSession, _node->getBufferSession(), RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
}

void HAL_LoRaWAN_API::sleep() {
	if (_radio == nullptr) {
		return;
	}
	LoRaMutexGuard _;
	_radio->sleep();
}

void HAL_LoRaWAN_API::wakeup() {
	if (_radio == nullptr) {
		return;
	}
	LoRaMutexGuard _;
	_radio->standby();
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

	JsonObject PARAM         = doc["PARAM"].to<JsonObject>();
	PARAM["TX_INTERVAL_SEC"] = "60";
	PARAM["DEVICE_SLEEP"]    = "false";

	JsonObject LORA_WAN = doc["LORA_WAN"].to<JsonObject>();
	LORA_WAN["ENABLE"]  = "false";
	LORA_WAN["ABP"]     = "false"; // "true" で ABP モード、"false" で OTAA モード
	// OTAA 用パラメータ
	LORA_WAN["OTAA_DEV_EUI"]  = getFactoryDevEUI();
	LORA_WAN["OTAA_JOIN_EUI"] = "0000000000000000";
	LORA_WAN["OTAA_APP_KEY"]  = getFactoryAppKey();
	// ABP 用パラメータ
	LORA_WAN["ABP_DEV_ADDR"]  = "00000000";
	LORA_WAN["ABP_NWK_S_KEY"] = "00000000000000000000000000000000";
	LORA_WAN["ABP_APP_S_KEY"] = "00000000000000000000000000000000";
	// その他共通パラメータ
	LORA_WAN["FPORT_NAME"]  = 9;
	LORA_WAN["FPORT_VALUE"] = 10;
	//  リージョンは AS923 固定
	//  クラスは A 固定
	//  NwkKey は無効（LoRaWAN 1.0 を想定）
	//  データレートは ADR でもよいが、常に最小(DR3 Dwellあり)を想定

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
		// Wire.end();
		if (SoftwareWire.begin(_sda, _scl, 100000) == true) {
			// ONLY ESP32 i2c begin return bool result
			SoftwareWire.setClockStretchTimeout(1000);
			_wires[port] = &SoftwareWire;
		}
		return true;
	default:
		return false;
	}

	return false;
}

uint8_t HAL_I2C_API::getSdaPin(uint8_t port) {
	switch (port) {
	case PORT_I2C::PORT_1:
		return EX_SDA;
	default:
		return 0;
	}
}

uint8_t HAL_I2C_API::getSclPin(uint8_t port) {
	switch (port) {
	case PORT_I2C::PORT_1:
		return EX_SCL;
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
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawPixel(x, y, (uint16_t)color);
}

void HAL_Display_API::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawChar(x, y, c, (uint16_t)color, (uint16_t)bg, size);
}

void HAL_Display_API::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawLine(x0, y0, x1, y1, (uint16_t)color);
}

void HAL_Display_API::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawFastVLine(x, y, h, (uint16_t)color);
}

void HAL_Display_API::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawFastHLine(x, y, w, (uint16_t)color);
}

void HAL_Display_API::fillScreen(uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.fillScreen((uint16_t)color);
}

int16_t HAL_Display_API::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	// Ignore font
	return GFX.drawChar(uniCode, x, y, font);
}

int16_t HAL_Display_API::drawChar(uint16_t uniCode, int32_t x, int32_t y) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return GFX.drawChar(uniCode, x, y);
}

int16_t HAL_Display_API::height(void) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return GFX.height();
}

int16_t HAL_Display_API::width(void) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return GFX.width();
}

int16_t HAL_Display_API::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	// Ignore font
	return GFX.drawString(string, poX, poY);
}

int16_t HAL_Display_API::drawString(const char *string, int32_t poX, int32_t poY) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return GFX.drawString(string, poX, poY);
}

int16_t HAL_Display_API::color565(uint8_t red, uint8_t green, uint8_t blue) {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

void HAL_Display_API::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawRect(x, y, w, h, (uint16_t)color);
}

void HAL_Display_API::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.fillRect(x, y, w, h, (uint16_t)color);
}

void HAL_Display_API::drawRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawRoundRect(x0, y0, w, h, radius, (uint16_t)color);
}

void HAL_Display_API::fillRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.fillRoundRect(x0, y0, w, h, radius, (uint16_t)color);
}

void HAL_Display_API::setRotation(uint8_t r) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.setRotation(r);
}

void HAL_Display_API::startWrite() {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.startWrite();
}

void HAL_Display_API::endWrite() {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.endWrite();
}

void HAL_Display_API::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawCircle(x0, y0, r, (uint16_t)color);
}

void HAL_Display_API::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.fillCircle(x0, y0, r, (uint16_t)color);
}

void HAL_Display_API::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawEllipse(x0, y0, rx, ry, color);
}

void HAL_Display_API::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.fillEllipse(x0, y0, rx, ry, color);
}

void HAL_Display_API::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.drawTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

void HAL_Display_API::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.fillTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

void HAL_Display_API::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.pushImage(x, y, w, h, data);
}

void HAL_Display_API::setCursor(int32_t x, int32_t y) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.setCursor(x, y);
}

int32_t HAL_Display_API::getCursorX() {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return GFX.getCursorX();
}

int32_t HAL_Display_API::getCursorY() {
	if (HAL.isWakeFromSleep()) {
		return 0;
	}
	return GFX.getCursorY();
}

void HAL_Display_API::setTextColor(uint16_t color) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.setTextColor(color);
}

void HAL_Display_API::setTextColor(uint16_t fgcolor, uint16_t bgcolor) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.setTextColor(fgcolor, bgcolor);
}

void HAL_Display_API::print(const char *string) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.print(string);
}

void HAL_Display_API::println(const char *string) {
	if (HAL.isWakeFromSleep()) {
		return;
	}
	GFX.println(string);
}

void HAL_Display_API::printf(const char *fmt, ...) {
	if (HAL.isWakeFromSleep()) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	GFX.vprintf(fmt, args);
	va_end(args);
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

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: LoRaWAN factory defaults
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

namespace {
	String bytesToHex(const uint8_t *data, uint8_t len) {
		String s;
		s.reserve(len * 2);
		for (uint8_t i = 0; i < len; i++) {
			if (data[i] < 0x10)
				s += '0';
			s += String(data[i], HEX);
		}
		s.toUpperCase();
		return s;
	}

	String getFactoryDevEUI() {
		uint64_t efuseMac = ESP.getEfuseMac();
		uint8_t mac[6];
		memcpy(mac, &efuseMac, 6);
		// MAC(6バイト) → EUI-64(8バイト): OUI後に 0xFF 0xFE を挿入
		uint8_t eui64[8] = {mac[0], mac[1], mac[2], 0xFF, 0xFE, mac[3], mac[4], mac[5]};
		return bytesToHex(eui64, 8);
	}

	String getFactoryAppKey() {
		// eFuse MAC を SHA-256 でハッシュし、先頭16バイトをデフォルトキーとする。
		// セキュリティが必要な場合はユーザーが別途設定すること。
		uint64_t efuseMac = ESP.getEfuseMac();
		uint8_t mac[6];
		memcpy(mac, &efuseMac, 6);
		uint8_t hash[32];
		mbedtls_sha256(mac, sizeof(mac), hash, 0);
		return bytesToHex(hash, 16);
	}
}

#endif /* HAL_API_M5STACK_UNIT_C6L_H */