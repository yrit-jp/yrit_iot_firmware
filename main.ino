#include "src/initialize.h"

struct Features features;
struct TxEvent_t txEventFlag = {false, false, false};
std::vector<TransmitterEntry> transmitters;

// ==================================================================
// MARK: setup
//! @brief 初期化処理
//! @see updateValue, transmitLoop, Sensor, IoTTxFTP, IoTTxMQTT
//! @details 主な機能はセンサデータの取得、バッファへの格納、表示更新
// ==================================================================
void setup() {
	HAL.begin();
	HAL.printf("Firmware Version: %d.%d %s\n",
	           APP_MAIN_VERSION,
	           APP_MINOR_VERSION,
	           APP_RELEASE_TAG);
	HAL.println(ASCII_ART);

	// Initialize Mutex
	HAL.initializeSemaphoreMutex("i2c");
	HAL.initializeSemaphoreMutex("ex_sensor_data");
	HAL.Display.initializeSemaphoreMutex();
	HAL.Tf.initializeSemaphoreMutex();

	// Initialize Logging setup function
	InitializeLog("/DEBUG_LOG/SETUP");

	updateDisplay(DisplayTiming::SYSTEM_INIT_BEGIN);

	// Initialize Internal Parameters
	InitializeSettingParameter();

	// Read features
	features = ReadFeatures();

	if (features.sleep_enable) {
		// スリープが有効の場合は省電力モードに切り替えられる
		HAL.setPowerMode(features.needs_ip_network);
	}

	if (not HAL.isWakeFromSleep()) {
		// Display Farmware verion
		uint8_t hold_sec = 3;
		updateDisplay(DisplayTiming::DISPLAY_FW_VERSION,
		              param_manager.getParameter<const char *>("PARAM", "devID"),
		              APP_MAIN_VERSION,
		              APP_MINOR_VERSION,
		              APP_RELEASE_TAG,
		              hold_sec);
		HAL.Info.DEVICE_ID     = param_manager.getParameter<const char *>("PARAM", "devID");
		HAL.Info.V_MAIN        = APP_MAIN_VERSION;
		HAL.Info.V_MINOR       = APP_MINOR_VERSION;
		HAL.Info.V_RELEASE_TAG = APP_RELEASE_TAG;
	}

	// Initialize LED
	InitializeLed();

	// Initialize GPIO
	InitializeGPIO();

	// Initialize Pulse Counter
	InitializePulseCounter();

	// Initialize I2C
	InitializeI2c();

	// Initialize I2C Device
	InitializeI2cDevice();

	// Initialize Modbus
	InitializeModbus();

	// Initialize Modbus Device
	InitializeModbusDevice();

	// Initialize Camera Device
	InitializeCameraDevice();

	struct tm timeinfo = {0};
	if (features.needs_ip_network) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::UNKNOWN) {
			// Initialize Wired LAN
			InitializeWiredLan();
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::UNKNOWN) {
			// Initialize Wireless LAN
			InitializeWirelessLan();
		}

		// Sync Time
		InitializeNTP();

		if (not HAL.NtpRtc.getCurrentTime(&timeinfo)) {
			HAL.println("Get localtime error");
			updateDisplay(DisplayTiming::NTP_SYNC_FAILED);
			HAL.delay(10000);
			HAL.reboot();
		}
	} else if (features.offline_mode) {
		// Offline mode
		updateDisplay(DisplayTiming::DISPLAY_OFFLINE_MODE);
		HAL.delay(3000);
	}

	if (features.ambient_enable) {
		// Ambient initialize
		InitializeAmbient();
		TransmitterEntry entry;
		entry.tx                           = tx_ambient.get();
		entry.display_timing.init_begin    = DisplayTiming::AMBIENT_INIT_BEGIN;
		entry.display_timing.connect_wifi  = DisplayTiming::AMBIENT_CONNECT_WIFI;
		entry.display_timing.buffer_update = DisplayTiming::AMBIENT_BUFFER_UPDATE;
		entry.display_timing.init_done     = DisplayTiming::AMBIENT_INIT_DONE;
		transmitters.push_back(entry);
	}
	if (features.ftp_enable) {
		// FTP initialize
		InitializeFTP();
		TransmitterEntry entry;
		entry.tx                           = tx_ftp.get();
		entry.display_timing.init_begin    = DisplayTiming::FTP_INIT_BEGIN;
		entry.display_timing.connect_wifi  = DisplayTiming::FTP_CONNECT_WIFI;
		entry.display_timing.buffer_update = DisplayTiming::FTP_BUFFER_UPDATE;
		entry.display_timing.init_done     = DisplayTiming::FTP_INIT_DONE;
		transmitters.push_back(entry);
	}
	if (features.http_enable) {
		// HTTP initialize
		InitializeHTTP();
		TransmitterEntry entry;
		entry.tx                           = tx_http.get();
		entry.display_timing.init_begin    = DisplayTiming::HTTP_INIT_BEGIN;
		entry.display_timing.connect_wifi  = DisplayTiming::HTTP_CONNECT_WIFI;
		entry.display_timing.buffer_update = DisplayTiming::HTTP_BUFFER_UPDATE;
		entry.display_timing.init_done     = DisplayTiming::HTTP_INIT_DONE;
		transmitters.push_back(entry);
	}
	if (features.mqtt_enable) {
		// MQTT initialize
		InitializeMQTT();
		TransmitterEntry entry;
		entry.tx                           = tx_mqtt.get();
		entry.display_timing.init_begin    = DisplayTiming::MQTT_INIT_BEGIN;
		entry.display_timing.connect_wifi  = DisplayTiming::MQTT_CONNECT_WIFI;
		entry.display_timing.buffer_update = DisplayTiming::MQTT_BUFFER_UPDATE;
		entry.display_timing.init_done     = DisplayTiming::MQTT_INIT_DONE;
		transmitters.push_back(entry);
	}
	if (features.lora_wan_enable) {
		// LoRaWAN initialize
		InitializeLoRaWAN();
		TransmitterEntry entry;
		entry.tx                           = tx_lora_wan.get();
		entry.display_timing.init_begin    = DisplayTiming::LORA_WAN_INIT_BEGIN;
		entry.display_timing.connect_wifi  = DisplayTiming::LORA_WAN_JOIN_BEGIN;
		entry.display_timing.buffer_update = DisplayTiming::LORA_WAN_BUFFER_UPDATE;
		entry.display_timing.init_done     = DisplayTiming::LORA_WAN_INIT_DONE;
		transmitters.push_back(entry);
	}

	// 全トランスミッターのハートビートを更新
	for (auto &entry : transmitters) {
		entry.tx->updateHeartbeat(timeinfo);
	}

	// Initialize timer interrupt
	InitializeTimerInterrupt();

	HAL.Led_Status.on();
	HAL.Led_Warning.off();
	updateDisplay(DisplayTiming::SYSTEM_INIT_DONE,
	              param_manager.getParameter<const char *>("PARAM", "devID"),
	              (int)sensors.size());
	HAL.println("Ready!");

	for (int i = 0; i < sensors.size(); i++) {
		if (sensors[i].enable) {
			sensors[i].display = true;
			updateDisplay(DisplayTiming::SET_DISPLAY_SENSOR_NAME, sensors[i].name.c_str());
			break;
		}
	}
	// Clear Json Object
	HAL.println("Clear setting template");
	param_manager.clearSettingTemplate();

	// Reset logging
	HAL.Log.flush();
	HAL.Log.loggingStop();
	InitializeLog("/DEBUG_LOG/LOOP");

	if (not features.sleep_enable) {
		// Create Transmit Task
		HAL_REGISTER_TRANSMIT_TASK();
	}

	if (features.needs_ip_network) {
		if (features.wait_enable) {
			WaitStartTiming();
		}

		// Setup timer correction
		if (not HAL.NtpRtc.getCurrentTime(&timeinfo)) {
			// TODO: Error handling
		}
		TxTimingHelper::begin(timeinfo, HAL.Info.TX_INTERVAL_SEC);
	} else if (features.lora_wan_enable) {
		HAL.Timer1.addSubTimer("transmit", HAL.Info.TX_INTERVAL_SEC, HAL.Info.TX_INTERVAL_SEC);
	}

	constexpr bool ENABLE_FIRST_INTERRUPT  = true;
	constexpr bool DISABLE_FIRST_INTERRUPT = false;
	// HAL.Timer0.start(ENABLE_FIRST_INTERRUPT);
	HAL.Timer1.start(ENABLE_FIRST_INTERRUPT);
	HAL.Timer2.start(ENABLE_FIRST_INTERRUPT);

	HAL.endSetup();
	updateDisplay(DisplayTiming::SETUP_END);
	printSensorData(true);

	// Create User Interactive Task
	HAL_REGISTER_USER_INTERACTIVE_TASK();

	// Reset pulse count
	HAL.Gpio.stopPulseCounterAll();
	HAL.Gpio.resetPulseCountAll();
	HAL.Gpio.startPulseCounterAll();
}

// ==================================================================
// MARK: loop
//! @brief メインループ処理
//! @see updateValue, transmitLoop, Sensor, IoTTxFTP, IoTTxMQTT, IoTTxHTTP, IoTTxAmbient, Display
//! @details 主な機能はセンサデータの取得、バッファへの格納、表示更新。
// ==================================================================
void loop() {
	static struct tm timeinfo = {0};

	if (HAL.Timer2.interrupted()) {
		// Interrupt every 100 ms
		// HAL.println("Interrupted OK Timer2");

		if (features.needs_ip_network) {
			if (not HAL.NtpRtc.getCurrentTime(&timeinfo)) {
				HAL.println("Get localtime error");
				return;
			}

			if (TxTimingHelper::shouldTransmit(timeinfo) == true) {
				// 送信タイミング
				txEventFlag.transmit = true;
			}
		}
	}

	if (HAL.Timer1.interrupted()) {
		// Interrupt every 1 second
		// HAL.println("Interrupted OK Timer1");

		if (HAL.Timer1.checkSubTimer("one_minutes") == true) {
			// About every 1 minutes
			HAL.periodicLoopTaskDebugMessage();
			updateDisplay(DisplayTiming::EVERY_ONE_MINUTES_EVENT);

			// Heartbeat tx set
			for (auto &entry : transmitters) {
				entry.tx->updateHeartbeat(timeinfo);
			}
		}

		if (HAL.Timer1.checkSubTimer("half_minutes") == true) {
			// About every 30 seconds
			updateDisplay(DisplayTiming::EVERY_HALF_MINUTES_EVENT);
		}

		if (features.wake_bat_enable) {
			HAL.preventBatterySleep(HAL.Timer1.checkSubTimer("wake_battery"));
		}

		if (features.lora_wan_enable and not features.needs_ip_network) {
			if (HAL.Timer1.checkSubTimer("transmit") == true) {
				txEventFlag.transmit = true;
			}
		}

		updateDisplay(DisplayTiming::EVERY_ONE_SECONDS_EVENT);
		HAL.Log.flush();
		HAL.Led_Status.toggle();
		interactiveAdminControl();
		HAL.delay(5);
	}

	if (HAL.Timer2.checkSubTimer("gpio") == true or
	    txEventFlag.transmit == true) {
		// GPIO sampling interrupt

		if (HAL.Gpio.getPins().size() > 0) {
			// HAL.println("[Info] GPIO sensor updated!");
			updateValue(TYPE_GPIO | TYPE_PULSE_TOTAL | TYPE_PULSE_DELTA);
			updateDisplay(DisplayTiming::SAMPLING_GPIO_SENSORS, timeinfo);
		}
		printSensorData(false, TYPE_GPIO | TYPE_PULSE_TOTAL | TYPE_PULSE_DELTA);
	}

	if (HAL.Timer2.checkSubTimer("i2c") == true or
	    txEventFlag.link_i2c == true or
	    txEventFlag.transmit == true) {
		// I2C sampling interrupt

		std::vector<I2CActiveDevice> act_dev = i2c_device->getAddressList();
		if (act_dev.size() > 0) {
			// HAL.println("[Info] I2C sensor updated!");
			updateValue(TYPE_I2C);
			updateDisplay(DisplayTiming::SAMPLING_I2C_SENSORS, timeinfo);
		}
		printSensorData(false, TYPE_I2C);
	}

	if (HAL.Timer2.checkSubTimer("camera") == true or
	    txEventFlag.transmit == true) {
		// Camera sampling interrupt

		if (features.camera_enable and HAL.Camera.isReady()) {
			// HAL.println("[Info] Camera sensor updated!");
			updateValue(TYPE_CAMERA);
			updateDisplay(DisplayTiming::SAMPLING_CAMERA_SENSOR, timeinfo);
		}
		printSensorData(false, TYPE_CAMERA);
	}

	if (HAL.Timer2.checkSubTimer("modbus") == true or
	    txEventFlag.link_modbus == true or
	    txEventFlag.transmit == true) {
		// Modbus sampling interrupt

		if (modbus_device) {
			// HAL.println("[Info] MODBUS sensor updated!");
			updateValue(TYPE_MODBUS);
			updateDisplay(DisplayTiming::SAMPLING_MODBUS_SENSORS, timeinfo);
		}
		printSensorData(false, TYPE_MODBUS);
	}

	// Store fetch values
	if (txEventFlag.transmit == true or txEventFlag.on_change_transmit == true) {
		if (txEventFlag.transmit == true) {
			// Ex は容量が大きいので定期送信のみに限定
			storeExceptionalValue(timeinfo); // storeValue でセンサフラグを管理しているため、Ex の方を先に処理する
		}

		storeValue(timeinfo);
		txEventFlag.transmit           = false;
		txEventFlag.on_change_transmit = false;
		txEventFlag.link_i2c           = false;
		txEventFlag.link_modbus        = false;
	}

	if (features.sleep_enable and (not features.offline_mode)) {
		void *TX_LOOP_EXPLICIT = (void *)1; // 明示的な1回呼び出し

		constexpr uint8_t MAX_RETRY = 3;
		for (uint8_t i = 0; i < MAX_RETRY; i++) {
			transmitLoop(TX_LOOP_EXPLICIT);

			bool has_pending = false;
			for (auto &entry : transmitters) {
				if (entry.tx->getPendingDataQuantity() > 0) {
					has_pending = true;
					break;
				}
			}
			if (not has_pending) {
				break;
			}
			HAL.printf("Tx failed, retry %d/%d\n", i + 1, MAX_RETRY);
			HAL.delay(1000);
		}

		HAL.system_sleep(HAL.Info.TX_INTERVAL_SEC * 1000);
		txEventFlag.transmit = true;
	} else {
		HAL.delay(10);
	}
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Logic Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// ==================================================================
//! @brief ユーザ入力応答タスク
//! @param[in] (pvParameters) 未使用
// ==================================================================
void userInteractiveLoop(void *pvParameters) {
	while (1) {
		HAL.update();

		if (HAL.BtnA_wasPressed()) {
			updateDisplay(DisplayTiming::BTNA_PRESSED);
			toggleSelectedSensor();
		}

		if (HAL.BtnB_wasPressed()) {
			HAL.Log.getCurrentLoggingState() ? HAL.Log.loggingStop() : HAL.Log.loggingStart();
			updateDisplay(DisplayTiming::BTNB_PRESSED);
		}

		if (HAL.BtnC_wasPressed()) {
			updateDisplay(DisplayTiming::BTNC_PRESSED);
		}

		HAL.delay(10);
	}
}

// ==================================================================
//! @brief ストアしたデータの送信処理タスク
//! @param[in] (pvParameters) 単一呼び出しの場合は nullptr 以外を渡す
//! @see  storeValue
// ==================================================================
void transmitLoop(void *pvParameters) {
	const bool single_shot = (pvParameters != nullptr);

	if (features.offline_mode) {
		if (single_shot) {
			return;
		}

		while (true) {
			HAL.delay(1000);
			if (HAL.Timer1.checkSubTimer("one_seconds_in_transmit") == true) {
			}
			if (HAL.Timer1.checkSubTimer("one_minutes_in_transmit") == true) {
			}
		}
	}

	while (true) {
		/* Transmit BEGIN */
		if (features.tx_random_wait_enable) {
			// Random wait
			HAL.delay(HAL.getRandomNumber(0, 300) * 10);
		}

		for (auto &entry : transmitters) {
			entry.tx->update();
			entry.tx->updateEx();
			entry.tx->transmitHeartbeat();
		}
		/* Transmit END */

		HAL.delay(5);

		/* Update Display BEGIN */
		for (auto &entry : transmitters) {
			if ((entry.tx->getLastResult() & (RESULT_TX_NOTHING | RESULT_TX_ERROR)) == 0) {
				updateDisplay(entry.display_timing.buffer_update, entry.tx->getPendingDataQuantity());
				entry.tx->printIndex();
			}
			if ((entry.tx->getLastResultEx() & (RESULT_TX_NOTHING | RESULT_TX_ERROR)) == 0) {
				updateDisplay(entry.display_timing.buffer_update, entry.tx->getPendingDataQuantity());
				entry.tx->printIndexEx();
			}
		}
		/* Update Display END */

		if (single_shot) {
			break;
		}

		/* periodic process */
		if (HAL.Timer1.checkSubTimer("one_seconds_in_transmit") == true) {
		}
		if (HAL.Timer1.checkSubTimer("one_minutes_in_transmit") == true) {
			HAL.periodicTxTaskDebugMessage();
		}

		HAL.delay(5);
	}
}

// ==================================================================
//! @brief センサデータの取得更新処理
//! @param[in] (target) センサタイプの指定
//! @param[in] (target_index) 更新センサのインデックス値（負数は全センサ）
//! @see storeValue, Sensor
//! @note 取得したセンサ値は sensors リスト（vector）に格納される。
// ==================================================================
void updateValue(uint16_t target, int target_index) {
	uint16_t sensor_num = sensors.size();

	for (int i = 0; i < sensor_num; i++) {
		if ((target & sensors[i].type) == 0) {
			continue;
		}

		if (target_index >= 0) {
			if (i < target_index) {
				continue;
			}
			if (i > target_index) {
				break;
			}
		}

		if (sensors[i].type == TYPE_GPIO) {
			if (sensors[i].enable == false) {
				continue;
			}

			if (sensors[i].GPIO.digital) {
				sensors[i].i_value.latest = HAL.Gpio.digitalRead(sensors[i].GPIO.pin);
			} else {
				sensors[i].i_value.latest = HAL.Gpio.analogRead(sensors[i].GPIO.pin);
			}
			sensors[i].updated     = true;
			sensors[i].tx_buffered = false;

			if (not sensors[i].GPIO.digital) {
				// 最大値の時は量子化しない
				if (((1U << HAL.Gpio.getAdcBits()) - 1) != sensors[i].i_value.latest) {
					// ((a >> x) << x)
					// Quantization for analog value
					sensors[i].i_value.latest = ((sensors[i].i_value.latest >> sensors[i].shift_level) << sensors[i].shift_level);
				}
			}

			if (sensors[i].tx_on_change == false) {
				continue;
			}

			if (sensors[i].i_value.latest != sensors[i].i_value.prev) {
				sensors[i].i_value.prev  = sensors[i].i_value.latest;
				sensors[i].value_changed = true;

				txEventFlag.on_change_transmit = true;
				txEventFlag.link_i2c |= sensors[i].GPIO.tx_link_i2c;
				txEventFlag.link_modbus |= sensors[i].GPIO.tx_link_modbus;
			}
		} else if (sensors[i].type == TYPE_PULSE_TOTAL) {
			if (sensors[i].enable == false) {
				continue;
			}

			// パルスカウント値の更新
			HAL.Gpio.updatePulseCount(sensors[i].GPIO.pin);

			sensors[i].i_value.prev   = sensors[i].i_value.latest;
			sensors[i].i_value.latest = HAL.Gpio.getLastPulseCount(sensors[i].GPIO.pin);
			sensors[i].updated        = true;
			sensors[i].tx_buffered    = false;

		} else if (sensors[i].type == TYPE_PULSE_DELTA) {
			if (sensors[i].enable == false) {
				continue;
			}

			// if (target_index > 0) {
			// 	HAL.printf("%d %d %d\n",
			// 	           HAL.Gpio.getLastPulseCount(sensors[i].GPIO.pin),
			// 	           static_cast<uint32_t>(sensors[i].i_value.prev),
			// 	           (HAL.Gpio.getLastPulseCount(sensors[i].GPIO.pin) - static_cast<uint32_t>(sensors[i].i_value.prev)) % HAL.Gpio.getMaxPulseCount());
			// }
			sensors[i].i_value.latest = (HAL.Gpio.getLastPulseCount(sensors[i].GPIO.pin) - static_cast<uint32_t>(sensors[i].i_value.prev)) % HAL.Gpio.getMaxPulseCount();
			sensors[i].updated        = true;
			sensors[i].tx_buffered    = false;

		} else if (sensors[i].type == TYPE_I2C) {
			if (sensors[i].display == false and
			    sensors[i].I2C.sub_id != 0 and
			    txEventFlag.transmit == false) {
				// 画面表示不要かつ送信不要の場合は高速化のためスキップ
				continue;
			}

			int32_t i_data = 0;
			double d_data  = 0;
			String s_data  = "";
			enum I2C_DATA_RESULT result;

			HAL.takeSemaphoreMutex("i2c", portMAX_DELAY);
			result = i2c_device->fetchData(
			    sensors[i].port,
			    sensors[i].I2C.address,
			    sensors[i].I2C.channel,
			    sensors[i].I2C.sub_address,
			    sensors[i].I2C.sub_id,
			    sensors[i].tx_buffered,
			    &d_data,
			    &i_data,
			    &s_data);
			HAL.giveSemaphoreMutex("i2c");

			sensors[i].I2C.result  = result;
			sensors[i].updated     = true;
			sensors[i].tx_buffered = false;

			if (result == I2C_RES_FAILED) {
				// Failed to get data
				// HAL.printf("Error %s\n", sensors[i].name.c_str());
				continue;
			}

			if ((result & I2C_RES_INT) != 0) {
				sensors[i].i_value.prev   = sensors[i].i_value.latest;
				sensors[i].i_value.latest = i_data;
			}
			if ((result & I2C_RES_DOUBLE) != 0) {
				sensors[i].d_value.prev   = sensors[i].d_value.latest;
				sensors[i].d_value.latest = d_data;
			}
			if ((result & I2C_RES_STRING) != 0) {
				sensors[i].s_value.latest = s_data;
			}
		} else if (sensors[i].type == TYPE_MODBUS and modbus_device) {
			if (sensors[i].display == false and
			    sensors[i].MODBUS.sub_id != 0 and
			    txEventFlag.transmit == false) {
				// 画面表示不要かつ送信不要の場合は高速化のためスキップ
				continue;
			}

			int32_t i_data = 0;
			double d_data  = 0;
			String s_data  = "";
			enum MODBUS_DATA_RESULT result;
			result = modbus_device->fetchData(
			    sensors[i].port,
			    sensors[i].MODBUS.id,
			    sensors[i].MODBUS.sub_id,
			    sensors[i].tx_buffered,
			    &d_data,
			    &i_data,
			    &s_data);
			sensors[i].MODBUS.result = result;
			sensors[i].updated       = true;
			sensors[i].tx_buffered   = false;

			if (result == MODBUS_RES_FAILED) {
				// Failed to get data
				// HAL.printf("Error %s\n", sensors[i].name.c_str());
				continue;
			}

			if ((result & MODBUS_RES_INT) != 0) {
				sensors[i].i_value.prev   = sensors[i].i_value.latest;
				sensors[i].i_value.latest = i_data;
			}
			if ((result & MODBUS_RES_DOUBLE) != 0) {
				sensors[i].d_value.prev   = sensors[i].d_value.latest;
				sensors[i].d_value.latest = d_data;
			}
			if ((result & MODBUS_RES_STRING) != 0) {
				sensors[i].s_value.latest = s_data;
			}
		} else if (sensors[i].type == TYPE_CAMERA) {
#if HAL_ENABLE_EXCEPTIONAL_BUFFER
			sensors[i].CAMERA.captured = false;

			if (sensors[i].display == false and
			    txEventFlag.transmit == false) {
				// 画面表示不要かつ送信不要の場合は高速化のためスキップ
				continue;
			}

			HAL.takeSemaphoreMutex("camera", portMAX_DELAY);

			if (HAL.Camera.updateCapture()) {
				sensors[i].updated         = true;
				sensors[i].tx_buffered     = false;
				sensors[i].CAMERA.captured = true;
			}

			HAL.giveSemaphoreMutex("camera");
#endif /* HAL_ENABLE_EXCEPTIONAL_BUFFER */
		}
	}
}

template <typename ValueType>
void addValueToBuffer(struct tm &timeinfo, const struct Sensor &sensor, ValueType value) {
	for (auto &entry : transmitters) {
		entry.tx->addValue(timeinfo, value, sensor.name, sensor.unit, sensor.port);
		entry.tx->printIndex();
	}
}

// ==================================================================
//! @brief センサ値を FIFO リングバッファに格納する
//! @param[in] (timeinfo) センサ値取得時間
//! @see updateValue, transmitLoop, Sensor, IoTTxFTP, IoTTxMQTT
//! @note 全てのデータは格納時には文字列に変換されます。
// ==================================================================
void storeValue(struct tm timeinfo) {
	bool flag_updated   = false;
	uint16_t sensor_num = sensors.size();

	for (int i = 0; i < sensor_num; i++) {
		if (sensors[i].updated == false) {
			continue;
		}
		flag_updated = true;

		if (sensors[i].type == TYPE_GPIO) {
			if (txEventFlag.on_change_transmit == true and txEventFlag.transmit == false) {
				if (sensors[i].tx_on_change == false) {
					// 変更時トリガの対象ではない
					continue;
				}
				if (sensors[i].value_changed == false) {
					// 今回変更をトリガしたセンサではない
					continue;
				}
			}

			addValueToBuffer(timeinfo, sensors[i], sensors[i].i_value.latest);
			HAL.printf("%s buffered! -> %d %s\n",
			           sensors[i].name.c_str(),
			           sensors[i].i_value.latest,
			           sensors[i].unit.c_str());
		} else if (sensors[i].type == TYPE_PULSE_TOTAL) {
			if (txEventFlag.transmit == false) {
				continue;
			}

			addValueToBuffer(timeinfo, sensors[i], sensors[i].i_value.latest);
			HAL.printf("%s buffered! -> %d %s\n",
			           sensors[i].name.c_str(),
			           sensors[i].i_value.latest,
			           sensors[i].unit.c_str());

		} else if (sensors[i].type == TYPE_PULSE_DELTA) {
			if (txEventFlag.transmit == false) {
				continue;
			}

			addValueToBuffer(timeinfo, sensors[i], sensors[i].i_value.latest);
			HAL.printf("%s buffered! -> %d %s\n",
			           sensors[i].name.c_str(),
			           sensors[i].i_value.latest,
			           sensors[i].unit.c_str());

			// 比較基準の更新
			sensors[i].i_value.prev = HAL.Gpio.getLastPulseCount(sensors[i].GPIO.pin);

		} else if (sensors[i].type == TYPE_I2C) {
			/* I2C sensor begin */
			if (txEventFlag.on_change_transmit == true and txEventFlag.transmit == false) {
				if (txEventFlag.link_i2c == false) {
					continue;
				}
			}

			if ((sensors[i].I2C.result & I2C_RES_INT) != 0) {
				addValueToBuffer(timeinfo, sensors[i], sensors[i].i_value.latest);
				HAL.printf("%s buffered! (int)    -> %d %s\n",
				           sensors[i].name.c_str(),
				           sensors[i].i_value.latest,
				           sensors[i].unit.c_str());
			}

			if ((sensors[i].I2C.result & I2C_RES_DOUBLE) != 0) {
				addValueToBuffer(timeinfo, sensors[i], sensors[i].d_value.latest);
				HAL.printf("%s buffered! (double) -> %f %s\n",
				           sensors[i].name.c_str(),
				           sensors[i].d_value.latest,
				           sensors[i].unit.c_str());
			}

			if ((sensors[i].I2C.result & I2C_RES_STRING) != 0) {
				addValueToBuffer(timeinfo, sensors[i], sensors[i].s_value.latest);
				HAL.printf("%s buffered! (string) -> %s %s\n",
				           sensors[i].name.c_str(),
				           sensors[i].s_value.latest.c_str(),
				           sensors[i].unit.c_str());
			}
			/* I2C sensor end */
		} else if (sensors[i].type == TYPE_MODBUS) {
			/* Modbus sensor begin */
			if (txEventFlag.on_change_transmit == true and txEventFlag.transmit == false) {
				if (txEventFlag.link_modbus == false) {
					continue;
				}
			}

			if ((sensors[i].MODBUS.result & MODBUS_RES_INT) != 0) {
				addValueToBuffer(timeinfo, sensors[i], sensors[i].i_value.latest);
				HAL.printf("%s buffered! (int)    -> %d %s\n",
				           sensors[i].name.c_str(),
				           sensors[i].i_value.latest,
				           sensors[i].unit.c_str());
			}

			if ((sensors[i].MODBUS.result & MODBUS_RES_DOUBLE) != 0) {
				addValueToBuffer(timeinfo, sensors[i], sensors[i].d_value.latest);
				HAL.printf("%s buffered! (double) -> %f %s\n",
				           sensors[i].name.c_str(),
				           sensors[i].d_value.latest,
				           sensors[i].unit.c_str());
			}

			if ((sensors[i].MODBUS.result & MODBUS_RES_STRING) != 0) {
				addValueToBuffer(timeinfo, sensors[i], sensors[i].s_value.latest);
				HAL.printf("%s buffered! (string) -> %s %s\n",
				           sensors[i].name.c_str(),
				           sensors[i].s_value.latest.c_str(),
				           sensors[i].unit.c_str());
			}
			/* Modbus sensor end */
		}

		sensors[i].tx_buffered   = true;
		sensors[i].updated       = false;
		sensors[i].value_changed = false;
	} /* end of for */

	if (not flag_updated) {
		return;
	}

	for (auto &entry : transmitters) {
		updateDisplay(entry.display_timing.buffer_update, entry.tx->getPendingDataQuantity());
	}
}

// ==================================================================
//! @brief カメラ画像やバッチ送信用JSONなど、容量が大きい特殊データを格納する
//! @param[in] (timeinfo) センサ値取得時間
//! @see updateValue, transmitLoop, storeValue, Sensor, IoTTxFTP, IoTTxMQTT
//! @note 定期送信タイミングでのみ呼び出される（storeValue とは別の Ex 用バッファに格納）。
// ==================================================================
void storeExceptionalValue(struct tm timeinfo) {
#if HAL_ENABLE_EXCEPTIONAL_BUFFER
	if (not features.batch_send_enable and
	    not features.camera_enable) {
		return;
	}

	bool flag_updated   = false;
	uint16_t sensor_num = sensors.size();
	JsonObject sensor;

	if (features.batch_send_enable) {
		HAL.takeSemaphoreMutex("ex_sensor_data", portMAX_DELAY);
		// 前回のセンサ値を削除
		ex_sensor_data.clear();
	}

	for (int i = 0; i < sensor_num; i++) {
		if (sensors[i].updated == false) {
			continue;
		}
		flag_updated = true;

		switch (sensors[i].type) {
		case TYPE_GPIO: {
			if (features.batch_send_enable) {
				auto gpio       = ex_sensor_data["gpio"].to<JsonObject>();
				auto sensor     = gpio[sensors[i].name].to<JsonObject>();
				sensor["value"] = sensors[i].i_value.latest;
			}
			break;
		}
		case TYPE_I2C: {
			if (features.batch_send_enable) {
				String port_name = "port" + String(sensors[i].port);
				if (ex_sensor_data["i2c"].isNull()) {
					// 既存データがない場合は新規作成
					auto i2c  = ex_sensor_data["i2c"].to<JsonObject>();
					auto port = i2c[port_name].to<JsonObject>();
					sensor    = port[sensors[i].name].to<JsonObject>();
				} else {
					sensor = ex_sensor_data["i2c"][port_name][sensors[i].name].to<JsonObject>();
				}

				sensor["unit"] = sensors[i].unit;

				if ((sensors[i].I2C.result & I2C_RES_INT) != 0) {
					sensor["value"] = sensors[i].i_value.latest;
				}
				if ((sensors[i].I2C.result & I2C_RES_DOUBLE) != 0) {
					sensor["value"] = sensors[i].d_value.latest;
				}
				if ((sensors[i].I2C.result & I2C_RES_STRING) != 0) {
					sensor["value"] = sensors[i].s_value.latest;
				}
			}
			break;
		}
		case TYPE_MODBUS: {
			if (features.batch_send_enable) {
				String port_name = "port" + String(sensors[i].port);
				if (ex_sensor_data["modbus"].isNull()) {
					// 既存データがない場合は新規作成
					auto modbus = ex_sensor_data["modbus"].to<JsonObject>();
					auto port   = modbus[port_name].to<JsonObject>();
					sensor      = port[sensors[i].name].to<JsonObject>();
				} else {
					sensor = ex_sensor_data["modbus"][port_name][sensors[i].name].to<JsonObject>();
				}
				sensor["unit"] = sensors[i].unit;

				if ((sensors[i].MODBUS.result & MODBUS_RES_INT) != 0) {
					sensor["value"] = sensors[i].i_value.latest;
				}
				if ((sensors[i].MODBUS.result & MODBUS_RES_DOUBLE) != 0) {
					sensor["value"] = sensors[i].d_value.latest;
				}
				if ((sensors[i].MODBUS.result & MODBUS_RES_STRING) != 0) {
					sensor["value"] = sensors[i].s_value.latest;
				}
			}
			break;
		}
		case TYPE_CAMERA: {
			constexpr uint8_t EX_STORAGE_NUMBER_0 = 0;

			// convert to jpeg image
			HAL.takeSemaphoreMutex("camera", portMAX_DELAY);
			HAL.Camera.convert2jpg(); // 処理結果判定が必要かも
			HAL.giveSemaphoreMutex("camera");

			for (auto &entry : transmitters) {
				entry.tx->addExceptionalValue(EX_STORAGE_NUMBER_0,
				                              timeinfo,
				                              sensors[i].name,
				                              sensors[i].unit,
				                              sensors[i].port);
				entry.tx->printIndex();
			}

			HAL.printf("%s update!\n", sensors[i].name.c_str());
			break;
		}
		default:
			break;
		}
	} /* end of for */

	if (features.batch_send_enable) {
		constexpr uint8_t EX_STORAGE_NUMBER_1 = 1;

		for (auto &entry : transmitters) {
			entry.tx->addExceptionalValue(EX_STORAGE_NUMBER_1,
			                              timeinfo,
			                              "Batch Transfer",
			                              "",
			                              0);
			entry.tx->printIndex();
		}

		HAL.giveSemaphoreMutex("ex_sensor_data");
	}

	if (not flag_updated) {
		return;
	}

	// JsonDocument doc;
	// doc["sensors"] = ex_sensor_data;
	// String output = "";
	// serializeJsonPretty(doc, output);
	// HAL.println(output.c_str());
#endif
}

// ==================================================================
//! @brief 表示用選択センサの切り替え
//! @see printSensorData
// ==================================================================
void toggleSelectedSensor() {
	bool _disp                = false;
	bool _all_false           = true;
	enum SensorType type      = TYPE_NONE;
	uint16_t sensor_num       = sensors.size();
	int selected_sensor_index = 0;

	for (int i = 0; i < sensor_num; i++) {
		if (_disp and sensors[i].enable) {
			sensors[i].display = true;
			updateDisplay(DisplayTiming::SET_DISPLAY_SENSOR_NAME,
			              sensors[i].name.c_str());

			type  = sensors[i].type;
			_disp = false;

			selected_sensor_index = i;
			break;
		}
		if (sensors[i].display) {
			sensors[i].display = false;
			_disp              = true;
			_all_false         = false;
		}
	}
	if (_disp == true or _all_false == true) {
		// 直前の表示が最後のセンサだった or Ready表示だった場合
		for (int i = 0; i < sensor_num; i++) {
			if (sensors[i].enable) {
				sensors[i].display = true;
				updateDisplay(DisplayTiming::SET_DISPLAY_SENSOR_NAME,
				              sensors[i].name.c_str());

				type = sensors[i].type;

				selected_sensor_index = i;
				break;
			}
		}
	}

	switch (type) {
	case TYPE_GPIO:
		updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
		break;
	case TYPE_PULSE_TOTAL:
		updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
		break;
	case TYPE_PULSE_DELTA:
		updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
		break;
	case TYPE_I2C:
		// 通信に時間がかかるので、前回値が無い or 更新間隔が長い場合のみ最新値を取得する
		if (sensors[selected_sensor_index].I2C.result == I2C_RES_FAILED) {
			updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
		} else {
			if (HAL.Timer2.getSubTimerLimitMillis("i2c") > 60 * 1000) {
				// センサ値の更新間隔が1分以上の場合は強制的に更新する
				updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
			}
		}
		break;
	case TYPE_MODBUS:
		// 通信に時間がかかるので、前回値が無い or 更新間隔が長い場合のみ最新値を取得する
		if (sensors[selected_sensor_index].MODBUS.result == MODBUS_RES_FAILED) {
			updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
		} else {
			if (HAL.Timer2.getSubTimerLimitMillis("modbus") > 60 * 1000) {
				// センサ値の更新間隔が1分以上の場合は強制的に更新する
				updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
			}
		}
		break;
	case TYPE_CAMERA:
		updateValue(type, selected_sensor_index); // 表示更新前に最新値を取得
		break;
	default:
		updateValue(type); // 表示更新前に最新値を取得
		break;
	}

	printSensorData(true, type);
}

// ==================================================================
//! @brief センサデータの表示要求機能
//! @param[in] (force_update) 更新していない値も表示要求する
//! @see updateDisplay
//! @note センサ値が更新されなかったものは無視される
// ==================================================================
void printSensorData(bool force_update, uint16_t target) {
	uint16_t sensor_num = sensors.size();

	for (int i = 0; i < sensor_num; i++) {
		if (force_update == false) {
			if (sensors[i].updated == false) {
				continue;
			}
			if ((target & sensors[i].type) == 0) {
				continue;
			}
		}

		switch (sensors[i].type) {
		case TYPE_GPIO:
			// Fallthrough
		case TYPE_PULSE_TOTAL:
			// Fallthrough
		case TYPE_PULSE_DELTA:
			updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_GPIO,
			              sensors[i].name.c_str(),
			              sensors[i].i_value.latest,
			              sensors[i].unit.c_str(),
			              sensors[i].display);
			break;
		case TYPE_I2C:
			if ((sensors[i].I2C.result & I2C_RES_INT) != 0) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_INT,
				              sensors[i].name.c_str(),
				              sensors[i].i_value.latest,
				              sensors[i].unit.c_str(),
				              sensors[i].display);
			} else if ((sensors[i].I2C.result & I2C_RES_DOUBLE) != 0) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_DOUBLE,
				              sensors[i].name.c_str(),
				              sensors[i].d_value.latest,
				              sensors[i].unit.c_str(),
				              sensors[i].display);
			} else if ((sensors[i].I2C.result & I2C_RES_STRING) != 0) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_STRING,
				              sensors[i].name.c_str(),
				              sensors[i].s_value.latest.c_str(),
				              sensors[i].unit.c_str(),
				              sensors[i].display);
			} else if (sensors[i].I2C.result == I2C_RES_FAILED) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_I2C_ERROR,
				              sensors[i].name.c_str(),
				              sensors[i].display);
			}
			break;
		case TYPE_MODBUS:
			if ((sensors[i].MODBUS.result & MODBUS_RES_INT) != 0) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_INT,
				              sensors[i].name.c_str(),
				              sensors[i].i_value.latest,
				              sensors[i].unit.c_str(),
				              sensors[i].display);
			} else if ((sensors[i].MODBUS.result & MODBUS_RES_DOUBLE) != 0) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_DOUBLE,
				              sensors[i].name.c_str(),
				              sensors[i].d_value.latest,
				              sensors[i].unit.c_str(),
				              sensors[i].display);
			} else if ((sensors[i].MODBUS.result & MODBUS_RES_STRING) != 0) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_STRING,
				              sensors[i].name.c_str(),
				              sensors[i].s_value.latest.c_str(),
				              sensors[i].unit.c_str(),
				              sensors[i].display);
			} else if (sensors[i].MODBUS.result == MODBUS_RES_FAILED) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_MODBUS_ERROR,
				              sensors[i].name.c_str(),
				              sensors[i].display);
			}
			break;
		case TYPE_CAMERA:
			if (sensors[i].CAMERA.captured) {
				updateDisplay(DisplayTiming::UPDATE_DISPLAY_VALUE_CAMERA,
				              sensors[i].name.c_str(),
				              sensors[i].display);
			}
			break;
		default:
			break;
		}
	}
}

// ==================================================================
//! @brief デバッグ/最終手段のデータサルベージ用
// ==================================================================
void interactiveAdminControl() {
	int c = HAL.readSerialInput();
	if (c < 0) {
		return;
	}

	switch (c) {
	case '\n':
	case '\r':
	case '\t':
		// Fallthrough
		// Ignore some characters
		return;
	default:
		break;
	}

	HAL.println("===== Interactive Admin Control =====");
	switch (c) {
	case 'a':
		if (features.ambient_enable) {
			tx_ambient->printPendingData();
		} else {
			HAL.println("Ambient is not enabled");
		}
		break;
	case 'f':
		if (features.ftp_enable) {
			tx_ftp->printPendingData();
		} else {
			HAL.println("FTP is not enabled");
		}
		break;
	case 'h':
		if (features.http_enable) {
			tx_http->printPendingData();
		} else {
			HAL.println("HTTP is not enabled");
		}
		break;
	case 'm':
		if (features.mqtt_enable) {
			tx_mqtt->printPendingData();
		} else {
			HAL.println("MQTT is not enabled");
		}
		break;
	case 'v':
		HAL.printf("FW version %d.%d %s\n", HAL.Info.V_MAIN, HAL.Info.V_MINOR, HAL.Info.V_RELEASE_TAG.c_str());
		break;
	case 'w':
		HAL.printf("Target WiFi SSID: %s\n", HAL.Info.WIFI_SSID.c_str());
		break;
	case 't':
		HAL.printf("Transmit interval sec: %d\n", HAL.Info.TX_INTERVAL_SEC);
		break;
	case '?':
		HAL.println("a: Print ambient pending data");
		HAL.println("f: Print ftp pending data");
		HAL.println("h: Print http pending data");
		HAL.println("m: Print mqtt pending data");
		HAL.println("v: Print firmware version");
		HAL.println("w: Print wireless lan SSID");
		break;
	default:
		HAL.println("Invalid command");
		HAL.println("Help command is \'?\'");
		break;
	}
	HAL.println("=====================================");
}