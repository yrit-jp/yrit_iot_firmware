#include "display.h"
#include "../user/user_display_design.h"
#include "fonts/font_genshin_gothic.h" // font file

std::unique_ptr<OpenFontRender> render;
struct SystemMessageRenderParam smrp;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Helper Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// メッセージボックスを消去し、フォント設定とカーソル設定を行う
void initMsgBox(CursorPosition position) {
	// メッセージボックスを消去
	HAL.Display.fillRect(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET,
	                     smrp.DISPLAY_WIDTH, smrp.MSGBOX_HEIGHT + smrp.BOTTOM_OFFSET, TFT_BLACK);

	// フォント設定とカーソル設定
	render->setFontSize(smrp.FONT_SIZE);
	switch (position) {
	case CursorPosition::SINGLE_LINE:
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT);
		break;
	case CursorPosition::DUAL_LINE:
		render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
		break;
	}
}

// セマフォを解放してコールバック実行後、再取得
template <typename Func, typename... Args>
static inline void callWithSemaphore(Func func, Args... args) {
	HAL.Display.giveSemaphoreMutex();
	func(render, args...);
	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Main Display Update Function
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

void updateDisplay(enum DisplayTiming dt, ...) {
	enum DisplayTiming _dt = dt;
	va_list ap;
	va_start(ap, dt);
	VaListGuard _(ap);

	cb_BeforeSystemDisplayUpdate(render, dt);

	dt = updateDisplayOverrideTop(render, dt, ap);

	HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

	switch (dt) {
	case DisplayTiming::SYSTEM_INIT_BEGIN:
		render.reset(new OpenFontRender);
		render->setDebugLevel(OFR_ERROR);
		render->showFreeTypeVersion();
		render->showCredit();
		render->setCacheSize(RENDER_FACE_CACHE_SIZE, RENDER_SIZES_CACHE_SIZE, RENDER_BYTES_CACHE_SIZE);
		if (render->loadFont(genshin_gothic, sizeof(genshin_gothic))) {
			HAL.println("Render initialize error");
			HAL.delay(10 * 1000);
			HAL.reboot();
			while (1)
				;
		}
		render->setDrawer(HAL.Display); // Set drawer object
		HAL.delay(1);

		break;
	case DisplayTiming::SS_INIT_BEGIN: {
		uint8_t wait_sec = va_arg(ap, int);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("起動モードの選択 （%d秒後に通常モードで起動）\n", wait_sec);
		render->seekCursor(20, 0);
		render->setFontColor(TFT_YELLOW);
		render->printf("設定モード");
		render->seekCursor(140, 0);
		render->setFontColor(TFT_WHITE);
		render->printf("通常モード");

		break;
	}
	case DisplayTiming::SS_MODE_ENTRY:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("設定パラメータをクリア（初期化）しますか?\n");
		render->seekCursor(45, 0);
		render->printf("No");
		render->seekCursor(185, 0);
		render->setFontColor(TFT_YELLOW);
		render->printf("Yes");
		render->setFontColor(TFT_WHITE);

		break;
	case DisplayTiming::SS_CLEAR_ALL:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("ファイルシステムの初期化中 ...\n（デフォルト設定を適用）");

		break;
	case DisplayTiming::SS_FORMAT:
		initMsgBox();
		render->printf("ファイルシステムの初期化中 ...");

		break;
	case DisplayTiming::SS_READY: {
		HAL.Display.fillRect(0, 0, smrp.DISPLAY_WIDTH, smrp.DISPLAY_HEIGHT, TFT_BLACK);
		HAL.delay(5);
		initMsgBox();
		render->setCursor(smrp.LEFT_OFFSET, 0);
		render->printf("Bluetooth LE 接続名\n");
		render->printf("%s", HAL.getChipUUID().c_str());

		// render.reset();
		break;
	}
	case DisplayTiming::SS_LOOP:

		break;
	case DisplayTiming::SS_INIT_DONE:

		break;
	case DisplayTiming::DISPLAY_FW_VERSION: {
		const char *dev_id      = va_arg(ap, const char *);
		int main_version        = va_arg(ap, int);
		int miner_version       = va_arg(ap, int);
		const char *release_tag = va_arg(ap, const char *);
		int hold_sec            = va_arg(ap, int);

		initMsgBox(CursorPosition::DUAL_LINE);
		for (uint8_t c = hold_sec; c != 0; c--) {
			render->setCursor(smrp.LEFT_OFFSET, smrp.DISPLAY_HEIGHT - smrp.MSGBOX_HEIGHT - smrp.BOTTOM_OFFSET);
			render->printf("デバイスID: %s\nファームウェア バージョン: %d.%d ", dev_id, main_version, miner_version);

			render->setFontColor(TFT_YELLOW);
			render->printf("%.6s", release_tag);
			render->setFontColor(TFT_WHITE);

			render->printf("    [");
			render->setFontColor(TFT_BLACK);
			int32_t x = render->getCursorX();
			int32_t y = render->getCursorY();
			render->printf("%d", c + 1);
			render->setCursor(x, y);
			render->setFontColor(TFT_WHITE);
			render->printf("%d]", c);

			HAL.Display.giveSemaphoreMutex();
			HAL.delay(1000);
			HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
		}
		initMsgBox();

		break;
	}
	case DisplayTiming::GPIO_INIT_BEGIN:
		initMsgBox();
		render->printf("GPIOデバイス 初期化準備中 ...");

		break;
	case DisplayTiming::GPIO_INIT_DONE:

		break;

	case DisplayTiming::PULSE_COUNTER_INIT_BEGIN:
		initMsgBox();
		render->printf("パルスカウンタ 初期化準備中 ...");

		break;
	case DisplayTiming::PULSE_COUNTER_INIT_TARGET: {
		uint8_t pin = va_arg(ap, int);

		initMsgBox();
		render->printf("ピン %d の初期化中", pin);

		break;
	}
	case DisplayTiming::PULSE_COUNTER_INIT_END:

		break;
	case DisplayTiming::I2C_INIT_DEVICE_BEGIN:
		initMsgBox();
		render->printf("I2Cデバイス 初期化準備中 ...");

		break;
	case DisplayTiming::I2C_INIT_DEVICE_TARGET: {
		uint8_t address = va_arg(ap, int);
		int progress    = va_arg(ap, int);
		int i2c_dev_num = va_arg(ap, int);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("（%d/%d）\nI2Cデバイス（0x%02X）の初期化中", progress, i2c_dev_num, address);

		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_FAILED: {
		uint8_t address = va_arg(ap, int);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_RED);
		render->printf("I2Cデバイス（0x%02X）の初期化に失敗しました\n10秒後に再起動します...", address);

		break;
	}
	case DisplayTiming::I2C_INIT_DEVICE_DONE:

		break;
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_BEGIN:
		initMsgBox();
		render->printf("Modbus 初期化準備中 ...");

		break;
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_TARGET: {
		uint8_t port = va_arg(ap, int);

		initMsgBox();
		render->printf("ポート %d 初期化中 ...", port);
		break;
	}
	case DisplayTiming::MODBUS_INIT_PERIPHERAL_DONE:

		break;
	case DisplayTiming::MODBUS_INIT_DEVICE_BEGIN:
		initMsgBox();
		render->printf("Modbusデバイス 初期化準備中 ...");

		break;
	case DisplayTiming::MODBUS_INIT_DEVICE_TARGET: {
		uint8_t port       = va_arg(ap, int);
		uint8_t id         = va_arg(ap, int);
		int progress       = va_arg(ap, int);
		int modbus_dev_num = va_arg(ap, int);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("（%d/%d） ID:%d / ポート:%d\nModbus デバイスの初期化中", progress, modbus_dev_num, id, port);

		break;
	}
	case DisplayTiming::MODBUS_INIT_DEVICE_DONE:

		break;
	case DisplayTiming::CAMERA_INIT_BEGIN:
		initMsgBox();
		render->printf("カメラの初期化中 ...");
		break;
	case DisplayTiming::CAMERA_INIT_FAILED:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_RED);
		render->printf("カメラの初期化に失敗しました\n10秒後に再起動します...");

		break;
	case DisplayTiming::CAMERA_INIT_DONE:

		break;
	case DisplayTiming::DISPLAY_OFFLINE_MODE:
		initMsgBox();
		render->printf("OFFLINE モードで起動中 ...");

		break;
	case DisplayTiming::ETH_INIT_BEGIN:
		initMsgBox();
		render->printf("有線LAN 接続中 ...");

		break;
	case DisplayTiming::ETH_INIT_SUCCESS:
		initMsgBox();
		render->printf("有線LAN の接続完了");

		break;
	case DisplayTiming::ETH_INIT_FAILED:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_YELLOW);
		render->printf("有線LAN の接続に失敗しました\nWiFi 接続へ切り替えます");
		render->setFontColor(TFT_WHITE);

		break;
	case DisplayTiming::WIFI_TRY_CONNECT: {
		const char *target_ssid = va_arg(ap, const char *);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("WiFi 接続中 ...\n(%.16s)", target_ssid);

		break;
	}
	case DisplayTiming::WIFI_SEARCH_TARGET:
		initMsgBox();
		render->printf("SSID 検索中 ...");

		break;
	case DisplayTiming::WIFI_CONNECT_SUCCESS:
		initMsgBox();
		render->printf("WiFi の接続完了");

		break;
	case DisplayTiming::WIFI_CONNECT_FAILED:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_RED);
		render->printf("WiFi の接続に失敗しました\n10秒後に再起動します...");

		break;
	case DisplayTiming::WIFI_TARGET_DETECTED: {
		const char *target_ssid = va_arg(ap, const char *);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->printf("SSID を発見しました\n (%.16s)", target_ssid);

		break;
	}
	case DisplayTiming::WIFI_TARGET_NOT_FOUND: {
		const char *target_ssid = va_arg(ap, const char *);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_YELLOW);
		render->printf("SSID が見つかりませんでした...\n(%.16s)", target_ssid);
		render->setFontColor(TFT_WHITE);

		break;
	}
	case DisplayTiming::NTP_INIT_BEGIN:
		initMsgBox();
		render->printf("時刻同期 初期化準備中 ...");

		break;
	case DisplayTiming::NTP_CONNECT_WIFI:
		initMsgBox();
		render->printf("WiFi 接続中（NTP） ...");

		break;
	case DisplayTiming::NTP_SYNC_START:
		initMsgBox();
		render->printf("NTP サーバと時刻を同期中 ...");

		break;
	case DisplayTiming::NTP_SYNC_FAILED:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_RED);
		render->printf("時刻の同期に失敗しました\n10秒後に再起動します...");

		break;
	case DisplayTiming::NTP_INIT_DONE:

		break;
	case DisplayTiming::HEARTBEAT_FAILED:
		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_RED);
		render->printf("時刻の取得に失敗しました\n5秒後に再起動します...");

		break;

		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
		//  MARK: Ambient
		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

	case DisplayTiming::AMBIENT_INIT_BEGIN:
		initMsgBox();
		render->printf("Ambient 初期化中 ...");

		break;
	case DisplayTiming::AMBIENT_CONNECT_WIFI:
		initMsgBox();
		render->printf("WiFi 接続中（Ambient） ...");

		break;
	case DisplayTiming::AMBIENT_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		callWithSemaphore(cb_AmbientBufferUpdate, remain_data_quantity);

		break;
	}
	case DisplayTiming::AMBIENT_INIT_DONE:
		break;

		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
		//  MARK: FTP
		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

	case DisplayTiming::FTP_INIT_BEGIN:
		initMsgBox();
		render->printf("FTP 初期化中 ...");

		break;
	case DisplayTiming::FTP_CONNECT_WIFI:
		initMsgBox();
		render->printf("WiFi 接続中（FTP） ...");

		break;
	case DisplayTiming::FTP_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		callWithSemaphore(cb_FtpBufferUpdate, remain_data_quantity);

		break;
	}
	case DisplayTiming::FTP_INIT_DONE:
		break;

		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
		//  MARK: HTTP
		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

	case DisplayTiming::HTTP_INIT_BEGIN:
		initMsgBox();
		render->printf("HTTP 初期化中 ...");

		break;
	case DisplayTiming::HTTP_CONNECT_WIFI:
		initMsgBox();
		render->printf("WiFi 接続中（HTTP） ...");

		break;
	case DisplayTiming::HTTP_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		callWithSemaphore(cb_HttpBufferUpdate, remain_data_quantity);

		break;
	}
	case DisplayTiming::HTTP_INIT_DONE:
		break;

		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
		//  MARK: MQTT
		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

	case DisplayTiming::MQTT_INIT_BEGIN:
		initMsgBox();
		render->printf("MQTT 初期化中 ...");

		break;
	case DisplayTiming::MQTT_CONNECT_WIFI:
		initMsgBox();
		render->printf("WiFi 接続中（MQTT） ...");

		break;
	case DisplayTiming::MQTT_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		callWithSemaphore(cb_MqttBufferUpdate, remain_data_quantity);

		break;
	}
	case DisplayTiming::MQTT_INIT_DONE:
		break;

		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
		//  MARK: LoRaWAN
		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

	case DisplayTiming::LORA_WAN_INIT_BEGIN: {
		initMsgBox();
		render->printf("LoRaWAN 初期化中 ...");

		break;
	}
	case DisplayTiming::LORA_WAN_JOIN_BEGIN: {
		initMsgBox();
		render->printf("LoRaWAN 接続中 ...");

		break;
	}
	case DisplayTiming::LORA_WAN_JOIN_DONE: {
		break;
	}
	case DisplayTiming::LORA_WAN_BUFFER_UPDATE: {
		int remain_data_quantity = va_arg(ap, int);

		callWithSemaphore(cb_LoraBufferUpdate, remain_data_quantity);

		break;
	}
	case DisplayTiming::LORA_WAN_INIT_FAILED: {
		const char *err_msg = va_arg(ap, const char *);

		initMsgBox(CursorPosition::DUAL_LINE);
		render->setFontColor(TFT_RED);
		render->printf("LoRaWAN の初期化に失敗しました（再起動中）\n");
		render->printf("%s", err_msg);

		break;
	}
	case DisplayTiming::LORA_WAN_INIT_DONE: {
		break;
	}

		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
		//  MARK: Timer
		/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

	case DisplayTiming::TI_INIT_BEGIN:
		initMsgBox();
		render->printf("タイマ 初期化中 ...");

		break;
	case DisplayTiming::TI_INIT_DONE:

		break;
	case DisplayTiming::SYSTEM_INIT_DONE: {
		const char *dev_id = va_arg(ap, const char *);
		int sensor_num     = va_arg(ap, int);

		initMsgBox();
		render->setFontColor(TFT_GREENYELLOW);
		render->printf("システムを起動する準備が完了しました!");
		render->setFontColor(TFT_WHITE);

		HAL.Display.giveSemaphoreMutex();
		HAL.delay(3000);
		HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

		HAL.Display.fillScreen(TFT_BLACK);

		// Apply user display design
		callWithSemaphore(cb_SystemReady, dev_id, sensor_num);

		break;
	}
	case DisplayTiming::WAIT_START_TIMING: {
		int sec = va_arg(ap, int);

		callWithSemaphore(cb_WaitTiming, sec);

		break;
	}
	case DisplayTiming::SETUP_END: {
		break;
	}
	case DisplayTiming::SET_DISPLAY_SENSOR_NAME:
		break;
	case DisplayTiming::SET_DISPLAY_WIDTH:
		break;
	case DisplayTiming::UPDATE_DISPLAY_VALUE_GPIO: {
		const char *sensor_name = va_arg(ap, const char *);
		int32_t value           = va_arg(ap, int32_t);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateGpioValue, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_INT: {
		const char *sensor_name = va_arg(ap, const char *);
		int32_t value           = va_arg(ap, int32_t);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateI2cValueInt, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_DOUBLE: {
		const char *sensor_name = va_arg(ap, const char *);
		double value            = va_arg(ap, double);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateI2cValueDouble, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_STRING: {
		const char *sensor_name = va_arg(ap, const char *);
		const char *value       = va_arg(ap, const char *);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateI2cValueString, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_ERROR: {
		const char *sensor_name = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateI2cValueError, sensor_name, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_INT: {
		const char *sensor_name = va_arg(ap, const char *);
		int32_t value           = va_arg(ap, int32_t);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateModbusValueInt, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_DOUBLE: {
		const char *sensor_name = va_arg(ap, const char *);
		double value            = va_arg(ap, double);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateModbusValueDouble, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_STRING: {
		const char *sensor_name = va_arg(ap, const char *);
		const char *value       = va_arg(ap, const char *);
		const char *unit        = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateModbusValueString, sensor_name, value, unit, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_ERROR: {
		const char *sensor_name = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateModbusValueError, sensor_name, selected);

		break;
	}
	case DisplayTiming::UPDATE_DISPLAY_VALUE_CAMERA: {
		const char *sensor_name = va_arg(ap, const char *);
		bool selected           = va_arg(ap, int);

		callWithSemaphore(cb_UpdateCameraValue, sensor_name, selected);

		break;
	}
	case DisplayTiming::EVERY_ONE_SECONDS_EVENT: {
		break;
	}
	case DisplayTiming::EVERY_HALF_MINUTES_EVENT: {
		break;
	}
	case DisplayTiming::EVERY_ONE_MINUTES_EVENT: {
		break;
	}
	case DisplayTiming::READ_MQTT_STATUS: {
		int status = va_arg(ap, int); // wl_status_t

		callWithSemaphore(cb_ReadMqttStatus, status);

		break;
	}
	case DisplayTiming::SAMPLING_GPIO_SENSORS: {
		struct tm timeinfo = va_arg(ap, struct tm);

		callWithSemaphore(cb_SamplingGpioSensors, timeinfo);

		break;
	}
	case DisplayTiming::SAMPLING_I2C_SENSORS: {
		struct tm timeinfo = va_arg(ap, struct tm);

		callWithSemaphore(cb_SamplingI2cSensors, timeinfo);

		break;
	}
	case DisplayTiming::SAMPLING_MODBUS_SENSORS: {
		struct tm timeinfo = va_arg(ap, struct tm);

		callWithSemaphore(cb_SamplingModbusSensors, timeinfo);

		break;
	}
	case DisplayTiming::SAMPLING_CAMERA_SENSOR: {
		struct tm timeinfo = va_arg(ap, struct tm);

		callWithSemaphore(cb_SamplingCameraSensor, timeinfo);

		break;
	}
	case DisplayTiming::BTNA_PRESSED: {
		break;
	}
	case DisplayTiming::BTNB_PRESSED: {
		break;
	}
	case DisplayTiming::BTNC_PRESSED: {
		break;
	}
	default:

		break;
	}

	HAL.Display.giveSemaphoreMutex();

	updateDisplayOverrideBottom(render, _dt, dt, ap);

	cb_AfterSystemDisplayUpdate(render, dt);

	HAL.delay(5); // For wdt

	// VaListGuard で自動的に終了する
	// va_end(ap);
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Override Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

__attribute__((weak)) enum DisplayTiming updateDisplayOverrideTop(std::unique_ptr<OpenFontRender> &render,
                                                                  enum DisplayTiming dt, va_list ap) {
	return dt;
}

__attribute__((weak)) void updateDisplayOverrideBottom(std::unique_ptr<OpenFontRender> &render,
                                                       enum DisplayTiming first_dt,
                                                       enum DisplayTiming current_dt,
                                                       va_list ap) {
}