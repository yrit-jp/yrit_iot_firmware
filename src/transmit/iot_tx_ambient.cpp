#include "iot_tx_ambient.h"

void IoTTxAmbient::setUniqueKey(const char *write_key, unsigned int channel_id) {
	_mutex_key    = "iot_tx_ambient";
	_DEBUG_PREFIX = "Ambient";

	_write_key  = write_key;
	_channel_id = channel_id;

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		_ambient.begin(_channel_id, _write_key.c_str(), HAL.Wlan.getClient("ambient"));
		break;
	case DATALINK_LAYER::ETHERNET: {
		{
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
			EthernetMutexGuard _;
			_ambient.begin(_channel_id, _write_key.c_str(), HAL.Lan.getClient("ambient"));
			/* -+-+-+-+- Mutex block END -+-+-+-+- */
		}
	} break;
	default:
		break;
	}
}

void IoTTxAmbient::setDataFilter(uint8_t data_number, String sensor_name) {
	_filter[sensor_name] = data_number;
}

bool IoTTxAmbient::isTargetSensor(String sensor_name) {
	auto n = _filter.count(sensor_name);
	if (n == 0) {
		return false;
	}
	return true;
}

bool IoTTxAmbient::preAddValue(const String &name) {
	return isTargetSensor(name);
}

bool IoTTxAmbient::preAddExceptionalValue(uint8_t storage) {
	constexpr uint8_t EX_STORAGE_NUMBER_0 = 0;
	constexpr uint8_t EX_STORAGE_NUMBER_1 = 1;

	switch (storage) {
	case EX_STORAGE_NUMBER_0:
		// カメラ画像
		debugMessage(LogCategory::AMBIENT, LogLevel::Warning, "Camera data is NOT supported!");
		return false;
	case EX_STORAGE_NUMBER_1:
		// バッチ送信
		debugMessage(LogCategory::AMBIENT, LogLevel::Warning, "Batch transfer is NOT supported!");
		return false;
	default:
		break;
	}
	return false;
}

IoTTx_Tx_Result IoTTxAmbient::transmit() {
	constexpr uint16_t TIMEOUT_MS = 4000;
	enum IoTTx_Tx_Result result   = RESULT_TX_ERROR;

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

	JsonDocument payload;
	while (_tx_idx.store != _tx_idx.transmit) {
		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
		// NOTE: maybe this check not needed
		auto n = _filter.count(_buffer[_tx_idx.transmit].name);
		if (n == 0) {
			// Not match target
			// debugMessage("Ambient", "Ignore Data", _buffer[_tx_idx.transmit].name.c_str(), "\n");
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			HAL.giveSemaphoreMutex(_mutex_key);
			continue;
		}
		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y-%m-%d %T.000", &_buffer[_tx_idx.transmit].time);

		String index                    = String(_filter[_buffer[_tx_idx.transmit].name]);
		payload["data"][0]["d" + index] = _buffer[_tx_idx.transmit].value;
		payload["writeKey"]             = _write_key;
		payload["data"][0]["created"]   = s_day;
		HAL.giveSemaphoreMutex(_mutex_key);

		char buffer[128] = {'\0'};
		serializeJson(payload, buffer, sizeof(buffer));
		// For debug
		// serializeJsonPretty(payload, Serial);

		int success;
		switch (HAL.getUseLinkLayer()) {
		case DATALINK_LAYER::WIFI:
			success = _ambient.bulk_send(buffer, TIMEOUT_MS);
			break;
		case DATALINK_LAYER::ETHERNET: {
			{
				/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */
				EthernetMutexGuard _;
				success = _ambient.bulk_send(buffer, TIMEOUT_MS);
				/* -+-+-+-+- Mutex block END -+-+-+-+- */
			}
		} break;
		default:
			break;
		}

		if (success > 0) {
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
			debugMessage(LogCategory::AMBIENT, LogLevel::Error, "Failed to send.");
			result = RESULT_TX_ERROR;
		}

		// For wait 5s interval (Ambient rejects consecutive requests of 5 seconds or less.)
		_penalty_wait.activate(5500);
		break;
	}

	return result;
}