#include "iot_tx_ambient.h"

void IoTTxAmbient::setUniqueKey(const char *write_key, unsigned int channel_id) {
	_write_key  = write_key;
	_channel_id = channel_id;

	switch (HAL.getUseLinkLayer()) {
	case DATALINK_LAYER::WIFI:
		_ambient.begin(_channel_id, _write_key.c_str(), HAL.Wlan.getClient("ambient"));
		break;
	case DATALINK_LAYER::ETHERNET:
		/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */ {
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
			_ambient.begin(_channel_id, _write_key.c_str(), HAL.Lan.getClient("ambient"));
			HAL.Lan.giveSemaphoreMutex(); // Give (release) mutex semaphore
		/* -+-+-+-+- Mutex block END -+-+-+-+- */ }
		break;
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

IoTTx_Tx_Result IoTTxAmbient::transmit() {
	constexpr uint16_t TIMEOUT_MS       = 4000;
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

	JsonDocument payload;
	while (_tx_idx.store != _tx_idx.transmit) {
		while (_mutex) {
			HAL.delay(1);
		}
		_mutex = true;
		// NOTE: maybe this check not needed
		auto n = _filter.count(_buffer[_tx_idx.transmit].name);
		if (n == 0) {
			// Not match target
			// debugMessage("Ambient", "Ignore Data", _buffer[_tx_idx.transmit].name.c_str(), "\n");
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_mutex           = false;
			continue;
		}
		char s_day[64] = {'\0'};
		strftime(s_day, 64, "%Y-%m-%d %T.000", &_buffer[_tx_idx.transmit].time);

		String index                    = String(_filter[_buffer[_tx_idx.transmit].name]);
		payload["data"][0]["d" + index] = _buffer[_tx_idx.transmit].value;
		_mutex                          = false;
		payload["writeKey"]             = _write_key;
		payload["data"][0]["created"]   = s_day;

		char buffer[128] = {'\0'};
		serializeJson(payload, buffer, sizeof(buffer));
		// For debug
		// serializeJsonPretty(payload, Serial);

		int success;
		switch (HAL.getUseLinkLayer()) {
		case DATALINK_LAYER::WIFI:
			success = _ambient.bulk_send(buffer, TIMEOUT_MS);
			break;
		case DATALINK_LAYER::ETHERNET:
			/* -+-+-+-+- Mutex block BEGIN -+-+-+-+- */ {
				HAL.Lan.takeSemaphoreMutex(portMAX_DELAY); // Take mutex semaphore
				success = _ambient.bulk_send(buffer, TIMEOUT_MS);
				HAL.Lan.giveSemaphoreMutex(); // Give (release) mutex semaphore
			/* -+-+-+-+- Mutex block END -+-+-+-+- */ }
			break;
		default:
			break;
		}

		if (success > 0) {
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
			// printIndex("Ambient:", "\n");
		} else {
			debugMessage("Ambient", "Error", "Failed to send.", "\n");
			result = RESULT_TX_ERROR;
		}

		// For wait 5s interval (Ambient rejects consecutive requests of 5 seconds or less.)
		_penalty_wait.enable             = true;     // Enable wait
		_penalty_wait.last_failed_millis = millis(); // Update last failed time
		_penalty_wait.wait_ms            = 5500;     // Wait interval time 1sec
		break;
	}

	return result;
}