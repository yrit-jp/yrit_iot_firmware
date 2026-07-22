#include "hal_wio_tracker_1110.h"
#ifdef HAL_API_WIO_TRACKER_1110_H

HAL_API HAL;

// Button API
#define DEBOUNCE_MS (10)
Button BtnA = Button(PIN_BUTTON1, true, DEBOUNCE_MS);
// Button BtnB = Button(WIO_KEY_B, true, DEBOUNCE_MS);
// Button BtnC = Button(WIO_KEY_A, true, DEBOUNCE_MS);

// Variables (timer)
#define TIM_0 (0)
#define TIM_1 (1)
#define TIM_2 (2)

extern "C" {
void TIMER1_IRQHandler();
void TIMER3_IRQHandler();
void TIMER4_IRQHandler();
} // extern "C"

struct Counter {
	uint32_t limit;
	uint32_t current;
};
std::unordered_map<std::string, struct Counter> sub_timer0;
std::unordered_map<std::string, struct Counter> sub_timer1;
std::unordered_map<std::string, struct Counter> sub_timer2;
static volatile bool timer0_flag = false;
static volatile bool timer1_flag = false;
static volatile bool timer2_flag = false;

// // Variables (BLE)
BLEService bleService(SERVICE_UUID);
BLECharacteristic bleTxChar(CHARACTERISTIC_UUID_TX);
BLECharacteristic bleRxChar(CHARACTERISTIC_UUID_RX);
TaskHandle_t TransmitTaskHandle;
TaskHandle_t UserTaskHandle;

// bool IsBatteryConnected = false;
// bool IsFirstLoop        = true;

static String bytesToHex(const uint8_t *data, uint8_t len);
String getFactoryDevEUI();
String getFactoryAppKey();

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_API::begin() {
	Serial.begin(115200);
	ledOn(LED_BUILTIN);
	uint8_t count = 15;
	while ((not Serial) and (count != 0)) {
		delay(100);
		count--;
	}
	Serial.flush();

	// Display.begin();
	// Display.setRotation(3);
	// Display.fillScreen(TFT_BLACK);
	// digitalWrite(LCD_BACKLIGHT, HIGH);

	pinMode(PIN_POWER_SUPPLY_GROVE, OUTPUT);
	digitalWrite(PIN_POWER_SUPPLY_GROVE, LOW);

	pinMode(PIN_GNSS_LNA, OUTPUT);
	digitalWrite(PIN_GNSS_LNA, LOW);

	ledOff(LED_BUILTIN);

	// Disable flash deep sleep
	ExternalFS.flashWakeUp();

	HAL.Info.DEVICE_NAME = "Wio Tracker 1110";
}

void HAL_API::update() {
	BtnA.read();
}

uint8_t HAL_API::waitForButtonPressed(uint16_t wait_ms) {
	uint8_t ret                     = (uint8_t)PRESSED_BUTTON::NONE;
	bool all_free_flag              = false;
	unsigned long long ts           = millis();
	constexpr uint16_t led_blink_ms = 100;
	unsigned long long led_blink_ts = millis();

	ledOn(LED_BUILTIN);
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
				ledOff(LED_BUILTIN);
				return ret;
			}
		}

		if ((millis() - led_blink_ts) > led_blink_ms) {
			led_blink_ts = millis();
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		}

		delay(5);
	}
	ledOff(LED_BUILTIN);
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
	uint32_t id0 = NRF_FICR->DEVICEID[0];
	uint32_t id1 = NRF_FICR->DEVICEID[1];

	String prefix = "";
	if (include_prefix) {
		prefix = "Wio-";
	}

	char bt_name[40] = {0};
	snprintf(bt_name, 40, "%08lX%08lX", id0, id1);
	return prefix + String(bt_name);
}

void HAL_API::printf(const char *fmt, ...) {
	char s[256] = {0};
	va_list args;
	::va_start(args, fmt);
	::vsnprintf(s, 256, fmt, args);
	::va_end(args);
	Serial.print(s);
	Serial.flush();
}

void HAL_API::println(const char *s) {
	Serial.println(s);
	Serial.flush();
}

void HAL_API::print(const char *s) {
	Serial.print(s);
	Serial.flush();
}

uint32_t HAL_API::getFreeHeapSize() {
	struct mallinfo mi = mallinfo();
	return mi.fordblks; // 空きブロックの合計バイト数
}

uint32_t HAL_API::getMinimumFreeHeapSize() {
	return getFreeHeapSize(); // 現在値を返す
}

void HAL_API::reboot() {
	NVIC_SystemReset();
}

extern "C" void tusb_hal_nrf_power_event(uint32_t event);

volatile bool rtc0_fired = false;
extern "C" void RTC0_IRQHandler(void) {
	// RTC2 は LoRa モジュールが使用済み
	if (NRF_RTC0->EVENTS_COMPARE[0]) {
		NRF_RTC0->EVENTS_COMPARE[0] = 0;
		NRF_RTC0->TASKS_STOP        = 1;
		NRF_RTC0->TASKS_CLEAR       = 1;
		rtc0_fired                  = true;
	}
}

#define DEEP_SLEEP // コメントを外すとスリープ後にリセット（深いスリープ）
void HAL_API::system_sleep(uint32_t ms) {
#ifdef DEEP_SLEEP
	uint32_t task_time = millis();

	// SCL, SDA のプルアップにより、センサを通ってLEDに電流が流れてしまうため
	// Wire を終了し、プルアップを無効化する
	Wire.end();
	NRF_P0->PIN_CNF[4] = (NRF_P0->PIN_CNF[4] & ~GPIO_PIN_CNF_PULL_Msk) | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos);
	NRF_P0->PIN_CNF[5] = (NRF_P0->PIN_CNF[5] & ~GPIO_PIN_CNF_PULL_Msk) | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos);

	ledOff(LED_BUILTIN);
	digitalWrite(PIN_POWER_SUPPLY_GROVE, LOW);

	constexpr uint16_t MANUAL_OFFSET = 1500;

	// 初期化にかかる時間を見越してスリープする
	ms = (ms > task_time) ? (ms - task_time + MANUAL_OFFSET) : 1;
#else
	// USB 再列挙待ち時間を見越してスリープする
	ms = (ms > 2000) ? (ms - 2000) : 0;
#endif
	uint32_t seconds = ms / 1000;
	HAL.printf("Sleep start: %d sec\n", seconds);
	HAL.delay(100);

	// USB 切断通知
	tusb_hal_nrf_power_event(1); // USB_EVT_REMOVED

	// 1. FreeRTOS 停止
	vTaskSuspendAll();
	NRF_RTC1->TASKS_STOP = 1;

	// 2. 全割り込み無効化（復帰用に保存）
	uint32_t nvic_iser[2];
	nvic_iser[0] = NVIC->ISER[0];
	nvic_iser[1] = NVIC->ISER[1];
	for (int i = 0; i < 48; i++) {
		NVIC_DisableIRQ((IRQn_Type)i);
	}

	// 3. USB 停止
	NRF_USBD->ENABLE           = 0;
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	// 4. LFCLK 確認
	if (not(NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk)) {
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
		NRF_CLOCK->TASKS_LFCLKSTART    = 1;
		while (not NRF_CLOCK->EVENTS_LFCLKSTARTED) {
		}
	}

	// 5. RTC0 スリープ（割り込み + WFE 方式）
	extern volatile bool rtc0_fired;
	static constexpr uint32_t MAX_SECONDS_PER_CYCLE = 0xFFFFFF / 8;

	while (seconds > 0) {
		uint32_t chunk = min(seconds, MAX_SECONDS_PER_CYCLE);
		seconds -= chunk;

		rtc0_fired = false;

		NRF_RTC0->TASKS_STOP        = 1;
		NRF_RTC0->TASKS_CLEAR       = 1;
		NRF_RTC0->PRESCALER         = 4095; // 8Hz
		NRF_RTC0->CC[0]             = chunk * 8;
		NRF_RTC0->EVTENSET          = RTC_EVTENSET_COMPARE0_Msk;
		NRF_RTC0->INTENSET          = RTC_INTENSET_COMPARE0_Msk;
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		NVIC_ClearPendingIRQ(RTC0_IRQn);
		NVIC_EnableIRQ(RTC0_IRQn);

		NRF_RTC0->TASKS_START = 1;

		while (not rtc0_fired) {
			__WFE();
		}

		NVIC_DisableIRQ(RTC0_IRQn);
	}

	// 6. 復帰
#ifdef DEEP_SLEEP
	NVIC_SystemReset();
#endif

	// --- System ON: 状態を復元して動作継続 ---

	// 割り込み状態を復元
	NVIC->ISER[0] = nvic_iser[0];
	NVIC->ISER[1] = nvic_iser[1];

	// RTC1（FreeRTOS tick）を再開
	NVIC_ClearPendingIRQ(RTC1_IRQn);
	NRF_RTC1->TASKS_START = 1;

	// FreeRTOS スケジューラを再開
	xTaskResumeAll();

	// USB を再接続
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (!NRF_CLOCK->EVENTS_HFCLKSTARTED) {
	}
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

	tusb_hal_nrf_power_event(0); // USB_EVT_DETECTED
	tusb_hal_nrf_power_event(2); // USB_EVT_READY

	delay(2000); // USB 再列挙待ち
}

void HAL_API::endSetup() {
	// オンボード QSPI フラッシュメモリを Deep Sleep モードにする
	ExternalFS.flashDeepSleep();

	// stop BLE
	Bluefruit.Advertising.stop();
	Bluefruit.disconnect(0);
	sd_softdevice_disable();
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
	HAL.printf("Free Heap: %5d Bytes\n", getFreeHeapSize());
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_Timer_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_Timer_API::begin(uint32_t interval_ms) {
	_interval_ms = interval_ms;

	switch (_timer_num) {
	case TIM_0: {
		NRF_TIMER1->TASKS_STOP = 1;
		NRF_TIMER1->MODE       = TIMER_MODE_MODE_Timer;
		NRF_TIMER1->BITMODE    = TIMER_BITMODE_BITMODE_32Bit;
		NRF_TIMER1->PRESCALER  = 4;                  // 16MHz / 2^4 = 1MHz (1usカウント)
		NRF_TIMER1->CC[0]      = interval_ms * 1000; // ms → us
		NRF_TIMER1->SHORTS     = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
		NRF_TIMER1->INTENSET   = TIMER_INTENSET_COMPARE0_Msk;
		NVIC_SetPriority(TIMER1_IRQn, 3);
		NVIC_EnableIRQ(TIMER1_IRQn);
		break;
	}
	case TIM_1: {
		NRF_TIMER3->TASKS_STOP = 1;
		NRF_TIMER3->MODE       = TIMER_MODE_MODE_Timer;
		NRF_TIMER3->BITMODE    = TIMER_BITMODE_BITMODE_32Bit;
		NRF_TIMER3->PRESCALER  = 4;
		NRF_TIMER3->CC[0]      = interval_ms * 1000;
		NRF_TIMER3->SHORTS     = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
		NRF_TIMER3->INTENSET   = TIMER_INTENSET_COMPARE0_Msk;
		NVIC_SetPriority(TIMER3_IRQn, 3);
		NVIC_EnableIRQ(TIMER3_IRQn);
		break;
	}
	case TIM_2: {
		NRF_TIMER4->TASKS_STOP = 1;
		NRF_TIMER4->MODE       = TIMER_MODE_MODE_Timer;
		NRF_TIMER4->BITMODE    = TIMER_BITMODE_BITMODE_32Bit;
		NRF_TIMER4->PRESCALER  = 4;
		NRF_TIMER4->CC[0]      = interval_ms * 1000;
		NRF_TIMER4->SHORTS     = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
		NRF_TIMER4->INTENSET   = TIMER_INTENSET_COMPARE0_Msk;
		NVIC_SetPriority(TIMER4_IRQn, 3);
		NVIC_EnableIRQ(TIMER4_IRQn);
		break;
	}
	default:
		return;
	}

	_inited = true;
}

void HAL_Timer_API::addSubTimer(std::string key, uint32_t count_limit, uint32_t initial_count) {
	__disable_irq();
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
	__enable_irq();
}

uint32_t HAL_Timer_API::getSubTimerLimit(std::string key) {
	uint32_t limit = 0;

	__disable_irq();
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
	__enable_irq();

	return limit;
}

uint32_t HAL_Timer_API::getSubTimerLimitMillis(std::string key) {
	return getSubTimerLimit(key) * _interval_ms;
}

void HAL_Timer_API::removeSubTimer(std::string key) {
	__disable_irq();
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
	__enable_irq();
}

bool HAL_Timer_API::checkSubTimer(std::string key, bool reset) {
	bool result = false;

	__disable_irq();
	switch (_timer_num) {
	case TIM_0: {
		auto itr = sub_timer0.find(key);
		if (itr != sub_timer0.end()) {
			result = (sub_timer0[key].limit <= sub_timer0[key].current);
			if (result && reset) {
				sub_timer0[key].current = 0;
			}
		}
		break;
	}
	case TIM_1: {
		auto itr = sub_timer1.find(key);
		if (itr != sub_timer1.end()) {
			result = (sub_timer1[key].limit <= sub_timer1[key].current);
			if (result && reset) {
				sub_timer1[key].current = 0;
			}
		}
		break;
	}
	case TIM_2: {
		auto itr = sub_timer2.find(key);
		if (itr != sub_timer2.end()) {
			result = (sub_timer2[key].limit <= sub_timer2[key].current);
			if (result && reset) {
				sub_timer2[key].current = 0;
			}
		}
		break;
	}
	default:
		break;
	}
	__enable_irq();

	return result;
}

void HAL_Timer_API::start(bool enable_first_interrupt) {
	if (!_inited) {
		return;
	}

	switch (_timer_num) {
	case TIM_0:
		NRF_TIMER1->TASKS_CLEAR = 1;
		NRF_TIMER1->TASKS_START = 1;
		if (enable_first_interrupt) {
			timer0_flag = true;
		}
		break;
	case TIM_1:
		NRF_TIMER3->TASKS_CLEAR = 1;
		NRF_TIMER3->TASKS_START = 1;
		if (enable_first_interrupt) {
			timer1_flag = true;
		}
		break;
	case TIM_2:
		NRF_TIMER4->TASKS_CLEAR = 1;
		NRF_TIMER4->TASKS_START = 1;
		if (enable_first_interrupt) {
			timer2_flag = true;
		}
		break;
	default:
		break;
	}
}

bool HAL_Timer_API::interrupted() {
	if (!_inited) {
		return false;
	}

	switch (_timer_num) {
	case TIM_0:
		if (timer0_flag) {
			timer0_flag = false;
			return true;
		}
		return false;
	case TIM_1:
		if (timer1_flag) {
			timer1_flag = false;
			return true;
		}
		return false;
	case TIM_2:
		if (timer2_flag) {
			timer2_flag = false;
			return true;
		}
		return false;
	default:
		break;
	}
	return false;
}

extern "C" {
void TIMER1_IRQHandler(void) {
	if (NRF_TIMER1->EVENTS_COMPARE[0]) {
		NRF_TIMER1->EVENTS_COMPARE[0] = 0;
#if __CORTEX_M == 4
		volatile uint32_t dummy = NRF_TIMER1->EVENTS_COMPARE[0];
		(void)dummy; // read-back barrier（Cortex-M4 erratum対策）
#endif
		for (auto &pair : sub_timer0) {
			pair.second.current++;
		}
		timer0_flag = true;
	}
}

void TIMER3_IRQHandler(void) {
	if (NRF_TIMER3->EVENTS_COMPARE[0]) {
		NRF_TIMER3->EVENTS_COMPARE[0] = 0;
#if __CORTEX_M == 4
		volatile uint32_t dummy = NRF_TIMER3->EVENTS_COMPARE[0];
		(void)dummy;
#endif
		for (auto &pair : sub_timer1) {
			pair.second.current++;
		}
		timer1_flag = true;
	}
}

void TIMER4_IRQHandler(void) {
	if (NRF_TIMER4->EVENTS_COMPARE[0]) {
		NRF_TIMER4->EVENTS_COMPARE[0] = 0;
#if __CORTEX_M == 4
		volatile uint32_t dummy = NRF_TIMER4->EVENTS_COMPARE[0];
		(void)dummy;
#endif
		for (auto &pair : sub_timer2) {
			pair.second.current++;
		}
		timer2_flag = true;
	}
}
} // extern "C"

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_LoRaWAN_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// 状態管理（ファイルスコープ）
enum class LoRaWanState {
	Startup,
	Ready,
	Joining,
	Joined,
	Transmitting,
	Failed,
};

static LoRaWanState _lorawan_state  = LoRaWanState::Startup;
static uint8_t _lorawan_dev_eui[8]  = {0};
static uint8_t _lorawan_join_eui[8] = {0};
static uint8_t _lorawan_app_key[16] = {0};

// イベントハンドラ（ファイルスコープ）
class LoRaWanEventHandlers : public LbmxEventHandlers {
protected:
	void reset(const LbmxEvent &event) override {
		// Dwell Time は自動的に設定される
		// UPLINK_DWELL_TIME_AS_923 (region_as_923_defs.h: LBM_WM1110)
		if (LbmxEngine::setRegion(SMTC_MODEM_REGION_AS_923_GRP1) != SMTC_MODEM_RC_OK) {
			_lorawan_state = LoRaWanState::Failed;
			return;
		}
		if (LbmxEngine::setOTAA(_lorawan_dev_eui, _lorawan_join_eui, _lorawan_app_key) != SMTC_MODEM_RC_OK) {
			_lorawan_state = LoRaWanState::Failed;
			return;
		}
		_lorawan_state = LoRaWanState::Ready;
	}

	void joined(const LbmxEvent &event) override {
		_lorawan_state = LoRaWanState::Joined;
	}

	void joinFail(const LbmxEvent &event) override {
		_lorawan_state = LoRaWanState::Failed;
	}

	void txDone(const LbmxEvent &event) override {
		static uint8_t tx_count = 0;
		tx_count++;

		uint8_t nb;
		smtc_modem_get_nb_trans(0, &nb);
		Serial.printf("txDone[%lu]: status=%d nb_trans=%d\n",
		              tx_count, event.event_data.txdone.status, nb);

		_lorawan_state = LoRaWanState::Joined;
	}

	void downData(const LbmxEvent &event) override {
		// Serial.printf("downData: fport=%d len=%d window=%d\n",
		//               event.event_data.downdata.fport,
		//               event.event_data.downdata.length,
		//               event.event_data.downdata.window);
	}
};

static LoRaWanEventHandlers _lorawan_event_handlers;

static void LoRaWanModemEventHandler() {
	static LbmxEvent event;
	while (event.fetch()) {
		_lorawan_event_handlers.invoke(event);
	}
}

static bool _initLbmEngine(LbmWm1110 **lbmWm1110) {
	*lbmWm1110 = &LbmWm1110::getInstance();
	(*lbmWm1110)->begin();
	LbmxEngine::begin((*lbmWm1110)->getRadio(), LoRaWanModemEventHandler);
	HAL.println("");
	LbmxEngine::printVersions((*lbmWm1110)->getRadio());
	return true;
}

bool HAL_LoRaWAN_API::beginOTAA(String dev_eui, String join_eui, String app_key) {
	if (_initialized) {
		return true;
	}

	if (!parseHexString(dev_eui, _lorawan_dev_eui, 8)) {
		return false;
	}
	if (!parseHexString(join_eui, _lorawan_join_eui, 8)) {
		return false;
	}
	if (!parseHexString(app_key, _lorawan_app_key, 16)) {
		return false;
	}

	_initLbmEngine(&_lbmWm1110);
	_initialized = true;
	return true;
}

bool HAL_LoRaWAN_API::beginABP(String dev_addr, String nwk_s_key, String app_s_key) {
	// Wio Tracker 1110 (LBM_WM1110) は ABP 非対応
	_last_error = "ABP not supported on Wio Tracker 1110";
	return false;
}

bool HAL_LoRaWAN_API::join(unsigned long timeout_ms) {
	if (not _initialized) {
		return false;
	}

	LbmxEngine::doWork();

	// 既にJoin済みの場合
	if (_lorawan_state == LoRaWanState::Joined) {
		return true;
	}

	// LBT が既に有効か確認し、無効なら有効化
	bool lbt_enabled;
	smtc_modem_lbt_get_state(0, &lbt_enabled);
	HAL.println("");
	HAL.printf("        LTB state: %s\n", lbt_enabled ? "On" : "Off");

	smtc_modem_lbt_set_parameters(
	    0,     // stack_id
	    10,    // 10ms リッスン（+ 2ms 安定化 = 合計 12ms）
	    -80,   // -80 dBm（ARIB STD-T108 準拠）
	    200000 // 探索周波数範囲 200 kHz（チャネル中心 ±100 kHz）
	);
	smtc_modem_lbt_set_state(0, true);

	smtc_modem_lbt_get_state(0, &lbt_enabled);
	HAL.printf("        LTB state: %s\n", lbt_enabled ? "On" : "Off");

	// JoinRequest はアップリンクなので届きやすいが、JoinAccept はダウンリンクなので
	// 弱電界では受信失敗しやすい。失敗時はリトライを行う。
	constexpr int MAX_JOIN_RETRIES    = 3;
	constexpr uint32_t RETRY_DELAY_MS = 5000; // デューティサイクル違反を避けるため間隔を空ける

	bool join_ok = false;
	for (int attempt = 0; attempt < MAX_JOIN_RETRIES; attempt++) {
		if (attempt > 0) {
			HAL.printf("        Retry %d/%d...\n", attempt, MAX_JOIN_RETRIES - 1);
			delay(RETRY_DELAY_MS);
		}

		_lorawan_state = LoRaWanState::Joining;
		// joinNetwork() の失敗はモデムのハードウェアエラーのためリトライしない
		if (LbmxEngine::joinNetwork() != SMTC_MODEM_RC_OK) {
			_lorawan_state = LoRaWanState::Failed;
			_last_error    = "Join network error";
			return false;
		}

		// JoinAccept 受信を待機（イベントハンドラが _lorawan_state を Joined に更新する）
		uint32_t start = millis();
		while (_lorawan_state == LoRaWanState::Joining) {
			if (millis() - start > timeout_ms) {
				_last_error = "Join timeout";
				break;
			}
			LbmxEngine::doWork();
			delay(100);
		}

		if (_lorawan_state == LoRaWanState::Joined) {
			join_ok = true;
			break;
		}
	}

	if (not join_ok) {
		return false;
	}

	// DM (Device Management) 自動送信を無効化
	// fPort=199 の定期送信を止め、MACコマンド応答との競合を減らす
	smtc_modem_dm_set_info_interval(SMTC_MODEM_DM_INFO_INTERVAL_IN_HOUR, 0);

	// ADR無効 + DR3固定（無くてもいいかも）
	// uint8_t custom_adr[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
	// for (int i = 0; i < SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH; i++) {
	// 	custom_adr[i] = 3; // 全スロットDR3
	// }
	// smtc_modem_adr_set_profile(0, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_adr);

	return true;
}

bool HAL_LoRaWAN_API::isJoined() {
	return _lorawan_state == LoRaWanState::Joined or _lorawan_state == LoRaWanState::Transmitting;
}

bool HAL_LoRaWAN_API::send(const uint8_t *data, size_t size, uint8_t fport) {
	if (not isJoined()) {
		return false;
	}

	uint8_t tx_max = 0;
	smtc_modem_get_next_tx_max_payload(0, &tx_max);
	HAL.printf("send: fport=%d size=%d tx_max=%d\n", fport, size, tx_max);

	// MACコマンド消化 + デューティサイクル待機
	for (int i = 0; i < 3; i++) {
		// デューティサイクル待機
		for (int j = 0; j < 30; j++) {
			int32_t duty_cycle;
			smtc_modem_get_duty_cycle_status(&duty_cycle);
			if (duty_cycle >= 0)
				break;
			// Serial.printf("send: duty cycle wait %d ms\n", -duty_cycle);
			LbmxEngine::doWork();
			HAL.delay(500);
		}

		// ペイロードサイズ確認
		uint8_t tx_max_payload = 0;
		smtc_modem_get_next_tx_max_payload(0, &tx_max_payload);
		if (size <= tx_max_payload) {
			break;
		}

		// MACコマンド消化
		HAL.printf("MAC flush: need=%d max=%d\n", size, tx_max_payload);
		_lorawan_state = LoRaWanState::Transmitting;
		smtc_modem_request_empty_uplink(0, true, fport, false);

		uint32_t start = millis();
		while (_lorawan_state == LoRaWanState::Transmitting) {
			if (millis() - start > 15000)
				break;
			LbmxEngine::doWork();
			HAL.delay(100);
		}
		_lorawan_state = LoRaWanState::Joined;
		LbmxEngine::doWork();
		HAL.delay(500);
		LbmxEngine::doWork();
	}

	// ADRで初期化されてしまうので毎回セットする
	// smtc_modem_set_nb_trans(0, _nb_trans); // 送信回数をセット

	_lorawan_state = LoRaWanState::Transmitting;

	constexpr bool CONFIRMED_OPTION = false;
	smtc_modem_return_code_t rc     = LbmxEngine::requestUplink(fport, CONFIRMED_OPTION, data, size);
	LbmxEngine::doWork();

	if (rc != SMTC_MODEM_RC_OK) {
		_last_error = "Send error: " + String(rc);
	}
	return rc == SMTC_MODEM_RC_OK;
}

bool HAL_LoRaWAN_API::isSupported() {
	return true;
}

String HAL_LoRaWAN_API::getDevEui() {
	return toHexString(_lorawan_dev_eui, 8);
}

String HAL_LoRaWAN_API::getJoinEui() {
	return toHexString(_lorawan_join_eui, 8);
}

String HAL_LoRaWAN_API::getLastError() {
	return _last_error;
}

bool HAL_LoRaWAN_API::waitTxDone(uint32_t timeout_ms) {
	if (not _initialized) {
		return false;
	}

	uint32_t start = millis();
	while (_lorawan_state == LoRaWanState::Transmitting) {
		if (millis() - start > timeout_ms) {
			return false;
		}
		LbmxEngine::doWork();
		HAL.delay(10);
	}

	if (_lorawan_state != LoRaWanState::Joined) {
		LbmxEngine::doWork();
		return false;
	}

	// 送信成功
	start = millis();

	constexpr uint32_t WAIT_TIME_MS = 3000; // もっと短くてもいいかも
	while ((millis() - start) < WAIT_TIME_MS) {
		HAL.LoRaWan.doWork();
		HAL.delay(100);
	}

	return true;
}

void HAL_LoRaWAN_API::doWork() {
	if (not _initialized) {
		return;
	}

	LbmxEngine::doWork();
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_LED_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void HAL_LED_API::on() {
	ledOn(LED_BUILTIN);
}

void HAL_LED_API::off() {
	ledOff(LED_BUILTIN);
}

void HAL_LED_API::toggle() {
	digitalWrite(LED_BUILTIN, not digitalRead(LED_BUILTIN));
}

// /*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
// //
// //  MARK: HAL_SETTING_FILE_API
// //
// /*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// TODO: 参照渡しに
bool HAL_SETTING_FILE_IO_API::saveSettings(String fpath, JsonDocument json) {
	if (ExternalFS.begin() == false) {
		HAL.println("Err LittleFS.");
		return false;
	}

	auto f = ExternalFS.open(fpath.c_str(), Adafruit_LittleFS_Namespace::FILE_O_WRITE);
	if (not f) {
		HAL.println("Open error.");
		ExternalFS.end();
		return false;
	}

	f.seek(0);    // 先頭に移動
	f.truncate(); // 現在位置（先頭）から先を削除 → ファイルが空になる

	serializeJson(json, f);
	// f.print(tmpl.c_str());

	f.close();
	ExternalFS.end();
	return true;
}

bool HAL_SETTING_FILE_IO_API::loadSettings(String fpath, JsonDocument &json) {
	if (ExternalFS.begin() == false) {
		HAL.println("Err LittleFS.");
		return false;
	}

	if (ExternalFS.exists(fpath.c_str()) == false) {
		HAL.println("File not exists.");
		ExternalFS.end();
		return false;
	}

	auto f = ExternalFS.open(fpath.c_str(), Adafruit_LittleFS_Namespace::FILE_O_READ);
	if (not f) {
		HAL.println("Open error.");
		ExternalFS.end();
		return false;
	}

	auto err = deserializeJson(json, f);
	f.close();
	ExternalFS.end();

	if (err) {
		HAL.println("JSON parse error.");
		HAL.println(err.c_str());
		return false;
	}
	return true;
}

bool HAL_SETTING_FILE_IO_API::formatStorage() {
	if (ExternalFS.begin() == false) {
		HAL.println("Err LittleFS.");
		return false;
	}
	bool ret = ExternalFS.format();
	ExternalFS.end();
	return ret;
}

JsonDocument HAL_SETTING_FILE_IO_API::deviceUniqueSettings() {
	JsonDocument doc;

	JsonObject PARAM         = doc["PARAM"].to<JsonObject>();
	PARAM["TX_INTERVAL_SEC"] = "60";
	PARAM["DEVICE_SLEEP"]    = "false";

	JsonObject SAMPLING   = doc["SAMPLING"].to<JsonObject>();
	SAMPLING["GPIO_MS"]   = "3000";
	SAMPLING["MODBUS_MS"] = "3000";

	JsonObject LORA_WAN = doc["LORA_WAN"].to<JsonObject>();
	LORA_WAN["ENABLE"]  = "false";
	// LORA_WAN["ABP"]     = "false"; // "true" で ABP モード、"false" で OTAA モード
	// OTAA 用パラメータ
	LORA_WAN["OTAA_DEV_EUI"]  = getFactoryDevEUI();
	LORA_WAN["OTAA_JOIN_EUI"] = "0000000000000000";
	LORA_WAN["OTAA_APP_KEY"]  = getFactoryAppKey();
	// ABP 用パラメータ
	// LORA_WAN["ABP_DEV_ADDR"]  = "00000000";
	// LORA_WAN["ABP_NWK_S_KEY"] = "00000000000000000000000000000000";
	// LORA_WAN["ABP_APP_S_KEY"] = "00000000000000000000000000000000";
	// その他共通パラメータ
	LORA_WAN["FPORT_NAME"]  = 9;
	LORA_WAN["FPORT_VALUE"] = 10;
	// LORA_WAN["NB_TRANS"]    = 1;
	//  リージョンは AS923 固定
	//  クラスは A 固定
	//  NwkKey は無効（LoRaWAN 1.0 を想定）
	//  アクティベーションは OTAA
	//  データレートは ADR でもよいが、常に最小(DR3 Dwellあり)を想定

	String modbus1        = HAL.Modbus.getModifier() + String(PORT_MODBUS::PORT_1);
	JsonObject MODBUS1    = doc[modbus1.c_str()].to<JsonObject>();
	MODBUS1["BAUDRATE"]   = "9600";
	MODBUS1["DATA_BIT"]   = "8";
	MODBUS1["PARITY"]     = "0";
	MODBUS1["TIMEOUT_MS"] = "200";
	MODBUS1["ID_MIN"]     = "1";
	MODBUS1["ID_MAX"]     = "5";

	String d13            = HAL.Gpio.getModifier() + "13";
	JsonObject D13        = doc[d13.c_str()].to<JsonObject>();
	D13["ENABLE"]         = "false";
	D13["NAME"]           = d13.c_str();
	D13["TX_ON_CHANGE"]   = "false";
	D13["TX_LINK_I2C"]    = "false";
	D13["TX_LINK_MODBUS"] = "false";
	D13["PULL_UP"]        = "false";

	String d14            = HAL.Gpio.getModifier() + "14";
	JsonObject D14        = doc[d14.c_str()].to<JsonObject>();
	D14["ENABLE"]         = "false";
	D14["NAME"]           = d14.c_str();
	D14["TX_ON_CHANGE"]   = "false";
	D14["TX_LINK_I2C"]    = "false";
	D14["TX_LINK_MODBUS"] = "false";
	D14["PULL_UP"]        = "false";

	String d15            = HAL.Gpio.getModifier() + "15";
	JsonObject D15        = doc[d15.c_str()].to<JsonObject>();
	D15["ENABLE"]         = "false";
	D15["NAME"]           = d15.c_str();
	D15["TX_ON_CHANGE"]   = "false";
	D15["TX_LINK_I2C"]    = "false";
	D15["TX_LINK_MODBUS"] = "false";
	D15["PULL_UP"]        = "false";

	String d16            = HAL.Gpio.getModifier() + "16";
	JsonObject D16        = doc[d16.c_str()].to<JsonObject>();
	D16["ENABLE"]         = "false";
	D16["NAME"]           = d16.c_str();
	D16["TX_ON_CHANGE"]   = "false";
	D16["TX_LINK_I2C"]    = "false";
	D16["TX_LINK_MODBUS"] = "false";
	D16["PULL_UP"]        = "false";

	String d28            = HAL.Gpio.getModifier() + "28";
	JsonObject D28        = doc[d28.c_str()].to<JsonObject>();
	D28["ENABLE"]         = "false";
	D28["NAME"]           = d28.c_str();
	D28["SHIFT_LEVEL"]    = "4";
	D28["TX_ON_CHANGE"]   = "false";
	D28["TX_LINK_I2C"]    = "false";
	D28["TX_LINK_MODBUS"] = "false";
	D28["DIGITAL"]        = "false";

	String d29            = HAL.Gpio.getModifier() + "29";
	JsonObject D29        = doc[d29.c_str()].to<JsonObject>();
	D29["ENABLE"]         = "false";
	D29["NAME"]           = d29.c_str();
	D29["SHIFT_LEVEL"]    = "4";
	D29["TX_ON_CHANGE"]   = "false";
	D29["TX_LINK_I2C"]    = "false";
	D29["TX_LINK_MODBUS"] = "false";
	D29["DIGITAL"]        = "false";

	String d30            = HAL.Gpio.getModifier() + "30";
	JsonObject D30        = doc[d30.c_str()].to<JsonObject>();
	D30["ENABLE"]         = "false";
	D30["NAME"]           = d30.c_str();
	D30["SHIFT_LEVEL"]    = "4";
	D30["TX_ON_CHANGE"]   = "false";
	D30["TX_LINK_I2C"]    = "false";
	D30["TX_LINK_MODBUS"] = "false";
	D30["DIGITAL"]        = "false";

	String d31            = HAL.Gpio.getModifier() + "31";
	JsonObject D31        = doc[d31.c_str()].to<JsonObject>();
	D31["ENABLE"]         = "false";
	D31["NAME"]           = d31.c_str();
	D31["SHIFT_LEVEL"]    = "4";
	D31["TX_ON_CHANGE"]   = "false";
	D31["TX_LINK_I2C"]    = "false";
	D31["TX_LINK_MODBUS"] = "false";
	D31["DIGITAL"]        = "false";

	return doc;
}

// /*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
// //
// //  MARK: HAL_BLE_API
// //
// /*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
IBluetoothLowEnergyControl::LocalBuffer IBluetoothLowEnergyControl::rx_buffer = {0};
IBluetoothLowEnergyControl::LocalBuffer IBluetoothLowEnergyControl::tx_buffer = {0};

// ESP32版の MyCharacteristicCallbacks::onWrite に相当
static void rx_write_callback(uint16_t conn_hdl,
                              BLECharacteristic *chr,
                              uint8_t *data, uint16_t len) {
	(void)conn_hdl;
	(void)chr;

	for (uint16_t i = 0; i < len; i++) {
		HAL_BLE_API::rx_buffer.data[HAL_BLE_API::rx_buffer.store_index] = data[i];
		HAL_BLE_API::rx_buffer.length += 1;
		HAL_BLE_API::rx_buffer.store_index =
		    ((HAL_BLE_API::rx_buffer.store_index + 1) == BLE_BUF_LENGTH)
		        ? 0
		        : (HAL_BLE_API::rx_buffer.store_index + 1);
	}
}

// ESP32版の MyCharacteristicCallbacks::onRead に相当
/**
 * @brief TX キャラクタリスティックの Read Authorize コールバック
 *
 * BLE の Read 操作では、1回の readValue() に対してデータが MTU を超える場合、
 * Read Blob（offset 付き連続 Read）が内部的に実行されます。
 * そのため、同一データに対してこのコールバックが offset を変えながら
 * 複数回呼ばれる場合があります。
 *
 * Bluefruit ライブラリは Read Authorize に対して自動で応答しないため、
 * コールバック内で sd_ble_gatts_rw_authorize_reply() を呼び出して
 * 明示的にレスポンスを返す必要があります。
 *
 * @param conn_hdl  接続ハンドル
 * @param chr       対象のキャラクタリスティック（未使用）
 * @param request   Read リクエスト情報（offset 等を含む）
 */
static void tx_read_callback(uint16_t conn_hdl,
                             BLECharacteristic *chr,
                             ble_gatts_evt_read_t *request) {
	(void)chr;

	// 現在の接続の MTU から、1回で返送可能な最大バイト数を算出
	// MTU には ATT ヘッダ 1 バイトが含まれるため、ペイロードは MTU - 1
	BLEConnection *conn = Bluefruit.Connection(conn_hdl);
	uint16_t mtu        = conn ? conn->getMtu() : 23;
	uint16_t max_chunk  = mtu - 1;

	uint16_t data_len = HAL_BLE_API::tx_buffer.length;
	uint16_t offset   = request->offset;

	// Serial.printf("tx_read_callback: len=%d, offset=%d, max_chunk=%d\n", data_len, offset, max_chunk);

	// 応答パラメータの共通部分を初期化
	ble_gatts_rw_authorize_reply_params_t reply = {0};
	reply.type                                  = BLE_GATTS_AUTHORIZE_TYPE_READ;
	reply.params.read.gatt_status               = BLE_GATT_STATUS_SUCCESS;
	reply.params.read.update                    = 1;
	reply.params.read.offset                    = offset;

	if (offset < data_len) {
		// 送信すべきデータが残っている場合
		uint16_t remaining = data_len - offset;
		uint16_t send_len  = (remaining > max_chunk) ? max_chunk : remaining;

		reply.params.read.len    = send_len;
		reply.params.read.p_data = HAL_BLE_API::tx_buffer.data + offset;

		// Serial.printf("  -> sending %d bytes from offset %d\n", send_len, offset);

		sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);

		// 全データの返送が完了したらバッファをクリアし、
		// txReady() が true を返すようにする
		if (remaining <= max_chunk) {
			HAL_BLE_API::tx_buffer.length = 0;
			// Serial.println("  -> buffer cleared");
		}
	} else {
		// 送信データなし（クリア済み or offset 超過）
		reply.params.read.len    = 0;
		reply.params.read.p_data = NULL;
		sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);
		// Serial.println("  -> empty response");
	}
}

void HAL_BLE_API::begin(String local_name) {
	if (local_name.length() > BLE_MAX_LOCAL_NAME_LEN) {
		local_name = local_name.substring(0, BLE_MAX_LOCAL_NAME_LEN);
	}

	// MTU (Maximum Transmission Unit) を最大に設定
	Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

	// Bluefruit初期化（Peripheral=1, Central=0）
	Bluefruit.begin(1, 0);
	Bluefruit.setTxPower(4);
	Bluefruit.setName(local_name.c_str());

	// Service開始
	bleService.begin();

	// TX Characteristic（クライアントがReadする）
	bleTxChar.setProperties(CHR_PROPS_READ);
	bleTxChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
	bleTxChar.setMaxLen(BLE_BUF_LENGTH);
	bleTxChar.setReadAuthorizeCallback(tx_read_callback);
	bleTxChar.begin();

	// RX Characteristic（クライアントがWriteする）
	bleRxChar.setProperties(CHR_PROPS_WRITE);
	bleRxChar.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
	bleRxChar.setMaxLen(BLE_BUF_LENGTH);
	bleRxChar.setWriteCallback(rx_write_callback);
	bleRxChar.begin();

	// アドバタイズ設定
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
	Bluefruit.Advertising.addTxPower();
	Bluefruit.Advertising.addService(bleService);
	Bluefruit.ScanResponse.addName();

	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244); // 単位: 0.625ms
	Bluefruit.Advertising.setFastTimeout(30);   // 30秒間はfast interval
	Bluefruit.Advertising.start(0);             // 0 = 無制限
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_GPIO_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

std::vector<uint8_t> HAL_GPIO_API::getPins() {
	std::vector<uint8_t> pins = {13, 14, 15, 16, 28, 29, 30, 31};
	return pins;
}

String HAL_GPIO_API::getModifier() {
	return "IO";
}

uint8_t HAL_GPIO_API::getAdcBits() {
	// wiring_analog_nRF52.cpp 参照
	// フルスケールは 3.6V なので注意
	return 10;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: HAL_I2C_API
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool HAL_I2C_API::begin(uint8_t port) {
	// uint8_t _sda;
	// uint8_t _scl;

	switch (port) {
	case PORT_I2C::PORT_1:
		// Wire.end();
		// _sda = HAL_I2C_API::getSdaPin(port);
		// _scl = HAL_I2C_API::getSclPin(port);
		Wire.end();

		Wire.begin();
		_wires[port] = &Wire;

		// Grove への電源供給を開始
		digitalWrite(PIN_POWER_SUPPLY_GROVE, HIGH);
		return true;
	default:
		return false;
	}

	return false;
}

uint8_t HAL_I2C_API::getSdaPin(uint8_t port) {
	return PIN_WIRE_SDA;
}

uint8_t HAL_I2C_API::getSclPin(uint8_t port) {
	return PIN_WIRE_SCL;
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
			HAL.printf(" (Parity odd is not supported) ");
			return false;
			break;
		case PARITY_EVEN:
			config = SERIAL_8E1;
			break;
		default:
			config = SERIAL_8N1;
			break;
		}
	} else {
		HAL.printf(" (Support data bit 8 only) ");
		return false;
	}

	uint8_t _TXD, _RXD;
	switch (port) {
	case PORT_MODBUS::PORT_1:
		_TXD = HAL_MODBUS_API::getTxPin(port);
		_RXD = HAL_MODBUS_API::getRxPin(port);

		// Serial1.end();
		// Serial1.setPins(_RXD, _TXD);
		Serial1.begin(baudrate, config);

		_serials[port]    = &Serial1;
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
		return PIN_SERIAL1_TX;
	default:
		return 0;
	}
}

uint8_t HAL_MODBUS_API::getRxPin(uint8_t port) {
	switch (port) {
	case PORT_MODBUS::PORT_1:
		return PIN_SERIAL1_RX;
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
	// M5.Display.drawPixel(x, y, (uint16_t)color);
}

void HAL_Display_API::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size) {
	// M5.Display.drawChar(x, y, c, (uint16_t)color, (uint16_t)bg, size);
}

void HAL_Display_API::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
	// M5.Display.drawLine(x0, y0, x1, y1, (uint16_t)color);
}

void HAL_Display_API::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
	// M5.Display.drawFastVLine(x, y, h, (uint16_t)color);
}

void HAL_Display_API::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) {
	// M5.Display.drawFastHLine(x, y, w, (uint16_t)color);
}

void HAL_Display_API::fillScreen(uint32_t color) {
	// M5.Display.fillScreen((uint16_t)color);
}

int16_t HAL_Display_API::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font) {
	// Ignore font
	// return M5.Display.drawChar(uniCode, x, y, font);
	return 0;
}

int16_t HAL_Display_API::drawChar(uint16_t uniCode, int32_t x, int32_t y) {
	// return M5.Display.drawChar(uniCode, x, y);
	return 0;
}

int16_t HAL_Display_API::height(void) {
	// return M5.Display.height();
	return 0;
}

int16_t HAL_Display_API::width(void) {
	// return M5.Display.width();
	return 0;
}

int16_t HAL_Display_API::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font) {
	// Ignore font
	// return M5.Display.drawString(string, poX, poY);
	return 0;
}

int16_t HAL_Display_API::drawString(const char *string, int32_t poX, int32_t poY) {
	// return M5.Display.drawString(string, poX, poY);
	return 0;
}

int16_t HAL_Display_API::color565(uint8_t red, uint8_t green, uint8_t blue) {
	// return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
	return 0;
}

void HAL_Display_API::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	// M5.Display.drawRect(x, y, w, h, (uint16_t)color);
}

void HAL_Display_API::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	// M5.Display.fillRect(x, y, w, h, (uint16_t)color);
}

void HAL_Display_API::drawRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) {
	// M5.Display.drawRoundRect(x0, y0, w, h, radius, (uint16_t)color);
}

void HAL_Display_API::fillRoundRect(int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t radius, uint32_t color) {
	// M5.Display.fillRoundRect(x0, y0, w, h, radius, (uint16_t)color);
}

void HAL_Display_API::setRotation(uint8_t r) {
	// M5.Display.setRotation(r);
}

void HAL_Display_API::startWrite() {
	// M5.Display.startWrite();
}

void HAL_Display_API::endWrite() {
	// M5.Display.endWrite();
}

void HAL_Display_API::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
	// M5.Display.drawCircle(x0, y0, r, (uint16_t)color);
}

void HAL_Display_API::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
	// M5.Display.fillCircle(x0, y0, r, (uint16_t)color);
}

void HAL_Display_API::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
	// M5.Display.drawEllipse(x0, y0, rx, ry, color);
}

void HAL_Display_API::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
	// M5.Display.fillEllipse(x0, y0, rx, ry, color);
}

void HAL_Display_API::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	// M5.Display.drawTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

void HAL_Display_API::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	// M5.Display.fillTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: utility
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

static String bytesToHex(const uint8_t *data, uint8_t len) {
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
	// 工場出荷時パラメータ読み取り (nRF52840 内蔵フラッシュ)
	constexpr uintptr_t FLASH_ADDR = (0xE9000 / 4096 - 1) * 4096;
	constexpr uint8_t OFFSET       = 0x04;
	constexpr uint8_t SIZE         = 8;
	return bytesToHex((const uint8_t *)(FLASH_ADDR + OFFSET), SIZE);
}

String getFactoryAppKey() {
	// 工場出荷時パラメータ読み取り (nRF52840 内蔵フラッシュ)
	constexpr uintptr_t FLASH_ADDR = (0xE9000 / 4096 - 1) * 4096;
	constexpr uint8_t OFFSET       = 0x24;
	constexpr uint8_t SIZE         = 16;
	return bytesToHex((const uint8_t *)(FLASH_ADDR + OFFSET), SIZE);
}

#endif /* HAL_API_WIO_H */