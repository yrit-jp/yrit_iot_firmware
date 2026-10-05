#include "iot_tx_http.h"

#ifndef HAL_HTTP_MAX_PACKET_SIZE
#define HAL_HTTP_MAX_PACKET_SIZE (65534)
#endif

bool IoTTxHTTP::setUrl(String url) {
	_mutex_key    = "iot_tx_http";
	_DEBUG_PREFIX = "HTTP";

	struct HttpHelper::UniformResourceLocator result;
	if (HttpHelper::tryParseUrl(url, result) == false) {
		return false;
	}
	_target = result;

	if (_target.protocol != "http" and _target.protocol != "https") {
		debugMessage(LogCategory::HTTP, LogLevel::Error, ("Unsupported protocol: " + _target.protocol).c_str());
		return false;
	}

	if (_target.port == 0) {
		_target.port = (_target.protocol == "https") ? 443 : 80;
	}

	//_target.path = "www." + _target.path;

	HAL.println("");
	debugMessage(LogCategory::HTTP, LogLevel::Info, ("Protocol: " + _target.protocol).c_str());
	debugMessage(LogCategory::HTTP, LogLevel::Info, ("Port    : " + String(_target.port)).c_str());
	debugMessage(LogCategory::HTTP, LogLevel::Info, ("Host    : " + _target.host).c_str());
	debugMessage(LogCategory::HTTP, LogLevel::Info, ("Path    : " + _target.path).c_str());

	return true;
}

IoTTx_Tx_Result IoTTxHTTP::transmit() {
	enum IoTTx_Tx_Result result    = RESULT_TX_ERROR;
	constexpr bool ENABLE_RECREATE = true;
	Client *my_client              = nullptr;

	if (_penalty_wait.isActive()) {
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan(15000)) {
			_penalty_wait.activate(5000);
			return result;
		}

		// Fetch client object
		my_client = HAL.Wlan.getClient("http", ENABLE_RECREATE);
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return result;
		}

		// Fetch client object
		my_client = HAL.Lan.getClient("http", ENABLE_RECREATE);
		break;
	default:
		break;
	}

	if (my_client == nullptr) {
		debugMessage(LogCategory::HTTP, LogLevel::Error, "Failed to begin HTTP client.");
		return RESULT_TX_ERROR;
	}

	while (_tx_idx.store != _tx_idx.transmit) {
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		HttpClient http_client(*my_client, _target.host, _target.port);

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

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
			payload["value"] = std::strtod(_buffer[_tx_idx.transmit].value.c_str(), nullptr);
			break;
		default:
			payload["value"] = _buffer[_tx_idx.transmit].value;
			break;
		}

		if (_unix_time_field.enable) {
			constexpr bool nano_precision = true;

			payload["time"] = convertToUnixTime(_buffer[_tx_idx.transmit].time, nano_precision);
		}
		HAL.giveSemaphoreMutex(_mutex_key);

		char buffer[256] = {'\0'};
		serializeJson(payload, buffer, sizeof(buffer));

		int response_code;
		{
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			EthernetMutexGuard _;
			http_client.post(_target.path, "application/json", buffer);
			response_code = http_client.responseStatusCode();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}

		switch (response_code) {
		case 200:
			/* fallthrough */
		case 302:
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			HAL.giveSemaphoreMutex(_mutex_key);
			result = RESULT_TX_SOME_DATA;
			// printIndex();
			break;
		default:
			debugMessage(LogCategory::HTTP, LogLevel::Error, "Failed to POST.");
			debugMessage(LogCategory::HTTP, LogLevel::Error, ("Return code: " + String(response_code)).c_str());
			// http_client.end();
			switch (response_code) {
			case -1:
				debugMessage(LogCategory::HTTP, LogLevel::Error, "CONNECTION_FAILED");
				break;
			case -2:
				debugMessage(LogCategory::HTTP, LogLevel::Error, "API_INVALID");
				break;
			case -3:
				debugMessage(LogCategory::HTTP, LogLevel::Error, "TIMED_OUT");
				break;
			case -4:
				debugMessage(LogCategory::HTTP, LogLevel::Error, "INVALID_RESPONSE_CODE");
				break;
			default:
				break;
			}

			if (_auto_disconnect) {
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
					HAL.Wlan.disconnectWlan();
				}
			}

			_penalty_wait.activate(1000);
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
#if HAL_ENABLE_EXCEPTIONAL_BUFFER
	enum IoTTx_Tx_Result result    = RESULT_TX_ERROR;
	constexpr bool ENABLE_RECREATE = true;
	Client *my_client              = nullptr;

	if (_penalty_wait.isActive()) {
		return result;
	}

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		if (not HAL.Wlan.connectWlan(15000)) {
			_penalty_wait.activate(5000);
			return result;
		}

		// Fetch client object
		my_client = HAL.Wlan.getClient("http", ENABLE_RECREATE);
		break;
	case DATALINK_LAYER::ETHERNET:
		if (HAL.Lan.isLinkUp() == false) {
			_penalty_wait.activate(5000);
			return result;
		}

		// Fetch client object
		my_client = HAL.Lan.getClient("http", ENABLE_RECREATE);
		break;
	default:
		break;
	}

	if (my_client == nullptr) {
		debugMessage(LogCategory::HTTP_EX, LogLevel::Error, "Failed to begin HTTP client.");
		return RESULT_TX_ERROR;
	}

	// for exceptional storage
	for (int storage = 0; storage < _EXCEPTIONAL_STORAGE_NUM; storage++) {
		if (_ex_buffer_info[storage].updated == false) {
			continue;
		}

		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
		}

		HttpClient http_client(*my_client, _target.host, _target.port);

		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.giveSemaphoreMutex();
		}
		/* -+-+-+-+- Mutex block END -+-+-+-+- */

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		JsonDocument payload_ex;
		payload_ex["devID"]    = _devID;
		payload_ex["sensName"] = _ex_buffer_info[storage].name;
		payload_ex["unit"]     = _ex_buffer_info[storage].unit;

		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_ex_buffer_info[storage].time);
		payload_ex["day"] = s_day;

		if (storage == 0) {
			// For storage number 0
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
					debugMessage(LogCategory::HTTP_EX, LogLevel::Error,
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
				debugMessage(LogCategory::HTTP_EX, LogLevel::Error,
				             String("Failed to copy image. Size: " + String(len)).c_str());
				HAL.giveSemaphoreMutex(_mutex_key);
				return result;
			}

		} else if (storage == 1) {
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
			buffer = new char[HAL_HTTP_MAX_PACKET_SIZE]();
			if (buffer == nullptr) {
				// メモリ確保の初期化に失敗した
				// 最初の１回だけ呼び出されるはずなので、再起動させエラーを知らせる
				HAL.reboot();
			}
		}
		memset(buffer, 0, HAL_HTTP_MAX_PACKET_SIZE);
		serializeJson(payload_ex, buffer, HAL_HTTP_MAX_PACKET_SIZE);
		// HAL.println(buffer);

		int response_code;
		{
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			EthernetMutexGuard _;
			http_client.post(_target.path, "application/json", buffer);
			response_code = http_client.responseStatusCode();
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}

		// delete[] buffer;
		switch (response_code) {
		case 200:
			/* fallthrough */
		case 302:
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_ex_buffer_info[storage].updated = false;
			HAL.giveSemaphoreMutex(_mutex_key);
			result = RESULT_TX_SOME_DATA;
			// printIndex();
			break;
		default:
			debugMessage(LogCategory::HTTP_EX, LogLevel::Error, "Failed to POST.");
			debugMessage(LogCategory::HTTP_EX, LogLevel::Error, ("Return code: " + String(response_code)).c_str());
			// http_client.end();
			switch (response_code) {
			case -1:
				debugMessage(LogCategory::HTTP_EX, LogLevel::Error, "CONNECTION_FAILED");
				break;
			case -2:
				debugMessage(LogCategory::HTTP_EX, LogLevel::Error, "API_INVALID");
				break;
			case -3:
				debugMessage(LogCategory::HTTP_EX, LogLevel::Error, "TIMED_OUT");
				break;
			case -4:
				debugMessage(LogCategory::HTTP_EX, LogLevel::Error, "INVALID_RESPONSE_CODE");
				break;
			default:
				break;
			}

			if (_auto_disconnect) {
				if (HAL.getUseLinkLayer() == DATALINK_LAYER::WIFI) {
					HAL.Wlan.disconnectWlan();
				}
			}

			_penalty_wait.activate(1000);
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