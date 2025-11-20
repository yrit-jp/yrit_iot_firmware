#ifndef SENSOR_TYPE_H
#define SENSOR_TYPE_H

enum SensorType {
	TYPE_GPIO        = 0b00000001,
	TYPE_I2C         = 0b00000010,
	TYPE_CAMERA      = 0b00000100,
	TYPE_MODBUS      = 0b00001000,
	TYPE_PULSE_TOTAL = 0b00010000,
	TYPE_PULSE_DELTA = 0b00100000,
	TYPE_NONE        = 0b11111111,
};

template <typename T>
struct ValueHistory {
	T latest;
	T prev;
};

struct Sensor {
	enum SensorType type;
	String name;
	String unit;
	bool enable;
	bool display;
	bool updated;       // センサデータの読み取りが行われた時に立つ
	bool tx_buffered;   // 送信用バッファにセンサ値が格納された時に立つ
	bool value_changed; // 変化時送信が有効のとき、値が変化すると立つ
	struct ValueHistory<int32_t> i_value;
	struct ValueHistory<double> d_value;
	struct ValueHistory<String> s_value;
	bool tx_on_change; // 値の変化時に、送信間隔を待たずに送信を行う
	uint8_t shift_level;
	uint8_t port;
	struct {
		uint8_t pin;
		bool digital;
		bool tx_link_i2c;
		bool tx_link_modbus;
	} GPIO;
	struct {
		uint8_t address;
		uint8_t sub_address;
		int8_t channel;
		// I2C_DATA_RESULT result;
		int result;
		int16_t sub_id;
	} I2C;
	struct {
		uint8_t id;
		int result;
		int16_t sub_id;
	} MODBUS;
	struct {
		bool captured;
	} CAMERA;
};

#endif /* SENSOR_TYPE_H */