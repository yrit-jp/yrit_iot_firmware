#include "initialize.h"

std::vector<struct Sensor> sensors;
ParamManager param_manager;
std::unique_ptr<I2C_Manager> i2c_device;
std::unique_ptr<Modbus_Manager> modbus_device;
std::unique_ptr<SettingBT> sbt;
std::unique_ptr<IoTTxAmbient> tx_ambient;
std::unique_ptr<IoTTxFTP> tx_ftp;
std::unique_ptr<IoTTxHTTP> tx_http;
std::unique_ptr<IoTTxMQTT> tx_mqtt;

constexpr bool DEFAULT_FALSE = false;
constexpr bool DEFAULT_TRUE  = true;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Initialize Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

//
//! @brief SDカードへのロギング機能の初期化を行います
void InitializeLog(const char *save_dir) {
	HAL.println("Initialize Log...");

	HAL.Log.setSaveDirectory(save_dir);
	HAL.Log.setFileBaseName("LOG_", ".log");
	if (HAL.Log.begin()) {
		HAL.println("Done");
		HAL.Log.loggingStart();
	} else {
		HAL.println("Failed!");
	}
}

//
//! @brief パラメータ設定機能の初期化を行います
void InitializeSettingParameter() {
	HAL.print("Initialize Setting Server... ");
	updateDisplay(DisplayTiming::SS_INIT_BEGIN, (INPUT_WAIT_TIME / 1000));

	auto pressed_btn = HAL.waitForButtonPressed(INPUT_WAIT_TIME);
	if ((pressed_btn & (uint8_t)PRESSED_BUTTON::PRESSED_BTN1) != 0) {
		HAL.println("");
		updateDisplay(DisplayTiming::SS_MODE_ENTRY);

		if (INPUT_WAIT_TIME > 1000) {
			// If device has a long wait time
			// wait button release
			while (HAL.BtnA_isPressed()) {
				// Wait Release
				HAL.update();
				HAL.delay(5);
			}
		}

		pressed_btn = HAL.waitForButtonPressed(INPUT_WAIT_TIME);
		if ((pressed_btn & (uint8_t)PRESSED_BUTTON::PRESSED_BTN2) != 0) {
			updateDisplay(DisplayTiming::SS_CLEAR_ALL);
			HAL.delay(3000);
			param_manager.begin(true);
		} else {
			updateDisplay(DisplayTiming::SS_FORMAT);
			param_manager.begin();
		}

		String chipid = HAL.getChipUUID();
		HAL.printf("%s\n", chipid.c_str());
		HAL.printf("Up to %d byte packets can be received.\n", ZIGBEE_API_MAX_FRAME_SIZE);
		updateDisplay(DisplayTiming::SS_READY);
		HAL.delay(300);
		sbt.reset(new SettingBT());
		sbt->begin(chipid);

		HAL.Log.loggingStop();
		while (true) {
			HAL.delay(5);
			HAL.update();
			updateDisplay(DisplayTiming::SS_LOOP);
			if (sbt->update() == ZAS_COMPLETE) {
				UpdateSettings();
			}
		}
	}

	updateDisplay(DisplayTiming::SS_FORMAT);
	param_manager.begin();
	updateDisplay(DisplayTiming::SS_INIT_DONE);
	HAL.println("Done!");

	// PRINT
	HAL.println("Current settings");
	String parameter_string = "";
	serializeJsonPretty(param_manager.getSettingJson(), parameter_string);
	HAL.print(parameter_string.c_str());
	HAL.println("");
}

//
//! @brief LED の初期化を行います
void InitializeLed() {
	HAL.print("Initialize LED... ");
	HAL.Led_Status.begin();
	HAL.Led_Warning.begin();

	if (param_manager.checkParameter("PARAM", "LED_BRIGHTNESS_PCT")) {
		double led_level = param_manager.getParameter<double>("PARAM", "LED_BRIGHTNESS_PCT");
		HAL.printf("\n    LED level: %3.1f%%\n", led_level);
		HAL.Led_Status.setBrightnessLevel(led_level);
		HAL.Led_Warning.setBrightnessLevel(led_level);
	}

	HAL.Led_Status.on();
	HAL.Led_Warning.on();
	HAL.println("Done!");
}

//
//! @brief GPIO 機能の初期化を行います
void InitializeGPIO() {
	HAL.print("Initialize GPIO... ");
	updateDisplay(DisplayTiming::GPIO_INIT_BEGIN);

	std::vector<uint8_t> pins = HAL.Gpio.getPins();     // Get GPIO pin list
	String modifier           = HAL.Gpio.getModifier(); // Get header name

	if (pins.size() == 0) {
		updateDisplay(DisplayTiming::GPIO_INIT_DONE);
		HAL.println("Skip (ignore)!");
		return;
	}

	for (int i = 0; i < pins.size(); i++) {
		uint8_t pin = pins[i];
		String key  = modifier + String(pin);
		if (param_manager.getParameter(key.c_str(), "ENABLE", DEFAULT_FALSE) == false) {
			// GPIO が無効の場合
			HAL.Gpio.setUnused(pin);
			continue;
		}

		constexpr uint8_t DEFAULT_LEVEL = 4; // "4" don't have special mean

		uint8_t shift_level = param_manager.getParameter<uint8_t>(key.c_str(), "SHIFT_LEVEL", DEFAULT_LEVEL);
		shift_level         = (shift_level > (HAL.Gpio.getAdcBits() - 1))
		                          ? HAL.Gpio.getAdcBits()
		                          : shift_level; // 12bit ADC

		Sensor gpio_sensor;
		gpio_sensor.type           = TYPE_GPIO;
		gpio_sensor.name           = param_manager.getParameter<const char *>(key.c_str(), "NAME");
		gpio_sensor.unit           = "";
		gpio_sensor.enable         = true; // false の場合は最初に除外済
		gpio_sensor.display        = false;
		gpio_sensor.updated        = false;
		gpio_sensor.tx_buffered    = false;
		gpio_sensor.value_changed  = false;
		gpio_sensor.i_value.latest = 0;
		gpio_sensor.i_value.prev   = -1;  // Digital and analog values are absolutely positive.
		gpio_sensor.d_value.latest = 0.0; // Not used in GPIO
		gpio_sensor.d_value.prev   = 0.0; // Not used in GPIO
		gpio_sensor.tx_on_change   = param_manager.getParameter(key.c_str(), "TX_ON_CHANGE", DEFAULT_FALSE);
		gpio_sensor.shift_level    = shift_level;
		gpio_sensor.port           = pin;

		gpio_sensor.GPIO.pin            = pin;
		gpio_sensor.GPIO.digital        = param_manager.getParameter(key.c_str(), "DIGITAL", DEFAULT_TRUE);
		gpio_sensor.GPIO.tx_link_i2c    = param_manager.getParameter(key.c_str(), "TX_LINK_I2C", DEFAULT_FALSE);
		gpio_sensor.GPIO.tx_link_modbus = param_manager.getParameter(key.c_str(), "TX_LINK_MODBUS", DEFAULT_FALSE);

		if (gpio_sensor.GPIO.digital) {
			bool pull_up = param_manager.getParameter(key.c_str(), "PULL_UP", DEFAULT_FALSE);
			HAL.Gpio.pinMode(pin, pull_up ? INPUT_PULLUP : INPUT);
		} else {
			HAL.Gpio.pinMode(pin, INPUT);
		}

		sensors.push_back(gpio_sensor);
	}

	updateDisplay(DisplayTiming::GPIO_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief パルスカウンタ機能の初期化を行います
void InitializePulseCounter() {
	HAL.print("Initialize Pulse Counter... ");
	updateDisplay(DisplayTiming::PULSE_COUNTER_INIT_BEGIN);

	std::vector<uint8_t> pins = HAL.Gpio.getPins();     // Get GPIO pin list
	String modifier           = HAL.Gpio.getModifier(); // Get header name

	if (pins.size() == 0) {
		updateDisplay(DisplayTiming::PULSE_COUNTER_INIT_END);
		HAL.println("Skip (ignore)!");
		return;
	}

	HAL.println("");
	for (int i = 0; i < pins.size(); i++) {
		uint8_t pin = pins[i];
		String key  = modifier + String(pin);
		HAL.printf("    Pin %d... ", pin);
		updateDisplay(DisplayTiming::PULSE_COUNTER_INIT_TARGET, pin);

		if (param_manager.getParameter(key.c_str(), "ENABLE", DEFAULT_FALSE) == false) {
			// GPIO が無効の場合
			HAL.println("Skip (ignore)! -> Disabled");
			continue;
		}

		if (param_manager.getParameter(key.c_str(), "DIGITAL", DEFAULT_TRUE) == false) {
			// アナログ入力モードの場合
			HAL.println("Skip (ignore)! -> Analog mode");
			continue;
		}

		bool edge_rising  = param_manager.getParameter(key.c_str(), "PULSE_COUNT_RISE", DEFAULT_FALSE);
		bool edge_falling = param_manager.getParameter(key.c_str(), "PULSE_COUNT_FALL", DEFAULT_FALSE);
		if (edge_rising == false and edge_falling == false) {
			// パルスカウント用の設定パラメータが存在しない または パルスカウント機能が無効になっている
			HAL.println("Skip (ignore)! -> Disable pulse count");
			continue;
		}

		// 検出エッジの決定
		PULSE_EDGE edge = (edge_rising and edge_falling)
		                      ? PULSE_EDGE::BOTH
		                      : ((edge_rising)
		                             ? PULSE_EDGE::RISE
		                             : PULSE_EDGE::FALL);

		if (HAL.Gpio.initializePulseCounter(pin, edge) == false) {
			// 初期化の失敗
			HAL.println("Skip (ignore)!");
			continue;
		}

		Sensor pulse_count_sensor;
		// 合計パルスのセット
		pulse_count_sensor.type = TYPE_PULSE_TOTAL;
		pulse_count_sensor.name = param_manager.getParameter<const char *>(key.c_str(), "NAME");
		pulse_count_sensor.name += "_Pulse_Total";
		pulse_count_sensor.unit           = "";
		pulse_count_sensor.enable         = true;
		pulse_count_sensor.display        = false;
		pulse_count_sensor.updated        = false;
		pulse_count_sensor.tx_buffered    = false;
		pulse_count_sensor.value_changed  = false;
		pulse_count_sensor.i_value.latest = 0;
		pulse_count_sensor.i_value.prev   = 0;   // values are absolutely positive.
		pulse_count_sensor.d_value.latest = 0.0; // Not used in GPIO
		pulse_count_sensor.d_value.prev   = 0.0; // Not used in GPIO
		pulse_count_sensor.tx_on_change   = false;
		pulse_count_sensor.shift_level    = 0;
		pulse_count_sensor.port           = pin;

		pulse_count_sensor.GPIO.pin            = pin;
		pulse_count_sensor.GPIO.digital        = true;
		pulse_count_sensor.GPIO.tx_link_i2c    = false;
		pulse_count_sensor.GPIO.tx_link_modbus = false;

		sensors.push_back(pulse_count_sensor);

		// 差分パルスのセット
		pulse_count_sensor.type = TYPE_PULSE_DELTA;
		pulse_count_sensor.name = param_manager.getParameter<const char *>(key.c_str(), "NAME");
		pulse_count_sensor.name += "_Pulse_Delta";

		sensors.push_back(pulse_count_sensor);

		switch (edge) {
		case PULSE_EDGE::BOTH:
			HAL.println("Success! (Both edge)");
			break;
		case PULSE_EDGE::RISE:
			HAL.println("Success! (Rising edge)");
			break;
		case PULSE_EDGE::FALL:
			HAL.println("Success! (Falling edge)");
			break;
		}
	}

	updateDisplay(DisplayTiming::PULSE_COUNTER_INIT_END);
	HAL.println("Done!");
}

//
//! @brief I2C 機能の初期化を行います
void InitializeI2c() {
	HAL.print("Initialize I2C ... ");
	updateDisplay(DisplayTiming::I2C_INIT_PERIPHERAL_BEGIN);

	std::vector<uint8_t> ports = HAL.I2c.getPorts();

	if (ports.size() == 0) {
		updateDisplay(DisplayTiming::I2C_INIT_PERIPHERAL_DONE);
		HAL.println("Skip (No port available)!");
		return;
	}

	HAL.println("");
	for (const uint8_t &port : ports) {
		updateDisplay(DisplayTiming::I2C_INIT_PERIPHERAL_TARGET, port);
		HAL.printf("    Port%d : ", port);

		bool result = HAL.I2c.begin(port);
		HAL.printf("%s\n", ((result == true) ? "Success!" : "Failed"));
	}

	updateDisplay(DisplayTiming::I2C_INIT_PERIPHERAL_DONE);
	HAL.println("Done!");
}

//
//! @brief I2C デバイスの初期化を行います
void InitializeI2cDevice() {
	std::vector<uint8_t> ports = HAL.I2c.getPorts();
	if (ports.size() == 0) {
		return;
	}

	HAL.print("Initialize I2C devices... \n");
	updateDisplay(DisplayTiming::I2C_INIT_DEVICE_BEGIN);

	uint8_t mask = 0; // param_manager.getParameter<uint8_t>("I2C", "FILTER_MASK");
	uint8_t addr = 0; // param_manager.getParameter<uint8_t>("I2C", "FILTER_ADDR");
	i2c_device.reset(new I2C_Manager());
	i2c_device->setAddressFilter(addr, mask);

	i2c_device->clearDetectedDevices();
	HAL.takeSemaphoreMutex("i2c", portMAX_DELAY);
	for (const uint8_t &port : ports) {
		if (HAL.I2c.getWire(port) == nullptr) {
			// 初期化に失敗したポート
			HAL.printf("  Port%d ignore because init failed.", port);
			continue;
		}
		HAL.printf("  Port%d scanning...", port);
		i2c_device->scanDevice(port);
		HAL.print("  Done!\n");
	}
	HAL.giveSemaphoreMutex("i2c");

	i2c_device->displayScanResult();
	std::vector<I2CActiveDevice> act_dev = i2c_device->getAddressList();
	for (int i = 0; i < act_dev.size(); i++) {
		updateDisplay(DisplayTiming::I2C_INIT_DEVICE_TARGET, act_dev[i].address, i + 1, act_dev.size());

		if (act_dev[i].address == act_dev[i].sub_address) {
			HAL.printf("    [%d] Init 0x%02X... ", act_dev[i].port, act_dev[i].address);
		} else {
			HAL.printf("    [%d] Init 0x%02X -(ch.%d)-> 0x%02X... ",
			           act_dev[i].port,
			           act_dev[i].address,
			           act_dev[i].channel,
			           act_dev[i].sub_address);
		}

		HAL.takeSemaphoreMutex("i2c", portMAX_DELAY);
		int8_t res = i2c_device->initializeDevice(act_dev[i].port, act_dev[i].address, act_dev[i].channel, act_dev[i].sub_address);
		HAL.giveSemaphoreMutex("i2c");

		if (res == I2C_IGNORE_DEVICE) {
			HAL.println("Skip (ignore)!");
			continue;
		}

		for (int k = 0; k < res; k++) {
			Sensor i2c_sensor;
			i2c_sensor.type = TYPE_I2C;
			i2c_sensor.name = i2c_device->getDeviceName(
			    act_dev[i].port,
			    act_dev[i].address,
			    act_dev[i].channel,
			    act_dev[i].sub_address,
			    k);
			i2c_sensor.unit = i2c_device->getDeviceUnit(
			    act_dev[i].port,
			    act_dev[i].address,
			    act_dev[i].channel,
			    act_dev[i].sub_address,
			    k);
			i2c_sensor.enable          = true;
			i2c_sensor.display         = false;
			i2c_sensor.updated         = false;
			i2c_sensor.tx_buffered     = false;
			i2c_sensor.value_changed   = false;
			i2c_sensor.i_value.latest  = 0;
			i2c_sensor.i_value.prev    = 0;
			i2c_sensor.d_value.latest  = 0.0;
			i2c_sensor.d_value.prev    = 0.0;
			i2c_sensor.s_value.latest  = "";
			i2c_sensor.s_value.prev    = "";
			i2c_sensor.tx_on_change    = false;
			i2c_sensor.shift_level     = 0;
			i2c_sensor.port            = act_dev[i].port;
			i2c_sensor.I2C.address     = act_dev[i].address;
			i2c_sensor.I2C.sub_address = act_dev[i].sub_address;
			i2c_sensor.I2C.channel     = act_dev[i].channel;
			i2c_sensor.I2C.result      = I2C_RES_FAILED;
			i2c_sensor.I2C.sub_id      = k;

			sensors.push_back(i2c_sensor);
		}
		// バスマルチプレクサで検出したデバイスがあれば更新する
		act_dev = i2c_device->getAddressList();

		HAL.println("Done!");
	}
	updateDisplay(DisplayTiming::I2C_INIT_DEVICE_DONE);

	HAL.println("Done!");

	i2c_device->displayScanResult();
}

//
//! @brief カメラ機能の初期化を行います
void InitializeCameraDevice() {
	if (HAL.Camera.isInstalled() == false) {
		// Camera device not installed
		return;
	}

	HAL.print("Initialize Camera device... ");
	updateDisplay(DisplayTiming::CAMERA_INIT_BEGIN);

	if (param_manager.getParameter("CAMERA", "ENABLE", DEFAULT_FALSE) == false) {
		HAL.println("Skip (ignore)!");
		updateDisplay(DisplayTiming::CAMERA_INIT_DONE);
		return;
	}

	bool result = HAL.Camera.begin();

	if (not result) {
		HAL.println("Skip (failed)!");
		updateDisplay(DisplayTiming::CAMERA_INIT_FAILED);
		HAL.delay(10000);

		HAL.reboot();
		return;
	}

	Sensor camera;
	camera.type            = TYPE_CAMERA;
	camera.name            = HAL.Camera.getSensorName() + "_" + HAL.Camera.getFrameSizeName();
	camera.unit            = "jpg";
	camera.enable          = true;
	camera.display         = false;
	camera.updated         = false;
	camera.tx_buffered     = false;
	camera.shift_level     = 0;
	camera.port            = 0;
	camera.CAMERA.captured = false;

	sensors.push_back(camera);

	updateDisplay(DisplayTiming::CAMERA_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief Modbus 機能の初期化を行います
void InitializeModbus() {
	HAL.print("Initialize Modbus ... ");
	updateDisplay(DisplayTiming::MODBUS_INIT_PERIPHERAL_BEGIN);

	std::vector<uint8_t> ports = HAL.Modbus.getPorts();
	String modifier            = HAL.Modbus.getModifier(); // Get header name

	if (ports.size() == 0) {
		updateDisplay(DisplayTiming::MODBUS_INIT_PERIPHERAL_DONE);
		HAL.println("Skip (No port available)!");
		return;
	}

	HAL.println("");
	for (const uint8_t &port : ports) {
		updateDisplay(DisplayTiming::MODBUS_INIT_PERIPHERAL_TARGET, port);
		HAL.printf("    Port%d : ", port);
		String key          = modifier + String(port);
		uint32_t baudrate   = param_manager.getParameter<uint32_t>(key.c_str(), "BAUDRATE");
		uint8_t data_bit    = param_manager.getParameter<uint8_t>(key.c_str(), "DATA_BIT");
		uint8_t parity      = param_manager.getParameter<uint8_t>(key.c_str(), "PARITY");
		uint32_t timeout_ms = param_manager.getParameter<uint32_t>(key.c_str(), "TIMEOUT_MS");

		bool result = HAL.Modbus.begin(port, baudrate, data_bit, parity, timeout_ms);
		HAL.printf("%s\n", ((result == true) ? "Success!" : "Failed"));
	}

	updateDisplay(DisplayTiming::MODBUS_INIT_PERIPHERAL_DONE);
	HAL.println("Done!");
}

//
//! @brief Modbus デバイスの初期化を行います
void InitializeModbusDevice() {
	bool modbus_enable         = false;
	std::vector<uint8_t> ports = HAL.Modbus.getPorts();
	if (ports.size() == 0) {
		return;
	}
	String modifier = HAL.Modbus.getModifier(); // Get header name

	HAL.println("Initialize Modbus device... ");
	updateDisplay(DisplayTiming::MODBUS_INIT_DEVICE_BEGIN);

	modbus_device.reset(new Modbus_Manager());
	modbus_device->clearPorts();
	modbus_device->clearDevices();
	for (const uint8_t &port : ports) {
		if (HAL.Modbus.getSerial(port) == nullptr) {
			continue;
		}
		String key     = modifier + String(port);
		uint8_t id_min = param_manager.getParameter<uint8_t>(key.c_str(), "ID_MIN");
		uint8_t id_max = param_manager.getParameter<uint8_t>(key.c_str(), "ID_MAX");

		modbus_device->addPort(port);
		modbus_device->scanDevice(port, id_min, id_max);
	}

	modbus_device->displayScanResult();
	auto act_dev = modbus_device->getIdList();

	for (int i = 0; i < act_dev.size(); i++) {
		updateDisplay(DisplayTiming::MODBUS_INIT_DEVICE_TARGET,
		              act_dev[i].port,
		              act_dev[i].id,
		              i + 1,
		              act_dev.size());

		HAL.printf("    Init ID: %d (Port: %d)... ", act_dev[i].id, act_dev[i].port);

		int8_t res = modbus_device->initializeDevice(act_dev[i].port, act_dev[i].id);
		if (res == MODBUS_IGNORE_DEVICE) {
			HAL.println("Skip (ignore)!");
			continue;
		}

		for (int k = 0; k < res; k++) {
			Sensor modbus_sensor;

			modbus_sensor.type = TYPE_MODBUS;
			modbus_sensor.name = modbus_device->getDeviceName(
			    act_dev[i].port,
			    act_dev[i].id,
			    k);
			modbus_sensor.unit = modbus_device->getDeviceUnit(
			    act_dev[i].port,
			    act_dev[i].id,
			    k);
			modbus_sensor.enable         = true;
			modbus_sensor.display        = false;
			modbus_sensor.updated        = false;
			modbus_sensor.tx_buffered    = false;
			modbus_sensor.value_changed  = false;
			modbus_sensor.i_value.latest = 0;
			modbus_sensor.i_value.prev   = 0;
			modbus_sensor.d_value.latest = 0.0;
			modbus_sensor.d_value.prev   = 0.0;
			modbus_sensor.s_value.latest = "";
			modbus_sensor.s_value.prev   = "";
			modbus_sensor.tx_on_change   = false;
			modbus_sensor.shift_level    = 0;
			modbus_sensor.port           = act_dev[i].port;
			modbus_sensor.MODBUS.id      = act_dev[i].id;
			modbus_sensor.MODBUS.result  = MODBUS_RES_FAILED;
			modbus_sensor.MODBUS.sub_id  = k;

			sensors.push_back(modbus_sensor);
		}
		modbus_enable = true;
		HAL.println("Done!");
	}

	updateDisplay(DisplayTiming::MODBUS_INIT_DEVICE_DONE);

	HAL.println("Done!");

	if (modbus_enable == true) {
		modbus_device->displayScanResult();
	} else {
		modbus_device.reset();
	}
}

//
//! @brief タイマ割込み機能の初期化を行います
void InitializeTimerInterrupt() {
	HAL.print("Initialize Timer Interrupt... ");
	updateDisplay(DisplayTiming::TI_INIT_BEGIN);

	uint32_t limit;
	uint64_t ms_sampling;
	uint64_t sec_sampling;
	constexpr uint8_t MINIMUM_INTERVAL_MS = 100; // Minimum 100 ms

	// Global interval 1 sec
	HAL.Timer1.begin(1 * 1000);

	uint64_t tx_interval_sec = param_manager.getParameter<uint32_t>("PARAM", "TX_INTERVAL_SEC");
	tx_interval_sec          = (tx_interval_sec < 1) ? 1 : tx_interval_sec;
	// HAL.Timer1.addSubTimer("transmit", tx_interval_sec, tx_interval_sec);
	HAL.Info.TX_INTERVAL_SEC = tx_interval_sec;

	HAL.Timer1.addSubTimer("wake_battery", 30, 0);
	HAL.Timer1.addSubTimer("half_minutes", 30, 30);
	HAL.Timer1.addSubTimer("one_minutes", 60, 60);

	HAL.Timer1.addSubTimer("one_seconds_in_transmit", 1, 1);
	HAL.Timer1.addSubTimer("one_minutes_in_transmit", 60, 60);

	/* Camera ========================================================================*/

	// Basic timer 100 ms
	HAL.Timer2.begin(MINIMUM_INTERVAL_MS);

	/* GPIO ==========================================================================*/
	if (HAL.Gpio.getPins().size() > 0) {
		// If GPIO Enable
		ms_sampling = param_manager.getParameter<uint32_t>("SAMPLING", "GPIO_MS");
		ms_sampling = (ms_sampling < MINIMUM_INTERVAL_MS) ? MINIMUM_INTERVAL_MS : ms_sampling; // Minimum 1ms
		if ((tx_interval_sec * 1000) < ms_sampling) {
			ms_sampling = tx_interval_sec * 1000;
		}
		limit = ms_sampling / MINIMUM_INTERVAL_MS;
		// HAL.printf("[DEBUG] GPIO interval_ms: %d \n", ms_sampling);
		// HAL.printf("[DEBUG] GPIO limit      : %d \n", limit);
		HAL.Timer2.addSubTimer("gpio", limit, limit);
	}
	/*================================================================================*/

	/* I2C ===========================================================================*/
	ms_sampling = param_manager.getParameter<uint32_t>("SAMPLING", "I2C_MS");
	ms_sampling = (ms_sampling < MINIMUM_INTERVAL_MS) ? MINIMUM_INTERVAL_MS : ms_sampling; // Minimum 1ms
	if ((tx_interval_sec * 1000) < ms_sampling) {
		ms_sampling = tx_interval_sec * 1000;
	}
	limit = ms_sampling / MINIMUM_INTERVAL_MS;
	// HAL.printf("[DEBUG] GPIO interval_ms: %d \n", ms_sampling);
	// HAL.printf("[DEBUG] GPIO limit      : %d \n", limit);
	HAL.Timer2.addSubTimer("i2c", limit, limit);
	/*================================================================================*/

	/* Modbus=========================================================================*/
	if (HAL.Modbus.getPorts().size() > 0) {
		ms_sampling = param_manager.getParameter<uint32_t>("SAMPLING", "MODBUS_MS");
		ms_sampling = (ms_sampling < MINIMUM_INTERVAL_MS) ? MINIMUM_INTERVAL_MS : ms_sampling; // Minimum 1ms
		if ((tx_interval_sec * 1000) < ms_sampling) {
			ms_sampling = tx_interval_sec * 1000;
		}
		limit = ms_sampling / MINIMUM_INTERVAL_MS;
		// HAL.printf("[DEBUG] GPIO interval_ms: %d \n", ms_sampling);
		// HAL.printf("[DEBUG] GPIO limit      : %d \n", limit);
		HAL.Timer2.addSubTimer("modbus", limit, limit);
	}
	/*================================================================================*/

	/* Camera ========================================================================*/
	if (HAL.Camera.isInstalled() and HAL.Camera.isReady()) {
		ms_sampling = param_manager.getParameter<uint32_t>("SAMPLING", "CAMERA_MS");
		ms_sampling = (ms_sampling < MINIMUM_INTERVAL_MS) ? MINIMUM_INTERVAL_MS : ms_sampling; // Minimum 1ms
		if ((tx_interval_sec * 1000) < ms_sampling) {
			ms_sampling = tx_interval_sec * 1000;
		}
		limit = ms_sampling / MINIMUM_INTERVAL_MS;
		// HAL.printf("[DEBUG] CAMERA interval_ms: %d \n", ms_sampling);
		// HAL.printf("[DEBUG] CAMERA limit      : %d \n", limit);
		HAL.Timer2.addSubTimer("camera", limit, limit);
	}
	/*================================================================================*/

	updateDisplay(DisplayTiming::TI_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief 有線 LAN 機能の初期化を行います
void InitializeWiredLan() {
	HAL.print("Initialize Wired Lan... ");

	if (HAL.Lan.isSupported() == false) {
		HAL.println("Skip (Not supported)");
		return;
	}

	updateDisplay(DisplayTiming::ETH_INIT_BEGIN);

	const char *static_ip      = param_manager.getParameter<const char *>("LAN", "STATIC_IP");
	const char *static_gateway = param_manager.getParameter<const char *>("LAN", "STATIC_GATEWAY");
	const char *static_subnet  = param_manager.getParameter<const char *>("LAN", "STATIC_SUBNET_MASK");
	const char *dns            = param_manager.getParameter<const char *>("LAN", "DNS");

	if (HAL.Lan.begin(static_ip, static_gateway, static_subnet, dns)) {
		HAL.printf("\n    Ethernet ready.\n");
		HAL.printf("    Status: %s\n", (HAL.Lan.isLinkUp() == true) ? "Link Up" : "Link Down");

		HAL.setUseLinkLayer(DATALINK_LAYER::ETHERNET);
		updateDisplay(DisplayTiming::ETH_INIT_SUCCESS);
		HAL.println("DONE!");
	} else {
		HAL.println("Failed!");
		updateDisplay(DisplayTiming::ETH_INIT_FAILED);

		HAL.delay(2000);
	}
}

//
//! @brief 無線 LAN 機能の初期化を行います
void InitializeWirelessLan() {
	HAL.print("Initialize Wireless Lan... \n");
	if (param_manager.checkParameter("WIFI", "SSID") == false or
	    param_manager.checkParameter("WIFI", "PASS") == false or
	    param_manager.checkParameter("WIFI", "STATIC_IP") == false or
	    param_manager.checkParameter("WIFI", "STATIC_GATEWAY") == false or
	    param_manager.checkParameter("WIFI", "STATIC_SUBNET_MASK") == false or
	    param_manager.checkParameter("WIFI", "DNS") == false) {
		// setting file might be broken
		updateDisplay(DisplayTiming::WIFI_CONNECT_FAILED);
		HAL.println("Failed!");
		HAL.println("WiFi setting might be broken.");
		HAL.delay(10000);

		HAL.reboot();
		return;
	}

	updateDisplay(DisplayTiming::WIFI_SEARCH_TARGET);

	const char *ssid           = param_manager.getParameter<const char *>("WIFI", "SSID");
	const char *pass           = param_manager.getParameter<const char *>("WIFI", "PASS");
	const char *static_ip      = param_manager.getParameter<const char *>("WIFI", "STATIC_IP");
	const char *static_gateway = param_manager.getParameter<const char *>("WIFI", "STATIC_GATEWAY");
	const char *static_subnet  = param_manager.getParameter<const char *>("WIFI", "STATIC_SUBNET_MASK");
	const char *dns1           = param_manager.getParameter<const char *>("WIFI", "DNS");
	const char *dns2           = ""; // param_manager.getParameter<const char *>("WIFI", "DNS2");

	if (HAL.Wlan.config(static_ip, static_gateway, static_subnet, dns1, dns2) == false) {
		// TODO: Show to LCD!!!
		HAL.println("[Info] Use DHCP mode...");
	} else {
		HAL.println("[Info] Use static IP mode...");
	}

	int32_t _rssi;
	bool _exist_same_name;
	bool detected_ssid = HAL.Wlan.detectTargetAccessPoint(ssid, &_rssi, &_exist_same_name);
	if (detected_ssid) {
		updateDisplay(DisplayTiming::WIFI_TARGET_DETECTED, ssid);
	} else {
		updateDisplay(DisplayTiming::WIFI_TARGET_NOT_FOUND, ssid);
	}
	HAL.delay(2000);

	updateDisplay(DisplayTiming::WIFI_TRY_CONNECT, ssid);
	if (not HAL.Wlan.connectWlan(ssid, pass, 15000)) {
		updateDisplay(DisplayTiming::WIFI_CONNECT_FAILED);

		HAL.println("Failed!");
		HAL.Wlan.disconnectWlan();
		HAL.println("Device will restart after 10 sec");
		HAL.delay(10000);

		HAL.reboot();
	}

	HAL.Info.WIFI_SSID = String(ssid);
	HAL.setUseLinkLayer(DATALINK_LAYER::WIFI);
	updateDisplay(DisplayTiming::WIFI_CONNECT_SUCCESS);
	HAL.println("Done!");
}

//
//! @brief NTP 機能の初期化を行います
void InitializeNTP() {
	HAL.print("Initialize NTP... ");

	String key = "";
	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::ETHERNET:
		key = "LAN";
		break;
	case DATALINK_LAYER::WIFI:
		key = "WIFI";
		break;
	default:
		key = "UNKNOWN";
	}

	if (param_manager.checkParameter(key.c_str(), "NTP_SERVER") == false) {
		// setting file might be broken
		updateDisplay(DisplayTiming::NTP_SYNC_FAILED);
		HAL.println("Failed!");
		HAL.printf("Network (%s) setting might be broken.\n", key.c_str());
		HAL.delay(10000);

		HAL.reboot();
		return;
	}

	updateDisplay(DisplayTiming::NTP_INIT_BEGIN);

	const char *server1 = param_manager.getParameter<const char *>(key.c_str(), "NTP_SERVER");
	HAL.NtpRtc.addNtpServer(server1);

	if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
		updateDisplay(DisplayTiming::NTP_CONNECT_WIFI);
		while (not HAL.Wlan.connectWlan()) {
			// led_blue.toggle();
			HAL.println("WiFi error");
			HAL.println("Retry after 1sec.");
			HAL.delay(1000);
		}
	}

	updateDisplay(DisplayTiming::NTP_SYNC_START);

	bool res = false;
	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::ETHERNET:
		res = HAL.NtpRtc.begin();
		break;
	case DATALINK_LAYER::WIFI:
		res = HAL.NtpRtc.begin();
		break;
	default:
		res = HAL.NtpRtc.begin();
		break;
	}

	if (not res) {
		updateDisplay(DisplayTiming::NTP_SYNC_FAILED);

		HAL.println("Failed!");
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
		HAL.println("Device will restart after 10 sec");
		HAL.delay(10000);

		HAL.reboot();
	}

	updateDisplay(DisplayTiming::NTP_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief Ambient 機能の初期化を行います
void InitializeAmbient() {
	HAL.print("Initialize Ambient... ");
	updateDisplay(DisplayTiming::AMBIENT_INIT_BEGIN);

	tx_ambient.reset(new IoTTxAmbient());
	tx_ambient->begin();
	// パラメータの読み取り可否は features で調査済み（けど、再チェックした方がいいかも？）
	const char *write_key   = param_manager.getParameter<const char *>("AMBIENT", "WRITE_KEY");
	unsigned int channel_id = param_manager.getParameter<unsigned int>("AMBIENT", "CHANNEL_ID");
	tx_ambient->setUniqueKey(write_key, channel_id);

	const char *d1 = param_manager.getParameter<const char *>("AMBIENT", "D1_SENSOR_NAME");
	const char *d2 = param_manager.getParameter<const char *>("AMBIENT", "D2_SENSOR_NAME");
	const char *d3 = param_manager.getParameter<const char *>("AMBIENT", "D3_SENSOR_NAME");
	const char *d4 = param_manager.getParameter<const char *>("AMBIENT", "D4_SENSOR_NAME");
	std::map<String, uint8_t> list;
	list[String(d1)] = 1;
	list[String(d2)] = 2;
	list[String(d3)] = 3;
	list[String(d4)] = 4;

	HAL.println("");
	for (int i = 0; i < sensors.size(); i++) {
		auto n = list.count(sensors[i].name);
		if (n == 0) {
			// No mach target
			HAL.printf("\t[Ignore] %s\n", sensors[i].name.c_str());
			continue;
		}
		HAL.printf("\t[Add   ] %s\n", sensors[i].name.c_str());
		tx_ambient->setDataFilter(list[sensors[i].name], sensors[i].name);
	}

	if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
		updateDisplay(DisplayTiming::AMBIENT_CONNECT_WIFI);
		while (not HAL.Wlan.connectWlan()) {
			// led_blue.toggle();

			HAL.println("WiFi error");
			HAL.println("Retry after 1sec.");
			HAL.delay(1000);
		}
	}

	// led_blue.off();
	updateDisplay(DisplayTiming::AMBIENT_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief FTP 機能の初期化を行います
void InitializeFTP() {
	HAL.print("Initialize FTP... ");
	updateDisplay(DisplayTiming::FTP_INIT_BEGIN);

	tx_ftp.reset(new IoTTxFTP());
	tx_ftp->begin();
	bool add_unix_time_field = true;
	bool nano_precision      = true;
	tx_ftp->configUnixTimeField(add_unix_time_field, nano_precision);
	// パラメータの読み取り可否は features で調査済み（けど、再チェックした方がいいかも？）
	const char *ftp_server = param_manager.getParameter<const char *>("FTP", "SERVER");
	const char *ftp_user   = param_manager.getParameter<const char *>("FTP", "USER");
	const char *ftp_pass   = param_manager.getParameter<const char *>("FTP", "PASS");

	tx_ftp->setFtpParam(ftp_server, ftp_user, ftp_pass, 1);

	String ftp_dir = param_manager.getParameter<const char *>("FTP", "DIRECTORY");
	if (not ftp_dir.endsWith("/")) {
		ftp_dir += "/";
	}

	const char *devID = param_manager.getParameter<const char *>("PARAM", "devID");
	tx_ftp->setFilePath(ftp_dir.c_str(), String(devID) + ".csv");
	tx_ftp->setHeartbeatFilePath(ftp_dir.c_str(), "final_time_" + String(devID) + ".txt");
	tx_ftp->setUniqueParam(devID);
	tx_ftp->enableAutoDisconnectWiFi(false);

	if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
		updateDisplay(DisplayTiming::FTP_CONNECT_WIFI);
		while (not HAL.Wlan.connectWlan()) {
			// led_blue.toggle();

			HAL.println("WiFi error");
			HAL.println("Retry after 1sec.");
			HAL.delay(1000);
		}
	}
	HAL.Info.FTP_SERVER = String(ftp_server) + String(ftp_dir);

	// led_blue.off();
	updateDisplay(DisplayTiming::FTP_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief HTTP 機能の初期化を行います
void InitializeHTTP() {
	HAL.print("Initialize HTTP... ");
	updateDisplay(DisplayTiming::HTTP_INIT_BEGIN);

	// パラメータの読み取り可否は features で調査済み（けど、再チェックした方がいいかも？）
	const char *devID = param_manager.getParameter<const char *>("PARAM", "devID");

	tx_http.reset(new IoTTxHTTP());
	tx_http->begin();
	tx_http->setUniqueParam(devID);
	const char *url = param_manager.getParameter<const char *>("HTTP", "URL");
	tx_http->setUrl(url);
	tx_http->enableAutoDisconnectWiFi(false);

	if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
		updateDisplay(DisplayTiming::HTTP_CONNECT_WIFI);
		while (not HAL.Wlan.connectWlan()) {
			// led_blue.toggle();

			HAL.println("WiFi error");
			HAL.println("Retry after 1sec.");
			HAL.delay(1000);
		}
	}
	HAL.Info.HTTP_SERVER = String(url);

	// led_blue.off();
	updateDisplay(DisplayTiming::HTTP_INIT_DONE);
	HAL.println("Done!");
}

//
//! @brief MQTT 機能の初期化を行います
void InitializeMQTT() {
	HAL.print("Initialize MQTT... ");
	updateDisplay(DisplayTiming::MQTT_INIT_BEGIN);

	tx_mqtt.reset(new IoTTxMQTT());
	tx_mqtt->begin();
	bool add_unix_time_field = true;
	bool nano_precision      = true;
	tx_mqtt->configUnixTimeField(add_unix_time_field, nano_precision);

	// パラメータの読み取り可否は features で調査済み（けど、再チェックした方がいいかも？）
	const char *devID = param_manager.getParameter<const char *>("PARAM", "devID");

	tx_mqtt->setUniqueParam(devID);

	const char *broker      = param_manager.getParameter<const char *>("MQTT", "BROKER");
	uint16_t port           = param_manager.getParameter<uint16_t>("MQTT", "PORT");
	const char *broker_user = param_manager.getParameter<const char *>("MQTT", "USER");
	const char *broker_pass = param_manager.getParameter<const char *>("MQTT", "PASS");
	const char *base_topic  = param_manager.getParameter<const char *>("MQTT", "BASE_TOPIC");
	bool retain             = param_manager.getParameter("MQTT", "RETAIN", DEFAULT_FALSE);
	tx_mqtt->setMqttParam(broker, port, base_topic, broker_user, broker_pass, retain);
	tx_mqtt->enableAutoDisconnectWiFi(false);

	if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
		updateDisplay(DisplayTiming::MQTT_CONNECT_WIFI);
		while (not HAL.Wlan.connectWlan()) {
			// led_blue.toggle();

			HAL.println("WiFi error");
			HAL.println("Retry after 1sec.");
			HAL.delay(1000);
		}
	}
	HAL.Info.MQTT_SERVER = String(broker) + " : " + String(port);

	// led_blue.off();
	updateDisplay(DisplayTiming::MQTT_INIT_DONE);
	HAL.println("Done!");
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Sub Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// ==============================================================
//! @brief 使用機能の読み取りを行います
//! @return 機能の有効/無効を格納した構造体を返す
// =============================================================
struct Features ReadFeatures() {
	struct Features feat;

	feat.wake_bat_enable   = param_manager.getParameter("PARAM", "WAKE_BAT", DEFAULT_FALSE);
	feat.camera_enable     = param_manager.getParameter("CAMERA", "ENABLE", DEFAULT_FALSE);
	feat.ambient_enable    = param_manager.getParameter("AMBIENT", "ENABLE", DEFAULT_FALSE);
	feat.ftp_enable        = param_manager.getParameter("FTP", "ENABLE", DEFAULT_FALSE);
	feat.http_enable       = param_manager.getParameter("HTTP", "ENABLE", DEFAULT_FALSE);
	feat.mqtt_enable       = param_manager.getParameter("MQTT", "ENABLE", DEFAULT_FALSE);
	feat.batch_send_enable = param_manager.getParameter("PARAM", "BATCH_DATA_SEND", DEFAULT_FALSE);

	feat.offline_mode = not(feat.ambient_enable or feat.ftp_enable or feat.http_enable or feat.mqtt_enable);

	feat.wait_enable = param_manager.getParameter("PARAM", "WAIT_00_SEC", DEFAULT_FALSE);

	constexpr uint8_t THRESHOLD_SEC = 30;
	feat.tx_random_wait_enable      = false;
	if (feat.wait_enable and HAL.Timer1.getSubTimerLimit("transmit") > THRESHOLD_SEC) {
		feat.tx_random_wait_enable = true;
	}

	return feat;
}

//
//! @brief 00秒待機処理
void WaitStartTiming() {
	HAL.println("Wait timing 00 second...");
	struct tm timeinfo = {0};
	if (not HAL.NtpRtc.getCurrentTime(&timeinfo)) {
		// TODO: Error handling
	}

	while (timeinfo.tm_sec != 0) {
		updateDisplay(DisplayTiming::WAIT_START_TIMING, timeinfo.tm_sec);
		HAL.delay(100);
		if (not HAL.NtpRtc.getCurrentTime(&timeinfo)) {
			// TODO: Error handling
		}
		HAL.update();
	}
}

//
//! @brief 内部設定ファイルの更新を行います
//! @see InitializeSettingParameter
//! @note 設定アプリケーションからのデータ受信完了時に呼び出されます。
// 該当するパラメータだけ更新し、更新後の設定ファイルを返信します。
void UpdateSettings() {
	// HAL.printf("esp_get_free_heap_size(): %6d\n", esp_get_free_heap_size());

	// uint8_t za_data[ZIGBEE_API_MAX_FRAME_SIZE] = {0};
	std::vector<uint8_t> za_data(ZIGBEE_API_MAX_FRAME_SIZE, 0);
	uint16_t za_data_len = 0;
	sbt->getRecvData(za_data.data(), &za_data_len);

	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, za_data.data());
	if (error) {
		// HAL.print(F("deserializeJson() failed: "));
		// HAL.println(error.f_str());
		String err = "{\"ERROR\":{\"MESSAGE\":\"" + String(error.f_str()) + "\"}}";
		sbt->send(err.c_str(), err.length());
		return;
	}

#ifdef PRINT_RECV_PACKET
	String output = "";
	HAL.println("===== Recv JSON =====");
	serializeJsonPretty(doc, output);
	HAL.println(output.c_str());
	output = "";
#endif

	auto root = param_manager.getSettingJson();

#ifdef PRINT_RECV_PACKET
	HAL.println("===== Old Settings =====");
	serializeJsonPretty(root, output);
	HAL.println(output.c_str());
	output = "";
#endif
	for (auto section : doc.as<JsonObject>()) {
		for (auto sub : doc[section.key().c_str()].as<JsonObject>()) {
			JsonVariant error = root[section.key().c_str()][sub.key().c_str()];
			if (not error.isNull()) {
				// HAL.print(root[section.key().c_str()][sub.key().c_str()].as<const char *>());
				// HAL.print("  ");
				// HAL.print(section.key().c_str());
				// HAL.print("  ");
				// HAL.println(sub.key().c_str());
				root[section.key().c_str()][sub.key().c_str()] = doc[section.key().c_str()][sub.key().c_str()];
			}
		}
	}

	HAL.delay(1);
	param_manager.setSettingTemplate(root);
	param_manager.reload();

	String json;
	serializeJson(root, json);
#ifdef PRINT_RECV_PACKET
	HAL.println("===== New Settings =====");
	serializeJsonPretty(root, output);
	HAL.println(output.c_str());
	output = "";
#endif
	HAL.delay(1);
	sbt->send(json.c_str(), json.length());
	root.clear();
	// root.garbageCollect();
	//  root.~BasicJsonDocument();
}