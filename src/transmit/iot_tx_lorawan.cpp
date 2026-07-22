#include "iot_tx_lorawan.h"

bool IoTTxLoRaWAN::setLoRaWanParam(uint8_t fport_name, uint8_t fport_value) {
	if (not isValidFPort(fport_name) or not isValidFPort(fport_value)) {
		return false;
	}

	_fport_name  = fport_name;
	_fport_value = fport_value;

	_DEBUG_PREFIX = "LoRaWAN";
	_mutex_key    = "iot_tx_lorawan";
	return true;
}

IoTTx_Tx_Result IoTTxLoRaWAN::update() {
	HAL.LoRaWan.doWork();
	IoTTx_Tx_Result result = IoTTx::update();
	HAL.LoRaWan.doWork();
	return result;
}

IoTTx_Tx_Result IoTTxLoRaWAN::transmit() {
	IoTTx_Tx_Result result = RESULT_TX_ERROR;

	if (_penalty_wait.isActive()) {
		return result;
	}

	// Join 状態を確認
	if (not HAL.LoRaWan.isJoined()) {
		if (HAL.LoRaWan.join() == false) {
			return result;
		}
	}

	while (_tx_idx.store != _tx_idx.transmit) {
		uint8_t payload_name[LORAWAN_MAX_PAYLOAD_SIZE]  = {0};
		uint8_t payload_value[LORAWAN_MAX_PAYLOAD_SIZE] = {0};
		size_t payload_name_size                        = 0;
		size_t payload_value_size                       = 0;

		HAL.LoRaWan.doWork();

		// 送信対象データとペア番号を取得
		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
		const TX_DATA tx_data = _buffer[_tx_idx.transmit];
		uint8_t pair_id       = _tx_idx.transmit & 0xFF;
		HAL.giveSemaphoreMutex(_mutex_key);

		// センサ名ペイロード生成
		bool success_name = makeSensorNamePayload(tx_data, pair_id, payload_name, sizeof(payload_name), &payload_name_size);
		if ((not success_name) or payload_name_size == 0) {
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Failed to create sensor name payload");
			_penalty_wait.activate(1000);
			return result;
		}

		// センサ値ペイロード生成
		bool success_value = makeSensorValuePayload(tx_data, pair_id, payload_value, sizeof(payload_value), &payload_value_size);
		if ((not success_value) or payload_value_size == 0) {
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Failed to create sensor value payload");
			_penalty_wait.activate(1000);
			return result;
		}

		// 1. センサ名を送信
		// HAL.printf("=== TX pair %d: name ===\n", pair_id);
		bool tx_result_name = HAL.LoRaWan.send(payload_name, payload_name_size, _fport_name);
		if (not tx_result_name) {
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Failed to send sensor name");
			_penalty_wait.activate(1000);
			return result;
		}

		// TX + RX1/RX2 ウィンドウ完了を待機してから次の送信へ
		if (not HAL.LoRaWan.waitTxDone()) {
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Timeout waiting for TX done (name)");
			_penalty_wait.activate(1000);
			return result;
		}

		HAL.LoRaWan.doWork();

		// 2. センサ値を送信
		// HAL.printf("=== TX pair %d: value ===\n", pair_id);
		bool tx_result_value = HAL.LoRaWan.send(payload_value, payload_value_size, _fport_value);
		if (not tx_result_value) {
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Failed to send sensor value");
			_penalty_wait.activate(1000);
			return result;
		}

		// TX + RX1/RX2 ウィンドウ完了を待機してから次の処理へ
		if (not HAL.LoRaWan.waitTxDone()) {
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Timeout waiting for TX done (value)");
			_penalty_wait.activate(1000);
			return result;
		}

		HAL.LoRaWan.doWork();

		if (tx_result_name and tx_result_value) {
			// 送信成功：バッファをクリアしてインデックスを進める
			HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);
			_buffer[_tx_idx.transmit].value = "";
			_buffer[_tx_idx.transmit].name  = "";
			_buffer[_tx_idx.transmit].unit  = "";

			_tx_idx.transmit    = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE) ? 0 : (_tx_idx.transmit + 1);
			_pending_data_count = (((_pending_data_count - 1) > 0) ? (_pending_data_count - 1) : 0);
			HAL.giveSemaphoreMutex(_mutex_key);

			result = RESULT_TX_SOME_DATA;
		} else {
			// 送信失敗：ペナルティを設定してループを抜ける
			debugMessage(LogCategory::LORA_WAN, LogLevel::Error, "Failed to send LoRaWAN data");
			HAL.LoRaWan.doWork();
			_penalty_wait.activate(1000);
			return result;
		}
	}

	HAL.LoRaWan.doWork();

	// 全データ送信完了
	result = RESULT_TX_ALL_DATA;
	return result;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Utility Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

bool IoTTxLoRaWAN::isValidFPort(uint8_t fport) const {
	return fport >= 1 and fport <= 223;
}

bool IoTTxLoRaWAN::makeSensorNamePayload(const TX_DATA &data,
                                         uint8_t pair_id,
                                         uint8_t *payload,
                                         size_t max_size,
                                         size_t *actual_size) {
	if (max_size < 2) {
		// 最低でも ペア番号(1byte) + 名前(1byte以上) が必要
		*actual_size = 0;
		return false;
	}

	// ペイロード構築
	size_t offset = 0;

	// ペア番号
	payload[offset] = pair_id;
	offset++;

	// センサ名をコピー
	size_t name_len  = data.name.length();
	size_t available = max_size - offset;

	if (name_len > available) {
		// 名前が長すぎる場合は切り詰める
		name_len = available;
	}

	memcpy(&payload[offset], data.name.c_str(), name_len);
	offset += name_len;

	*actual_size = offset;
	return true;
}

bool IoTTxLoRaWAN::makeSensorValuePayload(const TX_DATA &data,
                                          uint8_t pair_id,
                                          uint8_t *payload,
                                          size_t max_size,
                                          size_t *actual_size) {
	if (max_size < 6) {
		// 最低でも ペア番号(1) + データタイプ(1) + 値(4) が必要
		*actual_size = 0;
		return false;
	}

	// ペイロード構築
	size_t offset = 0;

	// ペア番号
	payload[offset] = pair_id;
	offset++;

	// データタイプと値のエンコード
	ESTIMATED_TYPE type = estimateType(data.value);

	switch (type) {
	case TYPE_INTEGER: {
		// 整数値（int32）
		int32_t value   = data.value.toInt();
		payload[offset] = LORAWAN_DATATYPE_INT32;
		offset++;
		memcpy(&payload[offset], &value, sizeof(int32_t));
		offset += sizeof(int32_t);
		break;
	}
	case TYPE_DOUBLE: {
		// 浮動小数点値（double）
		// ペア番号(1) + データタイプ(1) + double値(8) が必要
		if (offset + 1 + sizeof(double) > max_size) {
			*actual_size = 0;
			return false;
		}
		double value    = std::strtod(data.value.c_str(), nullptr);
		payload[offset] = LORAWAN_DATATYPE_DOUBLE;
		offset++;
		memcpy(&payload[offset], &value, sizeof(double));
		offset += sizeof(double);
		break;
	}
	default:
		// 文字列（数値に変換できない場合）
		// 現時点では未対応
		*actual_size = 0;
		return false;
	}

	// 単位を追加（残りの領域に）
	size_t unit_len  = data.unit.length();
	size_t available = max_size - offset;

	if (unit_len > available) {
		// 単位が長すぎる場合は切り詰める
		unit_len = available;
	}

	if (unit_len > 0) {
		memcpy(&payload[offset], data.unit.c_str(), unit_len);
		offset += unit_len;
	}

	*actual_size = offset;
	return true;
}
