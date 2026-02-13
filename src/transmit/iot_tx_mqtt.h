#ifndef MY_MQTT_H
#define MY_MQTT_H

#include "iot_tx.h"
#include <PubSubClient.h>
#include <arduino_base64.hpp>

class IoTTxMQTT : public IoTTx {
public:
	// ==============================================================
	//! @brief MQTT 送信に必要なパラメータを設定します
	//! @param[in] (broker) ブローカーアドレス
	//! @param[in] (port) ポート番号
	//! @param[in] (base_topic) ベーストピック
	//! @param[in] (broker_user) ブローカーユーザ名
	//! @param[in] (broker_pass) ブローカーパスワード
	//! @param[in] (retain) retain フラグ（true: 有効, false: 無効）
	// =============================================================
	void setMqttParam(String broker,
	                  uint16_t port,
	                  String base_topic,
	                  String broker_user = "",
	                  String broker_pass = "",
	                  bool retain        = false);

	// ==============================================================
	//! @brief MQTT クライアントの状態を更新します
	//! @note メインループ内で定期的に呼び出してください
	// =============================================================
	void clientLoop();

	// ==============================================================
	//! @brief MQTT ブローカーに接続します
	//! @return 接続成功の場合は true, 失敗の場合は false
	// =============================================================
	bool connectBroker();

	// ==============================================================
	//! @brief MQTT のステータスを取得します
	//! @return ステータス番号
	// =============================================================
	int status();

	// ==============================================================
	//! @brief ハートビートデータを送信します
	//! @return 送信成功の場合は true, 失敗の場合は false
	// =============================================================
	bool transmitHeartbeat() override;

	// ==============================================================
	//! @brief 通常データの送信処理（clientLoop を前後に呼び出し）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result update() override;

	// ==============================================================
	//! @brief 特殊データの送信処理（clientLoop を前後に呼び出し）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result updateEx() override;

protected:
	// ==============================================================
	//! @brief 送信処理を行います（通常のセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result transmit() override;

	// ==============================================================
	//! @brief 送信処理を行います（特殊なセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result transmitEx() override;

	// ==============================================================
	//! @brief MQTT トピックを生成します
	//! @return 生成したトピック文字列
	// =============================================================
	String makeTypicalTopic();

	String _broker;
	uint16_t _port;
	String _base_topic;
	String _broker_user;
	String _broker_pass;
	bool _retain;

	PubSubClient _client;
	PenaltyHelper _penalty_wait;
};

#endif