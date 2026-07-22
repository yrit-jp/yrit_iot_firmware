#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "../hal/hal.h"
#include "../user/user_i2c_sensor.h"
#include "i2c_type_def.h"

class I2C_Manager {
public:
	I2C_Manager();

	// ==============================================================
	//! @brief 検出済みの I2C デバイス情報を消去します
	// =============================================================
	void clearDetectedDevices();

	// ==============================================================
	//! @brief I2C デバイスのアドレスとスキャンします
	//! @param[in] (port) I2C ポート番号
	// =============================================================
	void scanDevice(uint8_t port);

	// ==============================================================
	//! @brief マルチプレクサ下にある I2C デバイスのアドレスとスキャンします
	//! @param[in] (port) I2C ポート番号
	//! @return ヒットした I2C スレーブアドレスの配列（vector）を返します。
	// =============================================================
	std::vector<uint8_t> scanDeviceUnderMultiplexer(uint8_t port);

	// ==============================================================
	//! @brief I2C の無効アドレスを指定する正規表現をセットします
	//! @param[in] (port) 適用するポート番号
	//! @param[in] (pattern) 無効アドレスにマッチする正規表現
	// =============================================================
	void setIgnoreAddressPattern(uint8_t port, const char *pattern);

	// ==============================================================
	//! @brief I2C の無効センサ名を指定する正規表現をセットします
	//! @param[in] (port) 適用するポート番号
	//! @param[in] (pattern) 無効センサ名にマッチする正規表現
	// =============================================================
	void setIgnoreNamePattern(uint8_t port, const char *pattern);

	// ==============================================================
	//! @brief 指定ポートにおいて、対象アドレスが無視対象かどうかを判定します
	//! @param[in] (port) I2C ポート番号
	//! @param[in] (address) 判定対象の I2C スレーブアドレス
	//! @return 無視対象であれば true, それ以外(範囲外ポートを含む)は false を返します。
	// ==============================================================
	bool isIgnoreSensorAddress(uint8_t port, uint8_t address);

	// ==============================================================
	//! @brief 指定ポートにおいて、対象センサ名が無視対象かどうかを判定します
	//! @param[in] (port) I2C ポート番号
	//! @param[in] (name) 判定対象のセンサ名
	//! @return 無視対象であれば true, それ以外(範囲外ポートを含む)は false を返します。
	// ==============================================================
	bool isIgnoreSensorName(uint8_t port, const char *name);

	// ==============================================================
	//! @brief 有効な I2C アドレスのリストを取得します
	//! @return 有効な I2C アドレスの配列（vector）を返します。
	//! @see enum I2CActiveDevice
	// =============================================================
	std::vector<I2CActiveDevice> getAddressList();

	// ==============================================================
	//! @brief I2C センサを初期化します
	//! @param[in] (port) I2C ポート番号
	//! @param[in] (address) ターゲット I2C スレーブアドレス
	//! @param[in] (bus_channel) マルチプレクサ用バスチャネル（-1がルート）
	//! @param[in] (sub_address) ターゲット I2C スレーブアドレス（マルチプレクサ使用時）
	//! @param[in] (use_sub_addr) sub_address による識別が必要な場合は true, それ以外は false
	//! @return 初期化結果とそのセンサから取得できるデータ種別数を返します。
	//! @ref I2C_INIT_RESULT
	// =============================================================
	I2C_INIT_RESULT initializeDevice(uint8_t port,
	                                 uint8_t address,
	                                 int8_t bus_channel,
	                                 uint8_t sub_address,
	                                 bool use_sub_addr = false);

	// ==============================================================
	//! @brief I2C センサの名前を取得します
	//! @param[in] (port) I2C ポート番号
	//! @param[in] (address) ターゲット I2C スレーブアドレス
	//! @param[in] (bus_channel) マルチプレクサ用バスチャネル（-1がルート）
	//! @param[in] (sub_address) ターゲット I2C スレーブアドレス（マルチプレクサ使用時）
	//! @param[in] (sub_id) 取得データ識別用 ID
	//! @param[in] (use_sub_addr) sub_address による識別が必要な場合は true, それ以外は false
	//! @return I2C センサの名前を表す String 型の文字列を返します。
	// =============================================================
	String getDeviceName(uint8_t port,
	                     uint8_t address,
	                     int8_t bus_channel,
	                     uint8_t sub_address,
	                     int16_t sub_id    = -1,
	                     bool use_sub_addr = false);

	// ==============================================================
	//! @brief I2C センサ値の単位を取得します
	//! @param[in] (port) I2C ポート番号
	//! @param[in] (address) ターゲット I2C スレーブアドレス
	//! @param[in] (bus_channel) マルチプレクサ用バスチャネル（-1がルート）
	//! @param[in] (sub_address) ターゲット I2C スレーブアドレス（マルチプレクサ使用時）
	//! @param[in] (sub_id) 取得データ識別用 ID
	//! @param[in] (use_sub_addr) sub_address による識別が必要な場合は true, それ以外は false
	//! @return センサ値の単位を表す String 型の文字列を返します。
	// =============================================================
	String getDeviceUnit(uint8_t port,
	                     uint8_t address,
	                     int8_t bus_channel,
	                     uint8_t sub_address,
	                     int16_t sub_id    = -1,
	                     bool use_sub_addr = false);

	// ==============================================================
	//! @brief I2C センサ値を取得します
	//! @param[in] (port) I2C ポート番号
	//! @param[in] (address) ターゲット I2C スレーブアドレス
	//! @param[in] (bus_channel) マルチプレクサ用バスチャネル（-1がルート）
	//! @param[in] (sub_address) ターゲット I2C スレーブアドレス（マルチプレクサ使用時）
	//! @param[in] (sub_id) 取得データ識別用 ID
	//! @param[in] (tx_buffered) 前回値が送信バッファに格納された場合 true, それ以外は false
	//! @param[out] (*d_data) 取得した double 型のセンサ値を格納するポインタ
	//! @param[out] (*i_data) 取得した int 型のセンサ値を格納するポインタ
	//! @param[out] (*s_data) 取得した String 型のセンサ値を格納するポインタ
	//! @param[in] (use_sub_addr) sub_address による識別が必要な場合は true, それ以外は false
	//! @return 取得結果と取得したデータ型を返します。
	//! @see enum I2C_DATA_RESULT
	// =============================================================
	I2C_DATA_RESULT fetchData(uint8_t port,
	                          uint8_t address,
	                          int8_t bus_channel,
	                          uint8_t sub_address,
	                          int16_t sub_id,
	                          bool tx_buffered,
	                          double *d_data,
	                          int32_t *i_data,
	                          String *s_data,
	                          bool use_sub_addr = false);

	// ==============================================================
	//! @brief スキャンによって検出した I2C デバイスの結果を表示します
	// =============================================================
	void displayScanResult();

protected:
	std::vector<I2CActiveDevice> _active_device;
	struct SensorFilterConfig {
		struct {
			String address; // 無視するアドレスを示す正規表現パターン
			String name;    // 無視するセンサ名を示す正規表現パターン
		} pattern;

		struct {
			std::vector<uint8_t> list; // 無視するアドレスのリスト
			bool valid = false;        // キャッシュ判定フラグ
		} cache;
	};
	struct SensorFilterConfig _ignore[PORT_I2C::PORT_5 + 1];

	// ==============================================================
	//! @brief 正規表現パターンをトップレベルの '|' で分割します
	//! @param[in] (regexp_pattern) 分割対象の正規表現パターン文字列（'\|' はリテラルの '|' として扱う）
	//! @return 分割された各パターン断片の std::vector<std::string> を返します。
	// ==============================================================
	std::vector<String> splitPatternByPipe(const char *regexp_pattern);

	// ==============================================================
	//! @brief 文字列が、分割済みパターン断片群のいずれかに完全一致するか判定します
	//! @param[in] (text) 判定対象の文字列
	//! @param[in] (fragments) splitPatternByPipe() で分割済みのパターン断片群
	//! @return いずれかの断片に完全一致すれば true, それ以外は false を返します。
	// ==============================================================
	bool matchesAnyFragment(const char *text, const std::vector<String> &fragments);

	// ==============================================================
	//! @brief 正規表現パターンに一致する I2C アドレスを 0x00〜0x7F の範囲で抽出します
	//! @param[in] (regexp_pattern) IGNORE_ADDR_REGEX に相当する正規表現パターン文字列
	//! @return パターンに一致した(無視すべき) I2C アドレスの std::vector<uint8_t> を返します。
	// ==============================================================
	std::vector<uint8_t> extractIgnoreAddress(const char *regexp_pattern);

	// ==============================================================
	//! @brief センサ名が無視対象パターンに一致するか判定します
	//! @param[in] (name) 判定対象のセンサ名
	//! @param[in] (regexp_pattern) IGNORE_NAME_REGEX に相当する正規表現パターン文字列
	//! @return パターンに一致すれば true(無視すべきセンサ), それ以外は false を返します。
	// ==============================================================
	bool matchNamePattern(const char *name, const char *regexp_pattern);
};

#endif