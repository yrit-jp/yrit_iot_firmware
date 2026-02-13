#include "user_i2c_sensor.h"

/* インクルードファイルを追加する（ここから）*/
#include "./user_sensors/i2c_sensors/i2c_ct_unit.h"

/* インクルードファイルを追加する（ここまで）*/

class I2C_User_Sensor_List {
public:
	/* ここに新規センサの宣言を追加する（ここから）*/
	std::unique_ptr<I2C_CT_Unit> i2c_ct_unit; // メモリ効率化のため、unique_ptr の使用を推奨します

	/* ここに新規センサの宣言を追加する（ここまで）*/
};
std::map<int, std::map<int, I2C_User_Sensor_List>> i2c_user_sensors;

// ==============================================================
//! @brief I2C デバイスの初期化を行います（ユーザ追加デバイス用）
//! @param[in] (port) I2C ポート番号
//! @param[in] (address) ターゲット I2C スレーブアドレス
//! @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
//! @return 初期化結果とそのセンサから取得できるデータ種別数を返します。
//! @see enum I2C_INIT_RESULT
// =============================================================
I2C_INIT_RESULT initializeUserI2cDevice(uint8_t port, uint8_t address, bool is_on_top_bus) {
	auto _wire = HAL.I2c.getWire(port);

	switch (address) {
		/* ここに新規センサの初期化処理を追加する（ここから）*/

	case 0x3F:
		if (!i2c_user_sensors[port][address].i2c_ct_unit) {
			// I2C_CT Unit
			// unique_ptr を初期化する
			i2c_user_sensors[port][address].i2c_ct_unit.reset(new I2C_CT_Unit());

			// センサの初期化と識別を行う
			if (i2c_user_sensors[port][address].i2c_ct_unit->init(_wire, address) == true) {
				// 該当センサだった場合は取得可能なセンサ数を返す
				return I2C_1_DATA_DEVICE;
			} else {
				// 該当センサではなかった場合は unique_ptr をリセットし、メモリを解放する
				i2c_user_sensors[port][address].i2c_ct_unit.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;

		/* ここに新規センサの初期化処理を追加する（ここまで）*/

	default:
		break;
	}

	return I2C_IGNORE_DEVICE;
}

// ==============================================================
//! @brief I2C センサの名前を取得します（ユーザ追加デバイス用）
//! @param[in] (port) I2C ポート番号
//! @param[in] (address) ターゲット I2C スレーブアドレス
//! @param[in] (sub_id) 取得データ識別用 ID
//! @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
//! @return I2C センサの名前を表す String 型の文字列を返します。
// =============================================================
String getUserI2cDeviceName(uint8_t port, uint8_t address, int16_t sub_id, bool is_on_top_bus) {
	char s_addr[5] = {0};
	snprintf(s_addr, 5, "0x%02X", address);

	switch (address) {
		/* ここに新規センサの名前取得処理を追加する（ここから）*/

	case 0x3F:
		if (i2c_user_sensors[port][address].i2c_ct_unit) {
			// I2C_CT Unit
			// 複数のセンサ値を取得できるデバイスは sub_id に応じて名前を変える
			// 「センサ名_センサ種別_(I2Cアドレス)」の命名規則を推奨
			// sub_id が範囲外の場合はセンサ名だけを返すことが望ましい
			switch (sub_id) {
			case 0:
				return "I2C_CT_Unit_(" + String(s_addr) + ")";
			default:
				return "I2C_CT_Unit";
			}
		}
		break;

		/* ここに新規センサの名前取得処理を追加する（ここまで）*/

	default:
		break;
	}

	return "";
}

// ==============================================================
//! @brief I2C センサ値の単位を取得します（ユーザ追加デバイス用）
//! @param[in] (port) I2C ポート番号
//! @param[in] (address) ターゲット I2C スレーブアドレス
//! @param[in] (sub_id) 取得データ識別用 ID
//! @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
//! @return
// =============================================================
String getUserI2cDeviceUnit(uint8_t port, uint8_t address, int16_t sub_id, bool is_on_top_bus) {

	switch (address) {
		/* ここに新規センサの単位取得処理を追加する（ここから）*/

	case 0x3F:
		if (i2c_user_sensors[port][address].i2c_ct_unit) {
			// I2C_CT Unit
			// 複数のセンサ値を取得できるデバイスは sub_id に応じて単位を変える
			switch (sub_id) {
			case 0:
				return "A";
			default:
				return "";
			}
		}
		break;

		/* ここに新規センサの単位取得処理を追加する（ここまで）*/

	default:
		break;
	}

	return "";
}

// ==============================================================
//! @brief I2C センサ値を取得します（ユーザ追加デバイス用）
//! @param[in] (port) I2C ポート番号
//! @param[in] (address) ターゲット I2C スレーブアドレス
//! @param[in] (sub_id) 取得データ識別用 ID
//! @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
//! @param[in] (tx_buffered) 前回値が送信バッファに格納された場合 true, それ以外は false
//! @param[out] (*d_data) 取得した double 型のセンサ値を格納するポインタ
//! @param[out] (*i_data) 取得した int 型のセンサ値を格納するポインタ
//! @param[out] (*s_data) 取得した String 型のセンサ値を格納するポインタ
//! @return 取得結果と取得したデータ型を返します。
//! @see enum I2C_DATA_RESULT
// =============================================================
I2C_DATA_RESULT fetchUserI2cDeviceData(uint8_t port,
                                       uint8_t address,
                                       int16_t sub_id,
                                       bool is_on_top_bus,
                                       bool tx_buffered,
                                       double *d_data,
                                       int32_t *i_data,
                                       String *s_data) {
	bool result;         // 一時使用用 bool 型変数
	uint32_t ui_tmp = 0; // 一時使用用 int 型変数

	switch (address) {
		/* ここに新規センサのセンサ値取得処理を追加する（ここから）*/

	case 0x3F:
		if (i2c_user_sensors[port][address].i2c_ct_unit) {
			// I2C_CT Unit
			// 複数のセンサ値を取得できるデバイスは sub_id に応じて処理を変える
			// 取得したセンサ値は型に応じて引数の d_data, i_data, s_data のどれかに格納する
			result = i2c_user_sensors[port][address].i2c_ct_unit->fetchMilliAmpere(&ui_tmp);
			if (result) {
				*i_data = ui_tmp;
				return I2C_RES_INT; // 成功した場合は取得したセンサ値の型を返す
			}
			return I2C_RES_FAILED; // 取得に失敗した場合は I2C_RES_FAILED を返す
		}

		/* ここに新規センサのセンサ値取得処理を追加する（ここまで）*/

	default:
		break;
	}

	return I2C_RES_FAILED;
}