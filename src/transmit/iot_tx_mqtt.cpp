#include "iot_tx_mqtt.h"

#ifndef HAL_MQTT_MAX_PACKET_SIZE
#define HAL_MQTT_MAX_PACKET_SIZE (1024)
#endif

void IoTTxMQTT::setMqttParam(String broker, uint16_t port, String base_topic, String broker_user, String broker_pass, bool retain) {
	if (broker.endsWith(".local")) {
		// Resolve mDNS address
		MyIPAddress ip;
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			ip = HAL.Wlan.resolveMDNS(broker);
		} else if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			ip = HAL.Lan.resolveMDNS(broker);
		}
		if (ip.toString() == "0.0.0.0") {
			debugMessage("MQTT", "Error", "Failed to resolve mDNS", "\n");
		} else {
			debugMessage("MQTT", "Success", ("mDNS Resolve: " + broker + " -> " + ip.toString()).c_str(), "\n");
		}
		_broker = ip.toString();
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
		_client.setSocketTimeout(4);
		_client.setClient(*(HAL.Wlan.getClient("mqtt")));
		break;
	case DATALINK_LAYER::ETHERNET:
		_client.setClient(*(HAL.Lan.getClient("mqtt")));
		break;
	default:
		break;
	}

	_client.setServer(_broker.c_str(), _port);
	_client.setBufferSize(HAL_MQTT_MAX_PACKET_SIZE);
}

void IoTTxMQTT::clientLoop() {
	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		if (_client.connected()) {
			_client.loop();
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}
}

bool IoTTxMQTT::connectBroker() {
	bool is_connected = false;
	bool result       = true;
	String clientID   = HAL.getChipUUID();

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		is_connected = _client.connected();

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (is_connected == false) {
		debugMessage("MQTT", "Info", "Start try to connect Broker...", "\n");
	} else {
		return true;
	}

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		if (is_connected == false) {
			_client.connect(clientID.c_str(), _broker_user.c_str(), _broker_pass.c_str());
			if (_client.connected()) {
				result = true;
			} else {
				result = false;
			}
		}

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (result == true) {
		debugMessage("MQTT", "Info", "Broker connected!", "\n");
		debugMessage("MQTT", "Info", ("ID: " + clientID).c_str(), "\n");
	} else {
		debugMessage("MQTT", "Error", "Broker connection failed.", "\n");
		debugMessage("MQTT", "Error", ("Status" + String(status())).c_str(), "\n");
		_client.disconnect();
	}

	return result;
}

int IoTTxMQTT::status() {
	int state;

	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		state = _client.state();

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	return state;
}

bool IoTTxMQTT::transmitHeartbeat() {
	static struct Penalty _penalty_wait = {false, 0, 0};

	if (_penalty_wait.enable) {
		if ((millis() - _penalty_wait.last_failed_millis) > _penalty_wait.wait_ms) {
			_penalty_wait.enable = false;
		}
		return false;
	}

	if (!_heartbeat_store) {
		return false;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (!HAL.Wlan.connectWlan()) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return false;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return false;
		}
		break;
	default:
		break;
	}

	if (!connectBroker()) {
		_penalty_wait.enable             = true;     // Enable wait
		_penalty_wait.last_failed_millis = millis(); // Update last failed time
		_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
		return false;
	}

	JsonDocument payload;
	payload["devID"]    = _devID;
	payload["sensName"] = "Heartbeat";

	String topic   = "Heartbeat/" + makeTypicalTopic();
	char s_day[64] = {'\0'};
	strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_heartbeat_tm);
	payload["day"] = s_day;

	char buffer[128] = {'\0'};
	serializeJson(payload, buffer, sizeof(buffer));

	constexpr bool RETAIN = true;
	int success;
	{
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		success = _client.publish(topic.c_str(), buffer, RETAIN);

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	}

	if (success) {
		// success to transmit
		_heartbeat_store = false;
	} else {
		debugMessage("MQTT HB", "Warning", "Failed to write heartbeat", "\n");
		_penalty_wait.enable             = true;     // Enable wait
		_penalty_wait.last_failed_millis = millis(); // Update last failed time
		_penalty_wait.wait_ms            = 1000;     // Wait interval time 1sec

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
	static struct Penalty _penalty_wait = {false, 0, 0};
	enum IoTTx_Tx_Result result         = RESULT_TX_ERROR;

	if (_penalty_wait.enable) {
		if ((millis() - _penalty_wait.last_failed_millis) > _penalty_wait.wait_ms) {
			_penalty_wait.enable = false;
		}
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (!HAL.Wlan.connectWlan()) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	default:
		break;
	}

	if (!connectBroker()) {
		_penalty_wait.enable             = true;     // Enable wait
		_penalty_wait.last_failed_millis = millis(); // Update last failed time
		_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
		return result;
	}

	String _topic = makeTypicalTopic();
	while (_tx_idx.store != _tx_idx.transmit) {
		clientLoop();

		while (_mutex) {
			HAL.delay(1);
		}
		_mutex = true;

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
			payload["value"] = _buffer[_tx_idx.transmit].value.toInt();
			break;
		case TYPE_DOUBLE:
			payload["value"] = _buffer[_tx_idx.transmit].value.toDouble();
			break;
		default:
			payload["value"] = _buffer[_tx_idx.transmit].value;
			break;
		}

		if (_unix_time_field.enable) {
			signed long long ts = mktime(&_buffer[_tx_idx.transmit].time);
			_mutex              = false;
			if (ts != -1) {
				String s = lld2String(ts);
				String p = _unix_time_field.nano_precision ? "000000000" : "";

				// Translate String to signed long long
				errno = 0;
				ts    = strtoll((String(s) + p).c_str(), NULL, 10);
				if (errno != ERANGE) {
					payload["time"] = ts;
				}
			}
		}
		_mutex = false;

		char buffer[256] = {'\0'};
		serializeJson(payload, buffer, sizeof(buffer));

		int success;
		{
			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
			}

			success = _client.publish(topic.c_str(), buffer, _retain);

			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				HAL.Lan.giveSemaphoreMutex();
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}
		}

		if (success) {
			// success to transmit
			while (_mutex) {
				HAL.delay(1);
			}
			_mutex                          = true;
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			_mutex              = false;
			result              = RESULT_TX_SOME_DATA;
			// printIndex("MQTT:", "\n");
		} else {
			debugMessage("MQTT", "Error", "Failed to publish.", "\n");
			clientLoop();
			if (_auto_disconnect) {
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
					HAL.Wlan.disconnectWlan();
				}
			}
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 1000;     // Wait interval time 1sec
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
#ifdef HAL_ENABLE_EXCEPTIONAL_BUFFER
	static struct Penalty _penalty_wait = {false, 0, 0};
	enum IoTTx_Tx_Result result         = RESULT_TX_ERROR;

	if (_penalty_wait.enable) {
		if ((millis() - _penalty_wait.last_failed_millis) > _penalty_wait.wait_ms) {
			_penalty_wait.enable = false;
		}
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (!HAL.Wlan.connectWlan()) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}
		break;
	default:
		break;
	}

	if (!connectBroker()) {
		_penalty_wait.enable             = true;     // Enable wait
		_penalty_wait.last_failed_millis = millis(); // Update last failed time
		_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
		return result;
	}

	String _topic = makeTypicalTopic();
	// for exceptional storage
	for (int storage = 0; storage < _EXCEPTIONAL_STORAGE_NUM; storage++) {
		clientLoop();
		if (_ex_buffer_info[storage].updated == false) {
			continue;
		}

		while (_mutex) {
			HAL.delay(1);
		}
		_mutex = true;

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

			HAL.takeSemaphoreMutex("camera", portMAX_DELAY);
			unsigned int len = base64::encodeLength(HAL.Camera.jpgImage.size());
			char *tmp_arr    = new char[len + 1]();

			base64::encode(&HAL.Camera.jpgImage[0], HAL.Camera.jpgImage.size(), tmp_arr);
			HAL.giveSemaphoreMutex("camera");

			auto value = payload_ex["value"].to<JsonObject>();
			if (copyArray(tmp_arr, value) == false) {
				// failed to copy
				debugMessage("MQTT Ex", "Error",
				             ("Failed to copy image. Size: " + String(len)).c_str(), "\n");
				_mutex = false;
				return result;
			}

			delete[] tmp_arr;
		} else if (storage == 1) {
			// For storage number 1
			topic = "Batch/" + topic;
			payload_ex.remove("unit");

			HAL.takeSemaphoreMutex("ex_sensor_data", portMAX_DELAY);

			payload_ex["sensors"] = ex_sensor_data;

			HAL.giveSemaphoreMutex("ex_sensor_data");
		} else {
			_mutex = false;
			break;
		}

		if (_unix_time_field.enable) {
			signed long long ts = mktime(&_ex_buffer_info[storage].time);
			_mutex              = false;
			if (ts != -1) {
				String s = lld2String(ts);
				String p = _unix_time_field.nano_precision ? "000000000" : "";

				// Translate String to signed long long
				errno = 0;
				ts    = strtoll((String(s) + p).c_str(), NULL, 10);
				if (errno != ERANGE) {
					payload_ex["time"] = ts;
				}
			}
		}
		_mutex = false;

		char *buffer = new char[HAL_MQTT_MAX_PACKET_SIZE]();
		serializeJson(payload_ex, buffer, HAL_MQTT_MAX_PACKET_SIZE);
		// serializeJson(payload_ex, Serial);
		// HAL.println("");

		int success;
		{
			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
			}

			success = _client.publish(topic.c_str(), buffer, _retain);

			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				HAL.Lan.giveSemaphoreMutex();
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}
		}

		delete[] buffer;
		if (success) {
			// success to transmit
			while (_mutex) {
				HAL.delay(1);
			}
			_mutex                           = true;
			_ex_buffer_info[storage].updated = false;
			_mutex                           = false;
			result                           = RESULT_TX_SOME_DATA;
			// printIndex("MQTT:", "\n");
		} else {
			debugMessage("MQTT Ex", "Error", "Failed to publish.", "\n");
			debugMessage("MQTT Ex", "Info",
			             ("Bufsize: " + String(_client.getBufferSize())).c_str(), "\n");

			clientLoop();
			if (_auto_disconnect) {
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
					HAL.Wlan.disconnectWlan();
				}
			}
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 1000;     // Wait interval time 1sec
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