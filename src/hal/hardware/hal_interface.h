// ==============================================================
//! @file hal_interface.h
//! @author takkaO
//! @copyright Copyright (c) 2023 takkaO
// =============================================================

#ifndef HAL_API_H
#define HAL_API_H

#define ARDUINOJSON_USE_LONG_LONG 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>
#include <DFRobot_RTU.h>
#include <Wire.h>
#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unordered_map>
#include <utility>
#include <vector>

#define SERVICE_UUID "B06A0001-1439-11EC-9BC3-52C4DDF835D4"           //!< BLE UART service UUID
#define CHARACTERISTIC_UUID_RX "B06A0002-1439-11EC-9BC3-52C4DDF835D4" //!< BLE Device Rx UUID
#define CHARACTERISTIC_UUID_TX "B06A0003-1439-11EC-9BC3-52C4DDF835D4" //!< BLE Device Tx UUID
#define BLE_BUF_LENGTH (256)
#define BLE_MAX_LOCAL_NAME_LEN (20)

// ==============================================================
//! @brief ボタン押下状態を表す列挙体
// =============================================================
enum class PRESSED_BUTTON : uint8_t {
	NONE         = 0,
	PRESSED_BTN1 = 1,
	PRESSED_BTN2 = 2,
};

// ==============================================================
//! @brief 検知パルスエッジを表す列挙体
// =============================================================
enum class PULSE_EDGE : uint8_t {
	RISE = 0,
	FALL = 1,
	BOTH = 2,
};

// ==============================================================
//! @brief データリンク層の定義
// =============================================================

enum class DATALINK_LAYER : uint8_t {
	UNKNOWN,
	WIFI,
	ETHERNET,
};

// ==============================================================
//! @brief IPAddress 中継用クラス
// =============================================================
class MyIPAddress {
public:
	MyIPAddress() {
		for (int i = 0; i < 4; i++) {
			_ip[i] = 0;
		}
	}

	MyIPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet) {
		_ip[0] = first_octet;
		_ip[1] = second_octet;
		_ip[2] = third_octet;
		_ip[3] = fourth_octet;
	}

	String toString() {
		return String(_ip[0]) + "." + String(_ip[1]) + "." + String(_ip[2]) + "." + String(_ip[3]);
	}

	void getArray(uint8_t *ip) {
		for (int i = 0; i < 4; i++) {
			*(ip + i) = _ip[i];
		}
	}

protected:
	uint8_t _ip[4];
};

// ==============================================================
//! @brief Client ダミークラス
// =============================================================
class DummyClient : public Client {
public:
	int connect(IPAddress ip, uint16_t port) override { return 0; }
	int connect(const char *host, uint16_t port) override { return 0; }
	size_t write(uint8_t) override { return 0; }
	size_t write(const uint8_t *buf, size_t size) override { return 0; }
	int available() override { return 0; }
	int read() override { return 0; }
	int read(uint8_t *buf, size_t size) override { return 0; }
	int peek() override { return 0; }
	void flush() override {}
	void stop() override {}
	uint8_t connected() override { return 0; }
	operator bool() override { return 0; }
};

// ==============================================================
//! @brief I2C ポート番号列挙体
// =============================================================
namespace PORT_I2C {
	enum : uint8_t {
		PORT_1 = 0x01,
		PORT_2,
		PORT_3,
		PORT_4,
		PORT_5,
	};
}

// ==============================================================
//! @brief Modbus ポート番号列挙体
// =============================================================
namespace PORT_MODBUS {
	enum : uint8_t {
		PORT_1 = 0x01,
		PORT_2,
		PORT_3,
		PORT_4,
		PORT_5,
	};
}

/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/
//
//  MARK: Interface class
//
/*_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/*/

// ==============================================================
// MARK: Hardware Abstraction Layer (HAL) Interface Class
//! @brief ハードウェア依存機能の抽象化クラス
// @tparam T セマフォオブジェクト型
// =============================================================
template <typename T>
class IHardwareAbstractionLayer {
public:
	// ==============================================================
	//! @brief ハードウェア依存機能の初期化を行います
	// =============================================================
	virtual void begin() = 0;

	// ==============================================================
	//! @brief ループごとに呼び出す処理を実行します
	// =============================================================
	virtual void update() = 0;

	// ==============================================================
	//! @brief 指定した時間内にボタンが押されるかを監視する
	//! @param[in] (wait_ms) 待機時間
	//! @return 押されたボタンのビットフラグ（PRESSED_BUTTON）
	//! @see PRESSED_BUTTON
	// =============================================================
	virtual uint8_t waitForButtonPressed(uint16_t wait_ms) = 0;

	// ==============================================================
	//! @brief ボタンAが押されたかを取得する
	//! @return 押されたら HIGH, それ以外は LOW を返す
	// =============================================================
	virtual uint8_t BtnA_wasPressed() = 0;

	// ==============================================================
	//! @brief ボタンBが押されたかを取得する
	//! @return 押されたら HIGH, それ以外は LOW を返す
	// =============================================================
	virtual uint8_t BtnB_wasPressed() = 0;

	// ==============================================================
	//! @brief ボタンCが押されたかを取得する
	//! @return 押されたら HIGH, それ以外は LOW を返す
	// =============================================================
	virtual uint8_t BtnC_wasPressed() = 0;

	// ==============================================================
	//! @brief ボタンAが押されているかを取得する
	//! @return 押されていたら HIGH, それ以外は LOW を返す
	// =============================================================
	virtual uint8_t BtnA_isPressed() = 0;

	// ==============================================================
	//! @brief ボタンBが押されているかを取得する
	//! @return 押されていたら HIGH, それ以外は LOW を返す
	// =============================================================
	virtual uint8_t BtnB_isPressed() = 0;

	// ==============================================================
	//! @brief ボタンCが押されているかを取得する
	//! @return 押されていたら HIGH, それ以外は LOW を返す
	// =============================================================
	virtual uint8_t BtnC_isPressed() = 0;

	// ==============================================================
	//! @brief チップのUUIDを取得する
	//! @return チップのUUID文字列を返す
	// =============================================================
	virtual String getChipUUID(bool include_prefix = true) = 0;

	// ==============================================================
	//! @brief システムを再起動します
	// =============================================================
	virtual void reboot() = 0;

	// ==============================================================
	//! @brief 指定された時間待ちます
	//! @param[in] (ms) 待ち時間（ミリ秒）
	// =============================================================
	virtual void delay(uint32_t ms) {
		::delay(ms);
	}

	// ==============================================================
	//! @brief 初期化（setup 関数）終了時に呼び出されるコールバック関数です
	// =============================================================
	virtual void endSetup() {
		// empty
	}

	// ==============================================================
	//! @brief バッテリー残量（％）を取得します
	//! @return バッテリー残量（％）、不明な場合は -1 を返す
	// =============================================================
	virtual int8_t getBatteryLevel() {
		return -1;
	}

	// ==============================================================
	//! @brief 空きヒープサイズ（バイト）を取得します
	//! @return 空きヒープサイズ、不明な場合は 0 を返す
	// =============================================================
	virtual uint32_t getFreeHeapSize() {
		return 0;
	}

	// ==============================================================
	//! @brief 稼働開始以降の最小空きヒープサイズ（バイト）を取得します
	//! @return 最小空きヒープサイズ、不明な場合は 0 を返す
	// =============================================================
	virtual uint32_t getMinimumFreeHeapSize() {
		return 0;
	}

	// ==============================================================
	//! @brief 乱数を取得します
	//! @param[in] (minimum) 乱数の下限値
	//! @param[in] (maximum) 乱数の上限値
	//! @return 取得した乱数
	//! @attention 生成される乱数の範囲は [minimum, maximum)
	// =============================================================
	virtual uint32_t getRandomNumber(uint32_t minimum = 0,
	                                 uint32_t maximum = UINT32_MAX) {
		return minimum;
	}

	// ==============================================================
	//! @brief バッテリースリープ機能の妨害用処理を実行します
	//! @param[in] (on_off) オンオフ
	// =============================================================
	virtual void preventBatterySleep(bool on_off) {
		// Empty
	}

	// ==============================================================
	//! @brief メインループ内で定期的に実行されるデバッグ用処理を実行します
	// =============================================================
	virtual void periodicLoopTaskDebugMessage() {
		// Empty
	}

	// ==============================================================
	//! @brief 送信ループ内で定期的に実行されるデバッグ用処理を実行します
	// =============================================================
	virtual void periodicTxTaskDebugMessage() {
		// Empty
	}

	// ==============================================================
	//! @brief デバッグコンソールからの入力を受け取ります
	//! @return 受信したシリアルデータの最初の1バイト、データがない場合は-1
	// =============================================================
	virtual int readSerialInput() {
		if (Serial.available() > 0) {
			return Serial.read();
		}
		return -1;
	}

	// ==============================================================
	//! @brief デバッグコンソールへの出力処理を行います
	//! @param[in] (*fmt) フォーマット文字列
	// =============================================================
	virtual void printf(const char *fmt, ...) {
		char s[256] = {0};
		va_list args;

		::va_start(args, fmt);
		::vsnprintf(s, 256, fmt, args);
		::va_end(args);

		Serial.print(s);
	}

	// ==============================================================
	//! @brief デバッグコンソールへの出力処理を行います
	//! @param[in] (*s) 出力する文字列
	// =============================================================
	virtual void println(const char *s) {
		Serial.println(s);
	}

	// ==============================================================
	//! @brief デバッグコンソールへの出力処理を行います
	//! @param[in] (*s) 出力する文字列
	// =============================================================
	virtual void print(const char *s) {
		Serial.print(s);
	}

	// ==============================================================
	//! @brief 使用するデータリンクレイヤを返します
	// =============================================================
	virtual DATALINK_LAYER getUseLinkLayer() {
		return _use_layer;
	}

	// ==============================================================
	//! @brief 使用するデータリンクレイヤをセットします
	//! @param[in] (layer) 使用するデータリンクレイヤの列挙体
	// =============================================================
	virtual void setUseLinkLayer(DATALINK_LAYER layer) {
		_use_layer = layer;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを初期化します
	//! @param[in] (key) 識別子（名前）
	//! @return 初期化が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool initializeSemaphoreMutex(String key) {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを取得します
	//! @param[in] (key) 識別子（名前）
	//! @param[in] (wait_tick_time) 最大待ち時間
	//! @return 取得が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool takeSemaphoreMutex(String key, uint32_t wait_tick_time) {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを解放します
	//! @param[in] (key) 識別子（名前）
	//! @return 解放が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool giveSemaphoreMutex(String key) {
		// Empty
		return true;
	}

protected:
	DATALINK_LAYER _use_layer = DATALINK_LAYER::UNKNOWN;
	std::map<String, T> _semaphore_mutex;
};

// ==============================================================
// MARK: Display Control Interface Class
//! @brief 表示系（Display）機能の抽象化クラス
// =============================================================
class IDisplayControl {
public:
	// ==============================================================
	//! @brief 表示系の指定されたピクセルを描画します
	//! @param[in] (x) 描画する x 座標
	//! @param[in] (y) 描画する y 座標
	//! @param[in] (color) 描画する色
	// =============================================================
	virtual void drawPixel(int32_t x, int32_t y, uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に文字を描画します
	//! @param[in] (x) 描画する x 座標
	//! @param[in] (y) 描画する y 座標
	//! @param[in] (c) 描画する文字
	//! @param[in] (color) 描画する前景色
	//! @param[in] (bg) 描画する背景色
	//! @param[in] (size) 描画する大きさ
	// =============================================================
	virtual void drawChar(int32_t x, int32_t y,
	                      uint16_t c, uint32_t color,
	                      uint32_t bg, uint8_t size) = 0;

	// ==============================================================
	//! @brief 表示系に線を描画します
	//! @param[in] (x0) 描画始点の x 座標
	//! @param[in] (y0) 描画始点の y 座標
	//! @param[in] (x1) 描画終点の x 座標
	//! @param[in] (y1) 描画終点の y 座標
	//! @param[in] (color) 描画する色
	// =============================================================
	virtual void drawLine(int32_t x0, int32_t y0,
	                      int32_t x1, int32_t y1,
	                      uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に垂直線を描画します
	//! @param[in] (x) 描画始点の x 座標
	//! @param[in] (y) 描画始点の y 座標
	//! @param[in] (h) 長さ
	//! @param[in] (color) 描画する色
	// =============================================================
	virtual void drawFastVLine(int32_t x, int32_t y,
	                           int32_t h, uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に水平線を描画します
	//! @param[in] (x) 描画始点の x 座標
	//! @param[in] (y) 描画始点の y 座標
	//! @param[in] (w) 長さ
	//! @param[in] (color) 描画する色
	// =============================================================
	virtual void drawFastHLine(int32_t x, int32_t y,
	                           int32_t w, uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系を単一色で塗りつぶします
	//! @param[in] (color) 描画する色
	// =============================================================
	virtual void fillScreen(uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に文字を描画します
	//! @param[in] (uniCode) 描画する文字
	//! @param[in] (x) 描画する x 座標
	//! @param[in] (y) 描画する y 座標
	//! @param[in] (font) フォント
	//! @return 描画した横幅
	// =============================================================
	virtual int16_t drawChar(uint16_t uniCode, int32_t x,
	                         int32_t y, uint8_t font) = 0;

	// ==============================================================
	//! @brief 表示系に文字を描画します
	//! @param[in] (uniCode) 描画する文字
	//! @param[in] (x) 描画する x 座標
	//! @param[in] (y) 描画する y 座標
	//! @return 描画した横幅
	// =============================================================
	virtual int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y) = 0;

	// ==============================================================
	//! @brief 表示系の画面高さを取得します
	//! @return 画面の縦幅
	// =============================================================
	virtual int16_t height(void) = 0;

	// ==============================================================
	//! @brief 表示系の画面幅を取得します
	//! @return 画面の横幅
	// =============================================================
	virtual int16_t width(void) = 0;

	// ==============================================================
	//! @brief 表示系に文字列を描画します
	//! @param[in] (string) 描画する文字列
	//! @param[in] (poX) 描画を開始する x 座標
	//! @param[in] (poY) 描画を開始する y 座標
	//! @param[in] (font) フォント
	//! @return 描画した横幅
	// =============================================================
	virtual int16_t drawString(const char *string, int32_t poX,
	                           int32_t poY, uint8_t font) = 0;

	// ==============================================================
	//! @brief 表示系に文字列を描画します
	//! @param[in] (string) 描画する文字列
	//! @param[in] (poX) 描画を開始する x 座標
	//! @param[in] (poY) 描画を開始する y 座標
	//! @return 描画した横幅
	// =============================================================
	virtual int16_t drawString(const char *string,
	                           int32_t poX,
	                           int32_t poY) = 0;

	// ==============================================================
	//! @brief 色変換を行います
	//! @param[in] (red) 赤色
	//! @param[in] (green) 緑色
	//! @param[in] (blue) 青色
	//! @return 16bit カラー
	// =============================================================
	virtual int16_t color565(uint8_t red,
	                         uint8_t green,
	                         uint8_t blue) = 0;

	// ==============================================================
	//! @brief 表示系に矩形を描画します
	//! @param[in] (x) 矩形左上の x 座標
	//! @param[in] (y) 矩形左上の y 座標
	//! @param[in] (w) 幅
	//! @param[in] (h) 高さ
	//! @param[in] (color) 色
	// =============================================================
	virtual void drawRect(int32_t x, int32_t y,
	                      int32_t w, int32_t h,
	                      uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に内部を塗りつぶした矩形を描画します
	//! @param[in] (x) 矩形左上の x 座標
	//! @param[in] (y) 矩形左上の y 座標
	//! @param[in] (w) 幅
	//! @param[in] (h) 高さ
	//! @param[in] (color) 色
	// =============================================================
	virtual void fillRect(int32_t x, int32_t y,
	                      int32_t w, int32_t h,
	                      uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に角の丸い矩形を描画します
	//! @param[in] (x0) 矩形左上の x 座標
	//! @param[in] (y0) 矩形左上の y 座標
	//! @param[in] (w) 幅
	//! @param[in] (h) 高さ
	//! @param[in] (radius) 角の半径
	//! @param[in] (color) 色
	// =============================================================
	virtual void drawRoundRect(int32_t x0, int32_t y0,
	                           int32_t w, int32_t h,
	                           int32_t radius, uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に角の丸い塗りつぶした矩形を描画します
	//! @param[in] (x0) 矩形左上の x 座標
	//! @param[in] (y0) 矩形左上の y 座標
	//! @param[in] (w) 幅
	//! @param[in] (h) 高さ
	//! @param[in] (radius) 角の半径
	//! @param[in] (color) 色
	// =============================================================
	virtual void fillRoundRect(int32_t x0, int32_t y0,
	                           int32_t w, int32_t h,
	                           int32_t radius, uint32_t color) = 0;

	// ==============================================================
	//! @brief 画面の方向を設定します
	//! @param[in] (r) 描画方向
	// =============================================================
	virtual void setRotation(uint8_t r) = 0;

	// ==============================================================
	//! @brief 描画を開始します
	// =============================================================
	virtual void startWrite() {}

	// ==============================================================
	//! @brief 描画を終了します
	// =============================================================
	virtual void endWrite() {}

	// ==============================================================
	//! @brief 表示系に円を描画します
	//! @param[in] (x0) 中心の x 座標
	//! @param[in] (y0) 中心の y 座標
	//! @param[in] (r) 半径
	//! @param[in] (color) 色
	// =============================================================
	virtual void drawCircle(int32_t x0, int32_t y0,
	                        int32_t r, uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に塗りつぶした円を描画します
	//! @param[in] (x0) 中心の x 座標
	//! @param[in] (y0) 中心の y 座標
	//! @param[in] (r) 半径
	//! @param[in] (color) 色
	// =============================================================
	virtual void fillCircle(int32_t x0, int32_t y0,
	                        int32_t r, uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に楕円を描画します
	//! @param[in] (x0) 中心の x 座標
	//! @param[in] (y0) 中心の y 座標
	//! @param[in] (rx) 横幅の半径
	//! @param[in] (ry) 縦幅の半径
	//! @param[in] (color) 色
	// =============================================================
	virtual void drawEllipse(int16_t x0, int16_t y0,
	                         int32_t rx, int32_t ry,
	                         uint16_t color) = 0;

	// ==============================================================
	//! @brief 表示系に塗りつぶした楕円を描画します
	//! @param[in] (x0) 中心の x 座標
	//! @param[in] (y0) 中心の y 座標
	//! @param[in] (rx) 横幅の半径
	//! @param[in] (ry) 縦幅の半径
	//! @param[in] (color) 色
	// =============================================================
	virtual void fillEllipse(int16_t x0, int16_t y0,
	                         int32_t rx, int32_t ry,
	                         uint16_t color) = 0;

	// ==============================================================
	//! @brief 表示系に三角形を描画します
	//! @param[in] (x0) 頂点１の x 座標
	//! @param[in] (y0) 頂点１の y 座標
	//! @param[in] (x1) 頂点２の x 座標
	//! @param[in] (y1) 頂点２の y 座標
	//! @param[in] (x2) 頂点３の x 座標
	//! @param[in] (y2) 頂点３の y 座標
	//! @param[in] (color) 色
	// =============================================================
	virtual void drawTriangle(int32_t x0, int32_t y0,
	                          int32_t x1, int32_t y1,
	                          int32_t x2, int32_t y2,
	                          uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に塗りつぶした三角形を描画します
	//! @param[in] (x0) 頂点１の x 座標
	//! @param[in] (y0) 頂点１の y 座標
	//! @param[in] (x1) 頂点２の x 座標
	//! @param[in] (y1) 頂点２の y 座標
	//! @param[in] (x2) 頂点３の x 座標
	//! @param[in] (y2) 頂点３の y 座標
	//! @param[in] (color) 色
	// =============================================================
	virtual void fillTriangle(int32_t x0, int32_t y0,
	                          int32_t x1, int32_t y1,
	                          int32_t x2, int32_t y2,
	                          uint32_t color) = 0;

	// ==============================================================
	//! @brief 表示系に画像を描画します
	//! @param[in] (x) 描画開始地点の x 座標
	//! @param[in] (y) 描画開始地点の y 座標
	//! @param[in] (w) 画像の横幅
	//! @param[in] (h) 画像の縦幅
	//! @param[in] (*data) 画像データ
	// =============================================================
	virtual void pushImage(int32_t x, int32_t y,
	                       int32_t w, int32_t h, uint16_t *data) {
		// Empty
		return;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを初期化します
	//! @return 初期化が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool initializeSemaphoreMutex() {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを取得します
	//! @param[in] (wait_tick_time) 最大待ち時間
	//! @return 取得が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool takeSemaphoreMutex(uint32_t wait_tick_time) {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを解放します
	//! @return 解放が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool giveSemaphoreMutex() {
		// Empty
		return true;
	}
};

// ==============================================================
// MARK: Timer Control Interface Class
//! @brief タイマ割込み機能の抽象化クラス
// =============================================================
class ITimerControl {
public:
	// ==============================================================
	//! @brief タイマコントロールクラスのインスタンス
	//! @param[in] (timer_num) タイマ識別番号
	// =============================================================
	ITimerControl(uint8_t timer_num) {
		_timer_num = timer_num;
	}

	// ==============================================================
	//! @brief タイマを初期化します
	//! @param[in] (interval_ms) 割込み周期（ミリ秒）
	// =============================================================
	virtual void begin(uint32_t interval_ms) = 0;

	// ==============================================================
	//! @brief サブタイマを追加します
	//! @param[in] (key) サブタイマの識別名
	//! @param[in] (count_limit) カウンタ閾値
	//! @param[in] (initial_count) 初期カウンタ値
	// =============================================================
	virtual void addSubTimer(std::string key,
	                         uint32_t count_limit,
	                         uint32_t initial_count = 0) = 0;
	// ==============================================================
	//! @brief サブタイマの閾値を取得します
	//! @param[in] (key) サブタイマの識別名
	//! @return 指定されたサブタイマの閾値
	// =============================================================
	virtual uint32_t getSubTimerLimit(std::string key) = 0;

	// ==============================================================
	//! @brief サブタイマの閾値をミリ秒として取得します
	//! @param[in] (key) サブタイマの識別名
	//! @return 指定されたサブタイマの閾値をミリ秒に変換した値
	// =============================================================
	virtual uint32_t getSubTimerLimitMillis(std::string key) = 0;

	// ==============================================================
	//! @brief サブタイマを削除します
	//! @param[in] (key) サブタイマの識別名
	// =============================================================
	virtual void removeSubTimer(std::string key) = 0;

	// ==============================================================
	//! @brief サブタイマのカウント値が閾値を超えていないか確認します
	//! @param[in] (key) サブタイマの識別名
	//! @param[in] (reset) 自動再初期化機能の有効化
	//! @return 閾値以上の場合は true, それ以外の場合は false が返ります
	// =============================================================
	virtual bool checkSubTimer(std::string key, bool reset = true) = 0;

	// ==============================================================
	//! @brief タイマをスタートします
	//! @param[in] (enable_first_interrupt) 最初の割り込みを有効化
	// =============================================================
	virtual void start(bool enable_first_interrupt = false) = 0;

	// ==============================================================
	//! @brief 割り込みが発生したか確認します
	//! @return 割り込みが発生していた場合は true, それ以外は false を返します
	// =============================================================
	virtual bool interrupted() = 0;

protected:
	volatile uint8_t _timer_num;
};

// ==============================================================
// MARK: Wireless Lan Control Interface Class
//! @brief 無線通信機能の抽象化クラス
// @tparam T client オブジェクト型
// =============================================================
template <typename T>
class IWirelessLanControl {
public:
	// ==============================================================
	//! @brief Client オブジェクトを取得します
	//! @param[in] (key) クライアント識別名
	//! @param[in] (recreate) 再生成する場合は true
	//! @return Client オブジェクトへのポインタを返します
	// =============================================================
	virtual T *getClient(String key, bool recreate = false) {
		if (recreate) {
			_clients.erase(key);
		}

		auto itr = _clients.find(key);
		if (itr != _clients.end()) {
			// Exist
			return _clients[key].get();
		}

		_clients[key] = std::unique_ptr<T>(new T());
		return _clients[key].get();
	}

	// ==============================================================
	//! @brief 設定されている SSID を取得します
	//! @return SSID
	// =============================================================
	virtual String getSSID() {
		return _ap_name;
	}

	// ==============================================================
	//! @brief 現在の状態を取得します
	//! @return ステータス番号を返します
	//! @note 実装によって意味が変化する可能性があります
	// =============================================================
	virtual int getStatus() = 0;

	// ==============================================================
	//! @brief 設定されているアクセスポイントの信号強度（RSSI）を取得します
	//! @return 信号強度（dBm）
	// =============================================================
	virtual int getRSSI() = 0;

	// ==============================================================
	//! @brief 無線通信機能のMACアドレスを取得します
	//! @return MACアドレスのベクトル
	// =============================================================
	virtual std::vector<uint8_t> fetchMacAddress() {
		return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	}

	// ==============================================================
	//! @brief 割り当てられた IP アドレスを取得します
	//! @return 割り当てられた IP アドレス
	//! @see MyIPAddress
	// =============================================================
	virtual MyIPAddress localIP() = 0;

	// ==============================================================
	//! @brief 静的情報を設定します
	//! @param[in] (ip) 指定するIPアドレス
	//! @param[in] (gateway) 指定するゲートウェイ
	//! @param[in] (subnet) 指定するサブネットマスク
	//! @param[in] (dns1) 指定するDNSサーバ1
	//! @param[in] (dns2) 指定するDNSサーバ2
	//! @return 接続成功の場合は true、失敗の場合は false を返します
	// =============================================================
	virtual bool config(String ip,
	                    String gateway,
	                    String subnet,
	                    String dns1,
	                    String dns2) = 0;

	// ==============================================================
	//! @brief 無線LANへの接続を試みます
	//! @param[in] (timeout_ms) タイムアウト時間（ミリ秒）
	//! @return 接続成功の場合は true、失敗の場合は false を返します
	//! @see HAL_WLAN_API::connectWlan(String ap_name, String pass, unsigned long timeout_ms)
	// =============================================================
	virtual bool connectWlan(unsigned long timeout_ms = 0) = 0;

	// ==============================================================
	//! @brief 無線LANへの接続を試みます
	//! @param[in] (ap_name) アクセスポイント名を指定します
	//! @param[in] (pass) アクセスポイントのパスワードを指定します
	//! @param[in] (timeout_ms) タイムアウト時間（ミリ秒）
	//! @return 接続成功の場合は true、失敗の場合は false を返します
	//! @see HAL_WLAN_API::connectWlan(String ap_name, String pass, unsigned long timeout_ms)
	// =============================================================
	virtual bool connectWlan(String ap_name, String pass,
	                         unsigned long timeout_ms = 0) = 0;

	// ==============================================================
	//! @brief 無線LANを切断します
	//! @param[in] (exit_sta_mode) STAモードを終了するか
	//! @param[in] (erase_ap_settings) アクセスポイント情報を消去するか
	// =============================================================
	virtual void disconnectWlan(bool exit_sta_mode     = true,
	                            bool erase_ap_settings = true) = 0;

	// ==============================================================
	//! @brief 対象のアクセスポイントが検出できるか調査します
	//! @param[in] (ap_name) 検索対象のアクセスポイント名
	//! @param[out] (*rssi) 検出された場合の信号強度
	//! @param[out] (*exist_same_name) 同名の対象アクセスポイント名が複数存在するか
	//! @param[in] (print_result) 検出したアクセスポイントを表示するか
	//! @return 対象のアクセスポイントが発見されれば true、見つからなければ false を返す
	//! @see HAL_WLAN_API::scanAccessPoint
	// =============================================================
	virtual bool detectTargetAccessPoint(String ap_name,
	                                     int32_t *rssi,
	                                     bool *exist_same_name,
	                                     bool print_result = true) = 0;

	// ==============================================================
	//! @brief mDNS による名前解決を行います
	//! @param[in] (host) 検索対象のホスト名
	//! @param[in] (try_count) 試行回数
	//! @return IPアドレスが返されます。見つからない場合は 0.0.0.0 が返されます。
	// =============================================================
	virtual MyIPAddress resolveMDNS(String host, uint8_t try_count = 2) = 0;

	// ==============================================================
	//! @brief アクセスポイントを探索します
	//! @param[in] (print_result) 検出したアクセスポイントを表示するか
	//! @return 検出したアクセスポイントのリスト（SSID）
	// =============================================================
	virtual std::map<int, String> scanAccessPoint(bool print_result = true) = 0;

	// ==============================================================
	//! @brief 無線接続がデバイスでサポートされているかを返します
	//! @return サポートされていれば true, それ以外は false を返します
	// =============================================================
	virtual bool isSupported() {
		return true;
	}

protected:
	String _ap_name = "";
	String _pass    = "";
	std::map<String, std::unique_ptr<T>> _clients;
};

// ==============================================================
// MARK: Wired Lan Control Interface Class
//! @brief 有線通信機能の抽象化クラス
// @tparam T client オブジェクト型
// =============================================================
template <typename T>
class IWiredLanControl {
public:
	// ==============================================================
	//! @brief Client オブジェクトを取得します
	//! @param[in] (key) クライアント識別名
	//! @param[in] (recreate) 再生成する場合は true
	//! @return Client オブジェクトへのポインタを返します
	// =============================================================
	virtual T *getClient(String key, bool recreate = false) {
		if (recreate) {
			_clients.erase(key);
		}

		auto itr = _clients.find(key);
		if (itr != _clients.end()) {
			// Exist
			return _clients[key].get();
		}

		_clients[key] = std::unique_ptr<T>(new T());
		return _clients[key].get();
	}

	// ==============================================================
	//! @brief 有線通信機能を初期化します
	//! @param[in] (ip) 固定IPアドレス（オプション）
	//! @param[in] (gateway) ゲートウェイ（オプション）
	//! @param[in] (subnet) サブネットマスク（オプション）
	//! @param[in] (dns) DNSサーバ（（オプション））
	//! @return 初期化に成功すれば true, それ以外は false を返す
	//! @note デフォルト引数を含む不正な値が指定された場合は DHCP を使用します
	// =============================================================
	virtual bool begin(String ip      = "",
	                   String gateway = "",
	                   String subnet  = "",
	                   String dns     = "") {
		return false;
	}

	// ==============================================================
	//! @brief 現在の状態を取得します
	//! @return ステータス番号
	//! @attention 実装によって番号の意味が異なる場合があります
	// =============================================================
	virtual int getStatus() {
		return 0;
	}

	// ==============================================================
	//! @brief 接続状態を取得します
	//! @return ネットワークに接続できている場合は true, それ以外は false を返す
	// =============================================================
	virtual bool isLinkUp() {
		return false;
	}

	// ==============================================================
	//! @brief 有線通信機能のMACアドレスを取得します
	//! @return MACアドレスのベクトル
	// =============================================================
	virtual std::vector<uint8_t> fetchMacAddress() {
		return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	}

	// ==============================================================
	//! @brief 割り当てられた IP アドレスを取得します
	//! @return 割り当てられた IP アドレス
	//! @see MyIPAddress
	// =============================================================
	virtual MyIPAddress localIP() {
		return MyIPAddress(0, 0, 0, 0);
	}

	// ==============================================================
	//! @brief mDNS による名前解決を行います
	//! @param[in] (host) 検索対象のホスト名
	//! @param[in] (try_count) 試行回数
	//! @return IPアドレスが返されます。見つからない場合は 0.0.0.0 が返されます。
	// =============================================================
	virtual MyIPAddress resolveMDNS(String host, uint8_t try_count = 2) {
		return MyIPAddress(0, 0, 0, 0);
	}

	// ==============================================================
	//! @brief 有線接続がデバイスでサポートされているかを返します
	//! @return サポートされていれば true, それ以外は false を返します
	// =============================================================
	virtual bool isSupported() {
		return false;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを初期化します
	//! @return 初期化が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool initializeSemaphoreMutex() {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを取得します
	//! @param[in] (wait_tick_time) 最大待ち時間
	//! @return 取得が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool takeSemaphoreMutex(uint32_t wait_tick_time) {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを解放します
	//! @return 解放が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool giveSemaphoreMutex() {
		// Empty
		return true;
	}

protected:
	std::map<String, std::unique_ptr<T>> _clients;
};

// ==============================================================
// MARK: NTP RTC Control Interface Class
//! @brief NTP 機能の抽象化クラス
// =============================================================
class INtpRtcControl {
public:
	// ==============================================================
	//! @brief 時刻同期に使用する NTP サーバを追加します
	//! @param[in] (*server) サーバ名
	// =============================================================
	virtual void addNtpServer(const char *server) {
		servers.push_back(server);
	}

	// ==============================================================
	//! @brief タイムゾーンを設定します
	//! @param[in] (*tz) 指定するタイムゾーン
	// =============================================================
	virtual void setTimeZone(const char *tz) {
		time_zone = tz;
	}

	// ==============================================================
	//! @brief リアルタイムクロックを初期化します
	//! @return 成功すれば true、それ以外は false を返す
	// =============================================================
	virtual bool begin() = 0;

	// ==============================================================
	//! @brief 時刻取得が可能かどうかを返します
	//! @return 可能な場合は true、それ以外は false を返す
	// =============================================================
	virtual bool ready() = 0;

	// ==============================================================
	//! @brief リアルタイムクロックを NTP サーバと再同期します
	//! @return 成功すれば true、失敗すれば false を返す
	// =============================================================
	virtual bool update() { return false; };

	// ==============================================================
	//! @brief 現在の時間を取得します
	//! @param[out] (*timeinfo) 取得した時間を格納します
	//! @return 成功すれば true、失敗すれば false を返す
	//! @details 時間は設定したタイムゾーンに準拠します
	// =============================================================
	virtual bool getCurrentTime(struct tm *timeinfo) = 0;

protected:
	std::vector<String> servers = {};
	String time_zone            = "JST-9";
};

// ==============================================================
// MARK: LED Control Interface Class
//! @brief LED 機能の抽象化クラス
// =============================================================
class ILedControl {
public:
	// ==============================================================
	//! @brief LED の初期化を行います
	// =============================================================
	virtual void begin() {
		// Empty
	}

	// ==============================================================
	//! @brief 点灯時の明るさレベルを設定します
	//! @param[in] (percentage) 明るさのレベル（%）
	// =============================================================
	virtual void setBrightnessLevel(double percentage = 100) {
		// Empty
	}

	// ==============================================================
	//! @brief LED を点灯させます
	// =============================================================
	virtual void on() {
		// Empty
	}

	// ==============================================================
	//! @brief LED を消灯させます
	// =============================================================
	virtual void off() {
		// Empty
	}

	// ==============================================================
	//! @brief LED の点灯状態を反転させます
	// =============================================================
	virtual void toggle() {
		// Empty
	}
};

// ==============================================================
// MARK: Setting File Input/Output Control Interface Class
//! @brief 設定ファイル入出力機能の抽象化クラス
// =============================================================
class ISettingFileInputOutputControl {
public:
	// ==============================================================
	//! @brief 設定ファイルを記憶領域に保存します
	//! @param[in] (fpath) 保存するファイルパス
	//! @param[in] (json) 保存する設定値
	//! @return 成功すれば true, それ以外は false を返します
	// =============================================================
	virtual bool saveSettings(String fpath, JsonDocument json) = 0;

	// ==============================================================
	//! @brief 設定ファイルを読み込みます
	//! @param[in] (fpath) 読み込むファイルパス
	//! @param[out] (&json) 読み取った設定値
	//! @return 成功すれば true, それ以外は false を返します
	// =============================================================
	virtual bool loadSettings(String fpath, JsonDocument &json) = 0;

	// ==============================================================
	//! @brief 設定ファイルがあるストレージをフォーマットします
	//! @return 成功すれば true、失敗すれば false を返す
	// =============================================================
	virtual bool formatStorage() = 0;

	// ==============================================================
	//! @brief デバイス固有の設定値を返します
	//! @return 固有設定を示した Json オブジェクト
	//! @note 継承先で実装します
	// =============================================================
	virtual JsonDocument deviceUniqueSettings() {
		JsonDocument ddoc;
		return ddoc;
	}

	// ==============================================================
	//! @brief プリセット設定項目（WiFi）を使用するかどうかを返します
	//! @return 使用する場合は true, それ以外は false を返す
	// =============================================================
	virtual bool usePresetWiFi() {
		return true;
	}

	// ==============================================================
	//! @brief プリセット設定項目（Lan）を使用するかどうかを返します
	//! @return 使用する場合は true, それ以外は false を返す
	// =============================================================
	virtual bool usePresetLan() {
		return false;
	}

	// ==============================================================
	//! @brief プリセット設定項目（Ambient）を使用するかどうかを返します
	//! @return 使用する場合は true, それ以外は false を返す
	// =============================================================
	virtual bool usePresetAmbient() {
		return true;
	}

	// ==============================================================
	//! @brief プリセット設定項目（FTP）を使用するかどうかを返します
	//! @return 使用する場合は true, それ以外は false を返す
	// =============================================================
	virtual bool usePresetFtp() {
		return true;
	}

	// ==============================================================
	//! @brief プリセット設定項目（HTTP）を使用するかどうかを返します
	//! @return 使用する場合は true, それ以外は false を返す
	// =============================================================
	virtual bool usePresetHttp() {
		return true;
	}

	// ==============================================================
	//! @brief プリセット設定項目（MQTT）を使用するかどうかを返します
	//! @return 使用する場合は true, それ以外は false を返す
	// =============================================================
	virtual bool usePresetMqtt() {
		return true;
	}
};

// ==============================================================
// MARK: Bluetooth Low Energy (BLE) Control Interface Class
//! @brief BLE 機能の抽象化クラス
// =============================================================
class IBluetoothLowEnergyControl {
public:
	// ==============================================================
	//! @brief
	// =============================================================
	IBluetoothLowEnergyControl() {
		// バッファ領域を確保
		rx_buffer.data = new uint8_t[BLE_BUF_LENGTH + 1];
		tx_buffer.data = new uint8_t[BLE_BUF_LENGTH + 1];
	}
	// ==============================================================
	//! @brief
	// =============================================================
	~IBluetoothLowEnergyControl() {
		// バッファ領域を解放
		delete[] rx_buffer.data;
		delete[] tx_buffer.data;
	}

	// ==============================================================
	//! @brief BLE 機能の初期化を行います
	//! @param[in] (local_name) アドバタイズで使用する個体名
	// =============================================================
	virtual void begin(String local_name) {
		// Empty
	}

	// ==============================================================
	//! @brief 受信データが存在するか確認します
	//! @return バッファに格納されている受信データ長
	// =============================================================
	virtual uint16_t available() {
		if (rx_buffer.store_index == rx_buffer.output_index) {
			rx_buffer.length = 0;
			return 0;
		}
		return rx_buffer.length;
	}

	// ==============================================================
	//! @brief 指定されたデータを送信バッファに格納します
	//! @param[in] (*data) 送信するデータ
	//! @param[in] (size) データサイズ
	//! @return 成功すれば true, それ以外は false を返します
	//! @see 送信データサイズは BLE_BUF_LENGTH 以下に収める必要があります
	// =============================================================
	virtual bool write(uint8_t *data, size_t size) {
		if (size > BLE_BUF_LENGTH) {
			return false;
		}

		std::fill_n(tx_buffer.data, size, 0x00);
		memcpy(tx_buffer.data, data, size);

		tx_buffer.length = size + 1;
		return true;
	}

	// ==============================================================
	//! @brief 受信バッファからデータを１つ読み出します
	//! @return 読みだしたデータ
	//! @note 事前に available メソッドで取得可能か確認すべきです
	// =============================================================
	virtual uint8_t read() {
		if (rx_buffer.store_index == rx_buffer.output_index) {
			return 0x00;
		}

		uint8_t ret            = rx_buffer.data[rx_buffer.output_index];
		rx_buffer.output_index = ((rx_buffer.output_index + 1) == BLE_BUF_LENGTH) ? 0 : (rx_buffer.output_index + 1);
		rx_buffer.length       = (rx_buffer.length > 0) ? (rx_buffer.length - 1) : 0;
		return ret;
	}

	// ==============================================================
	//! @brief 送信準備が完了しているか確認します
	//! @return 準備が完了している場合は true, それ以外は false が返ります
	// =============================================================
	virtual bool txReady() {
		return (tx_buffer.length == 0) ? true : false;
	}

	// Buffer control storage
	struct LocalBuffer {
		uint8_t *data;
		uint16_t store_index;
		uint16_t output_index;
		size_t length;
	};

	// グローバルな呼び出しが必要なため、静的変数にする
	static LocalBuffer rx_buffer;
	static LocalBuffer tx_buffer;
};

// ==============================================================
// MARK: TF Card Control Interface Class
//! @brief TFカードコントロール機能の抽象化クラス
// =============================================================
class ITfCardControl {
public:
	// ==============================================================
	//! @brief TFカード機能の初期化を行う
	//! @return 成功すれば true、失敗すれば false を返す
	// =============================================================
	virtual bool begin() {
		return false;
	}

	// ==============================================================
	//! @brief TFカードが利用可能か確認する
	//! @return 利用可能な場合は true, それ以外は false が返る
	// =============================================================
	virtual bool ready() {
		return _tf_available;
	}

	// ==============================================================
	//! @brief パスを結合します
	//! @param[in] (p1) パス1
	//! @param[in] (p2) パス2
	//! @return 適切に結合されたパス
	// =============================================================
	virtual String joinPath(String p1, String p2) {
		if (not(p1.endsWith("/") or p2.startsWith("/"))) {
			p1.concat("/");
		}
		p1.concat(p2);
		return p1;
	}

	// ==============================================================
	//! @brief 指定のファイル/フォルダが存在するかチェックする
	//! @param[in] (fpath) 対象へのパス
	//! @return 存在すれば true、存在しないまたは確認に失敗すれば false を返す
	// =============================================================
	virtual bool exist(String fpath) {
		return false;
	}

	// ==============================================================
	//! @brief 指定パスのフォルダを再帰的に作成する
	//! @param[in] (absolute_path) 作成するディレクトリまでの絶対パス
	//! @return 作成に成功 true、作成に失敗すれば false を返す
	// =============================================================
	virtual bool makeDirRecursive(String absolute_path) {
		return false;
	}

	// ==============================================================
	//! @brief 指定のファイルに文字列を出力する
	//! @param[in] (*fpath) 対象のファイル
	//! @param[in] (*mode) ファイルオープンモード
	//! @param[in] (*str) 書き込む文字列
	//! @return 書き込みに成功すれば true、失敗すれば false を返す
	// =============================================================
	virtual bool print(const char *fpath, const char *mode, const char *str) {
		return true;
	}

	// ==============================================================
	//! @brief 指定のファイルにフォーマット文字列を出力する
	//! @param[in] (*fpath) 対象のファイル
	//! @param[in] (*mode) ファイルオープンモード
	//! @param[in] (*fmt) フォーマット文字列
	//! @param[in] (...) フォーマット用変数
	//! @return 書き込みに成功すれば true、失敗すれば false を返す
	// =============================================================
	virtual bool printf(const char *fpath, const char *mode, const char *fmt, ...) {
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを初期化します
	//! @return 初期化が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool initializeSemaphoreMutex() {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを取得します
	//! @param[in] (wait_tick_time) 最大待ち時間
	//! @return 取得が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool takeSemaphoreMutex(uint32_t wait_tick_time) {
		// Empty
		return true;
	}

	// ==============================================================
	//! @brief 排他制御用のセマフォを解放します
	//! @return 解放が成功したら true, 失敗したら false が返ります
	// =============================================================
	virtual bool giveSemaphoreMutex() {
		// Empty
		return true;
	}

protected:
	bool _tf_available = false;
};

// ==============================================================
// MARK: Debug Log Control Interface Class
//! @brief デバッグログ機能の抽象化クラス
// =============================================================
class IDebugLogControl {
public:
	// ==============================================================
	//! @brief インスタンス
	// =============================================================
	IDebugLogControl() {
		_inited         = false;
		_logging        = false;
		_dpath          = "/";
		_fname          = "LOGFILE";
		_fext           = ".log";
		_limit_linage   = 2000;
		_current_linage = 0;

		_fnumber = 0;
	}

	// ==============================================================
	//! @brief 保存先ディレクトリをセットする
	//! @param[in] (*path) 保存先ディレクトリまでの絶対パス
	// =============================================================
	virtual void setSaveDirectory(const char *path) {
		_dpath = String(path);
		if (not _dpath.startsWith("/")) {
			_dpath = "/" + _dpath;
		}
		_current_linage = 0;
	}

	// ==============================================================
	//! @brief 保存ファイル名を設定する
	//! @param[in] (*fname) ファイル名
	//! @param[in] (*file_extension) 拡張子
	// =============================================================
	virtual void setFileBaseName(const char *fname, const char *file_extension) {
		_fname = String(fname);
		_fext  = String(file_extension);
		if (not _fext.startsWith(".")) {
			_fext = "." + _fext;
		}
		_current_linage = 0;
	}

	// ==============================================================
	//! @brief ファイル分割の指標とする書き込み行数を設定する
	//! @param[in] (limit_linage) 分割する閾値行数
	// =============================================================
	virtual void setAutoFileSplitLinage(uint16_t limit_linage) {
		_limit_linage = limit_linage;
	}

	// ==============================================================
	//! @brief ロギングを開始する
	// =============================================================
	virtual void loggingStart() {
		if (not _inited) {
			_logging = false;
		} else {
			_logging = true;
		}
	}

	// ==============================================================
	//! @brief ロギングを停止する
	// =============================================================
	virtual void loggingStop() {
		_logging = false;
	}

	// ==============================================================
	//! @brief 現在のロギング状態を取得する
	//! @return ロギング中であれば true, それ以外は false が返る
	// =============================================================
	virtual bool getCurrentLoggingState() {
		return _logging;
	}

	// ==============================================================
	//! @brief ログ機能の初期化を行う
	//! @return 成功すれば true、失敗すれば false を返す
	//! @note 初期化時にディレクトリ作成とファイル名の決定を行うため、変更は事前に行うこと
	//! @see setSaveDirectory, setFileBaseName
	// =============================================================
	virtual bool begin() {
		return false;
	}

	// ==============================================================
	//! @brief ログファイルに文字列を出力する
	//! @param[in] (*str) 出力する文字列
	//! @return 書き込みに成功すれば true、失敗すれば false を返す
	//! @details 書き込み文字列は一旦バッファへ格納される。
	//! @note 時刻取得が可能な場合は、記録時間も出力される
	// =============================================================
	virtual bool print(const char *str) {
		return false;
	}

	// ==============================================================
	//! @brief ログファイルにフォーマット文字列を出力する
	//! @param[in] (*fmt) フォーマット文字列
	//! @param[in] (...) フォーマット用変数
	//! @return 書き込みに成功すれば true、失敗すれば false を返す
	//! @details 書き込み文字列は一旦バッファへ格納される。
	//! @note 時刻取得が可能な場合は、記録時間も出力される
	// =============================================================
	virtual bool printf(const char *fmt, ...) {
		return false;
	}

	// ==============================================================
	//! @brief バッファに格納されている文字列を出力する
	// =============================================================
	virtual void flush() {
		// empty
	}

protected:
	bool _inited;
	bool _logging;
	String _dpath;
	String _fname;
	String _fext;
	uint16_t _limit_linage;
	uint16_t _current_linage;

	uint16_t _fnumber; // FAT32 max file count is 65534 per directory
};

// ==============================================================
// MARK: General Purpose Input/Output (GPIO) Control Interface Class
//! @brief GPIO の抽象化クラス
// =============================================================
class IGpioControl {
public:
	// ==============================================================
	//! @brief GPIO を使用するピンを取得します
	//! @return GPIO を使用するピン番号のリスト
	// =============================================================
	virtual std::vector<uint8_t> getPins() {
		std::vector<uint8_t> pins = {};
		return pins;
	}

	// ==============================================================
	//! @brief GPIO に使用する修飾子を取得します
	//! @return 修飾子の文字列
	//! @attention 設定ファイルの修飾子と一致させる必要があります
	// =============================================================
	virtual String getModifier() {
		return "";
	}

	// ==============================================================
	//! @brief GPIO の未使用ピンのモードをセットします
	//! @param[in] (pin) ピン番号
	// =============================================================
	virtual void setUnused(uint8_t pin) {
		::pinMode(pin, INPUT_PULLUP);
	}

	// ==============================================================
	//! @brief GPIO のピンモードを設定します
	//! @param[in] (pin) ピン番号
	//! @param[in] (mode) モード（INPUT, OUTPUT, INPUT_PULLUP）
	// =============================================================
	virtual void pinMode(uint8_t pin, uint8_t mode) {
		::pinMode(pin, mode);
	}

	// ==============================================================
	//! @brief 指定されたピンのデジタル値を読み取ります
	//! @param[in] (pin) ピン番号
	//! @return 読み取ったデジタル値（HIGH/LOW）
	// =============================================================
	virtual int digitalRead(uint8_t pin) {
		return ::digitalRead(pin);
	}

	// ==============================================================
	//! @brief 指定されたピンのアナログ値を読み取ります
	//! @param[in] (pin) ピン番号
	//! @return 読み取ったアナログ値
	// =============================================================
	virtual int analogRead(uint8_t pin) {
		return ::analogRead(pin);
	}

	// ==============================================================
	//! @brief analogRead が持つ ADC 分解能（ビット数）を取得します
	//! @return ADC分解能（ビット数）
	//! @attention デフォルトは ESP32 に合わせた 12 ビット
	// =============================================================
	virtual uint8_t getAdcBits() {
		return 12;
	}

	// ==============================================================
	//! @brief パルスカウント機能の初期化を行います
	//! @param[in] (pin) パルスカウントを行うピン番号
	//! @param[in] (edge) 検出エッジパターン（RISE, FALL, CHANGE）
	//! @return 成功すれば true, それ以外は false を返す
	//! @see PULSE_EDGE
	// =============================================================
	virtual bool initializePulseCounter(uint8_t pin, PULSE_EDGE edge) {
		return false;
	}

	// ==============================================================
	//! @brief パルスカウント機能を停止します
	//! @param[in] (pin) ピン番号
	//! @return 成功すれば true, それ以外は false を返す
	// =============================================================
	virtual bool stopPulseCounter(uint8_t pin) {
		return false;
	}

	// ==============================================================
	//! @brief パルスカウント機能を開始します
	//! @param[in] (pin) ピン番号
	//! @return 成功すれば true, それ以外は false を返す
	// =============================================================
	virtual bool startPulseCounter(uint8_t pin) {
		return false;
	}

	// ==============================================================
	//! @brief 全てのパルスカウント機能を停止します
	//! @return 成功すれば true, それ以外は false を返す
	// =============================================================
	virtual bool stopPulseCounterAll() {
		return false;
	}

	// ==============================================================
	//! @brief 全てのパルスカウント機能を開始します
	//! @return 成功すれば true, それ以外は false を返す
	// =============================================================
	virtual bool startPulseCounterAll() {
		return false;
	}

	// ==============================================================
	//! @brief パルスカウントの最大値を取得します
	//! @return パルスカウントの最大値
	// =============================================================
	virtual uint32_t getMaxPulseCount() {
		return 1;
	}

	// ==============================================================
	//! @brief パルスカウント値を更新します
	//! @param[in] (pin) 対象のピン番号
	//! @return 成功すれば true, それ以外は false を返す
	//! @note カウント値は自動でリセットされません
	//! @note 最大カウント値は各マイコン/プログラムに依存します
	//! @note 取得したカウント値は _count[pin] に格納し、getLastPulseCount メソッドで取得します
	// =============================================================
	virtual bool updatePulseCount(uint8_t pin) {
		return false;
	}

	// ==============================================================
	//! @brief パルスカウント値をリセットします
	//! @param[in] (pin) 対象のピン番号
	// =============================================================
	virtual void resetPulseCount(uint8_t pin) {
		return;
	}

	// ==============================================================
	//! @brief 全てのパルスカウント値をリセットします
	// =============================================================
	virtual void resetPulseCountAll() {
		return;
	}

	// ==============================================================
	//! @brief 最後に取得したパルスカウント値を取得します
	//! @param[in] (pin) ピン番号
	//! @return 最後に取得したパルスカウント値
	// =============================================================
	virtual uint32_t getLastPulseCount(uint8_t pin) {
		if (_count.find(pin) == _count.end()) {
			return 0; // 番号が見つからない場合、0を返す
		}
		return _count[pin];
	}

protected:
	std::map<uint8_t, uint32_t> _count;
};

// ==============================================================
// MARK: Wire I2C Control Interface Class
//! @brief I2Cの抽象化クラス
// @tparam T wire オブジェクト型
// =============================================================
template <typename T>
class IWireI2cControl {
public:
	// ==============================================================
	//! @brief Wire オブジェクトを取得します
	//! @param[in] (num) ポート番号
	//! @return Wire オブジェクトへのポインタを返します。
	//! @return 存在しない場合はヌルポインタを返します。
	// =============================================================
	virtual T *getWire(uint8_t port) {
		auto itr = _wires.find(port);
		if (itr != _wires.end()) {
			// Exist
			return _wires[port];
		}

		return nullptr;
	}

	// ==============================================================
	//! @brief I2C を使用するポート番号を取得します
	//! @return I2C を使用するポート番号のリスト
	// =============================================================
	virtual std::vector<uint8_t> getPorts() {
		std::vector<uint8_t> ports = {PORT_I2C::PORT_1};
		return ports;
	}

	// ==============================================================
	//! @brief SDA ピン番号を取得します
	//! @param[in] (num) ポート番号
	//! @return 指定したポートの SDA ピン番号
	// =============================================================
	virtual uint8_t getSdaPin(uint8_t port) {
		return SDA;
	}

	// ==============================================================
	//! @brief SCL ピン番号を取得します
	//! @param[in] (num) ポート番号
	//! @return 指定したポートの SCL ピン番号
	// =============================================================
	virtual uint8_t getSclPin(uint8_t port) {
		return SCL;
	}

	// ==============================================================
	//! @brief I2C 通信ポートを初期化します
	//! @param[in] (num) 初期化するポート番号
	//! @return 初期化に成功すれば true, それ以外は false を返します。
	// * =============================================================
	virtual bool begin(uint8_t port) {
		return false;
	}

protected:
	std::map<uint8_t, T *> _wires;
};

// ==============================================================
// MARK: Modbus Control Interface Class
//! @brief Modbus機能の抽象化クラス
// =============================================================
class IModbusControl {
public:
	// ==============================================================
	//! @brief Serial オブジェクトを取得します
	//! @param[in] (num) ポート番号
	//! @return Wire オブジェクトへのポインタを返します。
	//! @return 存在しない場合はヌルポインタを返します。
	//! @note _serials への値の格納は begin メソッド内で行います。
	// =============================================================
	virtual Stream *getSerial(uint8_t port) {
		auto itr = _serials.find(port);
		if (itr != _serials.end()) {
			// Exist
			return _serials[port];
		}

		return nullptr;
	}

	// ==============================================================
	//! @brief Modbus を使用するポート番号を取得します
	//! @return Modbus を使用するポート番号のリスト
	// =============================================================
	virtual std::vector<uint8_t> getPorts() {
		std::vector<uint8_t> ports = {};
		return ports;
	}

	// ==============================================================
	//! @brief Modbus に使用する修飾子を取得します
	//! @return 修飾子の文字列
	//! @attention 設定ファイルの修飾子と一致させる必要があります
	// =============================================================
	virtual String getModifier() {
		return "MODBUS";
	}

	// ==============================================================
	//! @brief タイムアウト時間を取得します
	//! @param[in] (num) ポート番号
	//! @return タイムアウト時間をミリ秒単位で返します。
	//! @return 存在しない場合は 200ms を返します。
	//! @note _timeout_ms への値の格納は begin メソッド内で行います。
	// =============================================================
	virtual uint32_t getTimeoutMs(uint8_t port) {
		constexpr uint32_t DEFAULT_TIMEOUT_MS = 200;

		auto itr = _timeout_ms.find(port);
		if (itr != _timeout_ms.end()) {
			// Exist
			return _timeout_ms[port];
		}

		return DEFAULT_TIMEOUT_MS;
	}

	// ==============================================================
	//! @brief Tx ピン番号を取得します
	//! @param[in] (num) ポート番号
	//! @return 指定したポートの Tx ピン番号
	// =============================================================
	virtual uint8_t getTxPin(uint8_t port) {
		return 0;
	}

	// ==============================================================
	//! @brief Rx ピン番号を取得します
	//! @param[in] (num) ポート番号
	//! @return 指定したポートの Rx ピン番号
	// =============================================================
	virtual uint8_t getRxPin(uint8_t port) {
		return 0;
	}

	// ==============================================================
	//! @brief RS485 通信ポートを初期化します
	//! @param[in] (num) 初期化するポート番号
	//! @param[in] (baudrate) ボーレート
	//! @param[in] (data_bit) データビット
	//! @param[in] (parity) パリティ
	//! @param[in] (timeout_ms) タイムアウト時間
	//! @return 初期化に成功すれば true, それ以外は false を返す
	//! @attention ストップビットは 1 に固定されます
	//! @attention パリティは（1:奇数）（2:偶数）（その他：なし）となります
	// =============================================================
	virtual bool begin(uint8_t port, uint32_t baudrate, uint8_t data_bit, uint8_t parity, uint32_t timeout_ms) {
		return false;
	}

protected:
	std::map<uint8_t, Stream *> _serials;
	std::map<uint8_t, uint32_t> _timeout_ms;
};

// ==============================================================
// MARK: Camera Control Interface Class
//! @brief カメラ機能の抽象化クラス
// =============================================================
class ICameraControl {
public:
	// 処理速度向上のため public で扱うが、原則書き込みは禁止
	std::vector<uint8_t> capturedImage; // begin で resize することを推奨
	std::vector<uint8_t> scaledImage;
	std::vector<uint8_t> jpgImage;

	// ==============================================================
	//! @brief カメラデバイスの初期化を行う
	//! @return 成功すれば true, それ以外は false を返す
	// =============================================================
	virtual bool begin() { return false; }

	// ==============================================================
	//! @brief カメラデバイスが搭載されているか確認する
	//! @return 搭載されていれば true, それ以外は false を返す
	// =============================================================
	virtual bool isInstalled() { return false; }

	// ==============================================================
	//! @brief カメラデバイスの準備が完了しているか確認する
	//! @return 準備が完了していれば true, それ以外は false を返す
	// =============================================================
	virtual bool isReady() { return _inited; }

	// ==============================================================
	//! @brief 画像を取得する
	//! @return 取得に成功すれば true, それ以外は false を返す
	//! @note 取得した画像はインスタンス変数 capturedImage に格納される
	// =============================================================
	virtual bool updateCapture() { return false; }

	// ==============================================================
	//! @brief キャプチャする画像のサイズ（フレームサイズ）を取得する
	//! @return フレームサイズ名称
	// =============================================================
	virtual String getFrameSizeName() { return "UNKNOWN"; }

	// ==============================================================
	//! @brief センサ名を取得する
	//! @return センサ名
	// =============================================================
	virtual String getSensorName() { return "UNKNOWN"; }

	// ==============================================================
	//! @brief 取得した画像の横幅を取得する
	//! @return capturedImage の横幅
	//! @note updateCapture 実行時に更新される
	// =============================================================
	virtual uint32_t capturedWidth() { return _capturedWidth; }

	// ==============================================================
	//! @brief 取得した画像の高さを取得する
	//! @return capturedImage の高さ
	//! @note updateCapture 実行時に更新される
	// =============================================================
	virtual uint32_t capturedHeight() { return _capturedHeight; }

	// ==============================================================
	//! @brief リサイズされた画像の横幅を取得する
	//! @return scaledImage の横幅
	//! @note resizeCapturedImage 実行時に更新される
	// =============================================================
	virtual uint32_t scaledWidth() { return _scaledWidth; }

	// ==============================================================
	//! @brief リサイズされた画像の高さを取得する
	//! @return scaledImage の高さ
	//! @note resizeCapturedImage 実行時に更新される
	// =============================================================
	virtual uint32_t scaledHeight() { return _scaledHeight; }

	// ==============================================================
	//! @brief capturedImage を jpg 形式に変換する
	//! @return 成功すれば true, それ以外は false を返す
	//! @note 変換結果は jpgImage に格納される
	// =============================================================
	virtual bool convert2jpg() { return false; }

	// ==============================================================
	//! @brief キャプチャした画像を任意の大きさに収まるようにリサイズする
	//! @param[in] (maxWidth) 最大幅
	//! @param[in] (maxHeight) 最大高さ
	//! @return 成功すれば true, それ以外は false を返す
	//! @note アスペクト比は保持される
	//! @note 変換結果は scaledImage に格納される
	// =============================================================
	virtual bool resizeCapturedImage(uint32_t maxWidth, uint32_t maxHeight) {
		if (_capturedWidth == 0 or _capturedHeight == 0) {
			return false;
		}

		double widthRatio  = maxWidth / static_cast<double>(_capturedWidth);
		double heightRatio = maxHeight / static_cast<double>(_capturedHeight);

		if (widthRatio < heightRatio) {
			return resizeCapturedImage(widthRatio);
		}
		return resizeCapturedImage(heightRatio);
	}

	// ==============================================================
	//! @brief キャプチャした画像を任意の倍率でリサイズする
	//! @param[in] (zoomRatio) 倍率
	//! @return 成功すれば true, それ以外は false を返す
	//! @note アスペクト比は保持される
	//! @note 変換結果は scaledImage に格納される
	// =============================================================
	virtual bool resizeCapturedImage(double zoomRatio) {
		if (zoomRatio <= 0.0) {
			return false;
		}

		_scaledWidth  = static_cast<uint32_t>(_capturedWidth * zoomRatio);
		_scaledHeight = static_cast<uint32_t>(_capturedHeight * zoomRatio);

		scaledImage.resize(_scaledWidth * _scaledHeight * 2);

		uint32_t y         = 0;
		uint32_t x         = 0;
		uint32_t baseIndex = 0;
		uint32_t srcIndex  = 0;
		uint32_t index     = 0;
		double ratio       = (1.0 / zoomRatio);
		for (y = 0; y < _scaledHeight; y++) {
			baseIndex = (_capturedWidth * static_cast<uint32_t>(y * ratio)) * 2;

			for (x = 0; x < _scaledWidth; x++) {
				srcIndex = (static_cast<uint32_t>(x * ratio) * 2) + baseIndex;

				scaledImage[index]     = capturedImage[srcIndex];
				scaledImage[index + 1] = capturedImage[srcIndex + 1];
				index += 2;
			}
		}
		return true;
	}

protected:
	bool _inited             = false;
	uint32_t _capturedWidth  = 0;
	uint32_t _capturedHeight = 0;
	uint32_t _scaledWidth    = 0;
	uint32_t _scaledHeight   = 0;
};

// ==============================================================
// MARK: Metadata Control Interface Class
//! @brief デバイス情報の抽象化クラス
// =============================================================
class IMetadataControl {
public:
	String DEVICE_NAME       = "Unknown";
	String DEVICE_ID         = "- - - -";
	uint16_t V_MAIN          = 0;
	uint16_t V_MINOR         = 0;
	String V_RELEASE_TAG     = "";
	uint32_t TX_INTERVAL_SEC = 0;
	String WIFI_SSID         = "- - - -";
	String FTP_SERVER        = "- - - -";
	String HTTP_SERVER       = "- - - -";
	String MQTT_SERVER       = "- - - -";
};

#endif /* HAL_API_H */