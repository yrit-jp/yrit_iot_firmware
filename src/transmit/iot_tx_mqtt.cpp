#include "iot_tx_mqtt.h"

#ifndef HAL_MQTT_MAX_PACKET_SIZE
#define HAL_MQTT_MAX_PACKET_SIZE (1024)
#endif

void IoTTxMQTT::setMqttParam(String broker, uint16_t port, String base_topic, String broker_user, String broker_pass, bool retain) {
	_mutex_key    = "iot_tx_mqtt";
	_DEBUG_PREFIX = "MQTT";

	if (broker.endsWith(".local")) {
		// Resolve mDNS address
		MyIPAddress ip;
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			ip = HAL.Wlan.resolveMDNS(broker);
		} else if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			ip = HAL.Lan.resolveMDNS(broker);
		}
		if (ip.toString() == "0.0.0.0") {
			// 解決に失敗した場合は 0.0.0.0 で固定してしまわないよう、
			// 元のホスト名のまま保持して以降の接続処理で再解決を試みられるようにする
			debugMessage(LogCategory::MQTT, LogLevel::Error, "Failed to resolve mDNS");
			_broker = broker;
		} else {
			debugMessage(LogCategory::MQTT, LogLevel::Info, ("mDNS Resolve: " + broker + " -> " + ip.toString()).c_str());
			_broker = ip.toString();
		}
	} else {
		_broker = broker;
	}

	_port        = port;
	_base_topic  = base_topic;
	_broker_user = broker_user;
	_broker_pass = broker_pass;
	_retain      = retain;

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		_client.setSocketTimeout(8);
		_client.setClient(*(HAL.Wlan.getClient("mqtt")));
		break;
	case DATALINK_LAYER::ETHERNET:
		_client.setClient(*(HAL.Lan.getClient("mqtt")));
		break;
	default:
		break;
	}

	_client.setServer(_broker.c_str(), _port);
	bool result = _client.setBufferSize(HAL_MQTT_MAX_PACKET_SIZE);
	if (result == false) {
		debugMessage(LogCategory::MQTT, LogLevel::Error, "Failed to set buffer.");
	}
}

void IoTTxMQTT::clientLoop() {
	/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
	EthernetMutexGuard _;
	if (_client.connected()) {
		_client.loop();
	}
	/* -+-+-+-+- Mutex block END -+-+-+-+- */
}

IoTTx_Tx_Result IoTTxMQTT::update() {
	clientLoop();
	IoTTx_Tx_Result result = IoTTx::update();
	clientLoop();
	return result;
}

IoTTx_Tx_Result IoTTxMQTT::updateEx() {
	clientLoop();
	IoTTx_Tx_Result result = IoTTx::updateEx();
	clientLoop();
	return result;
}

bool IoTTxMQTT::connectBroker() {
	bool is_connected = false;
	bool result       = true;
	String clientID   = HAL.getChipUUID();

	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		is_connected = _client.connected();
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (is_connected == false) {
		debugMessage(LogCategory::MQTT, LogLevel::Info, "Start try to connect Broker...");
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		if (is_connected == false) {
			_client.connect(clientID.c_str(), _broker_user.c_str(), _broker_pass.c_str());
			result = _client.connected();
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	} else {
		return true;
	}

	if (result == true) {
		debugMessage(LogCategory::MQTT, LogLevel::Info, "Broker connected!");
		debugMessage(LogCategory::MQTT, LogLevel::Info, ("ID: " + clientID).c_str());
	} else {
		debugMessage(LogCategory::MQTT, LogLevel::Error, "Broker connection failed.");
		debugMessage(LogCategory::MQTT, LogLevel::Error, ("Status: " + String(status())).c_str());
		_client.disconnect();
	}

	return result;
}

int IoTTxMQTT::status() {
	int state;
	/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
	EthernetMutexGuard _;
	state = _client.state();
	/* -+-+-+-+- Mutex block END -+-+-+-+- */
	return state;
}

bool IoTTxMQTT::transmitHeartbeat() {
	if (_penalty_wait.isActive()) {
		return false;
	}

	if (not _heartbeat_store) {
		return false;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan(15000)) {
			_penalty_wait.activate(5000);
			return false;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return false;
		}
		break;
	default:
		break;
	}

	if (not connectBroker()) {
		_penalty_wait.activate(5000);
		return false;
	}

	JsonDocument payload;
	payload["devID"]    = _devID;
	payload["sensName"] = "Heartbeat";

	String topic   = "Heartbeat/" + makeTypicalTopic();
	char s_day[64] = {'\0'};
	strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_heartbeat_tm);
	payload["day"] = s_day;

	payload["uptimeSec"]    = millis() / 1000;
	payload["freeHeapByte"] = HAL.getFreeHeapSize();
	payload["minHeapByte"]  = HAL.getMinimumFreeHeapSize();

	char buffer[256] = {'\0'};
	serializeJson(payload, buffer, sizeof(buffer));

	constexpr bool RETAIN = true;
	int success;
	{
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		EthernetMutexGuard _;
		success = _client.publish(topic.c_str(), buffer, RETAIN);
		/* -+-+-+-+- Mutex block END -+-+-+-+- */
	}

	if (success) {
		// success to transmit
		_heartbeat_store = false;
	} else {
		debugMessage(LogCategory::MQTT_HB, LogLevel::Warning, "Failed to write heartbeat", "\n");
		_penalty_wait.activate(1000);

		clientLoop();
		return false;
	}

	clientLoop();
	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}
	return true;
}

IoTTx_Tx_Result IoTTxMQTT::transmit() {
	enum IoTTx_Tx_Result result = RESULT_TX_ERROR;

	if (_penalty_wait.isActive()) {
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan(15000)) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	default:
		break;
	}

	if (not connectBroker()) {
		_penalty_wait.activate(5000);
		return result;
	}

	String _topic = makeTypicalTopic();
	while (_tx_idx.store != _tx_idx.transmit) {
		clientLoop();

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		JsonDocument payload;
		payload["devID"]    = _devID;
		payload["sensName"] = _buffer[_tx_idx.transmit].name;
		payload["unit"]     = _buffer[_tx_idx.transmit].unit;

		String topic   = _topic + _buffer[_tx_idx.transmit].name;
		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_buffer[_tx_idx.transmit].time);
		payload["day"] = s_day;

		auto type = estimateType(_buffer[_tx_idx.transmit].value);
		switch (type) {
		case TYPE_INTEGER:
			// TODO: std::strtol への変更を検討
			payload["value"] = _buffer[_tx_idx.transmit].value.toInt();
			break;
		case TYPE_DOUBLE:
			payload["value"] = std::strtod(_buffer[_tx_idx.transmit].value.c_str(), nullptr);
			break;
		default:
			payload["value"] = _buffer[_tx_idx.transmit].value;
			break;
		}

		if (_unix_time_field.enable) {
			constexpr bool nano_precision = true;
			payload["time"]               = convertToUnixTime(_buffer[_tx_idx.transmit].time, nano_precision);
		}
		HAL.giveSemaphoreMutex(_mutex_key);

		char buffer[256] = {'\0'};
		serializeJson(payload, buffer, sizeof(buffer));

		int success;
		{
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			EthernetMutexGuard _;
			success = _client.publish(topic.c_str(), buffer, _retain);
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}

		if (success) {
			// success to transmit
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			HAL.giveSemaphoreMutex(_mutex_key);
			result = RESULT_TX_SOME_DATA;
			// printIndex();
		} else {
			debugMessage(LogCategory::MQTT, LogLevel::Error, "Failed to publish.");
			clientLoop();
			if (_auto_disconnect) {
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
					HAL.Wlan.disconnectWlan();
				}
			}
			_penalty_wait.activate(1000);
			return result;
		}
	}

	clientLoop();
	result = RESULT_TX_ALL_DATA;
	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}

	return result;
}

IoTTx_Tx_Result IoTTxMQTT::transmitEx() {
#if HAL_ENABLE_EXCEPTIONAL_BUFFER
	enum IoTTx_Tx_Result result = RESULT_TX_ERROR;

	if (_penalty_wait.isActive()) {
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan(15000)) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return result;
		}
		break;
	default:
		break;
	}

	if (not connectBroker()) {
		_penalty_wait.activate(5000);
		return result;
	}

	String _topic = makeTypicalTopic();
	// for exceptional storage
	for (int storage = 0; storage < _EXCEPTIONAL_STORAGE_NUM; storage++) {
		clientLoop();
		if (_ex_buffer_info[storage].updated == false) {
			continue;
		}

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		JsonDocument payload_ex;
		payload_ex["devID"]    = _devID;
		payload_ex["sensName"] = _ex_buffer_info[storage].name;
		payload_ex["unit"]     = _ex_buffer_info[storage].unit;

		String topic   = _topic + _ex_buffer_info[storage].name;
		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_ex_buffer_info[storage].time);
		payload_ex["day"] = s_day;

		if (storage == 0) {
			// For storage number 0
			topic = "Camera/" + topic;

			static char *tmp_arr       = nullptr;
			static size_t tmp_arr_size = 0;

			// サイズ計算とエンコードの間に camera バッファが差し替わらないよう、
			// 一括でロックを保持する（途中でロックを解放しない）
			HAL.takeSemaphoreMutex("camera", portMAX_DELAY);
			unsigned int len = base64::encodeLength(HAL.Camera.jpgImage.size());

			if (tmp_arr_size < len + 1) {
				delete[] tmp_arr;
				tmp_arr = new char[len + 1]();
				if (tmp_arr == nullptr) {
					// メモリ確保に失敗した
					HAL.giveSemaphoreMutex("camera");
					debugMessage(LogCategory::MQTT_EX, LogLevel::Error,
					             ("Failed alloc buffer. Size: " + String(len + 1)).c_str());
					HAL.giveSemaphoreMutex(_mutex_key);
					return result;
				}
				tmp_arr_size = len + 1;
			}
			memset(tmp_arr, 0, tmp_arr_size);

			base64::encode(&HAL.Camera.jpgImage[0], HAL.Camera.jpgImage.size(), tmp_arr);
			HAL.giveSemaphoreMutex("camera");

			auto value = payload_ex["value"].to<JsonObject>();
			if (copyArray(tmp_arr, value) == false) {
				// failed to copy
				debugMessage(LogCategory::MQTT_EX, LogLevel::Error,
				             ("Failed to copy image. Size: " + String(len)).c_str());
				HAL.giveSemaphoreMutex(_mutex_key);
				return result;
			}

		} else if (storage == 1) {
			// For storage number 1
			topic = "Batch/" + topic;
			payload_ex.remove("unit");

			HAL.takeSemaphoreMutex("ex_sensor_data", portMAX_DELAY);

			payload_ex["sensors"] = ex_sensor_data;

			HAL.giveSemaphoreMutex("ex_sensor_data");
		} else {
			HAL.giveSemaphoreMutex(_mutex_key);
			continue;
		}

		if (_unix_time_field.enable) {
			constexpr bool nano_precision = true;
			payload_ex["time"]            = convertToUnixTime(_ex_buffer_info[storage].time, nano_precision);
		}
		HAL.giveSemaphoreMutex(_mutex_key);

		static char *buffer = nullptr;
		if (buffer == nullptr) {
			buffer = new char[HAL_MQTT_MAX_PACKET_SIZE]();
			if (buffer == nullptr) {
				// メモリ確保の初期化に失敗した
				// 最初の１回だけ呼び出されるはずなので、再起動させエラーを知らせる
				HAL.reboot();
			}
		}
		memset(buffer, 0, HAL_MQTT_MAX_PACKET_SIZE);
		serializeJson(payload_ex, buffer, HAL_MQTT_MAX_PACKET_SIZE);
		// serializeJson(payload_ex, Serial);
		// HAL.println("");

		int success;
		{
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			EthernetMutexGuard _;
			success = _client.publish(topic.c_str(), buffer, _retain);
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}

		// delete[] buffer;
		if (success) {
			// success to transmit
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_ex_buffer_info[storage].updated = false;
			HAL.giveSemaphoreMutex(_mutex_key);
			result = RESULT_TX_SOME_DATA;
			// printIndex();
		} else {
			debugMessage(LogCategory::MQTT_EX, LogLevel::Error, "Failed to publish.");
			debugMessage(LogCategory::MQTT_EX, LogLevel::Info,
			             ("Bufsize: " + String(_client.getBufferSize())).c_str(), "\n");

			clientLoop();
			if (_auto_disconnect) {
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
					HAL.Wlan.disconnectWlan();
				}
			}
			_penalty_wait.activate(1000);
			return result;
		}
	}

	clientLoop();
	result = RESULT_TX_ALL_DATA;
	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}
	return result;
#else
	return RESULT_TX_NOTHING;
#endif /* HAL_ENABLE_EXCEPTIONAL_BUFFER */
}

String IoTTxMQTT::makeTypicalTopic() {
	String _topic;
	if (_base_topic != "") {
		if (_base_topic.endsWith("/")) {
			_topic = _base_topic + _devID + "/";
		} else {
			_topic = _base_topic + "/" + _devID + "/";
		}
	} else {
		_topic = _devID + "/";
	}
	return _topic;
}