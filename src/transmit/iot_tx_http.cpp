#include "iot_tx_http.h"

#ifndef HAL_HTTP_MAX_PACKET_SIZE
#define HAL_HTTP_MAX_PACKET_SIZE (65534)
#endif

bool IoTTxHTTP::setUrl(String url) {
	struct HttpHelper::UniformResourceLocator result;
	if (HttpHelper::tryParseUrl(url, result) == false) {
		return false;
	}
	_target = result;

	if (_target.protocol != "http" && _target.protocol != "https") {
		debugMessage("Http", "Error", ("Unsupported protocol: " + _target.protocol).c_str());
		return false;
	}

	if (_target.port == 0) {
		_target.port = (_target.protocol == "https") ? 443 : 80;
	}

	//_target.path = "www." + _target.path;

	HAL.println("");
	debugMessage("Http", "Info: Protocol", _target.protocol.c_str(), "\n");
	debugMessage("Http", "Info: Port    ", String(_target.port).c_str(), "\n");
	debugMessage("Http", "Info: Host    ", _target.host.c_str(), "\n");
	debugMessage("Http", "Info: Path    ", _target.path.c_str(), "\n");

	return true;
}

IoTTx_Tx_Result IoTTxHTTP::transmit() {
	static struct Penalty _penalty_wait = {false, 0, 0};
	enum IoTTx_Tx_Result result         = RESULT_TX_ERROR;
	constexpr bool ENABLE_RECREATE      = true;
	Client *my_client                   = nullptr;

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

		// Fetch client object
		my_client = HAL.Wlan.getClient("http", ENABLE_RECREATE);
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}

		// Fetch client object
		my_client = HAL.Lan.getClient("http", ENABLE_RECREATE);
		break;
	default:
		break;
	}

	if (my_client == nullptr) {
		debugMessage("Http", "Error", "Failed to begin HTTP client.", "\n");
		return RESULT_TX_ERROR;
	}

	while (_tx_idx.store != _tx_idx.transmit) {
		while (_mutex) {
			HAL.delay(1);
		}
		_mutex = true;

		////////////////
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		HttpClient http_client(*my_client, _target.host, _target.port);

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
		////////////////

		JsonDocument payload;
		payload["devID"]    = _devID;
		payload["sensName"] = _buffer[_tx_idx.transmit].name;
		payload["unit"]     = _buffer[_tx_idx.transmit].unit;

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

		int response_code;
		{
			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
			}

			http_client.post(_target.path, "application/json", buffer);
			response_code = http_client.responseStatusCode();

			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				HAL.Lan.giveSemaphoreMutex();
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}
		}

		switch (response_code) {
		case 200:
			/* fallthrough */
		case 302:
			while (_mutex) {
				HAL.delay(1);
			}
			_mutex              = true;
			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			_mutex              = false;
			result              = RESULT_TX_SOME_DATA;
			// printIndex("Http:", "\n");
			break;
		default:
			debugMessage("Http", "Error", "Failed to POST.", "\n");
			debugMessage("Http", "Error", ("Return code: " + String(response_code)).c_str(), "\n");
			// http_client.end();
			switch (response_code) {
			case -1:
				debugMessage("Http Ex", "Error", "CONNECTION_FAILED", "\n");
				break;
			case -2:
				debugMessage("Http Ex", "Error", "API_INVALID", "\n");
				break;
			case -3:
				debugMessage("Http Ex", "Error", "TIMED_OUT", "\n");
				break;
			case -4:
				debugMessage("Http Ex", "Error", "INVALID_RESPONSE_CODE", "\n");
				break;
			default:
				break;
			}

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
	result = RESULT_TX_ALL_DATA;

	if (_auto_disconnect) {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
			HAL.Wlan.disconnectWlan();
		}
	}
	return result;
}

IoTTx_Tx_Result IoTTxHTTP::transmitEx() {
#ifdef HAL_ENABLE_EXCEPTIONAL_BUFFER
	static struct Penalty _penalty_wait = {false, 0, 0};
	enum IoTTx_Tx_Result result         = RESULT_TX_ERROR;
	constexpr bool ENABLE_RECREATE      = true;
	Client *my_client                   = nullptr;

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

		// Fetch client object
		my_client = HAL.Wlan.getClient("http", ENABLE_RECREATE);
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.enable             = true;     // Enable wait
			_penalty_wait.last_failed_millis = millis(); // Update last failed time
			_penalty_wait.wait_ms            = 5000;     // Wait interval time 5sec
			return result;
		}

		// Fetch client object
		my_client = HAL.Lan.getClient("http", ENABLE_RECREATE);
		break;
	default:
		break;
	}

	if (my_client == nullptr) {
		debugMessage("Http Ex", "Error", "Failed to begin HTTP client.", "\n");
		return RESULT_TX_ERROR;
	}

	// for exceptional storage
	for (int storage = 0; storage < _EXCEPTIONAL_STORAGE_NUM; storage++) {
		if (_ex_buffer_info[storage].updated == false) {
			continue;
		}

		while (_mutex) {
			HAL.delay(1);
		}
		_mutex = true;

		////////////////
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		HttpClient http_client(*my_client, _target.host, _target.port);

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
		////////////////

		JsonDocument payload_ex;
		payload_ex["devID"]    = _devID;
		payload_ex["sensName"] = _ex_buffer_info[storage].name;
		payload_ex["unit"]     = _ex_buffer_info[storage].unit;

		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_ex_buffer_info[storage].time);
		payload_ex["day"] = s_day;

		if (storage == 0) {
			// For storage number 0
			HAL.takeSemaphoreMutex("camera", portMAX_DELAY);
			unsigned int len = base64::encodeLength(HAL.Camera.jpgImage.size());
			char *tmp_arr    = new char[len + 1]();

			base64::encode(&HAL.Camera.jpgImage[0], HAL.Camera.jpgImage.size(), tmp_arr);
			HAL.giveSemaphoreMutex("camera");

			auto value = payload_ex["value"].to<JsonObject>();
			if (copyArray(tmp_arr, value) == false) {
				// failed to copy
				debugMessage("Http Ex", "Error",
				             String("Failed to copy image. Size: " + String(len)).c_str(), "\n");
				_mutex = false;
				return result;
			}

			delete[] tmp_arr;
		} else if (storage == 1) {
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

		uint32_t BUF_LEN = measureJson(payload_ex) + 1;
		char *buffer     = new char[BUF_LEN];
		memset(buffer, '\0', BUF_LEN);
		serializeJson(payload_ex, buffer, BUF_LEN);
		// HAL.println(buffer);

		int response_code;
		{
			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
			}

			http_client.post(_target.path, "application/json", buffer);
			response_code = http_client.responseStatusCode();

			if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
				HAL.Lan.giveSemaphoreMutex();
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}
		}

		delete[] buffer;
		switch (response_code) {
		case 200:
			/* fallthrough */
		case 302:
			while (_mutex) {
				HAL.delay(1);
			}
			_mutex                           = true;
			_ex_buffer_info[storage].updated = false;
			_mutex                           = false;
			result                           = RESULT_TX_SOME_DATA;
			// printIndex("Http:", "\n");
			break;
		default:
			debugMessage("Http Ex", "Error", "Failed to POST.", "\n");
			debugMessage("Http Ex", "Error", ("Return code: " + String(response_code)).c_str(), "\n");
			// http_client.end();
			switch (response_code) {
			case -1:
				debugMessage("Http Ex", "Error", "CONNECTION_FAILED", "\n");
				break;
			case -2:
				debugMessage("Http Ex", "Error", "API_INVALID", "\n");
				break;
			case -3:
				debugMessage("Http Ex", "Error", "TIMED_OUT", "\n");
				break;
			case -4:
				debugMessage("Http Ex", "Error", "INVALID_RESPONSE_CODE", "\n");
				break;
			default:
				break;
			}

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