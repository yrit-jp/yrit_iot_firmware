#include "modbus_manager.h"

// Modbus sensor library
#include "km_n1_flk.h"
#include "kw7m.h"
#include "sen0604.h"
#include "sensing_eye_73x.h"
#include "wld_pa.h"
#include "ypms-482d.h"
#include "ypms-482p.h"

class Modbus_Sensor_List {
public:
	// Supported sensors
	std::unique_ptr<KW7M> kw7m;
	std::unique_ptr<KM_N1_FLK> km_n1_flk;
	std::unique_ptr<YPMS_482P> ypms_482p;
	std::unique_ptr<SENSING_EYE_73X> sensing_eye_73x;
	std::unique_ptr<SEN0604> sen0604;
	std::unique_ptr<WLD_PA> wld_pa;
	std::unique_ptr<YPMS_482D> ypms_482d;
	std::unique_ptr<XY_MD02> xy_md02;
};

// port, id, unique_ptr
std::map<int, std::map<int, Modbus_Sensor_List>> modbus_sensors;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Modbus Sensor Control Function
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// MARK: initializeDevice
// ==================================================================
//! @brief Modbus デバイスの初期化を行います
//! @param [in] (port) 使用する I/O ポート
//! @param [in] (id) 初期化する Modbus デバイスのユニット番号
//! @return MODBUS_INIT_RESULT
// ==================================================================
MODBUS_INIT_RESULT Modbus_Manager::initializeDevice(uint8_t port, uint8_t id) {

	// ユーザ追加センサの初期化
	MODBUS_INIT_RESULT user_init = MODBUS_IGNORE_DEVICE;
	user_init                    = initializeUserModbusDevice(port, id);
	if (user_init != MODBUS_IGNORE_DEVICE) {
		return user_init;
	}

	// パラメータの取得
	auto _serial      = HAL.Modbus.getSerial(port);
	auto timeout_ms   = HAL.Modbus.getTimeoutMs(port);
	auto bus_reset_ms = timeout_ms < 200 ? 200 : timeout_ms;

	// バス状態の安定化用、Serialリセットでも良いかも
	HAL.delay(bus_reset_ms);

	if (!modbus_sensors[port][id].kw7m) {
		// Panasonic Eco-Power meter
		modbus_sensors[port][id].kw7m.reset(new KW7M(_serial));
		if (modbus_sensors[port][id].kw7m->begin(id, timeout_ms) == true) {
			// connect ok
			return MODBUS_2_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].kw7m.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].km_n1_flk) {
		// Omron small power monitor
		modbus_sensors[port][id].km_n1_flk.reset(new KM_N1_FLK(_serial));
		if (modbus_sensors[port][id].km_n1_flk->begin(id, timeout_ms) == true) {
			// connect ok
			return MODBUS_2_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].km_n1_flk.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].ypms_482p) {
		// Yamagata TOA DKK pH/ORP monitor
		modbus_sensors[port][id].ypms_482p.reset(new YPMS_482P(_serial));
		if (modbus_sensors[port][id].ypms_482p->begin(id, timeout_ms) == true) {
			// connect ok
			return MODBUS_1_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].ypms_482p.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].sensing_eye_73x) {
		// Techno-morioka EC monitor
		modbus_sensors[port][id].sensing_eye_73x.reset(new SENSING_EYE_73X(_serial));
		if (modbus_sensors[port][id].sensing_eye_73x->begin(id, timeout_ms) == true) {
			return MODBUS_2_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].sensing_eye_73x.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].sen0604) {
		// DFRobot SEN0604 Humid/Temp/EC/pH
		modbus_sensors[port][id].sen0604.reset(new SEN0604(_serial));
		if (modbus_sensors[port][id].sen0604->begin(id, timeout_ms) == true) {
			return MODBUS_4_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].sen0604.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].wld_pa) {
		// Watanabe WLD-PAXXR power monitor
		modbus_sensors[port][id].wld_pa.reset(new WLD_PA(_serial));
		if (modbus_sensors[port][id].wld_pa->begin(id, timeout_ms) == true) {
			return MODBUS_2_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].wld_pa.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].ypms_482d) {
		// Yamagata TOA DKK DO monitor
		modbus_sensors[port][id].ypms_482d.reset(new YPMS_482D(_serial));
		if (modbus_sensors[port][id].ypms_482d->begin(id, timeout_ms) == true) {
			// connect ok
			return MODBUS_5_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].ypms_482d.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	if (!modbus_sensors[port][id].xy_md02) {
		// XY-MD02
		modbus_sensors[port][id].xy_md02.reset(new XY_MD02(_serial));
		if (modbus_sensors[port][id].xy_md02->begin(id, timeout_ms) == true) {
			// connect ok
			return MODBUS_2_DATA_DEVICE;
		} else {
			modbus_sensors[port][id].xy_md02.reset();
			HAL.delay(bus_reset_ms); // バス状態の安定化用
		}
	}

	return MODBUS_IGNORE_DEVICE;
}

// MARK: getDeviceName
// ==================================================================
//! @brief Modbus デバイスの識別名を返します
//! @param [in] (port) 使用する I/O ポート
//! @param [in] (id) 初期化する Modbus デバイスのユニット番号
//! @param [in] (sub_id) 複数データを持つデバイスの場合の詳細識別用 ID
//! @return デバイスの識別名の文字列
// ==================================================================
String Modbus_Manager::getDeviceName(uint8_t port, uint8_t id, int16_t sub_id) {

	// ユーザ追加センサの名前取得
	String user_device_name = "";
	user_device_name        = getUserModbusDeviceName(port, id, sub_id);
	if (user_device_name.length() != 0) {
		return user_device_name;
	}

	if (modbus_sensors[port][id].kw7m) {
		// Panasonic Eco-Power meter
		//@ url: https://industry.panasonic.com/jp/ja/products/fasys/eco/eco/kw7m
		switch (sub_id) {
		case 0:
			return "KW7M_KWH_(" + String(id) + ")";
		case 1:
			return "KW7M_KW_(" + String(id) + ")";
		default:
			return "KW7M";
		}
	}

	if (modbus_sensors[port][id].km_n1_flk) {
		// Omron small power monitor
		//@ url: https://www.fa.omron.co.jp/products/family/3413/
		switch (sub_id) {
		case 0:
			return "KM_N1_FLK_KWH_(" + String(id) + ")";
		case 1:
			return "KM_N1_FLK_W_(" + String(id) + ")";
		default:
			return "KM_N1_FLK";
		}
	}

	if (modbus_sensors[port][id].ypms_482p) {
		// Yamagata TOA DKK pH/ORP monitor
		//@ note: ORP 版は未対応
		//@ url: https://www.y-dkk.com/shopytd/products/detail/37
		switch (sub_id) {
		case 0:
			// pH か ORP か判別ができないため、センサ種別は無し
			return "YPMS_482P_(" + String(id) + ")";
		default:
			return "YPMS_482P";
		}
	}

	if (modbus_sensors[port][id].sensing_eye_73x) {
		// Techno-morioka EC monitor
		//@ note: 電気伝導率か電気抵抗率かの自動判別はできない
		//@ url: https://techmo.co.jp/sensing_eye/products/electrical_conductivity_7732/
		//@ url: https://techmo.co.jp/sensing_eye/products/electrical_resistivity_7731/
		switch (sub_id) {
		case 0:
			return "SENSING_EYE_73X_EC_(" + String(id) + ")";
		case 1:
			return "SENSING_EYE_73X_Temp_(" + String(id) + ")";
		default:
			return "SENSING_EYE_73X";
		}
	}

	if (modbus_sensors[port][id].sen0604) {
		// DFRobot 土壌センサ
		//@ url: https://www.switch-science.com/products/9674
		switch (sub_id) {
		case 0:
			return "SEN0604_Moisture_(" + String(id) + ")";
		case 1:
			return "SEN0604_Temp_(" + String(id) + ")";
		case 2:
			return "SEN0604_EC_(" + String(id) + ")";
		case 3:
			return "SEN0604_pH_(" + String(id) + ")";
		default:
			return "SEN0604";
		}
	}

	if (modbus_sensors[port][id].wld_pa) {
		// Watanabe WLD-PAXXR power monitor
		//@ url: https://www.watanabe-electric.co.jp/products/detail/13482/
		switch (sub_id) {
		case 0:
			return "WLD-PA_R_KWH_(" + String(id) + ")";
		case 1:
			return "WLD-PA_R_W_(" + String(id) + ")";
		default:
			return "WLD-PA_R";
		}
	}

	if (modbus_sensors[port][id].ypms_482d) {
		// Yamagata TOA DKK DO monitor
		//@ url: https://www.y-dkk.com/shopytd/products/detail/40
		switch (sub_id) {
		case 0:
			// 溶存酸素量
			return "YPMS_482D_DO_(" + String(id) + ")";
		case 1:
			// 酸素濃度
			return "YPMS_482D_%O2_(" + String(id) + ")";
		case 2:
			// 酸素飽和度
			return "YPMS_482D_%SAT_(" + String(id) + ")";
		case 3:
			// 大気圧
			return "YPMS_482D_hPa_(" + String(id) + ")";
		case 4:
			// 温度
			return "YPMS_482D_\'C_(" + String(id) + ")";
		default:
			return "YPMS_482D";
		}
	}

	if (modbus_sensors[port][id].xy_md02) {
		// XY-MD02
		//@ url: https://www.idbsmart.cz/wp-content/uploads/2024/05/xy-md02-manual.pdf
		switch (sub_id) {
		case 0:
			return "XY-MD02_Humid_(" + String(id) + ")";
		case 1:
			return "XY-MD02_Temp_(" + String(id) + ")";
		default:
			return "XY-MD02";
		}
	}

	return "Unknown_device";
}

// MARK: getDeviceUnit
// ==================================================================
//! @brief Modbus デバイスが持つデータの単位を返します
//! @param [in] (port) 使用する I/O ポート
//! @param [in] (id) 初期化する Modbus デバイスのユニット番号
//! @param [in] (sub_id) 複数データを持つデバイスの場合の詳細識別用 ID
//! @return 単位の文字列
// ==================================================================
String Modbus_Manager::getDeviceUnit(uint8_t port, uint8_t id, int16_t sub_id) {

	// ユーザ追加センサの単位取得
	String user_device_unit = "";
	user_device_unit        = getUserModbusDeviceUnit(port, id, sub_id);
	if (user_device_unit.length() != 0) {
		return user_device_unit;
	}

	if (modbus_sensors[port][id].kw7m) {
		// Panasonic Eco-Power meter
		switch (sub_id) {
		case 0:
			//@ label: 積算電力量
			return "kWh";
		case 1:
			//@ label: 瞬間電力
			return "kW";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].km_n1_flk) {
		// Omron small power monitor
		switch (sub_id) {
		case 0:
			//@ label: 積算電力量
			return "kWh";
		case 1:
			//@ label: 瞬間電力
			return "W";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].ypms_482p) {
		// Yamagata TOA DKK pH/ORP monitor
		switch (sub_id) {
		case 0:
			//@ label: 水素イオン濃度
			return "pH";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].sensing_eye_73x) {
		// Techno-morioka EC monitor
		switch (sub_id) {
		case 0:
			//@ label: 電気伝導率・電気抵抗率
			return "";
		case 1:
			//@ label: 温度
			return "\'C";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].sen0604) {
		// DFRobot 土壌センサ
		switch (sub_id) {
		case 0:
			//@ label: 相対湿度
			return "%RH";
		case 1:
			//@ label: 温度
			return "\'C";
		case 2:
			//@ label: 電気伝導度
			return "uS/cm";
		case 3:
			//@ label: 水素イオン濃度
			return "pH";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].wld_pa) {
		// Watanabe WLD-PAXXR power monitor
		switch (sub_id) {
		case 0:
			//@ label: 積算電力量
			return "kWh";
		case 1:
			//@ label: 瞬間電力
			return "W";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].ypms_482d) {
		// Yamagata TOA DKK DO monitor
		switch (sub_id) {
		case 0:
			//@ label: 溶存酸素量
			return "mg/L";
		case 1:
			//@ label: 酸素濃度
			return "%O2";
		case 2:
			//@ label: 酸素飽和度
			return "%SAT";
		case 3:
			//@ label: 大気圧
			return "hPa";
		case 4:
			//@ label: 温度
			return "\'C";
		default:
			return "";
		}
	}

	if (modbus_sensors[port][id].xy_md02) {
		// XY-MD02
		switch (sub_id) {
		case 0:
			//@ label: 相対湿度
			return "%RH";
		case 1:
			//@ label: 温度
			return "\'C";
		default:
			return "";
		}
	}

	return "";
}

// MARK: fetchData
// ==================================================================
//! @brief Modbus デバイスからデータを取得します
//! @param [in] (port) 使用する I/O ポート
//! @param [in] (id) 初期化する Modbus デバイスのユニット番号
//! @param [in] (sub_id) 複数データを持つデバイスの場合の詳細識別用 ID
//! @param [in] (tx_buffered) 前回のデータが送信用のバッファに格納された場合は true
//! @param [out] (d_data) 返却する double 型のデータ
//! @param [out] (i_data) 返却する int 型のデータ
//! @param [out] (s_data) 返却する String 型のデータ
//! @return MODBUS_DATA_RESULT
// ==================================================================
MODBUS_DATA_RESULT Modbus_Manager::fetchData(uint8_t port,
                                             uint8_t id,
                                             int16_t sub_id,
                                             bool tx_buffered,
                                             double *d_data,
                                             int32_t *i_data,
                                             String *s_data) {

	bool result = false;

	// ユーザ追加センサのセンサ値取得
	MODBUS_DATA_RESULT user_result = MODBUS_RES_FAILED;
	user_result                    = fetchUserModbusDeviceData(port,
	                                                           id,
	                                                           sub_id,
	                                                           tx_buffered,
	                                                           d_data,
	                                                           i_data,
	                                                           s_data);
	if (user_result != MODBUS_RES_FAILED) {
		return user_result;
	}

	if (modbus_sensors[port][id].kw7m) {
		// Panasonic Eco-Power meter
		switch (sub_id) {
		case 0:
			// kWh
			result = modbus_sensors[port][id].kw7m->fetchIntegralEP(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 1:
			// kW
			result = modbus_sensors[port][id].kw7m->fetchInstantaneousEP(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		default:
			return MODBUS_RES_FAILED;
		}
	}

	if (modbus_sensors[port][id].km_n1_flk) {
		// Omron small power monitor
		switch (sub_id) {
		case 0:
			// kWh
			result = modbus_sensors[port][id].km_n1_flk->fetchIntegralEP(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 1:
			// kW
			result = modbus_sensors[port][id].km_n1_flk->fetchActivePower(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		default:
			return MODBUS_RES_FAILED;
		}
	}

	if (modbus_sensors[port][id].ypms_482p) {
		// Yamagata TOA DKK pH/ORP monitor
		result = modbus_sensors[port][id].ypms_482p->fetchPh(id, d_data);
		if (result) {
			return MODBUS_RES_DOUBLE;
		}
		return MODBUS_RES_FAILED;
	}

	if (modbus_sensors[port][id].sensing_eye_73x) {
		// Techno-morioka EC monitor
		switch (sub_id) {
		case 0:
			// EC
			result = modbus_sensors[port][id].sensing_eye_73x->fetchWaterQuality(id, i_data);
			if (result) {
				return MODBUS_RES_INT;
			}
			return MODBUS_RES_FAILED;
		case 1:
			// Temp
			result = modbus_sensors[port][id].sensing_eye_73x->fetchWaterTemperature(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		default:
			return MODBUS_RES_FAILED;
		}
	}

	if (modbus_sensors[port][id].sen0604) {
		switch (sub_id) {
		case 0:
			// Humid
			result = modbus_sensors[port][id].sen0604->fetchMoisture(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 1:
			// Temp
			result = modbus_sensors[port][id].sen0604->fetchTemperature(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 2:
			// EC
			result = modbus_sensors[port][id].sen0604->fetchElectricalConductivity(id, i_data);
			if (result) {
				return MODBUS_RES_INT;
			}
			return MODBUS_RES_FAILED;
		case 3:
			// pH
			result = modbus_sensors[port][id].sen0604->fetchPh(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		default:
			return MODBUS_RES_FAILED;
		}
	}

	if (modbus_sensors[port][id].wld_pa) {
		// Watanabe WLD-PAXXR power monitor
		switch (sub_id) {
		case 0:
			// kWh
			result = modbus_sensors[port][id].wld_pa->fetchIntegralEP(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 1:
			// W
			result = modbus_sensors[port][id].wld_pa->fetchInstantaneousEP(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		default:
			return MODBUS_RES_FAILED;
		}
	}

	if (modbus_sensors[port][id].ypms_482d) {
		// Yamagata TOA DKK DO monitor
		switch (sub_id) {
		case 0:
			result = modbus_sensors[port][id].ypms_482d->fetchDO(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 1:
			result = modbus_sensors[port][id].ypms_482d->fetchOxygenConcentration(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 2:
			result = modbus_sensors[port][id].ypms_482d->fetchOxygenSaturation(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 3:
			result = modbus_sensors[port][id].ypms_482d->fetchPressure(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		case 4:
			result = modbus_sensors[port][id].ypms_482d->fetchTemperature(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;
		default:
			return MODBUS_RES_FAILED;
		}
	}

	if (modbus_sensors[port][id].xy_md02) {
		switch (sub_id) {
		case 0:
			// 湿度
			result = modbus_user_sensors[port][id].xy_md02->fetchHumidity(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;

		case 1:
			// 温度
			result = modbus_user_sensors[port][id].xy_md02->fetchTemperature(id, d_data);
			if (result) {
				return MODBUS_RES_DOUBLE;
			}
			return MODBUS_RES_FAILED;

		default:
			return MODBUS_RES_FAILED;
		}
	}

	return MODBUS_RES_FAILED;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Logic Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// ==================================================================
//! @brief Modbus_Manager コンストラクタ
// ==================================================================
Modbus_Manager::Modbus_Manager() {
	_active_device.reserve(16); // for memory efficient
}

// ==================================================================
//! @brief Modbus ポートを追加します
//! @param [in] (port) 追加するポート番号
// ==================================================================
void Modbus_Manager::addPort(uint8_t port) {
	_ports.insert(port);
}

// ==================================================================
//! @brief Modbus ポートをクリアします
// ==================================================================
void Modbus_Manager::clearPorts() {
	_ports.clear();
}

// ==================================================================
//! @brief アクティブなデバイスのリストをクリアします
// ==================================================================
void Modbus_Manager::clearDevices() {
	_active_device.clear();
}

// ==================================================================
//! @brief アクティブなデバイスのリストを取得します
//! @return アクティブなデバイスのリスト（ModbusActiveDevice）
// ==================================================================
std::vector<ModbusActiveDevice> Modbus_Manager::getIdList() {
	return _active_device;
}

// ==================================================================
//! @brief Modbus をスキャンし、デバイスを検出します
//! @param [in] (port) スキャンするポート番号
//! @param [in] (min_id) 探索する ID の最小番号
//! @param [in] (max_id) 探索する ID の最大番号
//! @note 全範囲の ID 探索には時間がかかるため、範囲を制限可能にした
// ==================================================================
void Modbus_Manager::scanDevice(uint8_t port, uint8_t min_id, uint8_t max_id) {
	auto _serial = HAL.Modbus.getSerial(port);
	DFRobot_RTU modbus(_serial);

	// Validation range of min_id
	if (min_id < MODBUS_ID_MINIMUM) {
		min_id = MODBUS_ID_MINIMUM;
	}
	if (MODBUS_ID_MAXIMUM < min_id) {
		min_id = MODBUS_ID_MAXIMUM;
	}

	// Validation range of max_id
	if (max_id < MODBUS_ID_MINIMUM) {
		max_id = MODBUS_ID_MINIMUM;
	}
	if (MODBUS_ID_MAXIMUM < max_id) {
		max_id = MODBUS_ID_MAXIMUM;
	}

	// Validation
	if (max_id < min_id) {
		std::swap(min_id, max_id);
	}

	int index = _active_device.size();

	constexpr uint16_t DUMMY_ADDRESS = 0x0000;
	uint16_t data                    = 0;
	for (uint8_t id = min_id; id <= max_id; id++) {
		uint8_t err = modbus.readHoldingRegister(id,
		                                         DUMMY_ADDRESS,
		                                         &data,
		                                         1);

		// HAL.printf("\n%d ==> %d\n", id, (int)err);
		if (err != 0x09 && err != 0x0A) {
			// 0x09: eRTU_RECV_ERROR
			// oxoA: eRTU_MEMORY_ERROR
			_active_device.emplace_back();
			_active_device[index].id   = id;
			_active_device[index].port = port;
			index++;
			continue;
		}

		// HoldingRegister だけでは失敗する場合があるので、
		// InputRegister の読取りも試してみる
		err = modbus.readInputRegister(id,
		                               DUMMY_ADDRESS,
		                               &data,
		                               1);
		// HAL.printf("\n%d ==> %d\n", id, (int)err);
		if (err != 0x09 && err != 0x0A) {
			// 0x09: eRTU_RECV_ERROR
			// oxoA: eRTU_MEMORY_ERROR
			_active_device.emplace_back();
			_active_device[index].id   = id;
			_active_device[index].port = port;
			index++;
			continue;
		}
	}
}

// ==================================================================
//! @brief Modbus デバイスの探索結果を表示します
// ==================================================================
void Modbus_Manager::displayScanResult() {
	HAL.println("============== Scan modbus device result ===============");
	HAL.println("--------------------------------------------------------");
	HAL.printf("| %-40s| port |  id |\n", "Device name");
	HAL.println("|-----------------------------------------+------+-----|");
	for (int i = 0; i < _active_device.size(); i++) {
		HAL.printf("| %-40s|  %2d  | %3d |\n",
		           getDeviceName(_active_device[i].port, _active_device[i].id, -1).c_str(),
		           _active_device[i].port,
		           _active_device[i].id);
	}
	HAL.println("--------------------------------------------------------");
}