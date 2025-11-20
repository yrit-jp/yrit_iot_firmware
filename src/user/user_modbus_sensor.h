#ifndef USER_MODBUS_MANAGER_H
#define USER_MODBUS_MANAGER_H

#include "../modbus/modbus_type_def.h"

/*! ==============================================================
 * @brief MODBUS デバイスの初期化を行います（ユーザ追加デバイス用）
 * @param[in] (port) Modbus ポート番号
 * @param[in] (id) 初期化する Modbus デバイスのユニット番号
 * @return 初期化結果とそのセンサから取得できるデータ種別数を返します。
 * @see enum MODBUS_INIT_RESULT
 * ============================================================= */
MODBUS_INIT_RESULT initializeUserModbusDevice(uint8_t port, uint8_t id);

/*! ==============================================================
 * @brief MODBUS センサの名前を取得します（ユーザ追加デバイス用）
 * @param[in] (port) Modbus ポート番号
 * @param[in] (id) 初期化する Modbus デバイスのユニット番号
 * @param[in] (sub_id) 取得データ識別用 ID
 * @return MODBUS センサの名前を表す String 型の文字列を返します。
 * ============================================================= */
String getUserModbusDeviceName(uint8_t port, uint8_t id, int16_t sub_id);

/*! ==============================================================
 * @brief MODBUS センサ値の単位を取得します（ユーザ追加デバイス用）
 * @param[in] (port) Modbus ポート番号
 * @param[in] (id) 初期化する Modbus デバイスのユニット番号
 * @param[in] (sub_id) 取得データ識別用 ID
 * @return
 * ============================================================= */
String getUserModbusDeviceUnit(uint8_t port, uint8_t id, int16_t sub_id);

/*! ==============================================================
 * @brief MODBUS センサ値を取得します（ユーザ追加デバイス用）
 * @param[in] (port) Modbus ポート番号
 * @param[in] (id) 初期化する Modbus デバイスのユニット番号
 * @param[in] (sub_id) 取得データ識別用 ID
 * @param[in] (tx_buffered) 前回値が送信バッファに格納された場合 true, それ以外は false
 * @param[out] (*d_data) 取得した double 型のセンサ値を格納するポインタ
 * @param[out] (*i_data) 取得した int 型のセンサ値を格納するポインタ
 * @param[out] (*s_data) 取得した String 型のセンサ値を格納するポインタ
 * @return 取得結果と取得したデータ型を返します。
 * @see enum MODBUS_DATA_RESULT
 * ============================================================= */
MODBUS_DATA_RESULT fetchUserModbusDeviceData(uint8_t port,
                                             uint8_t id,
                                             int16_t sub_id,
                                             bool tx_buffered,
                                             double *d_data,
                                             int32_t *i_data,
                                             String *s_data);

#endif