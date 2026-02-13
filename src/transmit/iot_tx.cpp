#include "iot_tx.h"

void IoTTx::begin() {
	_tx_idx.store                   = 0;
	_tx_idx.transmit                = 0;
	_unix_time_field.enable         = false;
	_unix_time_field.nano_precision = false;

	_pending_data_count = 0;
	_heartbeat_store    = false;

	for (int i = 0; i < _EXCEPTIONAL_STORAGE_NUM; i++) {
		_ex_buffer_info[i].updated = false;
	}
}

IoTTx_Tx_Result IoTTx::getLastResult() {
	return _last_result;
}

IoTTx_Tx_Result IoTTx::getLastResultEx() {
	return _last_result_ex;
}

void IoTTx::setUniqueParam(String dev_id) {
	_devID = dev_id;
}

void IoTTx::configUnixTimeField(bool enable, bool nano_precision) {
	_unix_time_field.enable         = enable;
	_unix_time_field.nano_precision = nano_precision;
}

//
//! @brief 保留中のデータを出力する
void IoTTx::printPendingData() {
	struct TX_INDEX tmp_index = _tx_idx;

	while (_mutex) {
		HAL.delay(1);
	}
	_mutex = true;

	char s_day[64] = {'\0'};
	bool flag      = false;
	HAL.println(":::::::::::::::::::::::::");
	while (tmp_index.store != tmp_index.transmit) {
		flag = true;
		HAL.printf("%s\n", _buffer[tmp_index.transmit].name.c_str());
		strftime(s_day, 64, "%Y/%m/%d %H:%M:%S", &_buffer[tmp_index.transmit].time);
		HAL.printf("%s\n", s_day);
		HAL.printf("%s %s\n",
		           _buffer[tmp_index.transmit].value.c_str(),
		           _buffer[tmp_index.transmit].unit.c_str());
		HAL.println(":::::::::::::::::::::::::");
		tmp_index.transmit = ((tmp_index.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (tmp_index.transmit + 1);
	}
	if (flag == false) {
		HAL.println(":::::::::::::::::::::::::");
	}

	_mutex = false;
}

void IoTTx::printIndex(const char *str_begin, const char *str_end) {
	HAL.printf("%s [store, send] = [%d, %d] %s", str_begin, _tx_idx.store, _tx_idx.transmit, str_end);
}

void IoTTx::printIndexEx(const char *str_begin, const char *str_end) {
	uint8_t ex_pendings = 0;
	for (int i = 0; i < _EXCEPTIONAL_STORAGE_NUM; i++) {
		if (_ex_buffer_info[i].updated == false) {
			continue;
		}
		ex_pendings++;
	}
	HAL.printf("%s [ex_store] = [%d] %s", str_begin, ex_pendings, str_end);
}

void IoTTx::debugMessage(const char *category, const char *prefix, const char *msg, const char *end) {
	HAL.printf("[%s](%s) %s%s", category, prefix, msg, end);
}

void IoTTx::loop_forever() {
	while (1) {
		update();
		if (getLastResult() & (RESULT_TX_NOTHING | RESULT_TX_ERROR)) {
			// No data to transmit
			delay(1000);
		}

		updateEx();
		if (getLastResultEx() & (RESULT_TX_NOTHING | RESULT_TX_ERROR)) {
			// No data to transmit
			delay(1000);
		}
	}
}

IoTTx_Tx_Result IoTTx::update() {
	if (!txPending()) {
		_last_result = RESULT_TX_NOTHING;
	} else {
		_last_result = transmit();
	}
	return _last_result;
}

IoTTx_Tx_Result IoTTx::updateEx() {
	if (!txPendingEx()) {
		_last_result_ex = RESULT_TX_NOTHING;
	} else {
		_last_result_ex = transmitEx();
	}
	return _last_result_ex;
}

int IoTTx::getPendingDataQuantity() {
	uint8_t ex_pendings = 0;
	for (int i = 0; i < _EXCEPTIONAL_STORAGE_NUM; i++) {
		if (_ex_buffer_info[i].updated == false) {
			continue;
		}
		ex_pendings++;
	}
	return _pending_data_count + ex_pendings;
}

bool IoTTx::txPending() {
	if (_tx_idx.store == _tx_idx.transmit) {
		return false;
	}
	return true;
}

bool IoTTx::txPendingEx() {
	for (int storage = 0; storage < _EXCEPTIONAL_STORAGE_NUM; storage++) {
		if (_ex_buffer_info[storage].updated == true) {
			return true;
		}
	}
	return false;
}

void IoTTx::enableAutoDisconnectWiFi(bool enable) {
	_auto_disconnect = enable;
}

void IoTTx::updateHeartbeat(struct tm time_stamp) {
	_heartbeat_tm    = time_stamp;
	_heartbeat_store = true;
}

bool IoTTx::transmitHeartbeat() {
	return true;
}

IoTTx::ESTIMATED_TYPE IoTTx::estimateType(String value) {
	char *endp;
	errno = 0;
	strtol(value.c_str(), &endp, 10);
	if (errno == ERANGE) {
		return TYPE_STRING;
	}
	if (*endp == '\0') {
		return TYPE_INTEGER;
	}

	errno = 0;
	strtod(value.c_str(), &endp);
	if (errno == ERANGE) {
		return TYPE_STRING;
	}
	if (*endp == '\0') {
		return TYPE_DOUBLE;
	}

	return TYPE_STRING;
}

String IoTTx::lld2String(signed long long val, int base) {
	char buf[64] = {0};

	int i    = 62;
	int sign = (val < 0);
	if (sign) {
		val = -val;
	}

	if (val == 0) {
		return "0";
	}

	for (; val && i; --i, val /= base) {
		buf[i] = "0123456789abcdef"[val % base];
	}

	if (sign) {
		buf[i--] = '-';
	}

	return String(&buf[i + 1]);
}

IoTTx_Tx_Result IoTTx::transmit() {
	return RESULT_TX_ALL_DATA;
}

IoTTx_Tx_Result IoTTx::transmitEx() {
	return RESULT_TX_ALL_DATA;
}