#ifndef IOT_TX_LORAWAN_H
#define IOT_TX_LORAWAN_H

#include "iot_tx.h"

// LoRaWAN ペイロードの最大サイズ（AS923, DR3 最小想定）
// Dwell Time 制約により DR2 以下は使用不可（DR2: 11byte のみ）
#ifndef LORAWAN_MAX_PAYLOAD_SIZE
#define LORAWAN_MAX_PAYLOAD_SIZE (53)
#endif

// LoRaWAN FPort デフォルト値
#ifndef LORAWAN_FPORT_SENSOR_NAME_DEFAULT
#define LORAWAN_FPORT_SENSOR_NAME_DEFAULT (9) // センサ名送信用デフォルト
#endif

#ifndef LORAWAN_FPORT_SENSOR_VALUE_DEFAULT
#define LORAWAN_FPORT_SENSOR_VALUE_DEFAULT (10) // センサ値送信用デフォルト
#endif

// データタイプ定義（センサ値ペイロード用）
#define LORAWAN_DATATYPE_FLOAT (0x00)  // 4バイト浮動小数点（未使用）
#define LORAWAN_DATATYPE_INT32 (0x01)  // 4バイト整数
#define LORAWAN_DATATYPE_DOUBLE (0x02) // 8バイト浮動小数点

class IoTTxLoRaWAN : public IoTTx {
public:
	// ==============================================================
	//! @brief LoRaWAN 送信に必要なパラメータを設定します
	//! @param[in] (dev_eui) デバイス EUI
	//! @param[in] (join_eui) Join EUI / App EUI
	//! @param[in] (app_key) アプリケーションキー
	//! @param[in] (fport_name) センサ名送信用 FPort（デフォルト: 9）
	//! @param[in] (fport_value) センサ値送信用 FPort（デフォルト: 10）
	//! @return 設定に成功した場合は true、FPort が不正値の場合は false
	// =============================================================
	bool setLoRaWanParam(uint8_t fport_name  = LORAWAN_FPORT_SENSOR_NAME_DEFAULT,
	                     uint8_t fport_value = LORAWAN_FPORT_SENSOR_VALUE_DEFAULT);

	// ==============================================================
	//! @brief 通常データの送信処理（clientLoop を前後に呼び出し）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result update() override;

protected:
	uint8_t _fport_name;
	uint8_t _fport_value;

	PenaltyHelper _penalty_wait;

	// ==============================================================
	//! @brief 送信処理を行います（通常のセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note センサ名とセンサ値を分けて送信します
	//! @note FPort は setLoRaWanParam() で設定した値を使用します
	// =============================================================
	IoTTx_Tx_Result transmit() override;

	// ==============================================================
	//! @brief センサ名のバイナリペイロードを生成します
	//! @param[in] (data) 送信するセンサデータ
	//! @param[in] (pair_id) ペア番号
	//! @param[out] (*payload) 生成されたペイロードを格納するバッファ
	//! @param[in] (max_size) ペイロードの最大サイズ
	//! @param[out] (*actual_size) 実際に生成されたサイズ
	//! @return 生成に成功した場合は true, それ以外は false
	// =============================================================
	bool makeSensorNamePayload(const TX_DATA &data,
	                           uint8_t pair_id,
	                           uint8_t *payload,
	                           size_t max_size,
	                           size_t *actual_size);

	// ==============================================================
	//! @brief センサ値のバイナリペイロードを生成します
	//! @param[in] (data) 送信するセンサデータ
	//! @param[in] (pair_id) ペア番号
	//! @param[out] (*payload) 生成されたペイロードを格納するバッファ
	//! @param[in] (max_size) ペイロードの最大サイズ
	//! @param[out] (*actual_size) 実際に生成されたサイズ
	//! @return 生成に成功した場合は true, それ以外は false
	// =============================================================
	bool makeSensorValuePayload(const TX_DATA &data,
	                            uint8_t pair_id,
	                            uint8_t *payload,
	                            size_t max_size,
	                            size_t *actual_size);

	// ==============================================================
	//! @brief FPort の値が有効範囲（1〜223）かを検証します
	//! @param[in] (fport) 検証する FPort 値
	//! @return 有効な場合は true、それ以外は false
	// =============================================================
	bool isValidFPort(uint8_t fport) const;
};

#endif /* IOT_TX_LORAWAN_H */
