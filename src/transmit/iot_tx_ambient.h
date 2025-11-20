#ifndef MY_AMBIENT_H
#define MY_AMBIENT_H

#include "iot_tx.h"
#include <Ambient.h>

class IoTTxAmbient : public IoTTx {
public:
	/*! ==============================================================
	 * @brief 送信に必要な固有パラメータを設定します
	 * @param[in] (write_key) Ambient の Write Key
	 * @param[in] (channel_id) Ambient の Channel ID
	 * ============================================================= */
	void setUniqueKey(const char *write_key, unsigned int channel_id);

	/*! ==============================================================
	 * @brief データフィルタを設定します
	 * @param[in] (data_number) データ番号
	 * @param[in] (sensor_name) センサ名
	 * ============================================================= */
	void setDataFilter(uint8_t data_number, String sensor_name);

	/*! ==============================================================
	 * @brief センサ名が対象かどうかを確認します
	 * @param[in] (sensor_name) センサ名
	 * @return 対象の場合は true, それ以外は false
	 * ============================================================= */
	bool isTargetSensor(String sensor_name);

protected:
	/*! ==============================================================
	 * @brief データ送信処理を行います
	 * @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	 * ============================================================= */
	IoTTx_Tx_Result transmit() override;
	std::map<String, uint8_t> _filter;

	String _write_key = "";
	unsigned int _channel_id;

	Ambient _ambient;
};

#endif /* MY_AMBIENT_H */