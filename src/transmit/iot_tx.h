#ifndef IOTTX_H
#define IOTTX_H

#include "../exceptional_sensor_value.h"
#include "../hal/hal.h"
#include "penalty_helper.h"

#ifndef IOTTX_BUFFER_SIZE
#define IOTTX_BUFFER_SIZE (128) //(256)
#endif

enum IoTTx_Tx_Result {
	RESULT_TX_ERROR     = 1, //!< 送信エラー
	RESULT_TX_NOTHING   = 2, //!< 送信するデータはありませんでした
	RESULT_TX_SOME_DATA = 4, //!< 一部データを送信しました
	RESULT_TX_ALL_DATA  = 8  //!< すべてのデータを送信しました
};

enum class LogCategory : int {
	AMBIENT,
	AMBIENT_EX,
	AMBIENT_HB,
	FTP,
	FTP_EX,
	FTP_HB,
	HTTP,
	HTTP_EX,
	HTTP_HB,
	MQTT,
	MQTT_EX,
	MQTT_HB,
	LORA_WAN,
	LORA_WAN_EX,
	LORA_WAN_HB
};

enum class LogLevel : int {
	Error,
	Warning,
	Info,
	Debug,
};

class IoTTx {
public:
	// ==============================================================
	//! @brief 送信機能の初期化を行います
	// =============================================================
	void begin();

	// ==============================================================
	//! @brief 最後の送信結果を取得します（通常のセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result getLastResult();

	// ==============================================================
	//! @brief 最後の送信結果を取得します（特殊なセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	// =============================================================
	IoTTx_Tx_Result getLastResultEx();

	// ==============================================================
	//! @brief 送信に必要な固有パラメータを設定します
	//! @param[in] (dev_id) デバイスID
	// =============================================================
	void setUniqueParam(String dev_id);

	// ==============================================================
	//! @brief unixタイムフィールドに関する設定を行います
	//! @param[in] (enable) unixタイムも送信するか
	//! @param[in] (nano_precision) 時間精度にナノ秒を使用するか
	// =============================================================
	void configUnixTimeField(bool enable, bool nano_precision);

	// ==============================================================
	//! @brief センサデータを送信バッファに追加します
	//! @param[in] (time_stamp) タイムスタンプ
	//! @param[in] (value) 送信するデータ
	//! @param[in] (name) データの名前
	//! @param[in] (unit) データの単位
	//! @param[in] (port) センサが接続されているポート番号
	// =============================================================
	template <typename T, typename U>
	void addValue(struct tm time_stamp, T value, U name, String unit = "", uint8_t port = 0) {
		if (not preAddValue(String(name))) {
			return;
		}

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		_buffer[_tx_idx.store].time  = time_stamp;
		_buffer[_tx_idx.store].value = String(value);
		_buffer[_tx_idx.store].name  = String(name);
		_buffer[_tx_idx.store].unit  = unit;
		_buffer[_tx_idx.store].port  = port;

		_tx_idx.store = ((_tx_idx.store + 1) == IOTTX_BUFFER_SIZE)
		                    ? 0
		                    : (_tx_idx.store + 1);

		if (_tx_idx.store == _tx_idx.transmit) {
			_tx_idx.transmit = ((_tx_idx.transmit + 1) == IOTTX_BUFFER_SIZE)
			                       ? 0
			                       : (_tx_idx.transmit + 1);
		}
		_pending_data_count = ((_pending_data_count + 1) == IOTTX_BUFFER_SIZE)
		                          ? _pending_data_count
		                          : (_pending_data_count + 1);

		HAL.giveSemaphoreMutex(_mutex_key);
	}

	// ==============================================================
	//! @brief 特殊なセンサデータを送信バッファに追加します
	//! @param[in] (storage) 特殊なデータを格納するストレージ番号
	//! @param[in] (time_stamp) タイムスタンプ
	//! @param[in] (name) データの名前
	//! @param[in] (unit) データの単位
	//! @param[in] (port) センサが接続されているポート番号
	// =============================================================
	template <typename T>
	void addExceptionalValue(uint8_t storage, struct tm time_stamp, T name, String unit = "", uint8_t port = 0) {
#if HAL_ENABLE_EXCEPTIONAL_BUFFER
		if (_EXCEPTIONAL_STORAGE_NUM <= storage) {
			// Exceed array index
			return;
		}

		if (not preAddExceptionalValue(storage)) {
			return;
		}

		HAL.takeSemaphoreMutex(_mutex_key, portMAX_DELAY);

		_ex_buffer_info[storage].time    = time_stamp;
		_ex_buffer_info[storage].name    = String(name);
		_ex_buffer_info[storage].unit    = unit;
		_ex_buffer_info[storage].port    = port;
		_ex_buffer_info[storage].updated = true;

		// メモリ効率化のため、value はグローバル変数に直接代入する（ここには書かない）
		// グローバル変数は各 HAL クラスまたは ExceptionSensorValue.cpp で管理される
		// 追加時は _EXCEPTIONAL_STORAGE_NUM の個数を調整すること

		HAL.giveSemaphoreMutex(_mutex_key);
#endif /* HAL_ENABLE_EXCEPTIONAL_BUFFER */
	}

	// ==============================================================
	//! @brief 保留中のデータを出力する
	// =============================================================
	void printPendingData();

	// ==============================================================
	//! @brief 送信済みデータとバッファ格納済みデータのインデックスを表示する（通常センサデータ用）
	// =============================================================
	void printIndex();

	// ==============================================================
	//! @brief 送信済みデータとバッファ格納済みデータのインデックスを表示する（特殊なセンサデータ用）
	// =============================================================
	void printIndexEx();

	//
	//! @brief デバッグメッセージの表示
	//! @param[in] (category) カテゴリ文字列を指定する（ERROR, INFO など）
	//! @param[in] (prefix) 小項目識別子を指定する
	//! @param[in] (msg) 本文
	//! @param[in] (end) 文末に付加する文字列
	// void debugMessage(const char *category, const char *prefix, const char *msg, const char *end = "");
	void debugMessage(LogCategory log_category, LogLevel log_level, const char *msg, const char *end = "\n");

	// ==============================================================
	//! @brief 必要に応じて通常のセンサデータの送信を行います
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note 送信バッファにデータがない場合は何もしません
	// =============================================================
	virtual IoTTx_Tx_Result update();

	// ==============================================================
	//! @brief 必要に応じて特殊なセンサデータの送信を行います
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note 送信バッファにデータがない場合は何もしません
	// =============================================================
	virtual IoTTx_Tx_Result updateEx();

	// ==============================================================
	//! @brief 送信バッファに溜まっている未送信データ数を取得します
	//! @return 未送信データ数
	// =============================================================
	int getPendingDataQuantity();

	// ==============================================================
	//! @brief 送信バッファに未送信データがあるか確認します（通常のセンサデータ用）
	//! @return 未送信データがある場合は true, それ以外は false
	// =============================================================
	bool txPending();

	// ==============================================================
	//! @brief 送信バッファに未送信データがあるか確認します（特殊なセンサデータ用）
	//! @return 未送信データがある場合は true, それ以外は false
	// =============================================================
	bool txPendingEx();

	// ==============================================================
	//! @brief 自動WiFi切断機能の有効化/無効化を指定します
	//! @param[in] (enable) 有効にする場合は true, それ以外は false
	// =============================================================
	void enableAutoDisconnectWiFi(bool enable);

	// ==============================================================
	//! @brief ハートビートの送信リクエストを行います
	//! @param[in] (time_stamp) 送信時間
	// =============================================================
	void updateHeartbeat(struct tm time_stamp);

	// ==============================================================
	//! @brief ハートビートの送信処理を行います
	//! @return 送信成功の場合は true, 失敗の場合は false
	//! @note 継承先で実装します
	// =============================================================
	virtual bool transmitHeartbeat();

protected:
	// ==============================================================
	//! @brief addValue の前処理を行います
	//! @param[in] (name) データの名前
	//! @return 処理を続行する場合は true, 中断する場合は false
	//! @note 派生クラスでオーバーライドして個別の処理を追加できます
	// =============================================================
	virtual bool preAddValue(const String &name) { return true; }

	// ==============================================================
	//! @brief addExceptionalValue の前処理を行います
	//! @param[in] (storage) ストレージ番号
	//! @return 処理を続行する場合は true, 中断する場合は false
	//! @note 派生クラスでオーバーライドして個別の処理を追加できます
	// =============================================================
	virtual bool preAddExceptionalValue(uint8_t storage) { return true; }

	// ==============================================================
	//! @brief 送信処理を行います（通常のセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note 継承先で実装します
	// =============================================================
	virtual IoTTx_Tx_Result transmit();

	// ==============================================================
	//! @brief 送信処理を行います（特殊なセンサデータ用）
	//! @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	//! @note 継承先で実装します
	// =============================================================
	virtual IoTTx_Tx_Result transmitEx();

	IoTTx_Tx_Result _last_result    = RESULT_TX_ALL_DATA;
	IoTTx_Tx_Result _last_result_ex = RESULT_TX_ALL_DATA;

	enum ESTIMATED_TYPE {
		TYPE_STRING,
		TYPE_INTEGER,
		TYPE_DOUBLE
	};

	// ==============================================================
	//! @brief 文字列のデータ型を推定します
	//! @param[in] (value) 推定する文字列
	//! @return 推定されたデータ型（ESTIMATED_TYPE）
	// =============================================================
	ESTIMATED_TYPE estimateType(String value);

	// ==============================================================
	//! @brief long long 型の数値を文字列に変換する関数
	//! @param[in] (val) 変換する long long 型のデータ
	//! @param[in] (base) 変換する基数
	//! @return 変換した文字列を返す
	//! @note 一部マイコンでは標準変換関数が使えないため実装した
	// =============================================================
	String lld2String(signed long long val, int base = 10);

	signed long long convertToUnixTime(const struct tm &struct_time, bool nano_sec);

	String _devID;
	String _sensName;
	String _DEBUG_PREFIX = "IoTTx";

	struct UNIX_TIME_CONFIG {
		bool enable;
		bool nano_precision;
	};
	struct UNIX_TIME_CONFIG _unix_time_field;

	struct tm _heartbeat_tm;
	bool _heartbeat_store;

	struct TX_EXCEPTIONAL_DATA_INFO {
		struct tm time;
		bool updated;
		String name;
		String unit;
		uint8_t port;
	};
	static constexpr uint8_t _EXCEPTIONAL_STORAGE_NUM = EXCEPTIONAL_STORAGE_VALUE_NUM;
	struct TX_EXCEPTIONAL_DATA_INFO _ex_buffer_info[_EXCEPTIONAL_STORAGE_NUM];

	struct TX_DATA {
		struct tm time;
		String name;
		String value;
		String unit;
		uint8_t port;
	};
	struct TX_DATA _buffer[IOTTX_BUFFER_SIZE];

	struct TX_INDEX {
		unsigned int store;
		unsigned int transmit;
	};
	struct TX_INDEX _tx_idx;
	int _pending_data_count;

	String _wifi_ssid;
	String _wifi_pass;
	unsigned long _timeout_sec;

	bool _auto_disconnect = true;
	String _mutex_key;
};

class EthernetMutexGuard {
public:
	EthernetMutexGuard() {
		if (HAL.getUseLinkLayer() == DATALINK_LAYER::ETHERNET) {
			HAL.Lan.takeSemaphoreMutex(portMAX_DELAY);
			locked = true;
		}
	}

	~EthernetMutexGuard() {
		if (locked) {
			HAL.Lan.giveSemaphoreMutex();
		}
	}

	// コピーコンストラクタと代入演算子を削除
	EthernetMutexGuard(const EthernetMutexGuard &)            = delete;
	EthernetMutexGuard &operator=(const EthernetMutexGuard &) = delete;

private:
	bool locked = false;
};

#endif