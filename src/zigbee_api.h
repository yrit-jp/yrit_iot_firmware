#ifndef ZIGBEE_API_H
#define ZIGBEE_API_H

#include "hal/hal.h"
#ifndef HAL_BLE_TX_SIZE
#define HAL_BLE_TX_SIZE (128)
#endif /* HAL_BLE_TX_SIZE */

#ifndef ZIGBEE_API_MAX_FRAME_SIZE
#define ZIGBEE_API_MAX_FRAME_SIZE (3072) //(1024 * 3)
// #define ZIGBEE_API_MAX_FRAME_SIZE (1024)
#endif

#define PRINT_RECV_PACKET
// #define DEBUG_ZIGBEE_API_SEND
// #define DEBUG_ZIGBEE_API_RECV

enum ZigbeeApiStatus {
	ZAS_WAIT,             //!< 受信待ち
	ZAS_COMPLETE,         //!< 受信完了
	ZAS_RECEIVING,        //!< 受信中
	ZAS_SEND_COMPLETE,    //!< 送信完了
	ZAS_SEND_ERROR,       //!< 送信エラー
	ZAS_INVALID_CHECKSUM, //!< チェックサムエラー
};

struct RawFrame {
	std::vector<uint8_t> packet; // データパケット
	uint16_t idx;                // データパケットのインデックス
	uint16_t data_len;           // データパケットの長さ
	uint16_t frame_len;          //	フレーム全体の長さ(ヘッダ含む)
	bool escape;                 // エスケープフラグ
	enum ZigbeeApiStatus status; // 送受信状態
};

class SettingBT {
public:
	/*! ==============================================================
	 * @brief BLE の初期化を行います
	 * @param[in] (local_name) BLE のローカル名
	 * ============================================================= */
	virtual void begin(String local_name);

	/*! ==============================================================
	 * @brief BLE でデータの送信を行います
	 * @param[in] (payload) 送信する文字列のポインタ
	 * @param[in] (size) 送信する文字列のサイズ
	 * @return 送信が完了したら ZAS_SEND_COMPLETE, それ以外は ZAS_SEND_ERROR を返します
	 * ============================================================= */
	enum ZigbeeApiStatus send(const char *payload, unsigned int size);

	/*! ==============================================================
	 * @brief BLE でデータの受信を行います
	 * @return 受信が完了したら ZAS_COMPLETE, それ以外は ZAS_WAIT を返します
	 * ============================================================= */
	enum ZigbeeApiStatus update();

	/*! ==============================================================
	 * @brief 受信したデータを取得します
	 * @param[out] (data) データを格納するバッファ
	 * @param[out] (len) データ長
	 * ============================================================= */
	void getRecvData(uint8_t *data, uint16_t *len);

	/*! ==============================================================
	 * @brief 受信したパケットを表示します
	 * ============================================================= */
	void printRecvPacket();

private:
	/*! ==============================================================
	 * @brief 受信データをクリアします
	 * @param[out] (raw_frame) 受信データを格納する構造体
	 * ============================================================= */
	void clearRawFrame(struct RawFrame *raw_frame);

	/*! ==============================================================
	 * @brief チェックサムを計算します
	 * @param[in] (*array) チェックサムを計算するデータのポインタ
	 * @param[in] (size) データサイズ
	 * @return 計算したチェックサム値
	 * ============================================================= */
	uint8_t calcCheckSum(const uint8_t *array, size_t size);

	struct RawFrame frame;
};

#endif /* ZIGBEE_API_H */
