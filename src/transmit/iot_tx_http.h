#ifndef MY_HTTP_H
#define MY_HTTP_H

#include "http_helper.h"
#include "iot_tx.h"
#include <arduino_base64.hpp>

class IoTTxHTTP : public IoTTx {
public:
	// ==============================================================
	//! @brief 送信先URLを設定します
	//! @param[in] (url) 送信先URL
	//! @return 正しいURLの場合は true, それ以外は false
	// =============================================================
	bool setUrl(String url);

protected:
	// ==============================================================
	//! @brief 送信処理を行います（通常のセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note 送信は HTTP POST メソッドで行います
	// =============================================================
	IoTTx_Tx_Result transmit() override;

	// ==============================================================
	//! @brief 送信処理を行います（特殊なセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note 送信は HTTP POST メソッドで行います
	// =============================================================
	IoTTx_Tx_Result transmitEx() override;

	struct HttpHelper::UniformResourceLocator _target;
};

#endif /* MY_HTTP_H */