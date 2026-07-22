#include "i2c_manager.h"

#include <Regexp.h>

// I2C sensor library
#include "ADS1100.h"
#include "ATMICL_ONE.h"
#include "Adafruit_SGP30.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_TCS34725.h"
#include "AtlasScientific_EZO_DO.h"
#include "BH1750.h"
#include "Bme280.h"
#include "DFRobot_ADS1115.h"
#include "DFRobot_AHT20.h"
#include "DS2485.h"
#include "DS248X.h"
#include "DT_S1.h"
#include "I2C_GroveWaterLevelSensor.h"
#include "LTR_553ALS.h"
#include "M5AmMeterUnit.h"
#include "M5UnitWeightI2C.h"
#include "M5VoltMeterUnit.h"
#include "M5_EXTIO2.h"
#include "M5_KMeter.h"
#include "MCP23017.h"
#include "MCP9600.h"
#include "MFRC522_I2C.h"
#include "MODULE_4_20MA.h"
#include "OW_DS18XXX.h"
#include "OW_MAX3185.h"
#include "PCA9554.h"
#include "QMP6988.h"
#include "RCWL9620.h"
#include "SC16IS7XX.h"
#include "SensirionI2CSen5x.h"
#include "SensirionI2cSht4x.h"
#include "SparkFun_I2C_Mux_Arduino_Library.h"
#include "SparkFun_SCD30_Arduino_Library.h"
#include "SparkFun_SCD4x_Arduino_Library.h"
#include "TSL2561.h"
#include "UNIT_KMETER.h"
#include "VL53L0X.h"
#include "VL53L1X.h"
#include "bme68xLibrary.h"

class I2C_Sensor_List {
public:
	// Supported sensors
	std::unique_ptr<VL53L0X> vl53l0x;
	std::unique_ptr<Adafruit_SHT31> sht3x;
	std::unique_ptr<Bme280TwoWire> bmp280;
	std::unique_ptr<GroveWaterLevelSensor> gwls;
	std::unique_ptr<MCP23017> mcp23017;
	std::unique_ptr<Adafruit_TCS34725> tcs34725;
	std::unique_ptr<ADS1100> ads1100;
	std::unique_ptr<Adafruit_SGP30> sgp30;
	std::unique_ptr<QWIICMUX> tca9548a;
	std::unique_ptr<MFRC522> mfrc522;
	std::unique_ptr<TSL2561> tsl2561;
	std::unique_ptr<QMP6988> qmp6988;
	std::unique_ptr<PCA9554> pca9554;
	std::unique_ptr<SCD30> scd30;
	std::unique_ptr<SCD4x> scd4x;
	std::unique_ptr<ATMICL_ONE> ands;
	std::unique_ptr<ATMICL_ONE> an16;
	std::unique_ptr<ATMICL_ONE> sns2;
	std::unique_ptr<Bme280TwoWire> bme280;
	std::unique_ptr<RCWL9620> rcwl9620;
	std::unique_ptr<SC16IS7XX> sc16is7xx;
	std::unique_ptr<DFRobot_ADS1115> ads1115;
	std::unique_ptr<M5_KMeter> m5_kmeter;
	std::unique_ptr<MCP9600> mcp9600;
	std::unique_ptr<DT_S1> dt_s1;
	std::unique_ptr<EZO_DO> ezo_do;
	std::unique_ptr<BH1750> bh1750fvi;
	std::unique_ptr<M5_EXTIO2> m5_extio2;
	std::unique_ptr<M5AmMeterUnit> m5_am_meter;
	// std::unique_ptr<SensirionI2CSen5x> sen50;
	std::unique_ptr<SensirionI2CSen5x> sen54;
	// std::unique_ptr<SensirionI2CSen5x> sen55;
	std::unique_ptr<MODULE_4_20MA> m5_4_20mA_unit;
	std::unique_ptr<UNIT_KMETER> m5_kmeter_iso;
	std::unique_ptr<SensirionI2cSht4x> sht4x;
	std::unique_ptr<Bme68x> bme68x;
	std::unique_ptr<M5UnitWeightI2C> m5_weight;
	std::unique_ptr<DFRobot_AHT20> ahtxx;
	std::unique_ptr<LTR_553ALS> ltr_553als;
	std::unique_ptr<VL53L1X> vl53l1x;
	std::unique_ptr<M5VoltMeterUnit> m5_volt_meter;
	// 1-Wire to I2C bridge
	std::unique_ptr<DS248X> ds248x;
	std::unique_ptr<OW_DS18XXX<DS248X>> ow_ds18xxx_ds248x;
	std::unique_ptr<OW_MAX3185<DS248X>> ow_max3185_ds248x;
	std::unique_ptr<DS2485> ds2485;
	std::unique_ptr<OW_DS18XXX<DS2485>> ow_ds18xxx_ds2485;
	std::unique_ptr<OW_MAX3185<DS2485>> ow_max3185_ds2485;
};
// port, address, bus_channel, sub_address, unique_ptr
std::map<int, std::map<int, std::map<int, std::map<int, I2C_Sensor_List>>>> i2c_sensors;

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  I2C Sensor Control Function
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// MARK: initializeDevice
I2C_INIT_RESULT I2C_Manager::initializeDevice(uint8_t port,
                                              uint8_t address,
                                              int8_t bus_channel,
                                              uint8_t sub_address,
                                              bool use_sub_addr) {
	I2C_INIT_RESULT user_init = I2C_IGNORE_DEVICE;
	uint8_t _address          = address;

	auto _wire = HAL.I2c.getWire(port);
	if (_wire == nullptr) {
		return I2C_IGNORE_DEVICE;
	}
	uint8_t _sda = HAL.I2c.getSdaPin(port);
	uint8_t _scl = HAL.I2c.getSclPin(port);

	// ユーザ追加センサの初期化
	if (use_sub_addr) {
		_address  = sub_address;
		user_init = initializeUserI2cDevice(port, _address, false);
	} else {
		user_init = initializeUserI2cDevice(port, _address, true);
	}

	if (user_init != I2C_IGNORE_DEVICE) {
		return user_init;
	}

	switch (_address) {
	case 0x09:
		if (not i2c_sensors[port][address][bus_channel][sub_address].dt_s1) {
			// flexible thermistor
			i2c_sensors[port][address][bus_channel][sub_address].dt_s1.reset(new DT_S1(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].dt_s1->begin(_address)) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].dt_s1.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x18:
		// DS248X: 1-Wire to I2C bridge と配下の 1-wire デバイスを初期化
		if (not i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			// DS248X を初期化
			i2c_sensors[port][address][bus_channel][sub_address].ds248x.reset(new DS248X(_wire, _address));

			// DS18XXX センサーの初期化
			if (not i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x.reset(
				    new OW_DS18XXX<DS248X>(i2c_sensors[port][address][bus_channel][sub_address].ds248x.get()));

				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->begin()) {
					// OneWire DS18XXX
					uint16_t num = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getOneWireDeviceNum();

					uint8_t ow_addr[8] = {0};
					for (int i = 0; i < num; i++) {
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(i);
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getTargetDeviceAddress(ow_addr);

						String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getFamilyName(ow_addr[0]);

						HAL.printf("\n        (%d) %s [%02X%02X%02X%02X%02X%02X%02X%02X] %s", i + 1, pname.c_str(),
						           ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7],
						           i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->isParasiteMode() ? "Parasite" : "");

						// 温度
						i2c_sensors[port][address][bus_channel][sub_address].ds248x->pushSensor(ow_addr, static_cast<uint8_t>(I2C_1_DATA_DEVICE));

						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setResolution(12);
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getTempC();
					}
					HAL.printf("\n    ");
				} else {
					// begin 失敗時は ds18xxx をリセット
					i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			}

			// MAX3185 センサーの初期化
			if (not i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x.reset(
				    new OW_MAX3185<DS248X>(i2c_sensors[port][address][bus_channel][sub_address].ds248x.get()));

				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->begin()) {
					// OneWire MAX3185
					uint16_t num = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getOneWireDeviceNum();

					uint8_t ow_addr[8] = {0};
					for (int i = 0; i < num; i++) {
						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(i);
						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getTargetDeviceAddress(ow_addr);

						String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getFamilyName(ow_addr[0]);

						HAL.printf("\n        (%d) %s [%02X%02X%02X%02X%02X%02X%02X%02X] %s", i + 1, pname.c_str(),
						           ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7],
						           i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->isParasiteMode() ? "Parasite" : "");

						// 温度、冷接点温度
						i2c_sensors[port][address][bus_channel][sub_address].ds248x->pushSensor(ow_addr, static_cast<uint8_t>(I2C_2_DATA_DEVICE));

						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->convertT();
					}
					HAL.printf("\n    ");
				} else {
					// begin 失敗時は max3185 をリセット
					i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			}

			// 初期化できた 1-wire センサ数をチェック
			uint16_t sensor_value_count = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getSensorValueCount();
			if (sensor_value_count != 0) {
				return static_cast<I2C_INIT_RESULT>(sensor_value_count);
			} else {
				// 利用可能な 1-wire デバイスが存在しなかった
				i2c_sensors[port][address][bus_channel][sub_address].ds248x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x19:
	case 0x1A:
	case 0x1B:
		// DS248X: 1-Wire to I2C bridge と配下の 1-wire デバイスを初期化
		if (not i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			// DS248X を初期化
			i2c_sensors[port][address][bus_channel][sub_address].ds248x.reset(new DS248X(_wire, _address));

			// DS18XXX センサーの初期化
			if (not i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x.reset(
				    new OW_DS18XXX<DS248X>(i2c_sensors[port][address][bus_channel][sub_address].ds248x.get()));

				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->begin()) {
					// OneWire DS18XXX
					uint16_t num = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getOneWireDeviceNum();

					uint8_t ow_addr[8] = {0};
					for (int i = 0; i < num; i++) {
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(i);
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getTargetDeviceAddress(ow_addr);

						String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getFamilyName(ow_addr[0]);

						HAL.printf("\n        (%d) %s [%02X%02X%02X%02X%02X%02X%02X%02X] %s", i + 1, pname.c_str(),
						           ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7],
						           i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->isParasiteMode() ? "Parasite" : "");

						// 温度
						i2c_sensors[port][address][bus_channel][sub_address].ds248x->pushSensor(ow_addr, static_cast<uint8_t>(I2C_1_DATA_DEVICE));

						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setResolution(12);
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getTempC();
					}
					HAL.printf("\n    ");
				} else {
					// begin 失敗時は ds18xxx をリセット
					i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			}

			// MAX3185 センサーの初期化
			if (not i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x.reset(
				    new OW_MAX3185<DS248X>(i2c_sensors[port][address][bus_channel][sub_address].ds248x.get()));

				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->begin()) {
					// OneWire MAX3185
					uint16_t num = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getOneWireDeviceNum();

					uint8_t ow_addr[8] = {0};
					for (int i = 0; i < num; i++) {
						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(i);
						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getTargetDeviceAddress(ow_addr);

						String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getFamilyName(ow_addr[0]);

						HAL.printf("\n        (%d) %s [%02X%02X%02X%02X%02X%02X%02X%02X] %s", i + 1, pname.c_str(),
						           ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7],
						           i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->isParasiteMode() ? "Parasite" : "");

						// 温度、冷接点温度
						i2c_sensors[port][address][bus_channel][sub_address].ds248x->pushSensor(ow_addr, static_cast<uint8_t>(I2C_2_DATA_DEVICE));

						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->convertT();
					}
					HAL.printf("\n    ");
				} else {
					// begin 失敗時は max3185 をリセット
					i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			}

			// 初期化できた 1-wire センサ数をチェック
			uint16_t sensor_value_count = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getSensorValueCount();
			if (sensor_value_count != 0) {
				return static_cast<I2C_INIT_RESULT>(sensor_value_count);
			} else {
				// 利用可能な 1-wire デバイスが存在しなかった
				i2c_sensors[port][address][bus_channel][sub_address].ds248x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x20:
	case 0x21:
	case 0x22:
		if (not i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset(new PCA9554(_address, _wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].pca9554->begin() == true) {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554->setAllPinMode(PCA9554::ALL_INPUT);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset(new MCP23017(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017->init(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x23:
		if (not i2c_sensors[port][address][bus_channel][sub_address].ltr_553als) {
			// LTR-553ALS
			i2c_sensors[port][address][bus_channel][sub_address].ltr_553als.reset(new LTR_553ALS(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].ltr_553als->begin(_address) == true) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].ltr_553als.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset(new PCA9554(_address, _wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].pca9554->begin() == true) {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554->setAllPinMode(PCA9554::ALL_INPUT);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi.reset(new BH1750(_address));
			// 判定方法が無いため、決め打ち
			i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi->begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
			return I2C_1_DATA_DEVICE;
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset(new MCP23017(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017->init(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x24:
	case 0x25:
		if (not i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset(new PCA9554(_address, _wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].pca9554->begin() == true) {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554->setAllPinMode(PCA9554::ALL_INPUT);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset(new MCP23017(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017->init(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x26:
		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_weight) {
			// M5 Weight I2C Unit
			i2c_sensors[port][address][bus_channel][sub_address].m5_weight.reset(new M5UnitWeightI2C());
			if (i2c_sensors[port][address][bus_channel][sub_address].m5_weight->begin(_wire, _sda, _scl) == true) {
				if (i2c_sensors[port][address][bus_channel][sub_address].m5_weight->getI2CAddress() == 0x26) {
					i2c_sensors[port][address][bus_channel][sub_address].m5_weight->setOffset();
					constexpr double CALIBRATION_VALUE = 27.6; // 試験環境下で測定した値、目安値
					i2c_sensors[port][address][bus_channel][sub_address].m5_weight->setGapValue(CALIBRATION_VALUE);
					i2c_sensors[port][address][bus_channel][sub_address].m5_weight->setLPFilter(1);
					i2c_sensors[port][address][bus_channel][sub_address].m5_weight->setAvgFilter(10);
					i2c_sensors[port][address][bus_channel][sub_address].m5_weight->setEmaFilter(10);
					// HAL.printf("\nLP F: %d\n", i2c_sensors[port][address][bus_channel][sub_address].m5_weight->getLPFilter());
					// HAL.printf("Ave F: %d\n", i2c_sensors[port][address][bus_channel][sub_address].m5_weight->getAvgFilter());
					// HAL.printf("Ema F: %d\n", i2c_sensors[port][address][bus_channel][sub_address].m5_weight->getEmaFilter());
					return I2C_2_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].m5_weight.reset();
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].m5_weight.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset(new PCA9554(_address, _wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].pca9554->begin() == true) {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554->setAllPinMode(PCA9554::ALL_INPUT);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset(new MCP23017(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017->init(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x27:
		if (not i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset(new PCA9554(_address, _wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].pca9554->begin() == true) {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554->setAllPinMode(PCA9554::ALL_INPUT);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].pca9554.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset(new MCP23017(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017->init(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].mcp23017.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x28:
		if (not i2c_sensors[port][address][bus_channel][sub_address].mfrc522) {
			i2c_sensors[port][address][bus_channel][sub_address].mfrc522.reset(new MFRC522(_address, _wire));
			i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PCD_Init();
			auto versionReg = i2c_sensors[port][address][bus_channel][sub_address].mfrc522->VersionReg;
			uint8_t v       = i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PCD_ReadRegister(versionReg);
			if ((v == 0x00) or (v == 0xFF)) {
				i2c_sensors[port][address][bus_channel][sub_address].mfrc522.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			} else {
				// return I2C_1_DATA_DEVICE;
				return I2C_2_DATA_DEVICE;
			}
		}
		break;
	case 0x29:
		if (not i2c_sensors[port][address][bus_channel][sub_address].vl53l0x) {
			// ToF Unit
			i2c_sensors[port][address][bus_channel][sub_address].vl53l0x.reset(new VL53L0X());
			i2c_sensors[port][address][bus_channel][sub_address].vl53l0x->setBus(_wire);
			if (i2c_sensors[port][address][bus_channel][sub_address].vl53l0x->init()) {
				i2c_sensors[port][address][bus_channel][sub_address].vl53l0x->setTimeout(500);
				i2c_sensors[port][address][bus_channel][sub_address].vl53l0x->startContinuous(0);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].vl53l0x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].vl53l1x) {
			// ToF Unit
			i2c_sensors[port][address][bus_channel][sub_address].vl53l1x.reset(new VL53L1X());
			i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->setBus(_wire);
			i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->setTimeout(500);
			if (i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->init()) {
				i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->setDistanceMode(VL53L1X::Long);
				i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->setMeasurementTimingBudget(50000);
				i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->startContinuous(50);
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].vl53l1x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].tcs34725) {
			// Color Unit
			i2c_sensors[port][address][bus_channel][sub_address].tcs34725.reset(new Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X));
			if (i2c_sensors[port][address][bus_channel][sub_address].tcs34725->begin(_address, _wire)) {
				i2c_sensors[port][address][bus_channel][sub_address].tcs34725->setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);
				i2c_sensors[port][address][bus_channel][sub_address].tcs34725->setGain(TCS34725_GAIN_4X);
				return I2C_4_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].tcs34725.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			// Digital Light Sensor
			i2c_sensors[port][address][bus_channel][sub_address].tsl2561.reset(new TSL2561(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561->begin(_address)) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].tsl2561.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x38:
		if (not i2c_sensors[port][address][bus_channel][sub_address].ahtxx) {
			// AHT10, AHT20
			i2c_sensors[port][address][bus_channel][sub_address].ahtxx.reset(new DFRobot_AHT20(*_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].ahtxx->begin() == 0) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].ahtxx.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x39:
		if (not i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			// Digital Light Sensor
			i2c_sensors[port][address][bus_channel][sub_address].tsl2561.reset(new TSL2561(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561->begin(_address)) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].tsl2561.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x40:
		// DS2485: 1-Wire to I2C bridge と配下の 1-wire デバイスを初期化
		if (not i2c_sensors[port][address][bus_channel][sub_address].ds2485) {
			// DS2485 を初期化
			i2c_sensors[port][address][bus_channel][sub_address].ds2485.reset(new DS2485(_wire, _address));
			i2c_sensors[port][address][bus_channel][sub_address].ds2485->deviceReset();
			i2c_sensors[port][address][bus_channel][sub_address].ds2485->setRPUP();

			// DS18XXX センサーの初期化
			if (not i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485) {
				i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485.reset(
				    new OW_DS18XXX<DS2485>(i2c_sensors[port][address][bus_channel][sub_address].ds2485.get()));

				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->begin()) {
					// OneWire DS18XXX
					uint16_t num = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getOneWireDeviceNum();

					uint8_t ow_addr[8] = {0};
					for (int i = 0; i < num; i++) {
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->setTargetDevice(i);
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getTargetDeviceAddress(ow_addr);

						String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getFamilyName(ow_addr[0]);

						HAL.printf("\n        (%d) %s [%02X%02X%02X%02X%02X%02X%02X%02X] %s", i + 1, pname.c_str(),
						           ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7],
						           i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->isParasiteMode() ? "Parasite" : "");

						// 温度
						i2c_sensors[port][address][bus_channel][sub_address].ds2485->pushSensor(ow_addr, static_cast<uint8_t>(I2C_1_DATA_DEVICE));

						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->setResolution(12);
						i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getTempC();
					}
					HAL.printf("\n    ");
				} else {
					// begin 失敗時は ds18xxx をリセット
					i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			}

			// MAX3185 センサーの初期化
			if (not i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485) {
				i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485.reset(
				    new OW_MAX3185<DS2485>(i2c_sensors[port][address][bus_channel][sub_address].ds2485.get()));

				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->begin()) {
					// OneWire MAX3185
					uint16_t num = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getOneWireDeviceNum();

					uint8_t ow_addr[8] = {0};
					for (int i = 0; i < num; i++) {
						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->setTargetDevice(i);
						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getTargetDeviceAddress(ow_addr);

						String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getFamilyName(ow_addr[0]);

						HAL.printf("\n        (%d) %s [%02X%02X%02X%02X%02X%02X%02X%02X] %s", i + 1, pname.c_str(),
						           ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7],
						           i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->isParasiteMode() ? "Parasite" : "");

						// 温度、冷接点温度
						i2c_sensors[port][address][bus_channel][sub_address].ds2485->pushSensor(ow_addr, static_cast<uint8_t>(I2C_2_DATA_DEVICE));

						i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->convertT();
					}
					HAL.printf("\n    ");
				} else {
					// begin 失敗時は max3185 をリセット
					i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			}

			// 初期化できた 1-wire センサ数をチェック
			uint16_t sensor_value_count = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getSensorValueCount();
			if (sensor_value_count != 0) {
				return static_cast<I2C_INIT_RESULT>(sensor_value_count);
			} else {
				// 利用可能な 1-wire デバイスが存在しなかった
				i2c_sensors[port][address][bus_channel][sub_address].ds2485.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].sns2) {
			// ATMICL-ONE
			i2c_sensors[port][address][bus_channel][sub_address].sns2.reset(new ATMICL_ONE(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].sns2->begin(_address)) {
				return I2C_3_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sns2.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x44:
		if (not i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X -- Digital temperature / humidity sensor
			i2c_sensors[port][address][bus_channel][sub_address].sht4x.reset(new SensirionI2cSht4x());
			i2c_sensors[port][address][bus_channel][sub_address].sht4x->begin(*_wire, _address);
			i2c_sensors[port][address][bus_channel][sub_address].sht4x->softReset();
			uint32_t serialNumber = 0;
			if (i2c_sensors[port][address][bus_channel][sub_address].sht4x->serialNumber(serialNumber) == 0) {
				// No error, get serial number OK!
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sht4x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X -- Digital temperature / humidity sensor
			i2c_sensors[port][address][bus_channel][sub_address].sht3x.reset(new Adafruit_SHT31(_wire));

			// ENV4 トライによる謎フリーズ回避用処理 - 2024/01/02
			// なぜこれでうまくいくのかは謎、単なる delay だけではダメだった
			// 適当に色々試したら安定してうまくいく方法を見つけただけ
			// Wire リセット処理があれば不要かもしれない
			_wire->endTransmission();
			delay(10);
			// 			_wire->beginTransmission(_address);
			// 			_wire->write(0x00); // 適当コマンド
			// 			_wire->write(0xFF); // 適当コマンド
			// 			_wire->endTransmission();
			// 			delay(10);
			// 			_wire->beginTransmission(_address);
			// 			_wire->write(0x30); // ソフトリセットコマンド
			// 			_wire->write(0xA2); // ソフトリセットコマンド
			// 			_wire->write(0xAD); // CRC
			// 			_wire->endTransmission();
			// 			delay(10);
			// #if defined(TARGET__M5STACK_BASIC)
			// 			_wire->beginTransmission(_address);
			// 			_wire->write(0x2C); // 単発測定コマンド
			// 			_wire->write(0x10); // 単発測定コマンド
			// 			_wire->write(0xAA); // CRC
			// 			_wire->endTransmission();
			// 			delay(10);
			// 			_wire->beginTransmission(_address);
			// 			_wire->write(0x01); // 適当コマンド
			// 			_wire->write(0x01); // 適当コマンド
			// 			_wire->endTransmission();
			// 			delay(10);
			// #endif
			// ENV4 トライによる謎フリーズ回避用処理（ここまで）

			if (i2c_sensors[port][address][bus_channel][sub_address].sht3x->begin(_address)) {
				i2c_sensors[port][address][bus_channel][sub_address].sht3x->reset();
				i2c_sensors[port][address][bus_channel][sub_address].sht3x->heater(false);
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sht3x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x45:
		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_extio2) {
			// M5 ExtI/O 2
			i2c_sensors[port][address][bus_channel][sub_address].m5_extio2.reset(new M5_EXTIO2());
			if (i2c_sensors[port][address][bus_channel][sub_address].m5_extio2->begin(_wire)) {
				if (i2c_sensors[port][address][bus_channel][sub_address].m5_extio2->getVersion() >= 1) {
					i2c_sensors[port][address][bus_channel][sub_address].m5_extio2->setAllPinMode(DIGITAL_INPUT_MODE); // Set all pins to digital input mode.
					return I2C_1_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].m5_extio2.reset();
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].m5_extio2.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X -- Digital temperature / humidity sensor
			i2c_sensors[port][address][bus_channel][sub_address].sht4x.reset(new SensirionI2cSht4x());
			i2c_sensors[port][address][bus_channel][sub_address].sht4x->begin(*_wire, _address);
			i2c_sensors[port][address][bus_channel][sub_address].sht4x->softReset();
			uint32_t serialNumber = 0;
			if (i2c_sensors[port][address][bus_channel][sub_address].sht4x->serialNumber(serialNumber) == 0) {
				// No error, get serial number OK!
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sht4x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X -- Digital temperature / humidity sensor
			i2c_sensors[port][address][bus_channel][sub_address].sht3x.reset(new Adafruit_SHT31(_wire));

			// ENV4 トライによる謎フリーズ回避用処理 - 2024/01/02
			// なぜこれでうまくいくのかは謎、単なる delay だけではダメだった
			// 適当に色々試したら安定してうまくいく方法を見つけただけ
			// Wire リセット処理があれば不要かもしれない
			_wire->endTransmission();
			delay(10);
			_wire->beginTransmission(_address);
			_wire->write(0x00); // 適当コマンド
			_wire->write(0xFF); // 適当コマンド
			_wire->endTransmission();
			delay(10);
			_wire->beginTransmission(_address);
			_wire->write(0x30); // ソフトリセットコマンド
			_wire->write(0xA2); // ソフトリセットコマンド
			_wire->write(0xAD); // CRC
			_wire->endTransmission();
			delay(10);
#if defined(TARGET__M5STACK_BASIC)
			_wire->beginTransmission(_address);
			_wire->write(0x2C); // 単発測定コマンド
			_wire->write(0x10); // 単発測定コマンド
			_wire->write(0xAA); // CRC
			_wire->endTransmission();
			delay(10);
			_wire->beginTransmission(_address);
			_wire->write(0x01); // 適当コマンド
			_wire->write(0x01); // 適当コマンド
			_wire->endTransmission();
			delay(10);
#endif
			// ENV4 トライによる謎フリーズ回避用処理（ここまで）

			if (i2c_sensors[port][address][bus_channel][sub_address].sht3x->begin(_address)) {
				i2c_sensors[port][address][bus_channel][sub_address].sht3x->reset();
				i2c_sensors[port][address][bus_channel][sub_address].sht3x->heater(false);
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sht3x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x48:
		if (not i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			// ADC Unit
			i2c_sensors[port][address][bus_channel][sub_address].ads1115.reset(new DFRobot_ADS1115(_wire));
			i2c_sensors[port][address][bus_channel][sub_address].ads1115->setAddr_ADS1115(_address);

			// Change multiplexer register for test
			i2c_sensors[port][address][bus_channel][sub_address].ads1115->setMux(eADSMUX_5);
			uint8_t mux1 = i2c_sensors[port][address][bus_channel][sub_address].ads1115->getMux();

			// Change multiplexer register for test
			i2c_sensors[port][address][bus_channel][sub_address].ads1115->setMux(eADSMUX_8);
			uint8_t mux2 = i2c_sensors[port][address][bus_channel][sub_address].ads1115->getMux();

			if (mux1 == 4 and mux2 == 7) {
				// Target is ADS1115 OK
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setGain(eGAIN_TWOTHIRDS);  // 2/3x gain
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setMode(eMODE_SINGLE);     // single-shot mode
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setRate(eRATE_128);        // 128SPS (default)
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setOSMode(eOSMODE_SINGLE); // Set to start a single-conversion
				return I2C_4_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].ads1115.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].ads1100) {
			// ADC Unit
			i2c_sensors[port][address][bus_channel][sub_address].ads1100.reset(new ADS1100(_wire));
			i2c_sensors[port][address][bus_channel][sub_address].ads1100->getAddr_ADS1100(ADS1100_DEFAULT_ADDRESS); // 0x48, 1001 000 (ADDR = GND)
			i2c_sensors[port][address][bus_channel][sub_address].ads1100->setGain(GAIN_ONE);                        // 1x gain(default)
			i2c_sensors[port][address][bus_channel][sub_address].ads1100->setMode(MODE_CONTIN);                     // Continuous conversion mode (default)
			i2c_sensors[port][address][bus_channel][sub_address].ads1100->setRate(RATE_8);                          // 8SPS (default)
			i2c_sensors[port][address][bus_channel][sub_address].ads1100->setOSMode(OSMODE_SINGLE);                 // Set to start a single-conversion

			if (i2c_sensors[port][address][bus_channel][sub_address].ads1100->getGain() != GAIN_ONE) {
				// Failed to init
				i2c_sensors[port][address][bus_channel][sub_address].ads1100.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			} else if (i2c_sensors[port][address][bus_channel][sub_address].ads1100->getMode() != MODE_CONTIN) {
				// Failed to init
				i2c_sensors[port][address][bus_channel][sub_address].ads1100.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			} else if (i2c_sensors[port][address][bus_channel][sub_address].ads1100->getRate() != RATE_8) {
				// Failed to init
				i2c_sensors[port][address][bus_channel][sub_address].ads1100.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			} else {
				return I2C_2_DATA_DEVICE;
			}
		}
		break;
	case 0x49:
		if (not i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			// Digital Light Sensor
			i2c_sensors[port][address][bus_channel][sub_address].tsl2561.reset(new TSL2561(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561->begin(_address)) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].tsl2561.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			// ADC Unit
			i2c_sensors[port][address][bus_channel][sub_address].ads1115.reset(new DFRobot_ADS1115(_wire));
			i2c_sensors[port][address][bus_channel][sub_address].ads1115->setAddr_ADS1115(_address);

			// Change multiplexer register for test
			i2c_sensors[port][address][bus_channel][sub_address].ads1115->setMux(eADSMUX_5);
			uint8_t mux1 = i2c_sensors[port][address][bus_channel][sub_address].ads1115->getMux();

			// Change multiplexer register for test
			i2c_sensors[port][address][bus_channel][sub_address].ads1115->setMux(eADSMUX_8);
			uint8_t mux2 = i2c_sensors[port][address][bus_channel][sub_address].ads1115->getMux();

			if (mux1 == 4 and mux2 == 7) {
				// Target is ADS1115 OK
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setGain(eGAIN_TWOTHIRDS);  // 2/3x gain
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setMode(eMODE_SINGLE);     // single-shot mode
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setRate(eRATE_128);        // 128SPS (default)
				i2c_sensors[port][address][bus_channel][sub_address].ads1115->setOSMode(eOSMODE_SINGLE); // Set to start a single-conversion
				return I2C_4_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].ads1115.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x4D:
		if (not i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx) {
			// SC16IS7XX I2C-UART for A02YYUW, IP67 Ultrasonic distance sensor (A1-Vss, A0-Vss)
			i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx.reset(new SC16IS7XX(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->begin(_address)) {
				i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->disableFIFO();
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x50:
		if (not i2c_sensors[port][address][bus_channel][sub_address].ands) {
			// ATMICL-ONE
			i2c_sensors[port][address][bus_channel][sub_address].ands.reset(new ATMICL_ONE(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].ands->begin(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].ands.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x51:
		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter) {
			// M5 Ampere meter unit
			uint8_t root_address = (address == 0x51) ? 0x48 : address; // 0x51 以外の時はマルチプレクサ経由
			if (i2c_sensors[port][root_address][bus_channel][0x48].ads1115) {
				// 同じバス上にads1115 がいる場合、電流センサ用に再初期化
				i2c_sensors[port][root_address][bus_channel][0x48].ads1115->setGain(eGAIN_EIGHT);      // 512
				i2c_sensors[port][root_address][bus_channel][0x48].ads1115->setMode(eMODE_SINGLE);     // single-shot mode
				i2c_sensors[port][root_address][bus_channel][0x48].ads1115->setRate(eRATE_128);        // 8SPS (default)
				i2c_sensors[port][root_address][bus_channel][0x48].ads1115->setOSMode(eOSMODE_SINGLE); // Set to start a single-conversion

				i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter.reset(new M5AmMeterUnit(_wire));
				i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter->setGain(ADS1115_PGA_512);
				return I2C_1_DATA_DEVICE;
			}
		}
		break;
	case 0x53:
		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter) {
			// M5 Volt meter unit
			uint8_t root_address = (address == 0x53) ? 0x49 : address; // 0x53 以外の時はマルチプレクサ経由
			if (i2c_sensors[port][root_address][bus_channel][0x49].ads1115) {
				// 同じバス上にads1115 がいる場合、電圧センサ用に再初期化
				i2c_sensors[port][root_address][bus_channel][0x49].ads1115->setGain(eGAIN_EIGHT);      // 512
				i2c_sensors[port][root_address][bus_channel][0x49].ads1115->setMode(eMODE_SINGLE);     // single-shot mode
				i2c_sensors[port][root_address][bus_channel][0x49].ads1115->setRate(eRATE_128);        // 8SPS (default)
				i2c_sensors[port][root_address][bus_channel][0x49].ads1115->setOSMode(eOSMODE_SINGLE); // Set to start a single-conversion

				i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter.reset(new M5VoltMeterUnit(_wire));
				i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter->setGain(ADS1115_PGA_512);
				// | PGA      | Max Input Voltage(V) |
				// | PGA_6144 |        128           |
				// | PGA_4096 |        64            |
				// | PGA_2048 |        32            |
				// | PGA_512  |        16            |
				// | PGA_256  |        8             |
				return I2C_1_DATA_DEVICE;
			}
		}
		break;
	case 0x55:
		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit) {
			// M5 4-20mA Unit
			i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit.reset(new MODULE_4_20MA());
			i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit->begin(_wire, _address); // 100k
			if (i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit->getI2CAddress() == _address) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x56:
		if (not i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			// QMP6988 -- atmospheric pressure
			i2c_sensors[port][address][bus_channel][sub_address].qmp6988.reset(new QMP6988());
			if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988->init(_address, _wire)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].qmp6988.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x57:
		if (not i2c_sensors[port][address][bus_channel][sub_address].rcwl9620) {
			// RCWL-9620 -- ultra sonic distance sensor
			i2c_sensors[port][address][bus_channel][sub_address].rcwl9620.reset(new RCWL9620(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].rcwl9620->begin(_address)) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].rcwl9620.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x58:
		if (not i2c_sensors[port][address][bus_channel][sub_address].sgp30) {
			// SGP30
			i2c_sensors[port][address][bus_channel][sub_address].sgp30.reset(new Adafruit_SGP30());
			if (i2c_sensors[port][address][bus_channel][sub_address].sgp30->begin(_wire)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sgp30.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].an16) {
			// ATMICL-ONE
			i2c_sensors[port][address][bus_channel][sub_address].an16.reset(new ATMICL_ONE(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].an16->begin(_address)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].an16.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x5C:
		if (not i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi.reset(new BH1750());
			// 判定方法が無いため、決め打ち
			i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi->begin(BH1750::CONTINUOUS_HIGH_RES_MODE, _address, _wire);
			return I2C_1_DATA_DEVICE;
		}
		break;
	case 0x60:
		if (not i2c_sensors[port][address][bus_channel][sub_address].mcp9600) {
			// MCP9600
			i2c_sensors[port][address][bus_channel][sub_address].mcp9600.reset(new MCP9600(_wire, _address));
			// K 型熱電対を想定する
			if (i2c_sensors[port][address][bus_channel][sub_address].mcp9600->init(MCP9600::THER_TYPE_K) == MCP9600::NO_ERROR) {
				// Success to initialize
				int error = static_cast<int>(MCP9600::NO_ERROR);

				error |= static_cast<int>(i2c_sensors[port][address][bus_channel][sub_address].mcp9600->set_filt_coefficients(MCP9600::FILT_MID));                      // LPF の強度（中）
				error |= static_cast<int>(i2c_sensors[port][address][bus_channel][sub_address].mcp9600->set_cold_junc_resolution(MCP9600::COLD_JUNC_RESOLUTION_0_625)); // 温度分解能
				error |= static_cast<int>(i2c_sensors[port][address][bus_channel][sub_address].mcp9600->set_ADC_meas_resolution(MCP9600::ADC_18BIT_RESOLUTION));        // ADC 分解能
				error |= static_cast<int>(i2c_sensors[port][address][bus_channel][sub_address].mcp9600->set_sensor_mode(MCP9600::NORMAL_OPERATION));                    // 動作モード

				if (static_cast<MCP9600::err_t>(error) == MCP9600::NO_ERROR) {
					return I2C_3_DATA_DEVICE;
				}
				i2c_sensors[port][address][bus_channel][sub_address].mcp9600.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].mcp9600.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x61:
		if (not i2c_sensors[port][address][bus_channel][sub_address].scd30) {
			// SCD30
			i2c_sensors[port][address][bus_channel][sub_address].scd30.reset(new SCD30());
			if (i2c_sensors[port][address][bus_channel][sub_address].scd30->begin(*_wire)) {
				i2c_sensors[port][address][bus_channel][sub_address].scd30->reset(); // sensor reset
				return I2C_3_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].scd30.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].ezo_do) {
			// EZO_DO
			i2c_sensors[port][address][bus_channel][sub_address].ezo_do.reset(new EZO_DO(_address, _wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].ezo_do->begin()) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].ezo_do.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x62:
		if (not i2c_sensors[port][address][bus_channel][sub_address].scd4x) {
			// SCD4x
			i2c_sensors[port][address][bus_channel][sub_address].scd4x.reset(new SCD4x());
			if (i2c_sensors[port][address][bus_channel][sub_address].scd4x->begin(*_wire)) {
				return I2C_3_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].scd4x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x66:
		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter) {
			// M5 KMeter
			i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter.reset(new M5_KMeter());
			i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->begin(_wire, _address);
			bool result                          = i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->update();
			M5_KMeter::error_code_t latest_error = i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->getError();
			if (result or latest_error == M5_KMeter::err_open_circuit) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso) {
			// M5 KMeter Isolation
			i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso.reset(new UNIT_KMETER());
			if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso->begin(_wire, _address)) {
				char tmp[8] = {'\0'};
				i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso->getCelsiusTempString(tmp);
				// HAL.printf("%s\n", tmp);
				if (tmp[5] == '.') {
					return I2C_2_DATA_DEVICE;
				}
			}
			i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso.reset();
			_wire->end();
			HAL.delay(20);
			_wire->begin();
		}
		break;
	case 0x69:
		if (not i2c_sensors[port][address][bus_channel][sub_address].sen54) {
			// Sensirion SEN54
			uint16_t err;
			uint8_t pName[32] = {0};
			i2c_sensors[port][address][bus_channel][sub_address].sen54.reset(new SensirionI2CSen5x());
			i2c_sensors[port][address][bus_channel][sub_address].sen54->begin(*_wire);
			err = i2c_sensors[port][address][bus_channel][sub_address].sen54->deviceReset();

			if (not err) {
				err = i2c_sensors[port][address][bus_channel][sub_address].sen54->getProductName(pName, 32);
			}
			if (not err) {
				err = strcmp((char *)pName, "SEN54");
			}

			if (not err) {
				err = i2c_sensors[port][address][bus_channel][sub_address].sen54->setTemperatureOffsetSimple(0.0);
			}
			if (not err) {
				err = i2c_sensors[port][address][bus_channel][sub_address].sen54->startMeasurement();
			}

			if (not err) {
				return I2C_7_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].sen54.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x70:
		if (bus_channel == -1 and not i2c_sensors[port][_address][-1][_address].tca9548a) {
			// 初期化
			i2c_sensors[port][_address][-1][_address].tca9548a.reset(new QWIICMUX());
			// チャンネルスキャン
			if (i2c_sensors[port][_address][-1][_address].tca9548a->begin(_address, *_wire)) {
				const uint8_t TCA9548A_MAX_CHANNEL_NUM = 8;

				for (uint8_t ch = 0; ch < TCA9548A_MAX_CHANNEL_NUM; ch++) {
					i2c_sensors[port][_address][-1][_address].tca9548a->setPort(ch);
					std::vector<uint8_t> sub_addr_list = scanDeviceUnderMultiplexer(port);

					int i = _active_device.size();
					for (uint8_t sub : sub_addr_list) {
						_active_device.emplace_back();
						_active_device[i].address     = _address;
						_active_device[i].sub_address = sub;
						_active_device[i].channel     = ch;
						_active_device[i].port        = port;
						i++;
					}
				}

				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_0_DATA_DEVICE;
			} else {
				// 初期化失敗
				i2c_sensors[port][_address][-1][_address].tca9548a.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
				// map のリセットも必要？
			}
		} else if (bus_channel != -1 and i2c_sensors[port][_address][-1][_address].tca9548a) {
			// マルチプレクサの下についているデバイス
			i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
			I2C_INIT_RESULT result = initializeDevice(port, _address, bus_channel, sub_address, true);
			// Set all ports disable
			i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
			return result;
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			// QMP6988 -- atmospheric pressure
			i2c_sensors[port][address][bus_channel][sub_address].qmp6988.reset(new QMP6988());
			if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988->init(_address, _wire)) {
				return I2C_2_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].qmp6988.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		break;
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
		if (bus_channel == -1 and not i2c_sensors[port][_address][-1][_address].tca9548a) {
			// 初期化
			i2c_sensors[port][_address][-1][_address].tca9548a.reset(new QWIICMUX());
			// チャンネルスキャン
			if (i2c_sensors[port][_address][-1][_address].tca9548a->begin(_address, *_wire)) {
				const uint8_t TCA9548A_MAX_CHANNEL_NUM = 8;

				for (uint8_t ch = 0; ch < TCA9548A_MAX_CHANNEL_NUM; ch++) {
					i2c_sensors[port][_address][-1][_address].tca9548a->setPort(ch);
					std::vector<uint8_t> sub_addr_list = scanDeviceUnderMultiplexer(port);

					int i = _active_device.size();
					for (uint8_t sub : sub_addr_list) {
						_active_device.emplace_back();
						_active_device[i].address     = _address;
						_active_device[i].sub_address = sub;
						_active_device[i].channel     = ch;
						_active_device[i].port        = port;
						i++;
					}
				}

				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_0_DATA_DEVICE;
			} else {
				// 初期化失敗
				i2c_sensors[port][_address][-1][_address].tca9548a.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
				// map のリセットも必要？
			}
		} else if (bus_channel != -1 and i2c_sensors[port][_address][-1][_address].tca9548a) {
			// マルチプレクサの下についているデバイス
			i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
			I2C_INIT_RESULT result = initializeDevice(port, _address, bus_channel, sub_address, true);
			// Set all ports disable
			i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
			return result;
		}

		break;
	case 0x76:
		if (bus_channel == -1 and not i2c_sensors[port][_address][-1][_address].tca9548a) {
			// 初期化
			i2c_sensors[port][_address][-1][_address].tca9548a.reset(new QWIICMUX());
			// チャンネルスキャン
			if (i2c_sensors[port][_address][-1][_address].tca9548a->begin(_address, *_wire)) {
				const uint8_t TCA9548A_MAX_CHANNEL_NUM = 8;

				for (uint8_t ch = 0; ch < TCA9548A_MAX_CHANNEL_NUM; ch++) {
					i2c_sensors[port][_address][-1][_address].tca9548a->setPort(ch);
					std::vector<uint8_t> sub_addr_list = scanDeviceUnderMultiplexer(port);

					int i = _active_device.size();
					for (uint8_t sub : sub_addr_list) {
						_active_device.emplace_back();
						_active_device[i].address     = _address;
						_active_device[i].sub_address = sub;
						_active_device[i].channel     = ch;
						_active_device[i].port        = port;
						i++;
					}
				}

				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_0_DATA_DEVICE;
			} else {
				// 初期化失敗
				i2c_sensors[port][_address][-1][_address].tca9548a.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
				// map のリセットも必要？
			}
		} else if (bus_channel != -1 and i2c_sensors[port][_address][-1][_address].tca9548a) {
			// マルチプレクサの下についているデバイス
			i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
			I2C_INIT_RESULT result = initializeDevice(port, _address, bus_channel, sub_address, true);
			// Set all ports disable
			i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
			return result;
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280 -- Digital Barometer
			i2c_sensors[port][address][bus_channel][sub_address].bmp280.reset(new Bme280TwoWire());
			if (i2c_sensors[port][address][bus_channel][sub_address].bmp280->begin(static_cast<Bme280TwoWireAddress>(_address), _wire) == true) {
				uint8_t chip_id = i2c_sensors[port][address][bus_channel][sub_address].bmp280->getChipId();
				if (chip_id == 0x56 or chip_id == 0x57 or chip_id == 0x58) {
					// 0x56, 0x57: samples
					// 0x58: mass production

					// Use Indoor preset.
					// This is very low noise detection and minimum sampling rate is 25 Hz.
					i2c_sensors[port][address][bus_channel][sub_address].bmp280->setSettings(Bme280Settings::indoor());

					return I2C_2_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].bmp280.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].bmp280.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			i2c_sensors[port][address][bus_channel][sub_address].bme280.reset(new Bme280TwoWire());
			if (i2c_sensors[port][address][bus_channel][sub_address].bme280->begin(static_cast<Bme280TwoWireAddress>(_address), _wire) == true) {
				uint8_t chip_id = i2c_sensors[port][address][bus_channel][sub_address].bme280->getChipId();
				if (chip_id == 0x60) {
					// Use Indoor preset.
					// This is very low noise detection and minimum sampling rate is 25 Hz.
					i2c_sensors[port][address][bus_channel][sub_address].bme280->setSettings(Bme280Settings::indoor());
					return I2C_3_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].bme280.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].bme280.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			i2c_sensors[port][address][bus_channel][sub_address].bme68x.reset(new Bme68x());
			i2c_sensors[port][address][bus_channel][sub_address].bme68x->begin(_address, *_wire); // included soft reset
			HAL.delay(1);
			if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->readReg(BME68X_REG_CHIP_ID) == BME68X_CHIP_ID) {
					// Sensor is ready
					// Set the default configuration for temperature, pressure and humidity
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->setTPH();
					// Disable internal heater
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->writeReg(BME68X_REG_CTRL_GAS_0, 0b00001000); // 0x70
					// Enable gas conversion and heater step 0
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->writeReg(BME68X_REG_CTRL_GAS_1, 0b00100000); // 0x71
					return I2C_4_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x.reset();
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}

			} else {
				i2c_sensors[port][address][bus_channel][sub_address].bme68x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	case 0x77:
		if (bus_channel == -1 and not i2c_sensors[port][_address][-1][_address].tca9548a) {
			// 初期化
			i2c_sensors[port][_address][-1][_address].tca9548a.reset(new QWIICMUX());
			// チャンネルスキャン
			if (i2c_sensors[port][_address][-1][_address].tca9548a->begin(_address, *_wire)) {
				const uint8_t TCA9548A_MAX_CHANNEL_NUM = 8;

				for (uint8_t ch = 0; ch < TCA9548A_MAX_CHANNEL_NUM; ch++) {
					i2c_sensors[port][_address][-1][_address].tca9548a->setPort(ch);
					std::vector<uint8_t> sub_addr_list = scanDeviceUnderMultiplexer(port);

					int i = _active_device.size();
					for (uint8_t sub : sub_addr_list) {
						_active_device.emplace_back();
						_active_device[i].address     = _address;
						_active_device[i].sub_address = sub;
						_active_device[i].channel     = ch;
						_active_device[i].port        = port;
						i++;
					}
				}

				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_0_DATA_DEVICE;
			} else {
				// 初期化失敗
				i2c_sensors[port][_address][-1][_address].tca9548a.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
				// map のリセットも必要？
			}
		} else if (bus_channel != -1 and i2c_sensors[port][_address][-1][_address].tca9548a) {
			// マルチプレクサの下についているデバイス
			i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
			I2C_INIT_RESULT result = initializeDevice(port, _address, bus_channel, sub_address, true);
			// Set all ports disable
			i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
			return result;
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280 -- Digital Barometer
			i2c_sensors[port][address][bus_channel][sub_address].bmp280.reset(new Bme280TwoWire());
			if (i2c_sensors[port][address][bus_channel][sub_address].bmp280->begin(static_cast<Bme280TwoWireAddress>(_address), _wire) == true) {
				uint8_t chip_id = i2c_sensors[port][address][bus_channel][sub_address].bmp280->getChipId();
				if (chip_id == 0x56 or chip_id == 0x57 or chip_id == 0x58) {
					// 0x56, 0x57: samples
					// 0x58: mass production

					// Use Indoor preset.
					// This is very low noise detection and minimum sampling rate is 25 Hz.
					i2c_sensors[port][address][bus_channel][sub_address].bmp280->setSettings(Bme280Settings::indoor());

					return I2C_2_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].bmp280.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].bmp280.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			i2c_sensors[port][address][bus_channel][sub_address].bme280.reset(new Bme280TwoWire());
			if (i2c_sensors[port][address][bus_channel][sub_address].bme280->begin(static_cast<Bme280TwoWireAddress>(_address), _wire) == true) {
				uint8_t chip_id = i2c_sensors[port][address][bus_channel][sub_address].bme280->getChipId();
				if (chip_id == 0x60) {
					// Use Indoor preset.
					// This is very low noise detection and minimum sampling rate is 25 Hz.
					i2c_sensors[port][address][bus_channel][sub_address].bme280->setSettings(Bme280Settings::indoor());
					return I2C_3_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].bme280.reset();
					// 安定化のため Wire をリセットする
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].bme280.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			i2c_sensors[port][address][bus_channel][sub_address].bme68x.reset(new Bme68x());
			i2c_sensors[port][address][bus_channel][sub_address].bme68x->begin(_address, *_wire); // included soft reset
			HAL.delay(1);
			if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->readReg(BME68X_REG_CHIP_ID) == BME68X_CHIP_ID) {
					// Sensor is ready
					// Set the default configuration for temperature, pressure and humidity
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->setTPH();
					// Disable internal heater
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->writeReg(BME68X_REG_CTRL_GAS_0, 0b00001000); // 0x70
					// Enable gas conversion and heater step 0
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->writeReg(BME68X_REG_CTRL_GAS_1, 0b00100000); // 0x71
					return I2C_4_DATA_DEVICE;
				} else {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x.reset();
					_wire->end();
					HAL.delay(20);
					_wire->begin();
				}

			} else {
				i2c_sensors[port][address][bus_channel][sub_address].bme68x.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}

		if (not i2c_sensors[port][address][bus_channel][sub_address].gwls) {
			// Grove water level sensor (initialized also 0x78)
			i2c_sensors[port][address][bus_channel][sub_address].gwls.reset(new GroveWaterLevelSensor(_wire));
			if (i2c_sensors[port][address][bus_channel][sub_address].gwls->begin() == true) {
				return I2C_1_DATA_DEVICE;
			} else {
				i2c_sensors[port][address][bus_channel][sub_address].gwls.reset();
				// 安定化のため Wire をリセットする
				_wire->end();
				HAL.delay(20);
				_wire->begin();
			}
		}
		break;
	default:
		break;
	}
	return I2C_IGNORE_DEVICE;
}

// MARK: getDeviceName
String I2C_Manager::getDeviceName(uint8_t port,
                                  uint8_t address,
                                  int8_t bus_channel,
                                  uint8_t sub_address,
                                  int16_t sub_id,
                                  bool use_sub_addr) {
	String user_device_name = "";
	uint8_t _address        = address;
	if (use_sub_addr) {
		_address         = sub_address;
		user_device_name = getUserI2cDeviceName(port, _address, sub_id, false);
	} else {
		user_device_name = getUserI2cDeviceName(port, _address, sub_id, true);
	}

	if (user_device_name.length() != 0) {
		return user_device_name;
	}

	char s_addr[5] = {0};
	snprintf(s_addr, 5, "0x%02X", _address);

	switch (_address) {
	case 0x09:
		if (i2c_sensors[port][address][bus_channel][sub_address].dt_s1) {
			// flexible thermistor
			//@ url: https://www.semitec.co.jp/uploads/IoT_DigitalThermistor-1-.03.pdf
			switch (sub_id) {
			case 0:
				return "DT-S1_(" + String(s_addr) + ")";
			default:
				return "DT-S1";
			}
		}
		break;
	case 0x10:
		// BMM150 -- 3-Axis Geomagnetic Sensor
		return "BMM150";
	case 0x18:
		if (i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			// DS248X: 1-Wire to I2C bridge

			// sub_id < 0 の場合、DS248X 全体の情報を返す
			if (sub_id < 0) {
				auto total_nodes = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceCount();
				return "DS248X_(" + String(total_nodes) + "-nodes)";
			}

			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceValueId(sub_id);
			char ow_addr_str[128]  = {0};
			snprintf(ow_addr_str, 128, "_%02X%02X%02X%02X%02X%02X%02X%02X",
			         ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7]);

			// センサ名の取得
			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				// DS18XXX センサー
				//@ key: DS18B20
				//@ note: DS2482, DS2485, 等の 1-wire=I2C 変換 IC が必要です。
				//@ url: https://strawberry-linux.com/catalog/items?code=12331
				//@ url: https://www.analog.com/media/en/technical-documentation/data-sheets/ds2484.pdf
				//@ url: https://www.mouser.jp/datasheet/3/1014/1/DS2482_100.pdf
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(ow_addr) == true) {
					String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getFamilyName(ow_addr[0]);
					switch (value_id) {
					case 0:
						return pname + String(ow_addr_str);
					default:
						return pname + String(ow_addr_str);
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				// MAX3185 センサー
				//@ key: MAX31850K
				//@ note: DS2482, DS2485, 等の 1-wire=I2C 変換 IC が必要です。
				//@ url: https://www.switch-science.com/products/4102
				//@ url: https://www.analog.com/media/en/technical-documentation/data-sheets/ds2484.pdf
				//@ url: https://www.mouser.jp/datasheet/3/1014/1/DS2482_100.pdf
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(ow_addr) == true) {
					String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getFamilyName(ow_addr[0]);
					switch (value_id) {
					case 0:
						return pname + "_Temp_Junc" + String(ow_addr_str);
					case 1:
						return pname + "_Temp_Cold" + String(ow_addr_str);
					default:
						return pname + String(ow_addr_str);
					}
				}
			}
		}
		break;
	case 0x19:
	case 0x1A:
	case 0x1B:
		if (i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			// DS248X: 1-Wire to I2C bridge

			// sub_id < 0 の場合、DS248X 全体の情報を返す
			if (sub_id < 0) {
				auto total_nodes = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceCount();
				return "DS248X_(" + String(total_nodes) + "-nodes)";
			}

			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceValueId(sub_id);
			char ow_addr_str[128]  = {0};
			snprintf(ow_addr_str, 128, "_%02X%02X%02X%02X%02X%02X%02X%02X",
			         ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7]);

			// センサ名の取得
			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				// DS18XXX センサー
				//@ key: DS18B20
				//@ note: DS2482, DS2485, 等の 1-wire=I2C 変換 IC が必要です。
				//@ url: https://strawberry-linux.com/catalog/items?code=12331
				//@ url: https://www.analog.com/media/en/technical-documentation/data-sheets/ds2484.pdf
				//@ url: https://www.mouser.jp/datasheet/3/1014/1/DS2482_100.pdf
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(ow_addr) == true) {
					String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getFamilyName(ow_addr[0]);
					switch (value_id) {
					case 0:
						return pname + String(ow_addr_str);
					default:
						return pname + String(ow_addr_str);
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				// MAX3185 センサー
				//@ key: MAX31850K
				//@ note: DS2482, DS2485, 等の 1-wire=I2C 変換 IC が必要です。
				//@ url: https://www.switch-science.com/products/4102
				//@ url: https://www.analog.com/media/en/technical-documentation/data-sheets/ds2484.pdf
				//@ url: https://www.mouser.jp/datasheet/3/1014/1/DS2482_100.pdf
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(ow_addr) == true) {
					String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getFamilyName(ow_addr[0]);
					switch (value_id) {
					case 0:
						return pname + "_Temp_Junc" + String(ow_addr_str);
					case 1:
						return pname + "_Temp_Cold" + String(ow_addr_str);
					default:
						return pname + String(ow_addr_str);
					}
				}
			}
		}
		break;
	case 0x20:
	case 0x21:
	case 0x22:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ url: https://akizukidenshi.com/catalog/g/g109486/
			switch (sub_id) {
			case 0:
				return "MCP23017_GPIO_A_(" + String(s_addr) + ")";
			case 1:
				return "MCP23017_GPIO_B_(" + String(s_addr) + ")";
			default:
				return "MCP23017";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ url: https://www.switch-science.com/products/6552
			switch (sub_id) {
			case 0:
				return "PCA9554_(" + String(s_addr) + ")";
			default:
				return "PCA9554";
			}
		}
		break;
	case 0x23:
		if (i2c_sensors[port][address][bus_channel][sub_address].ltr_553als) {
			// LTR_553ALS
			//@ url: https://optoelectronics.liteon.com/upload/download/DS86-2014-0007/LTR-553ALS-01_DS_V1.pdf
			switch (sub_id) {
			case 0:
				return "LTR-553ALS_Lux_(" + String(s_addr) + ")";
			case 1:
				return "LTR-553ALS_Proximity_(" + String(s_addr) + ")";
			default:
				return "LTR-553ALS";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ url: https://akizukidenshi.com/catalog/g/g109486/
			switch (sub_id) {
			case 0:
				return "MCP23017_GPIO_A_(" + String(s_addr) + ")";
			case 1:
				return "MCP23017_GPIO_B_(" + String(s_addr) + ")";
			default:
				return "MCP23017";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ url: https://www.switch-science.com/products/6552
			switch (sub_id) {
			case 0:
				return "PCA9554_(" + String(s_addr) + ")";
			default:
				return "PCA9554";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			// BH1750FVI
			//@ url: https://www.switch-science.com/products/7985
			switch (sub_id) {
			case 0:
				return "BH1750FVI_(" + String(s_addr) + ")";
			default:
				return "BH1750FVI";
			}
		}
		break;
	case 0x24:
	case 0x25:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ url: https://akizukidenshi.com/catalog/g/g109486/
			switch (sub_id) {
			case 0:
				return "MCP23017_GPIO_A_(" + String(s_addr) + ")";
			case 1:
				return "MCP23017_GPIO_B_(" + String(s_addr) + ")";
			default:
				return "MCP23017";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ url: https://www.switch-science.com/products/6552
			switch (sub_id) {
			case 0:
				return "PCA9554_(" + String(s_addr) + ")";
			default:
				return "PCA9554";
			}
		}
		break;
	case 0x26:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_weight) {
			// M5 Weight I2C Unit
			//@ url: https://www.switch-science.com/products/9568
			switch (sub_id) {
			case 0:
				return "M5_Weight_Unit_(" + String(s_addr) + ")";
			case 1:
				return "M5_Weight_Unit_ADC_(" + String(s_addr) + ")";
			default:
				return "M5_Weight_Unit";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ url: https://akizukidenshi.com/catalog/g/g109486/
			switch (sub_id) {
			case 0:
				return "MCP23017_GPIO_A_(" + String(s_addr) + ")";
			case 1:
				return "MCP23017_GPIO_B_(" + String(s_addr) + ")";
			default:
				return "MCP23017";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ url: https://www.switch-science.com/products/6552
			switch (sub_id) {
			case 0:
				return "PCA9554_(" + String(s_addr) + ")";
			default:
				return "PCA9554";
			}
		}
		break;
	case 0x27:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ url: https://akizukidenshi.com/catalog/g/g109486/
			switch (sub_id) {
			case 0:
				return "MCP23017_GPIO_A_(" + String(s_addr) + ")";
			case 1:
				return "MCP23017_GPIO_B_(" + String(s_addr) + ")";
			default:
				return "MCP23017";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ url: https://www.switch-science.com/products/6552
			switch (sub_id) {
			case 0:
				return "PCA9554_(" + String(s_addr) + ")";
			default:
				return "PCA9554";
			}
		}
		break;
	case 0x28:
		if (i2c_sensors[port][address][bus_channel][sub_address].mfrc522) {
			// RFID Unit
			//@ url: https://shop.m5stack.com/products/rfid-sensor-unit?variant=16804784701530
			switch (sub_id) {
			case 0:
				return "MFRC522_(" + String(s_addr) + ")";
			case 1:
				return "MFRC522_Old_(" + String(s_addr) + ")";
			default:
				return "MFRC522";
			}
		}
		break;
	case 0x29:
		if (i2c_sensors[port][address][bus_channel][sub_address].vl53l0x) {
			//@ url: https://www.switch-science.com/products/5219
			switch (sub_id) {
			case 0:
				return "VL53L0X_(" + String(s_addr) + ")";
			default:
				return "VL53L0X";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].vl53l1x) {
			//@ url: https://www.switch-science.com/products/9427
			switch (sub_id) {
			case 0:
				return "VL53L1X_(" + String(s_addr) + ")";
			default:
				return "VL53L1X";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].tcs34725) {
			// Color Unit
			//@ url: https://www.switch-science.com/products/5218
			switch (sub_id) {
			case 0:
				return "TCS34725_(" + String(s_addr) + ")";
			case 1:
				return "TCS34725_R_(" + String(s_addr) + ")";
			case 2:
				return "TCS34725_G_(" + String(s_addr) + ")";
			case 3:
				return "TCS34725_B_(" + String(s_addr) + ")";
			default:
				return "TCS34725";
			}
		}
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			//@ url: https://www.switch-science.com/products/1174
			switch (sub_id) {
			case 0:
				return "TLS2561_(" + String(s_addr) + ")";
			default:
				return "TLS2561";
			}
		}
		break;
	case 0x38:
		if (i2c_sensors[port][address][bus_channel][sub_address].ahtxx) {
			// AHT10, AHT20
			//@ url: https://www.switch-science.com/products/8600
			switch (sub_id) {
			case 0:
				return "AHTxx_Humid_(" + String(s_addr) + ")";
			case 1:
				return "AHTxx_Temp_(" + String(s_addr) + ")";
			default:
				return "AHTxx";
			}
		}
		break;
	case 0x39:
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			//@ url: https://www.switch-science.com/products/1174
			switch (sub_id) {
			case 0:
				return "TLS2561_(" + String(s_addr) + ")";
			default:
				return "TLS2561";
			}
		}
		break;
	case 0x3E:
		// LCD device
		return "AQM0802A";
	case 0x40:
		if (i2c_sensors[port][address][bus_channel][sub_address].ds2485) {
			// DS248X: 1-Wire to I2C bridge

			// sub_id < 0 の場合、DS2485 全体の情報を返す
			if (sub_id < 0) {
				auto total_nodes = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceCount();
				return "DS2485_(" + String(total_nodes) + "-nodes)";
			}

			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceValueId(sub_id);
			char ow_addr_str[128]  = {0};
			snprintf(ow_addr_str, 128, "_%02X%02X%02X%02X%02X%02X%02X%02X",
			         ow_addr[0], ow_addr[1], ow_addr[2], ow_addr[3], ow_addr[4], ow_addr[5], ow_addr[6], ow_addr[7]);

			// センサ名の取得
			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485) {
				// DS18XXX センサー
				//@ key: DS18B20
				//@ note: DS2482, DS2485, 等の 1-wire=I2C 変換 IC が必要です。
				//@ url: https://strawberry-linux.com/catalog/items?code=12331
				//@ url: https://www.switch-science.com/products/8650
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->setTargetDevice(ow_addr) == true) {
					String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getFamilyName(ow_addr[0]);
					switch (value_id) {
					case 0:
						return pname + String(ow_addr_str);
					default:
						return pname + String(ow_addr_str);
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485) {
				// MAX3185 センサー
				//@ key: MAX31850K
				//@ note: DS2482, DS2485, 等の 1-wire=I2C 変換 IC が必要です。
				//@ url: https://www.switch-science.com/products/4102
				//@ url: https://www.switch-science.com/products/8650
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->setTargetDevice(ow_addr) == true) {
					String pname = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getFamilyName(ow_addr[0]);
					switch (value_id) {
					case 0:
						return pname + "_Temp_Junc" + String(ow_addr_str);
					case 1:
						return pname + "_Temp_Cold" + String(ow_addr_str);
					default:
						return pname + String(ow_addr_str);
					}
				}
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sns2) {
			// ATMICL-ONE
			//@ url: https://booth.pm/ja/items/2307810
			switch (sub_id) {
			case 0:
				return "SNS2_State_(" + String(s_addr) + ")";
			case 1:
				return "SNS2_Count_(" + String(s_addr) + ")";
			case 2:
				return "SNS2_Frequency_(" + String(s_addr) + ")";
			default:
				return "SNS2";
			}
		}
		break;
	case 0x44:
		if (i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X
			//@ url: https://www.switch-science.com/products/9270
			switch (sub_id) {
			case 0:
				return "SHT4X_Humid_(" + String(s_addr) + ")";
			case 1:
				return "SHT4X_Temp_(" + String(s_addr) + ")";
			default:
				return "SHT4X";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X
			//@ url: https://www.switch-science.com/products/7254
			switch (sub_id) {
			case 0:
				return "SHT3X_Humid_(" + String(s_addr) + ")";
			case 1:
				return "SHT3X_Temp_(" + String(s_addr) + ")";
			default:
				return "SHT3X";
			}
		}
		break;
	case 0x45:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_extio2) {
			// M5 ExtI/O 2
			//@ url: https://www.switch-science.com/products/8187
			switch (sub_id) {
			case 0:
				return "M5_EXT_IO2_(" + String(s_addr) + ")";
			default:
				return "M5_EXT_IO2";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X
			//@ url: https://www.switch-science.com/products/9270
			switch (sub_id) {
			case 0:
				return "SHT4X_Humid_(" + String(s_addr) + ")";
			case 1:
				return "SHT4X_Temp_(" + String(s_addr) + ")";
			default:
				return "SHT4X";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X
			//@ url: https://www.switch-science.com/products/7254
			switch (sub_id) {
			case 0:
				return "SHT3X_Humid_(" + String(s_addr) + ")";
			case 1:
				return "SHT3X_Temp_(" + String(s_addr) + ")";
			default:
				return "SHT3X";
			}
		}
		break;
	case 0x48:
		if (i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			// ADC Unit
			//@ url: https://www.switch-science.com/products/4430
			switch (sub_id) {
			case 0:
				return "ADS1115_ch0_(" + String(s_addr) + ")";
			case 1:
				return "ADS1115_ch1_(" + String(s_addr) + ")";
			case 2:
				return "ADS1115_ch2_(" + String(s_addr) + ")";
			case 3:
				return "ADS1115_ch3_(" + String(s_addr) + ")";

			default:
				return "ADS1115";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ads1100) {
			// ADC Unit
			//@ url: https://www.switch-science.com/products/5221
			switch (sub_id) {
			case 0:
				return "ADS1100_(" + String(s_addr) + ")";
			case 1:
				return "ADS1100_M5_Unit_(" + String(s_addr) + ")";
			default:
				return "ADS1100";
			}
		}
		break;
	case 0x49:
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			//@ url: https://www.switch-science.com/products/1174
			switch (sub_id) {
			case 0:
				return "TLS2561_(" + String(s_addr) + ")";
			default:
				return "TLS2561";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			// ADC Unit
			//@ url: https://www.switch-science.com/products/4430
			switch (sub_id) {
			case 0:
				return "ADS1115_ch0_(" + String(s_addr) + ")";
			case 1:
				return "ADS1115_ch1_(" + String(s_addr) + ")";
			case 2:
				return "ADS1115_ch2_(" + String(s_addr) + ")";
			case 3:
				return "ADS1115_ch3_(" + String(s_addr) + ")";

			default:
				return "ADS1115";
			}
		}
		break;
	case 0x4D:
		if (i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx) {
			//@ note: UART=I2C 変換基板で I2C に対応させている
			//@ url: https://www.switch-science.com/products/6027
			//@ url: https://www.mouser.jp/new/dfrobot/dfrobot-a02yyuw-ultrasonic-sensor/
			switch (sub_id) {
			case 0:
				return "SC16IS7XX-A02YYUW_(" + String(s_addr) + ")";
			default:
				return "SC16IS7XX-A02YYUW";
			}
		}
		break;
	case 0x50:
		if (i2c_sensors[port][address][bus_channel][sub_address].ands) {
			// ATMICL ONE ANDS
			//@ url: https://booth.pm/ja/items/2307810
			switch (sub_id) {
			case 0:
				return "ANDS_Current_(" + String(s_addr) + ")";
			case 1:
				return "ANDS_Frequency_(" + String(s_addr) + ")";
			default:
				return "ANDS";
			}
		}
		break;
	case 0x51:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter) {
			// M5 Ampere meter unit
			//@ note: ADS1115との併用不可
			//@ url: https://www.switch-science.com/products/6739
			switch (sub_id) {
			case 0:
				return "M5AMeter_(" + String(s_addr) + ")";
			default:
				return "M5AMeter";
			}
		}
		break;
	case 0x53:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter) {
			// M5 Volt meter unit
			//@ note: ADS1115との併用不可
			//@ url: https://www.switch-science.com/products/6740
			switch (sub_id) {
			case 0:
				return "M5VMeter_(" + String(s_addr) + ")";
			default:
				return "M5VMeter";
			}
		}
		break;
	case 0x55:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit) {
			// M5 4-20mA Unit
			//@ url: https://www.switch-science.com/products/9126
			switch (sub_id) {
			case 0:
				return "M5_4-20mA_Unit_(" + String(s_addr) + ")";
			default:
				return "M5_4-20mA_Unit";
			}
		}
		break;
	case 0x56:
		if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			//@ url: https://www.switch-science.com/products/7254
			switch (sub_id) {
			case 0:
				return "QMP6988_Pressure_(" + String(s_addr) + ")";
			case 1:
				return "QMP6988_Temp_(" + String(s_addr) + ")";
			default:
				return "QMP6988";
			}
		}
		break;
	case 0x57:
		if (i2c_sensors[port][address][bus_channel][sub_address].rcwl9620) {
			// RCWL-9620 -- ultra sonic distance sensor
			//@ url: https://www.switch-science.com/products/7631
			switch (sub_id) {
			case 0:
				return "RCWL-9620_(" + String(s_addr) + ")";
			default:
				return "RCWL-9620";
			}
		}
		break;
	case 0x58:
		if (i2c_sensors[port][address][bus_channel][sub_address].sgp30) {
			//@ url: https://www.switch-science.com/products/6619
			switch (sub_id) {
			case 0:
				return "SGP30_TVOC_(" + String(s_addr) + ")";
			case 1:
				return "SGP30_eCO2_(" + String(s_addr) + ")";
			default:
				return "SGP30";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].an16) {
			// ATMICL ONE ANDS
			//@ url: https://booth.pm/ja/items/2307810
			switch (sub_id) {
			case 0:
				return "AN16_Current_(" + String(s_addr) + ")";
			case 1:
				return "AN16_Frequency_(" + String(s_addr) + ")";
			default:
				return "AN16";
			}
		}
		break;
	case 0x5A:
		// CST226 -- Touch controller IC
		return "CST226";
	case 0x5C:
		if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			// BH1750FVI
			//@ url: https://www.switch-science.com/products/7985
			switch (sub_id) {
			case 0:
				return "BH1750FVI_(" + String(s_addr) + ")";
			default:
				return "BH1750FVI";
			}
		}
		break;
	case 0x60:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp9600) {
			// MCP9600
			//@ note: K型熱電対のみ対応
			//@ url: https://www.switch-science.com/products/5345
			switch (sub_id) {
			case 0:
				return "MCP9600_Temp_Junc_(" + String(s_addr) + ")";
			case 1:
				return "MCP9600_Temp_Cold_(" + String(s_addr) + ")";
			case 2:
				return "MCP9600_Temp_Delta_(" + String(s_addr) + ")";
			default:
				return "MCP9600";
			}
		}
		break;
	case 0x61:
		if (i2c_sensors[port][address][bus_channel][sub_address].scd30) {
			//@url：https://www.switch-science.com/products/7000
			switch (sub_id) {
			case 0:
				return "SCD30_CO2_(" + String(s_addr) + ")";
			case 1:
				return "SCD30_Temp_(" + String(s_addr) + ")";
			case 2:
				return "SCD30_Humid_(" + String(s_addr) + ")";
			default:
				return "SCD30";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ezo_do) {
			//@ url: https://atlas-scientific.com/embedded-solutions/ezo-dissolved-oxygen-circuit/
			switch (sub_id) {
			case 0:
				return "EZO-DO_(" + String(s_addr) + ")";
			case 1:
				return "EZO-DO_Saturation_(" + String(s_addr) + ")";
			default:
				return "EZO-DO";
			}
		}
		break;
	case 0x62:
		if (i2c_sensors[port][address][bus_channel][sub_address].scd4x) {
			// SCD4x
			//@ url: https://www.switch-science.com/products/8496
			//@ url: https://www.switch-science.com/products/8668
			switch (sub_id) {
			case 0:
				return "SCD4x_CO2_(" + String(s_addr) + ")";
			case 1:
				return "SCD4x_Temp_(" + String(s_addr) + ")";
			case 2:
				return "SCD4x_Humid_(" + String(s_addr) + ")";
			default:
				return "SCD4x";
			}
		}
		break;
	case 0x66:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter) {
			//@ url: https://www.switch-science.com/products/7655
			switch (sub_id) {
			case 0:
				return "M5_KMeter_Temp_(" + String(s_addr) + ")";
			case 1:
				return "M5_KMeter_Unit_Temp_(" + String(s_addr) + ")";
			default:
				return "M5_KMeter";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso) {
			//@ url: https://www.switch-science.com/products/9007
			switch (sub_id) {
			case 0:
				return "M5_KMeter_ISO_Temp_(" + String(s_addr) + ")";
			case 1:
				return "M5_KMeter_ISO_Internal_Temp_(" + String(s_addr) + ")";
			default:
				return "M5_KMeter_ISO";
			}
		}
		break;
	case 0x68:
		// MPU6886 -- 6-Axis MotionTracking Sensor
		return "MPU6886";
	case 0x69:
		if (i2c_sensors[port][address][bus_channel][sub_address].sen54) {
			//@ url: https://www.switch-science.com/products/8405
			switch (sub_id) {
			case 0:
				return "SEN54_PM1.0_(" + String(s_addr) + ")";
			case 1:
				return "SEN54_PM2.5_(" + String(s_addr) + ")";
			case 2:
				return "SEN54_PM4.0_(" + String(s_addr) + ")";
			case 3:
				return "SEN54_PM10_(" + String(s_addr) + ")";
			case 4:
				return "SEN54_Humid_(" + String(s_addr) + ")";
			case 5:
				return "SEN54_Temp_(" + String(s_addr) + ")";
			case 6:
				return "SEN54_VOC_(" + String(s_addr) + ")";
			default:
				return "SEN54";
			}
		}
		break;
	case 0x6A:
		// SY6970 -- Power management IC
		return "SY6970";
	case 0x70:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			//@ url: https://www.switch-science.com/products/5698
			//@ url: https://www.switch-science.com/products/7588
			//@ url: https://www.switch-science.com/products/10192
			//@ note: I2C マルチプレクサ
			if (address == sub_address) {
				return "TCA9548A";
			} else {
				return "(TCA9548A_" + String(bus_channel) + "ch)" + getDeviceName(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			//@ url: https://www.switch-science.com/products/7254
			switch (sub_id) {
			case 0:
				return "QMP6988_Pressure_(" + String(s_addr) + ")";
			case 1:
				return "QMP6988_Temp_(" + String(s_addr) + ")";
			default:
				return "QMP6988";
			}
		}
		break;
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			//@ url: https://www.switch-science.com/products/5698
			//@ url: https://www.switch-science.com/products/7588
			//@ url: https://www.switch-science.com/products/10192
			//@ note: I2C マルチプレクサ
			if (address == sub_address) {
				return "TCA9548A";
			} else {
				return "(TCA9548A_" + String(bus_channel) + "ch)" + getDeviceName(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

#ifdef TARGET__M5STACK_BASIC
		// IP5306 -- Power management chip in M5Stack V2.7
		return "IP5306";
#endif

		break;
	case 0x76:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			//@ url: https://www.switch-science.com/products/5698
			//@ url: https://www.switch-science.com/products/7588
			//@ url: https://www.switch-science.com/products/10192
			//@ note: I2C マルチプレクサ
			if (address == sub_address) {
				return "TCA9548A";
			} else {
				return "(TCA9548A_" + String(bus_channel) + "ch)" + getDeviceName(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280
			//@ url: https://www.switch-science.com/products/3373
			switch (sub_id) {
			case 0:
				return "BMP280_Pressure_(" + String(s_addr) + ")";
			case 1:
				return "BMP280_Temp_(" + String(s_addr) + ")";
			default:
				return "BMP280";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			//@ url: https://www.switch-science.com/products/2236
			switch (sub_id) {
			case 0:
				return "BME280_Pressure_(" + String(s_addr) + ")";
			case 1:
				return "BME280_Temp_(" + String(s_addr) + ")";
			case 2:
				return "BME280_Humid_(" + String(s_addr) + ")";
			default:
				return "BME280";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			//@ url: https://www.switch-science.com/products/9274
			switch (sub_id) {
			case 0:
				return "BME68x_Temp_(" + String(s_addr) + ")";
			case 1:
				return "BME68x_Humid_(" + String(s_addr) + ")";
			case 2:
				return "BME68x_Pressure_(" + String(s_addr) + ")";
			case 3:
				return "BME68x_Gas_Registance_(" + String(s_addr) + ")";
			default:
				return "BME68x";
			}
		}
		break;
	case 0x77:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			//@ url: https://www.switch-science.com/products/5698
			//@ url: https://www.switch-science.com/products/7588
			//@ url: https://www.switch-science.com/products/10192
			//@ note: I2C マルチプレクサ
			if (address == sub_address) {
				return "TCA9548A";
			} else {
				return "(TCA9548A_" + String(bus_channel) + "ch)" + getDeviceName(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280
			//@ url: https://www.switch-science.com/products/3373
			switch (sub_id) {
			case 0:
				return "BMP280_Pressure_(" + String(s_addr) + ")";
			case 1:
				return "BMP280_Temp_(" + String(s_addr) + ")";
			default:
				return "BMP280";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			//@ url: https://www.switch-science.com/products/2236
			switch (sub_id) {
			case 0:
				return "BME280_Pressure_(" + String(s_addr) + ")";
			case 1:
				return "BME280_Temp_(" + String(s_addr) + ")";
			case 2:
				return "BME280_Humid_(" + String(s_addr) + ")";
			default:
				return "BME280";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			//@ url: https://www.switch-science.com/products/9274
			switch (sub_id) {
			case 0:
				return "BME68x_Temp_(" + String(s_addr) + ")";
			case 1:
				return "BME68x_Humid_(" + String(s_addr) + ")";
			case 2:
				return "BME68x_Pressure_(" + String(s_addr) + ")";
			case 3:
				return "BME68x_Gas_Registance_(" + String(s_addr) + ")";
			default:
				return "BME68x";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].gwls) {
			// Grove water level sensor
			//@ url: https://www.switch-science.com/products/6282
			switch (sub_id) {
			case 0:
				return "WaterLevelSensor_(" + String(s_addr) + ")";
			default:
				return "WaterLevelSensor";
			}
		}
		break;
	default:
		break;
	}
	return "Unknown_device";
}

// MARK: getDeviceUnit
String I2C_Manager::getDeviceUnit(uint8_t port,
                                  uint8_t address,
                                  int8_t bus_channel,
                                  uint8_t sub_address,
                                  int16_t sub_id, bool use_sub_addr) {
	String user_device_unit = "";
	uint8_t _address        = address;
	if (use_sub_addr) {
		_address         = sub_address;
		user_device_unit = getUserI2cDeviceUnit(port, _address, sub_id, false);
	} else {
		user_device_unit = getUserI2cDeviceUnit(port, _address, sub_id, true);
	}

	if (user_device_unit.length() != 0) {
		return user_device_unit;
	}

	switch (_address) {
	case 0x09:
		if (i2c_sensors[port][address][bus_channel][sub_address].dt_s1) {
			// flexible thermistor
			switch (sub_id) {
			case 0:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}
		break;
	case 0x18:
		// DS248X: DS18XXX と MAX3185 の両方とも温度センサー
		if (i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceValueId(sub_id);

			// センサ名の取得
			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				// DS18XXX センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						//@ label: 温度
						return "\'C";
					default:
						return "";
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				// MAX3185 センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						//@ label: 温度
						return "\'C";
					case 1:
						//@ label: 冷接点温度
						return "\'C";
					default:
						return "";
					}
				}
			}
		}
		break;
	case 0x19:
	case 0x1A:
	case 0x1B:
		// DS248X: DS18XXX と MAX3185 の両方とも温度センサー
		if (i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceValueId(sub_id);

			// センサ名の取得
			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				// DS18XXX センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						//@ label: 温度
						return "\'C";
					default:
						return "";
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				// MAX3185 センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						//@ label: 温度
						return "\'C";
					case 1:
						//@ label: 冷接点温度
						return "\'C";
					default:
						return "";
					}
				}
			}
		}
		break;
	case 0x20:
	case 0x21:
	case 0x22:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}
		break;
	case 0x23:
		if (i2c_sensors[port][address][bus_channel][sub_address].ltr_553als) {
			// LTR_553ALS
			switch (sub_id) {
			case 0:
				//@ label: 照度
				return "lux";
			case 1:
				//@ label: 近接判定
				return "";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			// BH1750FVI
			switch (sub_id) {
			case 0:
				//@ label: 照度
				return "lux";
			default:
				return "";
			}
		}
		break;
	case 0x24:
	case 0x25:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}
		break;
	case 0x26:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_weight) {
			// M5 Weight I2C Unit
			switch (sub_id) {
			case 0:
				//@ label: 重量（推定値）
				return "g";
			case 1:
				//@ label: 重量（AD 値）
				return "";
			default:
				return "";
			}
		}
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}
		break;
	case 0x27:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			//@ label: 電圧値（デジタル）
			//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
			return "";
		}
		break;
	case 0x28:
		if (i2c_sensors[port][address][bus_channel][sub_address].mfrc522) {
			// RFID Unit
			//@ label: RFID の ID 値
			return "";
		}
		break;
	case 0x29:
		if (i2c_sensors[port][address][bus_channel][sub_address].vl53l0x) {
			//@ label: 距離
			return "mm";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].vl53l1x) {
			//@ label: 距離
			return "mm";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].tcs34725) {
			// Color Unit
			switch (sub_id) {
			case 0:
				//@ label: RGB 強度（RRGGBB）
				return "RGB";
			case 1:
				//@ label: 赤色強度（0-255）
				return "R";
			case 2:
				//@ label: 緑色強度（0-255）
				return "G";
			case 3:
				//@ label: 青色強度（0-255）
				return "B";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			//@ label: 照度
			return "lux";
		}
		break;
	case 0x38:
		if (i2c_sensors[port][address][bus_channel][sub_address].ahtxx) {
			// AHT10, AHT20
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
		break;
	case 0x39:
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			//@ label: 照度
			return "lux";
		}
		break;
	case 0x40:
		// DS2485: DS18XXX と MAX3185 の両方とも温度センサー
		if (i2c_sensors[port][address][bus_channel][sub_address].ds2485) {
			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceValueId(sub_id);

			// センサ名の取得
			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485) {
				// DS18XXX センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						//@ label: 温度
						return "\'C";
					default:
						return "";
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485) {
				// MAX3185 センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						//@ label: 温度
						return "\'C";
					case 1:
						//@ label: 冷接点温度
						return "\'C";
					default:
						return "";
					}
				}
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sns2) {
			// ATMICL-ONE
			switch (sub_id) {
			case 0:
				//@ label: 直流電流状態（0/1）
				return "";
			case 1:
				//@ label: パルスカウント値
				return "";
			case 2:
				//@ label: パルス周波数
				return "Hz";
			default:
				return "";
			}
		}
		break;
	case 0x44:
		if (i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X
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

		if (i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X
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
		break;
	case 0x45:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_extio2) {
			// M5 ExtI/O 2
			switch (sub_id) {
			case 0:
				//@ label: 電圧値（デジタル）
				//@ note: 出力値は各端子のH/Lを各ビットに割り当てた値
				return "";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X
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

		if (i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X
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
		break;
	case 0x48:
		if (i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			// ADC Unit
			//@ label: 電圧値（アナログ）
			return "mV";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ads1100) {
			// ADC Unit
			//@ label: 電圧値（アナログ）
			return "mV";
		}
		break;
	case 0x49:
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			//@ label: 照度
			return "lux";
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			// ADC Unit
			//@ label: 電圧値（アナログ）
			return "mV";
		}
		break;
	case 0x4D:
		if (i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx) {
			//@ label: 距離
			return "mm";
		}
		break;
	case 0x50:
		if (i2c_sensors[port][address][bus_channel][sub_address].ands) {
			// ATMICL ONE ANDS
			switch (sub_id) {
			case 0:
				//@ label: 交流電流値
				return "A";
			case 1:
				//@ label: 交流周波数
				return "Hz";
			default:
				return "";
			}
		}
		break;
	case 0x51:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter) {
			// M5 Ampere meter unit
			switch (sub_id) {
			case 0:
				//@ label: 直流電流値
				return "mA";
			default:
				return "";
			}
		}
		break;
	case 0x53:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter) {
			// M5 Volt meter unit
			switch (sub_id) {
			case 0:
				//@ label: 直流電圧値
				return "mV";
			default:
				return "";
			}
		}
		break;
	case 0x55:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit) {
			// M5 4-20mA Unit
			switch (sub_id) {
			case 0:
				//@ label: 電流値
				//@ note: 4-20mA 出力計測用
				return "mA";
			default:
				return "";
			}
		}
		break;
	case 0x56:
		if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			switch (sub_id) {
			case 0:
				//@ label: 大気圧
				return "Pa";
			case 1:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}
		break;
	case 0x57:
		if (i2c_sensors[port][address][bus_channel][sub_address].rcwl9620) {
			// RCWL-9620 -- ultra sonic distance sensor
			//@ label: 距離
			return "mm";
		}
		break;
	case 0x58:
		if (i2c_sensors[port][address][bus_channel][sub_address].sgp30) {
			switch (sub_id) {
			case 0:
				//@ label: TVOC 濃度
				return "ppb";
			case 1:
				//@ label: eCO2 濃度
				//@ note: CO2 相当の換算値
				return "ppm";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].an16) {
			// ATMICL ONE AN16
			switch (sub_id) {
			case 0:
				//@ label: 交流電流値
				return "A";
			case 1:
				//@ label: 交流周波数
				return "Hz";
			default:
				return "";
			}
		}
		break;
	case 0x5C:
		if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			// BH1750FVI
			switch (sub_id) {
			case 0:
				//@ label: 照度
				return "lux";
			default:
				return "";
			}
		}
		break;

	case 0x60:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp9600) {
			// MCP9600
			switch (sub_id) {
			case 0:
				// Fall Through
				//@ label: 温度
			case 1:
				// Fall Through
				//@ label: 温度
			case 2:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}
		break;
	case 0x61:
		if (i2c_sensors[port][address][bus_channel][sub_address].scd30) {
			switch (sub_id) {
			case 0:
				//@ label: CO2 濃度
				return "ppm";
			case 1:
				//@ label: 温度
				return "\'C";
			case 2:
				//@ label: 相対湿度
				return "%RH";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ezo_do) {
			switch (sub_id) {
			case 0:
				//@ label: 溶存酸素量
				return "mg/L";
			case 1:
				//@ label: 酸素飽和率
				return "%";
			default:
				return "";
			}
		}
		break;
	case 0x62:
		if (i2c_sensors[port][address][bus_channel][sub_address].scd4x) {
			switch (sub_id) {
			case 0:
				//@ label: CO2 濃度
				return "ppm";
			case 1:
				//@ label: 温度
				return "\'C";
			case 2:
				//@ label: 相対湿度
				return "%RH";
			default:
				return "";
			}
		}
		break;
	case 0x66:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter) {
			switch (sub_id) {
			case 0:
				//@ label: 温度
				return "\'C";
			case 1:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso) {
			switch (sub_id) {
			case 0:
				//@ label: 温度
				return "\'C";
			case 1:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}

		break;
	case 0x69:
		if (i2c_sensors[port][address][bus_channel][sub_address].sen54) {
			switch (sub_id) {
			case 0:
				//@ label: PM1.0 濃度
				return "ug/m^3";
			case 1:
				//@ label: PM2.5 濃度
				return "ug/m^3";
			case 2:
				//@ label: PM4.0 濃度
				return "ug/m^3";
			case 3:
				//@ label: PM10 濃度
				return "ug/m^3";
			case 4:
				//@ label: 相対湿度
				return "%RH";
			case 5:
				//@ label: 温度
				return "\'C";
			case 6:
				//@ label: VOC 指標値
				return "";
			default:
				return "";
			}
		}
		break;
	case 0x70:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				return "";
			} else {
				return getDeviceUnit(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			switch (sub_id) {
			case 0:
				//@ label: 大気圧
				return "Pa";
			case 1:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}
		break;
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				return "";
			} else {
				return getDeviceUnit(port, address, bus_channel, sub_address, sub_id, true);
			}
		}
		break;
	case 0x76:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				return "";
			} else {
				return getDeviceUnit(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280
			switch (sub_id) {
			case 0:
				//@ label: 大気圧
				return "Pa";
			case 1:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			switch (sub_id) {
			case 0:
				//@ label: 大気圧
				return "Pa";
			case 1:
				//@ label: 温度
				return "\'C";
			case 2:
				//@ label: 相対湿度
				return "%RH";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			switch (sub_id) {
			case 0:
				//@ label: 温度
				return "\'C";
			case 1:
				//@ label: 相対湿度
				return "%RH";
			case 2:
				//@ label: 大気圧
				return "Pa";
			case 3:
				//@ label: 抵抗値（ガス）
				return "ohm";
			default:
				return "";
			}
		}
		break;
	case 0x77:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				return "";
			} else {
				return getDeviceUnit(port, address, bus_channel, sub_address, sub_id, true);
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280
			switch (sub_id) {
			case 0:
				//@ label: 大気圧
				return "Pa";
			case 1:
				//@ label: 温度
				return "\'C";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			switch (sub_id) {
			case 0:
				//@ label: 大気圧
				return "Pa";
			case 1:
				//@ label: 温度
				return "\'C";
			case 2:
				//@ label: 相対湿度
				return "%RH";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			switch (sub_id) {
			case 0:
				//@ label: 温度
				return "\'C";
			case 1:
				//@ label: 相対湿度
				return "%RH";
			case 2:
				//@ label: 大気圧
				return "Pa";
			case 3:
				//@ label: 抵抗値（ガス）
				return "ohm";
			default:
				return "";
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].gwls) {
			// Grove water level sensor
			//@ label: 相対水位
			return "%";
		}
		break;
	default:
		break;
	}
	return "";
}

// MARK: fetchData
I2C_DATA_RESULT I2C_Manager::fetchData(uint8_t port,
                                       uint8_t address,
                                       int8_t bus_channel,
                                       uint8_t sub_address,
                                       int16_t sub_id,
                                       bool tx_buffered,
                                       double *d_data,
                                       int32_t *i_data,
                                       String *s_data,
                                       bool use_sub_addr) {
	I2C_DATA_RESULT user_result = I2C_RES_FAILED;
	bool result;
	uint32_t ui_tmp  = 0;
	uint8_t _address = address;

	auto _wire = HAL.I2c.getWire(port);
	if (_wire == nullptr) {
		return I2C_RES_FAILED;
	}

	if (use_sub_addr) {
		_address    = sub_address;
		user_result = fetchUserI2cDeviceData(port,
		                                     _address,
		                                     sub_id,
		                                     false,
		                                     tx_buffered,
		                                     d_data,
		                                     i_data,
		                                     s_data);
	} else {
		user_result = fetchUserI2cDeviceData(port,
		                                     _address,
		                                     sub_id,
		                                     true,
		                                     tx_buffered,
		                                     d_data,
		                                     i_data,
		                                     s_data);
	}

	if (user_result != I2C_RES_FAILED) {
		return user_result;
	}

	switch (_address) {
	case 0x09:
		if (i2c_sensors[port][address][bus_channel][sub_address].dt_s1) {
			// flexible thermistor
			result = i2c_sensors[port][address][bus_channel][sub_address].dt_s1->getTemperature(d_data);
			if (result) {
				return I2C_RES_DOUBLE;
			} else {
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x18:
		// DS248X が初期化されている場合のみデータ取得
		if (i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceValueId(sub_id);

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				// DS18XXX センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getTempC();
						result  = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getLastMeasurementResult();
						if (result == false) {
							return I2C_RES_FAILED;
						}
						return I2C_RES_DOUBLE;
					default:
						return I2C_RES_FAILED;
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				// MAX3185 センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						// 温度
						result = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->convertT();
						if (result == true) {
							*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getTempC();
							return I2C_RES_DOUBLE;
						}
						return I2C_RES_FAILED;
					case 1: {
						// 冷接点温度
						if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->checkColdTemperatureUpdated() == false) {
							// 更新が必要な場合は値を更新
							result = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->convertT();
							if (result == false) {
								return I2C_RES_FAILED;
							}
						}

						int status = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getCircuitState();
						switch (status) {
						case MAX3185::CIRCUIT_OK:
							*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getColdTempC();
							return I2C_RES_DOUBLE;
						case MAX3185::CIRCUIT_SHORT_TO_VDD:
							HAL.println("Error circuit short to VDD");
							break;
						case MAX3185::CIRCUIT_SHORT_TO_GND:
							HAL.println("Error circuit short to GND");
							break;
						case MAX3185::CIRCUIT_OPEN:
							HAL.println("Error circuit Open");
							break;
						default:
							break;
						}
						return I2C_RES_FAILED;
					}
					default:
						return I2C_RES_FAILED;
					}
				}
			}
		}
		return I2C_RES_FAILED;
	case 0x19:
	case 0x1A:
	case 0x1B:
		// DS248X が初期化されている場合のみデータ取得
		if (i2c_sensors[port][address][bus_channel][sub_address].ds248x) {
			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds248x->getDeviceValueId(sub_id);

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x) {
				// DS18XXX センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getTempC();
						result  = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds248x->getLastMeasurementResult();
						if (result == false) {
							return I2C_RES_FAILED;
						}
						return I2C_RES_DOUBLE;
					default:
						return I2C_RES_FAILED;
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x) {
				// MAX3185 センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						// 温度
						result = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->convertT();
						if (result == true) {
							*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getTempC();
							return I2C_RES_DOUBLE;
						}
						return I2C_RES_FAILED;
					case 1: {
						// 冷接点温度
						if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->checkColdTemperatureUpdated() == false) {
							// 更新が必要な場合は値を更新
							result = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->convertT();
							if (result == false) {
								return I2C_RES_FAILED;
							}
						}

						int status = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getCircuitState();
						switch (status) {
						case MAX3185::CIRCUIT_OK:
							*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds248x->getColdTempC();
							return I2C_RES_DOUBLE;
						case MAX3185::CIRCUIT_SHORT_TO_VDD:
							HAL.println("Error circuit short to VDD");
							break;
						case MAX3185::CIRCUIT_SHORT_TO_GND:
							HAL.println("Error circuit short to GND");
							break;
						case MAX3185::CIRCUIT_OPEN:
							HAL.println("Error circuit Open");
							break;
						default:
							break;
						}
						return I2C_RES_FAILED;
					}
					default:
						return I2C_RES_FAILED;
					}
				}
			}
		}
		return I2C_RES_FAILED;
	case 0x20:
	case 0x21:
	case 0x22:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioA(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioB(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			uint8_t tmp;
			result = i2c_sensors[port][address][bus_channel][sub_address].pca9554->readAllPin(tmp);
			if (result) {
				*i_data = tmp;
				return I2C_RES_INT;
			}
			return I2C_RES_FAILED;
		}
		return I2C_RES_FAILED;
	case 0x23:
		if (i2c_sensors[port][address][bus_channel][sub_address].ltr_553als) {
			// LTR-553ALS
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].ltr_553als->getLuxValue();
				return I2C_RES_DOUBLE;
			case 1:
				*i_data = i2c_sensors[port][address][bus_channel][sub_address].ltr_553als->getPSvalue();
				return I2C_RES_INT;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioA(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioB(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			uint8_t tmp;
			result = i2c_sensors[port][address][bus_channel][sub_address].pca9554->readAllPin(tmp);
			if (result) {
				*i_data = tmp;
				return I2C_RES_INT;
			}
			return I2C_RES_FAILED;
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			// BH1750FVI
			if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi->measurementReady(false)) {
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi->readLightLevel();
				return I2C_RES_DOUBLE;
			}
			return I2C_RES_FAILED;
		}
		return I2C_RES_FAILED;
	case 0x24:
	case 0x25:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioA(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioB(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			uint8_t tmp;
			result = i2c_sensors[port][address][bus_channel][sub_address].pca9554->readAllPin(tmp);
			if (result) {
				*i_data = tmp;
				return I2C_RES_INT;
			}
			return I2C_RES_FAILED;
		}
		return I2C_RES_FAILED;
	case 0x26:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_weight) {
			// M5 Weight I2C Unit
			switch (sub_id) {
			case 0:
				*d_data = static_cast<double>(i2c_sensors[port][address][bus_channel][sub_address].m5_weight->getWeight());
				return I2C_RES_DOUBLE;
			case 1:
				*i_data = i2c_sensors[port][address][bus_channel][sub_address].m5_weight->getRawADC();
				return I2C_RES_INT;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioA(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioB(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			uint8_t tmp;
			result = i2c_sensors[port][address][bus_channel][sub_address].pca9554->readAllPin(tmp);
			if (result) {
				*i_data = tmp;
				return I2C_RES_INT;
			}
			return I2C_RES_FAILED;
		}
		return I2C_RES_FAILED;
	case 0x27:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp23017) {
			// MCP23017
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioA(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].mcp23017->getStatusGpioB(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].pca9554) {
			// PCA9554
			uint8_t tmp;
			result = i2c_sensors[port][address][bus_channel][sub_address].pca9554->readAllPin(tmp);
			if (result) {
				*i_data = tmp;
				return I2C_RES_INT;
			}
			return I2C_RES_FAILED;
		}
		return I2C_RES_FAILED;
	case 0x28:
		if (i2c_sensors[port][address][bus_channel][sub_address].mfrc522) {
			// RFID Unit
			if (not i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PICC_IsNewCardPresent()) {
				return I2C_RES_FAILED;
			}
			if (not i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PICC_ReadCardSerial()) {
				return I2C_RES_FAILED;
			}

			auto sak      = i2c_sensors[port][address][bus_channel][sub_address].mfrc522->uid.sak;
			auto piccType = i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PICC_GetType(sak);
			// Check is the PICC of Classic MIFARE type
			if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI and
			    piccType != MFRC522::PICC_TYPE_MIFARE_1K and
			    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
				return I2C_RES_FAILED;
			}
			String s_id = "";
			switch (sub_id) {
			case 0:
				for (int i = 0; i < i2c_sensors[port][address][bus_channel][sub_address].mfrc522->uid.size; i++) {
					constexpr int NUM = 3; // 00 to FF + null string
					char s_tmp[NUM]   = {0};
					snprintf(s_tmp, NUM, "%02X", i2c_sensors[port][address][bus_channel][sub_address].mfrc522->uid.uidByte[i]);
					s_id += String(s_tmp);
				}
				break;
			case 1:
				// Old included bug
				for (int i = 0; i < i2c_sensors[port][address][bus_channel][sub_address].mfrc522->uid.size; i++) {
					s_id += String(i2c_sensors[port][address][bus_channel][sub_address].mfrc522->uid.uidByte[i], HEX);
				}
				break;
			default:
				return I2C_RES_FAILED;
			}

			s_id.toUpperCase();
			*s_data = s_id;

			i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PICC_HaltA();
			i2c_sensors[port][address][bus_channel][sub_address].mfrc522->PCD_StopCrypto1();
			return I2C_RES_STRING;
		}
		return I2C_RES_FAILED;
	case 0x29:
		if (i2c_sensors[port][address][bus_channel][sub_address].vl53l0x) {
			// ToF sensor
			*i_data = i2c_sensors[port][address][bus_channel][sub_address].vl53l0x->readRangeContinuousMillimeters();
			if (i2c_sensors[port][address][bus_channel][sub_address].vl53l0x->timeoutOccurred()) {
				return I2C_RES_FAILED;
			}
			if (*i_data >= 8190) {
				// Out of range error
				return I2C_RES_FAILED;
			}
			return I2C_RES_INT;
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].vl53l1x) {
			// ToF sensor
			*i_data = i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->readRangeContinuousMillimeters();
			if (i2c_sensors[port][address][bus_channel][sub_address].vl53l1x->timeoutOccurred()) {
				return I2C_RES_FAILED;
			}
			// if (*i_data >= 8190) {
			// 	// Out of range error
			// 	return I2C_RES_FAILED;
			// }
			return I2C_RES_INT;
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].tcs34725) {
			// Color Unit
			static uint16_t clear, red, green, blue;
			double r, g, b;

			switch (sub_id) {
			case 0: {
				i2c_sensors[port][address][bus_channel][sub_address].tcs34725->getRawData(&red, &green, &blue, &clear);
				clear = clear == 0 ? 1 : clear;
				r     = ((double)red / (uint32_t)clear) * 256;
				g     = ((double)green / (uint32_t)clear) * 256;
				b     = ((double)blue / (uint32_t)clear) * 256;

				*i_data = 0;
				*i_data = ((int)r << 16) + ((int)g << 8) + ((int)b << 0);
				return I2C_RES_INT;
			}
			case 1: {
				r       = ((double)red / (uint32_t)clear) * 256;
				*i_data = 0;
				*i_data = (int)r;
				return I2C_RES_INT;
			}
			case 2: {
				g       = ((double)green / (uint32_t)clear) * 256;
				*i_data = 0;
				*i_data = (int)g;
				return I2C_RES_INT;
			}
			case 3: {
				b       = ((double)blue / (uint32_t)clear) * 256;
				*i_data = 0;
				*i_data = (int)b;
				return I2C_RES_INT;
			}
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			// Digital Light Sensor
			result = i2c_sensors[port][address][bus_channel][sub_address].tsl2561->readVisibleLux(&ui_tmp);
			if (result) {
				*i_data = ui_tmp;
				return I2C_RES_INT;
			} else {
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x38:
		if (i2c_sensors[port][address][bus_channel][sub_address].ahtxx) {
			// AHT10, AHT20
			static bool _result = false;
			if (sub_id == 0) {
				_result = i2c_sensors[port][address][bus_channel][sub_address].ahtxx->startMeasurementReady();
			}

			if (_result == false) {
				return I2C_RES_FAILED;
			}

			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].ahtxx->getHumidity_RH();
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].ahtxx->getTemperature_C();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x39:
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			// Digital Light Sensor
			result = i2c_sensors[port][address][bus_channel][sub_address].tsl2561->readVisibleLux(&ui_tmp);
			if (result) {
				*i_data = ui_tmp;
				return I2C_RES_INT;
			} else {
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x40:
		// DS2485 が初期化されている場合のみデータ取得
		if (i2c_sensors[port][address][bus_channel][sub_address].ds2485) {
			const uint8_t *ow_addr = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceAddress(sub_id);
			uint8_t value_id       = i2c_sensors[port][address][bus_channel][sub_address].ds2485->getDeviceValueId(sub_id);

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485) {
				// DS18XXX センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getTempC();
						result  = i2c_sensors[port][address][bus_channel][sub_address].ow_ds18xxx_ds2485->getLastMeasurementResult();
						if (result == false) {
							return I2C_RES_FAILED;
						}
						return I2C_RES_DOUBLE;
					default:
						return I2C_RES_FAILED;
					}
				}
			}

			if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485) {
				// MAX3185 センサー
				if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->setTargetDevice(ow_addr) == true) {
					switch (value_id) {
					case 0:
						// 温度
						result = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->convertT();
						if (result == true) {
							*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getTempC();
							return I2C_RES_DOUBLE;
						}
						return I2C_RES_FAILED;
					case 1: {
						// 冷接点温度
						if (i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->checkColdTemperatureUpdated() == false) {
							// 更新が必要な場合は値を更新
							result = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->convertT();
							if (result == false) {
								return I2C_RES_FAILED;
							}
						}

						int status = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getCircuitState();
						switch (status) {
						case MAX3185::CIRCUIT_OK:
							*d_data = i2c_sensors[port][address][bus_channel][sub_address].ow_max3185_ds2485->getColdTempC();
							return I2C_RES_DOUBLE;
						case MAX3185::CIRCUIT_SHORT_TO_VDD:
							HAL.println("Error circuit short to VDD");
							break;
						case MAX3185::CIRCUIT_SHORT_TO_GND:
							HAL.println("Error circuit short to GND");
							break;
						case MAX3185::CIRCUIT_OPEN:
							HAL.println("Error circuit Open");
							break;
						default:
							break;
						}
						return I2C_RES_FAILED;
					}
					default:
						return I2C_RES_FAILED;
					}
				}
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sns2) {
			// ATMICL-ONE
			uint16_t u16i_tmp = 0;
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].sns2->readDcState(&u16i_tmp);
				if (result) {
					*i_data = (int32_t)u16i_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].sns2->readDcCounter(&ui_tmp);
				if (result) {
					*i_data = ui_tmp;

					if (tx_buffered == true) {
						i2c_sensors[port][address][bus_channel][sub_address].sns2->clearDcCounter();
					}

					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			case 2:
				result = i2c_sensors[port][address][bus_channel][sub_address].sns2->readDcFrequency(&u16i_tmp);
				if (result) {
					*i_data = (int32_t)u16i_tmp;
					return I2C_RES_INT;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x44:
		if (i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X
			float aTemp  = 0.0;
			float aHumid = 0.0;

			switch (sub_id) {
			case 0:
				ui_tmp = i2c_sensors[port][address][bus_channel][sub_address].sht4x->measureHighPrecision(aTemp, aHumid);
				if (ui_tmp == 0) {
					*d_data = (double)aHumid;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				// i2c_sensors[port][address][bus_channel][sub_address].sht3x->GetTempHum();
				ui_tmp = i2c_sensors[port][address][bus_channel][sub_address].sht4x->measureHighPrecision(aTemp, aHumid);
				if (ui_tmp == 0) {
					*d_data = (double)aTemp;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].sht3x->readHumidity();
				if (not isnan(*d_data)) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				// i2c_sensors[port][address][bus_channel][sub_address].sht3x->GetTempHum();
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].sht3x->readTemperature();
				if (not isnan(*d_data)) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x45:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_extio2) {
			// M5 ExtI/O 2
			switch (sub_id) {
			case 0:
				//*i_data = i2c_sensors[port][address][bus_channel][sub_address].m5_extio2->getAllDigitalInputs();
				*i_data = 0;
				for (uint8_t i = 0; i < 8; i++) {
					*i_data |= (i2c_sensors[port][address][bus_channel][sub_address].m5_extio2->getDigitalInput(i) << i);
				}
				return I2C_RES_INT;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht4x) {
			// SHT4X
			float aTemp  = 0.0;
			float aHumid = 0.0;

			switch (sub_id) {
			case 0:
				ui_tmp = i2c_sensors[port][address][bus_channel][sub_address].sht4x->measureHighPrecision(aTemp, aHumid);
				if (ui_tmp == 0) {
					*d_data = (double)aHumid;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				// i2c_sensors[port][address][bus_channel][sub_address].sht3x->GetTempHum();
				ui_tmp = i2c_sensors[port][address][bus_channel][sub_address].sht4x->measureHighPrecision(aTemp, aHumid);
				if (ui_tmp == 0) {
					*d_data = (double)aTemp;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].sht3x) {
			// SHT3X
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].sht3x->readHumidity();
				if (not isnan(*d_data)) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				// i2c_sensors[port][address][bus_channel][sub_address].sht3x->GetTempHum();
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].sht3x->readTemperature();
				if (not isnan(*d_data)) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x48:
		if (i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			if (sub_id > 3) {
				// Out of range
				return I2C_RES_FAILED;
			}
			if (not i2c_sensors[port][address][bus_channel][sub_address].ads1115->checkADS1115()) {
				// Disconnected
				return I2C_RES_FAILED;
			}

			*i_data = i2c_sensors[port][address][bus_channel][sub_address].ads1115->readVoltage(sub_id);
			HAL.delay(1);
			return I2C_RES_INT;
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ads1100) {
			const double Vdd            = 3.3; // Power supply assume 3.3V
			const uint16_t MINIMUM_CODE = 32768;
			uint16_t raw_value          = i2c_sensors[port][address][bus_channel][sub_address].ads1100->Measure_Differential();

			switch (sub_id) {
			case 0:
				*i_data = static_cast<int32_t>(raw_value * (Vdd / MINIMUM_CODE) * 1000);
				return I2C_RES_INT;
			case 1:
				// Scaled 0V-5V to 0V-12V
				*i_data = static_cast<int32_t>(4 * raw_value * (Vdd / MINIMUM_CODE) * 1000);
				return I2C_RES_INT;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x49:
		if (i2c_sensors[port][address][bus_channel][sub_address].tsl2561) {
			// Digital Light Sensor
			result = i2c_sensors[port][address][bus_channel][sub_address].tsl2561->readVisibleLux(&ui_tmp);
			if (result) {
				*i_data = ui_tmp;
				return I2C_RES_INT;
			} else {
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ads1115) {
			if (sub_id > 3) {
				// Out of range
				return I2C_RES_FAILED;
			}
			if (not i2c_sensors[port][address][bus_channel][sub_address].ads1115->checkADS1115()) {
				// Disconnected
				return I2C_RES_FAILED;
			}

			*i_data = i2c_sensors[port][address][bus_channel][sub_address].ads1115->readVoltage(sub_id);
			return I2C_RES_INT;
		}
		return I2C_RES_FAILED;
	case 0x4D:
		if (i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx) {
			i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->enableFIFO();
			constexpr uint16_t TIMEOUT_MS     = 300;
			unsigned long long ref_time       = millis();
			constexpr uint8_t OUTPUT_FORM_LEN = 4;
			uint8_t data[OUTPUT_FORM_LEN]     = {0};

			while (data[0] != 0xFF and ((millis() - ref_time) < TIMEOUT_MS)) {
				if (i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->rxAvailable() == 0) {
					HAL.delay(10);
					continue;
				}
				i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->readByte(&data[0]);
			}

			uint8_t i = 1;
			while (i != OUTPUT_FORM_LEN and ((millis() - ref_time) < TIMEOUT_MS)) {
				if (i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->rxAvailable() == 0) {
					HAL.delay(10);
					continue;
				}
				result = i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->readByte(&data[i]);
				i      = (result) ? (i + 1) : i;
			}

			if (((millis() - ref_time) > TIMEOUT_MS)) {
				// HAL.println("TIMEOUT ERROR");
				return I2C_RES_FAILED;
			}
			i2c_sensors[port][address][bus_channel][sub_address].sc16is7xx->disableFIFO();

			uint8_t checksum = (data[0] + data[1] + data[2]) & 0x00FF;
			if (data[3] != checksum) {
				return I2C_RES_FAILED;
			}

			*i_data = (data[1] << 8) + data[2];
			if (*i_data < 30) {
				// Shorter than blind zone distance (3cm)
				return I2C_RES_FAILED;
			}

			*i_data = (*i_data > 4500) ? 4500 : *i_data;
			return I2C_RES_INT;
		}
		return I2C_RES_FAILED;
	case 0x50:
		if (i2c_sensors[port][address][bus_channel][sub_address].ands) {
			// ATMICL ONE ANDS
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].ands->readAcCurrent(d_data);
				if (result) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].ands->readAcFrequency(d_data);
				if (result) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x51:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter) {
			// M5 Ampere meter unit
			uint8_t root_address = (address == 0x51) ? 0x48 : address; // 0x51 以外の時はマルチプレクサ経由
			int16_t _tmp         = 0;
			switch (sub_id) {
			case 0:
				HAL.delay(1);
				i2c_sensors[port][root_address][bus_channel][0x48].ads1115->setMux(eADSMUX_1);
				_tmp = i2c_sensors[port][root_address][bus_channel][0x48].ads1115->readAdValue(255);
				HAL.delay(1);
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].m5_am_meter->calculateAmpere(_tmp);
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x53:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter) {
			// M5 Ampere meter unit
			uint8_t root_address = (address == 0x53) ? 0x49 : address; // 0x51 以外の時はマルチプレクサ経由
			int16_t _tmp         = 0;
			switch (sub_id) {
			case 0:
				HAL.delay(1);
				i2c_sensors[port][root_address][bus_channel][0x49].ads1115->setMux(eADSMUX_1);
				_tmp = i2c_sensors[port][root_address][bus_channel][0x49].ads1115->readAdValue(255);
				HAL.delay(1);
				*i_data = static_cast<int32_t>(i2c_sensors[port][address][bus_channel][sub_address].m5_volt_meter->calculateVoltage(_tmp));
				return I2C_RES_INT;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x55:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit) {
			// M5 4-20mA Unit
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].m5_4_20mA_unit->getCurrentValue(0) / 100.0;
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x56:
		if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].qmp6988->calcPressure();
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].qmp6988->calcTemperature();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x57:
		if (i2c_sensors[port][address][bus_channel][sub_address].rcwl9620) {
			// RCWL-9620 -- ultra sonic distance sensor
			if (i2c_sensors[port][address][bus_channel][sub_address].rcwl9620->getDistance(d_data)) {
				return I2C_RES_DOUBLE;
			}
			return I2C_RES_FAILED;
		}
		return I2C_RES_FAILED;
	case 0x58:
		if (i2c_sensors[port][address][bus_channel][sub_address].sgp30) {
			if (i2c_sensors[port][address][bus_channel][sub_address].sgp30->IAQmeasure()) {
				switch (sub_id) {
				case 0:
					*i_data = i2c_sensors[port][address][bus_channel][sub_address].sgp30->TVOC;
					return I2C_RES_INT;
				case 1:
					*i_data = i2c_sensors[port][address][bus_channel][sub_address].sgp30->eCO2;
					return I2C_RES_INT;
				default:
					return I2C_RES_FAILED;
				}
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].an16) {
			// ATMICL ONE AN16
			switch (sub_id) {
			case 0:
				result = i2c_sensors[port][address][bus_channel][sub_address].an16->readAcCurrent(d_data);
				if (result) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				result = i2c_sensors[port][address][bus_channel][sub_address].an16->readAcFrequency(d_data);
				if (result) {
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x5C:
		if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi) {
			// BH1750FVI
			if (i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi->measurementReady(false)) {
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bh1750fvi->readLightLevel();
				return I2C_RES_DOUBLE;
			}
		}
		return I2C_RES_FAILED;
	case 0x60:
		if (i2c_sensors[port][address][bus_channel][sub_address].mcp9600) {
			// MCP9600
			MCP9600::err_t error = MCP9600::NO_ERROR;

			switch (sub_id) {
			case 0:
				error = i2c_sensors[port][address][bus_channel][sub_address].mcp9600->read_hot_junc(d_data);
				if (error != MCP9600::NO_ERROR) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 1:
				error = i2c_sensors[port][address][bus_channel][sub_address].mcp9600->read_cold_junc(d_data);
				if (error != MCP9600::NO_ERROR) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 2:
				error = i2c_sensors[port][address][bus_channel][sub_address].mcp9600->read_junc_temp_delta(d_data);
				if (error != MCP9600::NO_ERROR) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x61:
		if (i2c_sensors[port][address][bus_channel][sub_address].scd30) {
			if (sub_id == 0) {
				if (i2c_sensors[port][address][bus_channel][sub_address].scd30->dataAvailable()) {
					i2c_sensors[port][address][bus_channel][sub_address].scd30->readMeasurement();
				}
			}
			switch (sub_id) {
			case 0:
				*i_data = i2c_sensors[port][address][bus_channel][sub_address].scd30->getCO2();
				return I2C_RES_INT;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].scd30->getTemperature();
				return I2C_RES_DOUBLE;
			case 2:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].scd30->getHumidity();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].ezo_do) {
			if (sub_id == 0) {
				i2c_sensors[port][address][bus_channel][sub_address].ezo_do->read();
			}
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].ezo_do->getDissolvedOxygen();
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].ezo_do->getSaturation();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x62:
		if (i2c_sensors[port][address][bus_channel][sub_address].scd4x) {
			if (sub_id == 0) {
				i2c_sensors[port][address][bus_channel][sub_address].scd4x->readMeasurement();
			}
			switch (sub_id) {
			case 0:
				*i_data = i2c_sensors[port][address][bus_channel][sub_address].scd4x->getCO2();
				return ((400 <= *i_data) ? I2C_RES_INT : I2C_RES_FAILED);
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].scd4x->getTemperature();
				return I2C_RES_DOUBLE;
			case 2:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].scd4x->getHumidity();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x66:
		if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter) {
			switch (sub_id) {
			case 0:
				if (not i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->update()) {
					return I2C_RES_FAILED;
				}
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->getTemperature();
				return I2C_RES_DOUBLE;
			case 1:
				if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->getError() != M5_KMeter::err_ok) {
					return I2C_RES_FAILED;
				}
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter->getInternalTemp();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso) {
			switch (sub_id) {
			case 0:
				if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso->getReadyStatus() == 0) {
					*d_data = i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso->getCelsiusTempValue() / 100.0;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				if (i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso->getReadyStatus() == 0) {
					*d_data = i2c_sensors[port][address][bus_channel][sub_address].m5_kmeter_iso->getInternalCelsiusTempValue() / 100.0;
					return I2C_RES_DOUBLE;
				}
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x69:
		if (i2c_sensors[port][address][bus_channel][sub_address].sen54) {
			float massConcentrationPm1p0;
			float massConcentrationPm2p5;
			float massConcentrationPm4p0;
			float massConcentrationPm10p0;
			float ambientHumidity;
			float ambientTemperature;
			float vocIndex;
			float noxIndex;

			uint16_t err = i2c_sensors[port][address][bus_channel][sub_address].sen54->readMeasuredValues(massConcentrationPm1p0,
			                                                                                              massConcentrationPm2p5,
			                                                                                              massConcentrationPm4p0,
			                                                                                              massConcentrationPm10p0,
			                                                                                              ambientHumidity,
			                                                                                              ambientTemperature,
			                                                                                              vocIndex,
			                                                                                              noxIndex);
			if (err) {
				return I2C_RES_FAILED;
			}

			switch (sub_id) {
			case 0:
				*d_data = (double)massConcentrationPm1p0;
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = (double)massConcentrationPm2p5;
				return I2C_RES_DOUBLE;
			case 2:
				*d_data = (double)massConcentrationPm4p0;
				return I2C_RES_DOUBLE;
			case 3:
				*d_data = (double)massConcentrationPm10p0;
				return I2C_RES_DOUBLE;
			case 4:
				*d_data = (double)ambientHumidity;
				return I2C_RES_DOUBLE;
			case 5:
				*d_data = (double)ambientTemperature;
				return I2C_RES_DOUBLE;
			case 6:
				*d_data = (double)vocIndex;
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x70:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				// i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_RES_FAILED;
			} else {
				i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
				I2C_DATA_RESULT result = fetchData(port,
				                                   address,
				                                   bus_channel,
				                                   sub_address,
				                                   sub_id,
				                                   tx_buffered,
				                                   d_data,
				                                   i_data,
				                                   s_data,
				                                   true);
				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return result;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].qmp6988) {
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].qmp6988->calcPressure();
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].qmp6988->calcTemperature();
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				// i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_RES_FAILED;
			} else {
				i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
				I2C_DATA_RESULT result = fetchData(port,
				                                   address,
				                                   bus_channel,
				                                   sub_address,
				                                   sub_id,
				                                   tx_buffered,
				                                   d_data,
				                                   i_data,
				                                   s_data,
				                                   true);
				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return result;
			}
		}
		return I2C_RES_FAILED;
	case 0x76:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				// i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_RES_FAILED;
			} else {
				i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
				I2C_DATA_RESULT result = fetchData(port,
				                                   address,
				                                   bus_channel,
				                                   sub_address,
				                                   sub_id,
				                                   tx_buffered,
				                                   d_data,
				                                   i_data,
				                                   s_data,
				                                   true);
				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return result;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bmp280->getPressure();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bmp280->getTemperature();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bme280->getPressure();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bme280->getTemperature();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 2:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bme280->getHumidity();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			bme68xData data;

			switch (sub_id) {
			case 0:
				i2c_sensors[port][address][bus_channel][sub_address].bme68x->setOpMode(BME68X_FORCED_MODE);
				// us to ms
				HAL.delay((i2c_sensors[port][address][bus_channel][sub_address].bme68x->getMeasDur() / 1000) + 1);

				i2c_sensors[port][address][bus_channel][sub_address].bme68x->fetchData();
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.temperature;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.humidity;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 2:
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.pressure;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 3:
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.gas_resistance;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}
		return I2C_RES_FAILED;
	case 0x77:
		if (i2c_sensors[port][_address][-1][_address].tca9548a) {
			if (address == sub_address) {
				// i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return I2C_RES_FAILED;
			} else {
				i2c_sensors[port][_address][-1][_address].tca9548a->setPort(bus_channel);
				I2C_DATA_RESULT result = fetchData(port,
				                                   address,
				                                   bus_channel,
				                                   sub_address,
				                                   sub_id,
				                                   tx_buffered,
				                                   d_data,
				                                   i_data,
				                                   s_data,
				                                   true);
				// Set all ports disable
				i2c_sensors[port][_address][-1][_address].tca9548a->setPortState(0x00);
				return result;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bmp280) {
			// BMP280
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bmp280->getPressure();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bmp280->getTemperature();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme280) {
			// BME280 -- Digital Temp/Humid/Pressure sensor
			switch (sub_id) {
			case 0:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bme280->getPressure();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 1:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bme280->getTemperature();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			case 2:
				*d_data = i2c_sensors[port][address][bus_channel][sub_address].bme280->getHumidity();
				if (isnan(*d_data)) {
					return I2C_RES_FAILED;
				}
				return I2C_RES_DOUBLE;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].bme68x) {
			// BME68X -- Digital Temp/Humid/Pressure/Gas sensor
			bme68xData data;

			switch (sub_id) {
			case 0:
				i2c_sensors[port][address][bus_channel][sub_address].bme68x->setOpMode(BME68X_FORCED_MODE);
				// us to ms
				HAL.delay((i2c_sensors[port][address][bus_channel][sub_address].bme68x->getMeasDur() / 1000) + 1);

				i2c_sensors[port][address][bus_channel][sub_address].bme68x->fetchData();
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.temperature;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 1:
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.humidity;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 2:
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.pressure;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			case 3:
				if (i2c_sensors[port][address][bus_channel][sub_address].bme68x->checkStatus() == BME68X_OK) {
					i2c_sensors[port][address][bus_channel][sub_address].bme68x->getData(data);
					*d_data = (double)data.gas_resistance;
					return I2C_RES_DOUBLE;
				}
				return I2C_RES_FAILED;
			default:
				return I2C_RES_FAILED;
			}
		}

		if (i2c_sensors[port][address][bus_channel][sub_address].gwls) {
			// Grove water level sensor
			*i_data = i2c_sensors[port][address][bus_channel][sub_address].gwls->fetchWaterLevel();
			if (*i_data < 0) {
				return I2C_RES_FAILED;
			} else {
				return I2C_RES_INT;
			}
		}
		return I2C_RES_FAILED;
	default:
		break;
	}

	return I2C_RES_FAILED;
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Logic Functions
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

I2C_Manager::I2C_Manager() {
	_active_device.reserve(10); // for memory efficient
}

void I2C_Manager::clearDetectedDevices() {
	i2c_sensors.clear();
	_active_device.clear();
}

void I2C_Manager::setIgnoreAddressPattern(uint8_t port, const char *pattern) {
	if (port > PORT_I2C::PORT_5) {
		return; // 範囲外ポートは無視
	}
	_ignore[port].pattern.address = pattern;
}

void I2C_Manager::setIgnoreNamePattern(uint8_t port, const char *pattern) {
	if (port > PORT_I2C::PORT_5) {
		return; // 範囲外ポートは無視
	}
	_ignore[port].pattern.name = pattern;
}

bool I2C_Manager::isIgnoreSensorAddress(uint8_t port, uint8_t address) {
	if (port > PORT_I2C::PORT_5) {
		return false; // 範囲外ポートは許可
	}

	auto &config = _ignore[port];

	if (!config.cache.valid) {
		config.cache.list  = extractIgnoreAddress(config.pattern.address.c_str());
		config.cache.valid = true;
	}

	return std::find(config.cache.list.begin(),
	                 config.cache.list.end(),
	                 address) != config.cache.list.end();
}

bool I2C_Manager::isIgnoreSensorName(uint8_t port, const char *name) {
	if (port > PORT_I2C::PORT_5) {
		return false; // 範囲外ポートは許可
	}

	return matchNamePattern(name, _ignore[port].pattern.name.c_str());
}

void I2C_Manager::scanDevice(uint8_t port) {
	auto _wire = HAL.I2c.getWire(port);
	if (_wire == nullptr) {
		return;
	}

	int i = _active_device.size();
	for (uint8_t addr = I2C_ADDRESS_MINIMUM; addr < I2C_ADDRESS_MAXIMUM; addr++) {
		_wire->beginTransmission(addr);
		int8_t error = _wire->endTransmission(true);

		if (error != 0) {
			_wire->end();   // arduino-esp32 の不具合対策用暫定措置（ESP-IDF #9777）
			_wire->begin(); // v2.0.11 (ESP-IDF v4.4.5) で解決済みのはずだが直ってないっぽい
			continue;
		}

		_active_device.emplace_back();
		_active_device[i].address     = addr;
		_active_device[i].sub_address = addr;
		_active_device[i].channel     = -1;
		_active_device[i].port        = port;
		i++;
	}
}

std::vector<uint8_t> I2C_Manager::scanDeviceUnderMultiplexer(uint8_t port) {
	std::vector<uint8_t> v_addrs;

	auto _wire = HAL.I2c.getWire(port);
	if (_wire == nullptr) {
		return v_addrs;
	}

	for (uint8_t sub_addr = I2C_ADDRESS_MINIMUM; sub_addr < I2C_ADDRESS_MAXIMUM; sub_addr++) {
		_wire->beginTransmission(sub_addr);
		int8_t error = _wire->endTransmission(true);

		if (error != 0) {
			_wire->end();   // arduino-esp32 の不具合対策用暫定措置（ESP-IDF #9777）
			_wire->begin(); // v2.0.11 (ESP-IDF v4.4.5) で解決済みのはずだが直ってないっぽい
			continue;
		}

		auto res = std::find_if(_active_device.cbegin(), _active_device.cend(),
		                        [sub_addr](I2CActiveDevice device) {
			                        return (sub_addr == device.address);
		                        });

		if (res != std::end(_active_device)) {
			// 既にメインバス（-1）上に同じアドレスのデバイスが存在する
			continue;
		}

		if (std::find(v_addrs.begin(), v_addrs.end(), sub_addr) != v_addrs.end()) {
			// 同じマルチプレクサの同じチャネルに同じアドレスが存在する
			continue;
		}

		v_addrs.push_back(sub_addr);
	}

	return v_addrs;
}

std::vector<I2CActiveDevice> I2C_Manager::getAddressList() {
	return _active_device;
}

void I2C_Manager::displayScanResult() {
	HAL.println("======================== Scan i2c device result ========================");
	HAL.println("------------------------------------------------------------------------");
	HAL.printf("| %-40s| addr  | addr2 | ch | port  |\n", "Device name");
	HAL.println("|-----------------------------------------+-------+-------+----+-------|");
	for (int i = 0; i < _active_device.size(); i++) {
		HAL.printf("| %-40s| 0x%02X  | 0x%02X  | %2d | %3d   |\n",
		           getDeviceName(
		               _active_device[i].port,
		               _active_device[i].address,
		               _active_device[i].channel,
		               _active_device[i].sub_address)
		               .c_str(),
		           _active_device[i].address,
		           _active_device[i].sub_address,
		           _active_device[i].channel,
		           _active_device[i].port);
	}
	HAL.println("------------------------------------------------------------------------");
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  Logic Functions (internal)
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

std::vector<String> I2C_Manager::splitPatternByPipe(const char *regexp_pattern) {
	std::vector<String> result;
	String current;

	for (const char *p = regexp_pattern; *p != '\0'; ++p) {
		if (*p == '\\' && *(p + 1) == '|') {
			current += '|';
			++p;
		} else if (*p == '|') {
			result.push_back(current);
			current.remove(0);
		} else {
			current += *p;
		}
	}
	result.push_back(current);

	return result;
}

bool I2C_Manager::matchesAnyFragment(const char *text, const std::vector<String> &fragments) {
	size_t textLen = ::strlen(text);
	std::vector<char> textBuf(text, text + textLen + 1);

	for (const auto &fragment : fragments) {
		if (fragment.length() == 0) {
			continue; // 空断片は常に不一致(意図しない全マッチを防ぐ)
		}

		MatchState ms;
		ms.Target(textBuf.data());

		char result = ms.Match(fragment.c_str());

		if (result == REGEXP_MATCHED &&
		    ms.MatchStart == 0 &&
		    ms.MatchLength == textLen) {
			return true;
		}
	}

	return false;
}

std::vector<uint8_t> I2C_Manager::extractIgnoreAddress(const char *regexp_pattern) {
	std::vector<uint8_t> ignoredList;
	std::vector<String> fragments = splitPatternByPipe(regexp_pattern);

	for (int addr = 0x00; addr <= 0x7F; ++addr) {
		char hexBuf[8];
		char decBuf[8];

		std::snprintf(hexBuf, sizeof(hexBuf), "0x%02X", addr);
		std::snprintf(decBuf, sizeof(decBuf), "%d", addr);

		if (matchesAnyFragment(hexBuf, fragments) ||
		    matchesAnyFragment(decBuf, fragments)) {
			ignoredList.push_back(static_cast<uint8_t>(addr));
		}
	}

	return ignoredList;
}

bool I2C_Manager::matchNamePattern(const char *name, const char *regexp_pattern) {
	std::vector<String> fragments = splitPatternByPipe(regexp_pattern);
	return matchesAnyFragment(name, fragments);
}