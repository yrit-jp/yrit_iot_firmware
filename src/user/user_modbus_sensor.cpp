#include "user_modbus_sensor.h"

/* インクルードファイルを追加する（ここから）*/
#include "./user_sensors/modbus_sensors/lib_xy-md02.h"

/* インクルードファイルを追加する（ここまで）*/

class Modbus_User_Sensor_List {
public:
	/* ここに新規センサの宣言を追加する（ここから）*/
	std::unique_ptr<XY_MD02> xy_md02; // メモリ効率化のため、unique_ptr の使用を推奨します

	/* ここに新規センサの宣言を追加する（ここまで）*/
};
std::map<int, std::map<int, Modbus_User_Sensor_List>> modbus_user_sensors;

// ==============================================================
//! @brief MODBUS デバイスの初期化を行います（ユーザ追加デバイス用）
//! @param[in] (port) Modbus ポート番号
//! @param[in] (id) 初期化する Modbus デバイスのユニット番号
//! @return 初期化結果とそのセンサから取得できるデータ種別数を返します。
//! @see enum MODBUS_INIT_RESULT
// =============================================================
MODBUS_INIT_RESULT initializeUserModbusDevice(uint8_t port, uint8_t id) {
	auto _serial      = HAL.Modbus.getSerial(port);
	auto timeout_ms   = HAL.Modbus.getTimeoutMs(port);
	auto bus_reset_ms = timeout_ms < 200 ? 200 : timeout_ms;

	/* ここに新規センサの初期化処理を追加する（ここから）*/

	if (!modbus_user_sensors[port][id].xy_md02) {
		// unique_ptr を初期化する
		modbus_user_sensors[port][id].xy_md02.reset(new XY_MD02(_serial));

		// センサ識別のための判定を行う
		if (modbus_user_sensors[port][id].xy_md02->begin(id, timeout_ms) == true) {
			// 該当センサだった場合は取得可能なセンサ数を返す
			return MODBUS_2_DATA_DEVICE;
		} else {
			// 該当センサではなかった場合は unique_ptr をリセットし、メモリを解放する
			modbus_user_sensors[port][id].xy_md02.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	/* ここに新規センサの初期化処理を追加する（ここまで）*/

	return MODBUS_IGNORE_DEVICE;
}

// ==============================================================
//! @brief MODBUS センサの名前を取得します（ユーザ追加デバイス用）
//! @param[in] (port) Modbus ポート番号
//! @param[in] (id) 初期化する Modbus デバイスのユニット番号
//! @param[in] (sub_id) 取得データ識別用 ID
//! @return MODBUS センサの名前を表す String 型の文字列を返します。
// =============================================================
String getUserModbusDeviceName(uint8_t port, uint8_t id, int16_t sub_id) {
	/* ここに新規センサの名前取得処理を追加する（ここから）*/

	if (modbus_user_sensors[port][id].xy_md02) {
		// 複数のセンサ値を取得できるデバイスは sub_id に応じて名前を変える
		// 「センサ名_センサ種別_(Modbusユニット番号)」の命名規則を推奨
		// sub_id が範囲外の場合はセンサ名だけを返すことが望ましい
		switch (sub_id) {
		case 0:
			return "XY-MD02_Humid_(" + String(id) + ")";
		case 1:
			return "XY-MD02_Temp_(" + String(id) + ")";
		default:
			return "XY-MD02";
		}
	}

	/* ここに新規センサの名前取得処理を追加する（ここまで）*/

	return "";
}

// ==============================================================
//! @brief MODBUS センサ値の単位を取得します（ユーザ追加デバイス用）
//! @param[in] (port) Modbus ポート番号
//! @param[in] (id) 初期化する Modbus デバイスのユニット番号
//! @param[in] (sub_id) 取得データ識別用 ID
//! @return
// =============================================================
String getUserModbusDeviceUnit(uint8_t port, uint8_t id, int16_t sub_id) {
	/* ここに新規センサの単位取得処理を追加する（ここから）*/

	if (modbus_user_sensors[port][id].xy_md02) {
		// 複数のセンサ値を取得できるデバイスは sub_id に応じて単位を変える
		switch (sub_id) {
		case 0:
			return "%RH";
		case 1:
			return "\'C";
		default:
			return "";
		}
	}

	/* ここに新規センサの単位取得処理を追加する（ここまで）*/

	return "";
}

// ==============================================================
//! @brief MODBUS センサ値を取得します（ユーザ追加デバイス用）
//! @param[in] (port) Modbus ポート番号
//! @param[in] (id) 初期化する Modbus デバイスのユニット番号
//! @param[in] (sub_id) 取得データ識別用 ID
//! @param[in] (tx_buffered) 前回値が送信バッファに格納された場合 true, それ以外は false
//! @param[out] (*d_data) 取得した double 型のセンサ値を格納するポインタ
//! @param[out] (*i_data) 取得した int 型のセンサ値を格納するポインタ
//! @param[out] (*s_data) 取得した String 型のセンサ値を格納するポインタ
//! @return 取得結果と取得したデータ型を返します。
//! @see enum MODBUS_DATA_RESULT
// =============================================================
MODBUS_DATA_RESULT fetchUserModbusDeviceData(uint8_t port,
                                             uint8_t id,
                                             int16_t sub_id,
                                             bool tx_buffered,
                                             double *d_data,
                                             int32_t *i_data,
                                             String *s_data) {
	bool result; // 一時使用用 bool 型変数

	/* ここに新規センサのセンサ値取得処理を追加する（ここから）*/

	if (modbus_user_sensors[port][id].xy_md02) {
		// 複数のセンサ値を取得できるデバイスは sub_id に応じて処理を変える
		// 取得したセンサ値は型に応じて引数の d_data, i_data, s_data のどれかに格納する
		switch (sub_id) {
		case 0:
			// 湿度取得
			result = modbus_user_sensors[port][id].xy_md02->fetchHumidity(id, d_data);
			if (result) {
				// 成功した場合は取得したセンサ値の型を返す
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED; // 取得に失敗した場合は MODBUS_RES_FAILED を返す

		case 1:
			// 温度取得
			result = modbus_user_sensors[port][id].xy_md02->fetchTemperature(id, d_data);
			if (result) {
				// 成功した場合は取得したセンサ値の型を返す
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;

		default:
			return MODBUS_RES_FAILED; // 取得に失敗した場合は MODBUS_RES_FAILED を返す
		}
	}

	/* ここに新規センサのセンサ値取得処理を追加する（ここまで）*/

	return MODBUS_RES_FAILED;
}