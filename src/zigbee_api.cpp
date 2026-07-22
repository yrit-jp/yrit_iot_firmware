#include "zigbee_api.h"

void SettingBT::begin(String local_name) {
	HAL.Ble.begin(local_name);
	frame.packet.resize(ZIGBEE_API_MAX_FRAME_SIZE);
	clearRawFrame(&frame);
	// clearRawFrame(&tx);
}

void SettingBT::getRecvData(uint8_t *data, uint16_t *len) {
	for (int i = 0; i < frame.data_len; i++) {
		*(data + i) = frame.packet[i + 3];
	}
	*len = frame.data_len;
}

enum ZigbeeApiStatus SettingBT::send(const char *payload, unsigned int size) {
#ifdef DEBUG_ZIGBEE_API_SEND
	HAL.printf("data size: %d\n", size);
#endif

	uint8_t tmp = 0;
	clearRawFrame(&frame);
	// Checksum は生データに対して計算する
	uint8_t checksum = calcCheckSum((const uint8_t *)payload, size);
	frame.data_len   = size;
	frame.frame_len  = frame.data_len + 4; // delimiter, len, len, checksum

	frame.packet[frame.idx] = 0x7E;
	frame.idx++;

	tmp = (frame.data_len >> 8);
	if (tmp == 0x7E or tmp == 0x7D) {
		frame.packet[frame.idx] = 0x7D;
		frame.idx++;
		frame.packet[frame.idx] = tmp ^ 0x20;
		frame.frame_len++;
	} else {
		frame.packet[frame.idx] = tmp;
	}
	frame.idx++;
	tmp = (frame.data_len & 0xFF);
	if (tmp == 0x7E or tmp == 0x7D) {
		frame.packet[frame.idx] = 0x7D;
		frame.idx++;
		frame.packet[frame.idx] = tmp ^ 0x20;
		frame.frame_len++;
	} else {
		frame.packet[frame.idx] = tmp;
	}
	frame.idx++;

	for (int i = 0; i < frame.data_len; i++) {
		if (payload[i] == 0x7E or payload[i] == 0x7D) {
			if (frame.idx + 1 >= ZIGBEE_API_MAX_FRAME_SIZE) {
				HAL.println("[Error] Frame buffer overflow (escape in payload)");
				return ZAS_SEND_ERROR;
			}
			frame.packet[frame.idx] = 0x7D;
			frame.idx++;
			frame.packet[frame.idx] = payload[i] ^ 0x20;
			frame.frame_len++;
		} else {
			if (frame.idx >= ZIGBEE_API_MAX_FRAME_SIZE) {
				HAL.println("[Error] Frame buffer overflow (payload)");
				return ZAS_SEND_ERROR;
			}
			frame.packet[frame.idx] = payload[i];
		}
		frame.idx++;
	}

	if (checksum == 0x7E or checksum == 0x7D) {
		if (frame.idx + 1 >= ZIGBEE_API_MAX_FRAME_SIZE) {
			HAL.println("[Error] Frame buffer overflow (escape in checksum)");
			return ZAS_SEND_ERROR;
		}
		frame.packet[frame.idx] = 0x7D;
		frame.idx++;
		frame.packet[frame.idx] = checksum ^ 0x20;
		frame.frame_len++;
	} else {
		if (frame.idx >= ZIGBEE_API_MAX_FRAME_SIZE) {
			HAL.println("[Error] Frame buffer overflow (checksum)");
			return ZAS_SEND_ERROR;
		}
		frame.packet[frame.idx] = checksum;
	}

#ifdef DEBUG_ZIGBEE_API_SEND
	for (int i = 0, k = 0; i < frame.frame_len; i++, k++) {
		HAL.printf("0x%02X ", frame.packet[i]);
		if (k % 16 == 0) {
			HAL.printf("\n");
		}
	}
#else
	// 長文になるとディレイが必要になる
	if (frame.frame_len < 2000) {
		HAL.delay(frame.frame_len);
	} else {
		HAL.delay(2000);
	}
#endif

	constexpr unsigned int TIMEOUT_MS = 5000;
	unsigned long long previous       = millis();
	if (frame.frame_len >= HAL_BLE_TX_SIZE) {
		// 最大送信容量単位で送信
		int i = 0;
		for (i = 0; i < (frame.frame_len / HAL_BLE_TX_SIZE); i++) {
			while (not HAL.Ble.txReady()) {
				HAL.delay(15);

				if ((millis() - previous) > TIMEOUT_MS) {
					HAL.println("[Error] DETECT TIMEOUT!");
					return ZAS_SEND_ERROR;
				}
			};
			HAL.delay(20);
			// HAL.Ble.write(&frame.packet[i * HAL_BLE_TX_SIZE], HAL_BLE_TX_SIZE);
			HAL.Ble.write(frame.packet.data() + (i * HAL_BLE_TX_SIZE), HAL_BLE_TX_SIZE);
			previous = millis();
		}

		// 残りのデータを送信
		while (not HAL.Ble.txReady()) {
			HAL.delay(15);

			if ((millis() - previous) > TIMEOUT_MS) {
				HAL.println("[Error] DETECT TIMEOUT!");
				return ZAS_SEND_ERROR;
			}
		};
		HAL.delay(20);
		// HAL.Ble.write(&frame.packet[i * HAL_BLE_TX_SIZE], (frame.frame_len - (i * HAL_BLE_TX_SIZE)));
		HAL.Ble.write(frame.packet.data() + (i * HAL_BLE_TX_SIZE), (frame.frame_len - (i * HAL_BLE_TX_SIZE)));
	} else {
		// HAL.Ble.write(frame.packet, frame.frame_len);
		HAL.Ble.write(frame.packet.data(), frame.frame_len);
	}
	return ZAS_SEND_COMPLETE;
}

enum ZigbeeApiStatus SettingBT::update() {
	if (HAL.Ble.available() == 0) {
		if (frame.status == ZAS_COMPLETE or frame.status == ZAS_INVALID_CHECKSUM) {
			frame.status = ZAS_WAIT;
		}
		return frame.status;
	}

	uint8_t packet = HAL.Ble.read();
	if (packet == 0x7E) {
		// start delimiter
		HAL.println("Start delimiter");
		clearRawFrame(&frame);
		// HAL.printf("%d %d\n", frame.idx, frame.frame_len);
		frame.packet[frame.idx] = packet;
		frame.status            = ZAS_RECEIVING;
		frame.idx++;
#ifdef DEBUG_ZIGBEE_API_RECV
		HAL.printf("%02X ", packet);
#endif
		return frame.status;
	} else if (frame.status != ZAS_RECEIVING) {
// missing start delimiter
#ifdef DEBUG_ZIGBEE_API_RECV
		HAL.printf("Missing start delimiter! Recv:%02X\n", packet);
#endif
		frame.status = ZAS_WAIT;
		return frame.status;
	}
#ifdef DEBUG_ZIGBEE_API_RECV
	HAL.printf("%02X ", packet);
#endif

	/* Escape sequence */
	if (packet == 0x7D) {
		frame.escape = true;
		return frame.status;
	}
	if (frame.escape) {
		packet       = packet ^ 0x20;
		frame.escape = false;
	}

	frame.packet[frame.idx] = packet;
	frame.idx               = ((frame.idx + 1) == ZIGBEE_API_MAX_FRAME_SIZE) ? frame.idx : frame.idx + 1;

	if (frame.idx < 3) {
		/* Wait until receiveing length packet */
		return frame.status;
	}
	if (frame.idx == 3) {
		/* Set length */
		// uint16_t のまま加算するとオーバーフローで frame_len が小さくラップし、
		// 直後の calcCheckSum / getRecvData でバッファ外を読み書きしてしまうため、
		// 一旦 uint32_t で計算してからバッファサイズを超えないか検証する
		uint32_t declared_len = (static_cast<uint32_t>(frame.packet[1]) << 8) + frame.packet[2];
		uint32_t total_len    = declared_len + 4; // start delimiter, len(msb), len(lsb), checksum

		if (total_len > ZIGBEE_API_MAX_FRAME_SIZE) {
			HAL.println("[Error] Invalid frame length (exceeds buffer size)");
			clearRawFrame(&frame);
			return frame.status;
		}

		frame.data_len  = (uint16_t)declared_len;
		frame.frame_len = (uint16_t)total_len;
		HAL.printf("Frame Size: %d\n", frame.frame_len);
		// HAL.printf("%d %d\n", frame.idx, frame.frame_len);
	}
	// HAL.printf("%d %d\n", frame.idx, frame.frame_len);
	if (frame.idx == frame.frame_len) {
		/* Calcurate checksum */
		uint8_t checksum = calcCheckSum(&frame.packet[3], frame.data_len);
		if (checksum == frame.packet[frame.idx - 1]) {
			frame.status = ZAS_COMPLETE;
#ifdef PRINT_RECV_PACKET
			printRecvPacket();
#endif
		} else {
			HAL.println("Invalid Checksum");
			frame.status = ZAS_INVALID_CHECKSUM;
		}
	}
	return frame.status;
}

void SettingBT::printRecvPacket() {
	if (frame.status != ZAS_COMPLETE) {
		return;
	}
	uint8_t count = 0;
	bool flag     = false;
	HAL.println("========================== PACKET ==========================");
	for (int i = 0; i < frame.frame_len; i++) {
		count++;
		flag = false;
		HAL.printf("%02X ", frame.packet[i]);
		if (count == 20) {
			HAL.println("");
			count = 0;
			flag  = true;
		}
	}

	if (flag == false) {
		HAL.println("");
	}
	HAL.println("============================================================");
}

/**
 *
 */

void SettingBT::clearRawFrame(struct RawFrame *raw_frame) {
	for (int i = 0; i < ZIGBEE_API_MAX_FRAME_SIZE; i++) {
		raw_frame->packet[i] = 0x00;
	}

	raw_frame->idx       = 0;
	raw_frame->data_len  = 0;
	raw_frame->frame_len = 0;
	raw_frame->escape    = false;
	raw_frame->status    = ZAS_WAIT;
}

uint8_t SettingBT::calcCheckSum(const uint8_t *array, size_t size) {
	uint8_t checksum = 0x00;
	for (int i = 0; i < size; i++) {
		checksum += *(array + i);
	}
	checksum = 0xFF - (checksum & 0xFF);
	return checksum;
}
