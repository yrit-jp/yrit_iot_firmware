# ファームウェアアーキテクチャ概要

このドキュメントでは、本ファームウェアの全体的なアーキテクチャと、各コンポーネントの役割、データフローについて説明します。

## 目次

1. [概要](#概要)
2. [ディレクトリ構成](#ディレクトリ構成)
3. [主要コンポーネント](#主要コンポーネント)
4. [初期化フロー](#初期化フロー)
5. [メインループ](#メインループ)
6. [データフロー](#データフロー)
7. [タスクアーキテクチャ](#タスクアーキテクチャ)
8. [拡張ポイント](#拡張ポイント)

---

## 概要

このファームウェアは、**IoTセンサデバイス**向けのデータ収集・送信システムです。以下の特徴を持ちます:

- **マルチセンサ対応**: GPIO、I2C、Modbus、カメラなど複数の種類のセンサをサポート
- **マルチプロトコル送信**: Ambient、FTP、HTTP、MQTTなど複数のプロトコルで送信可能
- **マルチデバイス対応**: HAL（Hardware Abstraction Layer）により、12種類以上のハードウェアデバイスに対応
- **FreeRTOSベース**: タイマー割り込みとタスク機能を活用した効率的な処理
- **ユーザカスタマイズ**: センサ追加や表示カスタマイズが容易

---

## ディレクトリ構成

```
src/
├── config/           # 設定パラメータ管理
│   ├── param_manager.h
│   └── setting_parameters.h/cpp
├── display/          # 表示関連
│   ├── display.h/cpp
│   └── fonts/
├── hal/              # ハードウェア抽象化層
│   └── hardware/     # デバイス固有の実装
│       ├── esp32_devkit/
│       ├── m5stack_basic/
│       ├── m5stack_core2/
│       ├── m5stick_c_plus/
│       ├── lilygo_t_display_s3_pro/
│       ├── wio_terminal/
│       └── ...
├── i2c/              # I2Cセンサ管理
│   └── i2c_manager.h/cpp
├── modbus/           # Modbusセンサ管理
│   └── modbus_manager.h/cpp
├── transmit/         # データ送信管理
│   ├── iot_tx.h/cpp              # 共通送信基底クラス
│   ├── iot_tx_ambient.h/cpp      # Ambient送信
│   ├── iot_tx_ftp.h/cpp          # FTP送信
│   ├── iot_tx_http.h/cpp         # HTTP送信
│   ├── iot_tx_mqtt.h/cpp         # MQTT送信
│   ├── http_helper.h/cpp         # HTTP補助関数
│   └── tx_timing_helper.h/cpp    # 送信タイミング管理
└── user/             # ユーザカスタマイズ領域
    ├── user_display_default/     # デフォルト表示実装
    ├── user_display_design.h/cpp # 表示カスタマイズ
    ├── user_i2c_sensor.h/cpp     # I2Cセンサ追加
    ├── user_modbus_sensor.h/cpp  # Modbusセンサ追加
    └── user_sensors/             # センサ実装例
        ├── i2c_sensors/
        └── modbus_sensors/
```

### ディレクトリの役割

| ディレクトリ | 役割 | カスタマイズ可否 |
|------------|------|----------------|
| `config/` | 設定パラメータの読み込み・管理 | △ |
| `display/` | 画面表示処理の中心ロジック | × |
| `hal/` | ハードウェア固有の処理を抽象化 | ○ (新デバイス追加時) |
| `i2c/` | I2Cセンサの管理と通信 | △ |
| `modbus/` | Modbusセンサの管理と通信 | △ |
| `transmit/` | データの送信処理 | △ |
| `user/` | **ユーザカスタマイズ推奨領域** | ○ |

---

## 主要コンポーネント

### 1. HAL (Hardware Abstraction Layer)

**役割**: ハードウェア固有の処理を抽象化し、デバイス依存のコードを隠蔽

**主要インターフェース**:
- `HAL_Display_API`: 画面表示
- `HAL_Timer_API`: タイマー管理
- `HAL_WLAN_API`: Wi-Fi通信
- `HAL_WiredLan_API`: 有線LAN通信
- `HAL_Gpio_API`: GPIO制御
- `HAL_NtpRtc_API`: NTPと時刻管理
- `HAL_Led_API`: LED制御
- `HAL_Log_API`: ログ記録
- `HAL_Camera_API`: カメラ制御
- 他多数...

**対応デバイス**:
- M5Stack シリーズ (Basic, Core2, Core S3, Tough, Atom S3など)
- M5Stick シリーズ (C Plus, C Plus2)
- LilyGO T-Display S3 Pro
- Wio Terminal
- ESP32 DevKit

詳細は [`HOW_TO_ADD_NEW_DEVICE.md`](HOW_TO_ADD_NEW_DEVICE.md) を参照してください。

### 2. Sensor Manager

**役割**: センサデータの取得と管理

**センサタイプ**:
```cpp
enum SensorType {
    TYPE_GPIO        = 0b00000001,  // デジタル/アナログGPIO
    TYPE_I2C         = 0b00000010,  // I2Cセンサ
    TYPE_CAMERA      = 0b00000100,  // カメラ
    TYPE_MODBUS      = 0b00001000,  // Modbusセンサ
    TYPE_PULSE_TOTAL = 0b00010000,  // パルスカウンタ（累積）
    TYPE_PULSE_DELTA = 0b00100000,  // パルスカウンタ（差分）
    TYPE_NONE        = 0b11111111,
};
```

**Sensor構造体**:
```cpp
struct Sensor {
    SensorType type;
    String name;              // センサ名
    String unit;              // 単位
    bool enable;              // 有効/無効
    bool display;             // 表示対象フラグ
    bool updated;             // 更新フラグ
    bool tx_buffered;         // 送信バッファ格納フラグ
    ValueHistory<int32_t> i_value;    // 整数値履歴
    ValueHistory<double> d_value;     // 浮動小数点値履歴
    ValueHistory<String> s_value;     // 文字列値履歴
    // センサタイプ別の設定
    GPIO, I2C, MODBUS, CAMERA;
};
```

全てのセンサは `std::vector<Sensor> sensors` に格納されます。

#### I2C Manager

- **4次元マップ構造**: `[port][address][bus_channel][sub_address]`
- マルチプレクサ対応 (TCA9548Aなど)
- ユーザセンサと組み込みセンサの両方をサポート

詳細は以下を参照:
- [`HOW_TO_ADD_I2C_SENSOR.md`](HOW_TO_ADD_I2C_SENSOR.md)
- [`HOW_TO_ADD_I2C_SENSOR_TO_MANAGER.md`](HOW_TO_ADD_I2C_SENSOR_TO_MANAGER.md)

#### Modbus Manager

- RS485/RS232通信に対応
- 複数のModbusデバイスを同一バスで管理
- ユーザセンサと組み込みセンサの両方をサポート

### 3. Transmit System

**役割**: センサデータをクラウドサービスやサーバに送信

**対応プロトコル**:
- **Ambient**: IoTデータ可視化サービス
- **FTP**: ファイル転送プロトコル
- **HTTP**: HTTPポスト送信
- **MQTT**: 軽量なメッセージングプロトコル

**主要機能**:
- FIFOリングバッファによるデータ蓄積
- 送信失敗時のリトライ機能
- 送信タイミング管理（定期送信、変化時送信）
- Exceptional Buffer（カメラ画像、バッチ送信）

**クラス階層**:
```
IoTTx (基底クラス)
├── IoTTxAmbient
├── IoTTxFTP
├── IoTTxHTTP
└── IoTTxMQTT
```

各クラスは `update()` メソッドでデータを送信し、`addValue()` でデータをバッファに追加します。

### 4. Display System

**役割**: センサデータや状態を画面に表示

**主要機能**:
- OpenFontRenderライブラリを使用した日本語フォント表示
- 複数の画面サイズに対応（128x128、240x135、320x240、222x480など）
- コールバック関数による表示カスタマイズ

**表示タイミング**:
```cpp
enum DisplayTiming {
    SYSTEM_INIT_BEGIN,                // システム初期化開始
    DISPLAY_FW_VERSION,               // ファームウェアバージョン表示
    SYSTEM_INIT_DONE,                 // システム初期化完了
    UPDATE_DISPLAY_VALUE_I2C_INT,     // I2Cセンサ値更新（整数）
    UPDATE_DISPLAY_VALUE_I2C_DOUBLE,  // I2Cセンサ値更新（浮動小数点）
    UPDATE_DISPLAY_VALUE_MODBUS_INT,  // Modbusセンサ値更新（整数）
    // 他多数...
};
```

詳細は [`HOW_TO_CUSTOMIZE_DISPLAY.md`](HOW_TO_CUSTOMIZE_DISPLAY.md) を参照してください。

### 5. Config System

**役割**: 設定パラメータの管理

**主要機能**:
- JSON形式の設定ファイル読み込み
- デバイスID、Wi-Fi設定、送信間隔などの設定管理
- 機能フラグ（オフラインモード、各種送信プロトコルの有効/無効など）

```cpp
struct Features {
    bool wake_bat_enable;
    bool wait_enable;
    bool tx_random_wait_enable;
    bool camera_enable;
    bool ambient_enable;
    bool ftp_enable;
    bool http_enable;
    bool mqtt_enable;
    bool batch_send_enable;
    bool offline_mode;
};
```

---

## 初期化フロー

`setup()` 関数での初期化は以下の順序で実行されます:

```
1. HAL初期化
   ├─ HAL.begin()
   ├─ セマフォ/ミューテックス初期化
   └─ ディスプレイ初期化

2. ログシステム初期化
   └─ InitializeLog()

3. 設定パラメータ読み込み
   └─ InitializeSettingParameter()

4. ファームウェアバージョン表示
   └─ updateDisplay(DISPLAY_FW_VERSION)

5. 周辺機能初期化
   ├─ InitializeLed()
   ├─ InitializeGPIO()
   └─ InitializePulseCounter()

6. センサ初期化
   ├─ InitializeI2c()           // I2Cバス初期化
   ├─ InitializeI2cDevice()     // I2Cセンサ検出
   ├─ InitializeModbus()        // Modbusバス初期化
   ├─ InitializeModbusDevice()  // Modbusセンサ検出
   └─ InitializeCameraDevice()  // カメラ初期化

7. ネットワーク初期化（オフラインモードでない場合）
   ├─ InitializeWiredLan()      // 有線LAN試行
   ├─ InitializeWirelessLan()   // 無線LAN試行（有線失敗時）
   └─ InitializeNTP()           // 時刻同期

8. 送信システム初期化
   ├─ InitializeAmbient()       // 有効時のみ
   ├─ InitializeFTP()           // 有効時のみ
   ├─ InitializeHTTP()          // 有効時のみ
   └─ InitializeMQTT()          // 有効時のみ

9. タイマー割り込み開始
   ├─ InitializeTimerInterrupt()
   ├─ HAL.Timer1.start()        // 1秒タイマー
   └─ HAL.Timer2.start()        // 100msタイマー

10. FreeRTOSタスク作成
    ├─ HAL_REGISTER_TRANSMIT_TASK()        // 送信タスク
    └─ HAL_REGISTER_USER_INTERACTIVE_TASK() // ユーザ入力応答タスク
```

---

## メインループ

`loop()` 関数は、センサデータの取得と送信のトリガーを管理します。

### ループ処理フロー

```
┌─────────────────────────────────────────────────────┐
│                   loop() 関数                        │
└─────────────────────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ Timer2 割り込み (100ms毎)             │
    │  └─ 送信タイミングチェック            │
    │     └─ txEventFlag.transmit = true    │
    └───────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ Timer1 割り込み (1秒毎)               │
    │  ├─ 1分毎: ハートビート更新           │
    │  ├─ 30秒毎: 表示更新                  │
    │  └─ 毎秒: ログフラッシュ、入力処理    │
    └───────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ GPIOセンササンプリング                │
    │  └─ updateValue(TYPE_GPIO)            │
    └───────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ I2Cセンササンプリング                 │
    │  └─ updateValue(TYPE_I2C)             │
    └───────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ Modbusセンササンプリング              │
    │  └─ updateValue(TYPE_MODBUS)          │
    └───────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ カメラサンプリング                    │
    │  └─ updateValue(TYPE_CAMERA)          │
    └───────────────────────────────────────┘
                        │
                        ▼
    ┌───────────────────────────────────────┐
    │ 送信トリガー検出時                    │
    │  ├─ storeExceptionalValue()           │
    │  └─ storeValue()                      │
    └───────────────────────────────────────┘
```

### サンプリング間隔の制御

各センサタイプは、独立したサブタイマーで制御されます:

```cpp
if (HAL.Timer2.checkSubTimer("gpio") == true ||
    txEventFlag.transmit == true) {
    updateValue(TYPE_GPIO | TYPE_PULSE_TOTAL | TYPE_PULSE_DELTA);
}

if (HAL.Timer2.checkSubTimer("i2c") == true ||
    txEventFlag.link_i2c == true ||
    txEventFlag.transmit == true) {
    updateValue(TYPE_I2C);
}

if (HAL.Timer2.checkSubTimer("modbus") == true ||
    txEventFlag.link_modbus == true ||
    txEventFlag.transmit == true) {
    updateValue(TYPE_MODBUS);
}
```

これにより、センサタイプごとに異なるサンプリング間隔を設定できます。

---

## データフロー

### センサデータの流れ

```
センサハードウェア
    │
    ▼
┌─────────────────────────────┐
│  updateValue()              │  センサ値取得
│  ├─ GPIO → analogRead()     │
│  ├─ I2C → fetchData()       │
│  ├─ Modbus → fetchData()    │
│  └─ Camera → updateCapture()│
└─────────────────────────────┘
    │
    │ sensors[i].i_value.latest = value
    │ sensors[i].updated = true
    │
    ▼
┌─────────────────────────────┐
│  printSensorData()          │  画面表示
│  └─ updateDisplay()         │
└─────────────────────────────┘
    │
    ▼
┌─────────────────────────────┐
│  storeValue()               │  送信バッファに格納
│  ├─ tx_ambient->addValue()  │
│  ├─ tx_ftp->addValue()      │
│  ├─ tx_http->addValue()     │
│  └─ tx_mqtt->addValue()     │
└─────────────────────────────┘
    │
    │ sensors[i].tx_buffered = true
    │
    ▼
┌─────────────────────────────┐
│  transmitLoop() タスク      │  別タスクで送信処理
│  ├─ tx_ambient->update()    │
│  ├─ tx_ftp->update()        │
│  ├─ tx_http->update()       │
│  └─ tx_mqtt->update()       │
└─────────────────────────────┘
    │
    ▼
クラウドサービス/サーバ
```

### データ送信のトリガー

データ送信には2つのモードがあります:

1. **定期送信** (`txEventFlag.transmit`)
   - `TxTimingHelper::shouldTransmit()` が `true` を返した時
   - 設定された送信間隔（例: 1分、5分、1時間）で送信

2. **変化時送信** (`txEventFlag.on_change_transmit`)
   - GPIOセンサの値が変化した時
   - `sensors[i].tx_on_change = true` のセンサのみ対象
   - リンク設定により、I2CやModbusセンサも連動送信可能

---

## タスクアーキテクチャ

ファームウェアは **FreeRTOS** をベースとしており、以下の3つの主要なタスクで動作します:

### 1. メインタスク (loop関数)

- **優先度**: デフォルト（デバイス依存）
- **役割**: センサデータの取得、表示更新、送信トリガーの管理
- **実行間隔**: 約10ms (`HAL.delay(10)`)

### 2. 送信タスク (transmitLoop)

- **優先度**: 設定可能（通常は低優先度）
- **役割**: データの送信処理
- **実行間隔**: 約10ms (`HAL.delay(10)`)
- **特徴**:
  - メインタスクと独立して動作
  - 送信処理中もセンサデータ取得が継続可能

### 3. ユーザ入力応答タスク (userInteractiveLoop)

- **優先度**: 設定可能
- **役割**: ボタン入力の検出と処理
- **実行間隔**: 約10ms (`HAL.delay(10)`)
- **主な処理**:
  - BtnA: センサ表示切り替え
  - BtnB: ログ記録開始/停止
  - BtnC: カスタム処理

### タスク間の同期

セマフォ/ミューテックスを使用して、リソースの競合を防ぎます:

```cpp
// I2Cバスの排他制御
HAL.takeSemaphoreMutex("i2c", portMAX_DELAY);
result = i2c_device->fetchData(...);
HAL.giveSemaphoreMutex("i2c");

// 表示の排他制御
HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
// 表示処理
HAL.Display.returnSemaphoreMutex();
```

---

## 拡張ポイント

ファームウェアは以下の方法でカスタマイズ可能です:

### 1. I2Cセンサの追加

**簡易方法**: `user_i2c_sensor.cpp` に追加

詳細: [`HOW_TO_ADD_I2C_SENSOR.md`](HOW_TO_ADD_I2C_SENSOR.md)

**高度な方法**: `i2c_manager.cpp` に組み込み

詳細: [`HOW_TO_ADD_I2C_SENSOR_TO_MANAGER.md`](HOW_TO_ADD_I2C_SENSOR_TO_MANAGER.md)

### 2. Modbusセンサの追加

**方法**: `user_modbus_sensor.cpp` に追加

- 4つの関数を実装（init, getName, getUnit, fetchData）
- サブIDを使用して複数の値を取得可能

### 3. 表示のカスタマイズ

**方法**: `user_display_design.cpp` のコールバック関数をオーバーライド

詳細: [`HOW_TO_CUSTOMIZE_DISPLAY.md`](HOW_TO_CUSTOMIZE_DISPLAY.md)

### 4. 新しいデバイスの追加

**方法**: `src/hal/hardware/` に新しいデバイスディレクトリを作成

詳細: [`HOW_TO_ADD_NEW_DEVICE.md`](HOW_TO_ADD_NEW_DEVICE.md)

### 5. 新しい送信プロトコルの追加

**方法**: `IoTTx` クラスを継承して新しいクラスを作成

手順:
1. `src/transmit/` に `iot_tx_yourprotocol.h/cpp` を作成
2. `IoTTx` を継承
3. 仮想関数をオーバーライド:
   - `transmit()`: データ送信処理
   - `transmitEx()`: 例外データ送信処理
4. `initialize.h/cpp` に初期化関数を追加
5. `main.ino` で初期化と更新を呼び出し

---

## まとめ

このファームウェアのアーキテクチャは、以下の設計原則に基づいています:

1. **レイヤー化**: HALによるハードウェア抽象化、明確な責任分離
2. **拡張性**: ユーザカスタマイズ領域の提供、プラグイン的なセンサ追加
3. **マルチタスク**: FreeRTOSによる並行処理
4. **柔軟性**: 複数のセンサタイプ、複数の送信プロトコルに対応
5. **保守性**: コンポーネント間の疎結合、明確なデータフロー

これらの設計により、多様なハードウェアとユースケースに対応可能な、堅牢で拡張性の高いIoTファームウェアとなっています。
