#ifndef USER_I2C_MANAGER_H
#define USER_I2C_MANAGER_H

#include "../i2c/i2c_type_def.h"

/*! ==============================================================
 * @brief I2C デバイスの初期化を行います（ユーザ追加デバイス用）
 * @param[in] (port) I2C ポート番号
 * @param[in] (address) ターゲット I2C スレーブアドレス
 * @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
 * @return 初期化結果とそのセンサから取得できるデータ種別数を返します。
 * @see enum I2C_INIT_RESULT
 * ============================================================= */
I2C_INIT_RESULT initializeUserI2cDevice(uint8_t port, uint8_t address, bool is_on_top_bus);

/*! ==============================================================
 * @brief I2C センサの名前を取得します（ユーザ追加デバイス用）
 * @param[in] (port) I2C ポート番号
 * @param[in] (address) ターゲット I2C スレーブアドレス
 * @param[in] (sub_id) 取得データ識別用 ID
 * @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
 * @return I2C センサの名前を表す String 型の文字列を返します。
 * ============================================================= */
String getUserI2cDeviceName(uint8_t port, uint8_t address, int16_t sub_id, bool is_on_top_bus);

/*! ==============================================================
 * @brief I2C センサ値の単位を取得します（ユーザ追加デバイス用）
 * @param[in] (port) I2C ポート番号
 * @param[in] (address) ターゲット I2C スレーブアドレス
 * @param[in] (sub_id) 取得データ識別用 ID
 * @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
 * @return
 * ============================================================= */
String getUserI2cDeviceUnit(uint8_t port, uint8_t address, int16_t sub_id, bool is_on_top_bus);

/*! ==============================================================
 * @brief I2C センサ値を取得します（ユーザ追加デバイス用）
 * @param[in] (port) I2C ポート番号
 * @param[in] (address) ターゲット I2C スレーブアドレス
 * @param[in] (sub_id) 取得データ識別用 ID
 * @param[in] (is_on_top_bus) マルチプレクサ下にある場合は false それ以外は true
 * @param[in] (tx_buffered) 前回値が送信バッファに格納された場合 true, それ以外は false
 * @param[out] (*d_data) 取得した double 型のセンサ値を格納するポインタ
 * @param[out] (*i_data) 取得した int 型のセンサ値を格納するポインタ
 * @param[out] (*s_data) 取得した String 型のセンサ値を格納するポインタ
 * @return 取得結果と取得したデータ型を返します。
 * @see enum I2C_DATA_RESULT
 * ============================================================= */
I2C_DATA_RESULT fetchUserI2cDeviceData(uint8_t port,
                                       uint8_t address,
                                       int16_t sub_id,
                                       bool is_on_top_bus,
                                       bool tx_buffered,
                                       double *d_data,
                                       int32_t *i_data,
                                       String *s_data);

#endif