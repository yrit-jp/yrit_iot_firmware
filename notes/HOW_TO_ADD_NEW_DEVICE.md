# 新しいデバイスの追加方法

本ドキュメントでは、YRIT IoT Firmwareに新しいハードウェアデバイス（ボード）を追加する方法を説明します。

## 目次

1. [はじめに](#-はじめに)
2. [デバイス追加の全体像](#-デバイス追加の全体像)
3. [必要なファイルとディレクトリ構成](#-必要なファイルとディレクトリ構成)
4. [実装手順](#-実装手順)
5. [HALインターフェース一覧](#-halインターフェース一覧)
6. [hal.h への登録](#-halh-への登録)
7. [ディスプレイオーバーライド](#-ディスプレイオーバーライド)
8. [参考実装](#-参考実装)

---

## 📋 はじめに

YRIT IoT Firmwareは、HAL（Hardware Abstraction Layer：ハードウェア抽象化層）を使用して、様々なハードウェアプラットフォームをサポートしています。
新規ハードウェアやカスタムハードウェアを使用する場合、いくつかのクラスや関数を実装することで対応させることが可能です。

---

## 🔍 デバイス追加の全体像

### 実装する内容

```
1. HALクラスの実装
   ├── ディスプレイ制御
   ├── タイマー制御
   ├── ネットワーク制御（WiFi/LAN）
   ├── NTP/RTC制御
   ├── LED制御
   ├── 設定ファイルI/O
   ├── BLE制御
   ├── SDカード制御
   ├── デバッグログ
   ├── GPIO制御
   ├── I2C制御
   ├── Modbus制御
   ├── カメラ制御
   └── メタデータ制御

2. hal.h への登録
   └── デバイス識別とコンパイル設定

3. ディスプレイオーバーライド（オプション）
   └── カスタム画面表示の実装
```

---

## 📁 必要なファイルとディレクトリ構成

新しいデバイスを追加する際の推奨ディレクトリ構成：

```
src/hal/hardware/
└── your_device/                          ← 新しいデバイス用ディレクトリ
    ├── hal_your_device.h                  ← HALクラスのヘッダー
    ├── hal_your_device.cpp                ← HALクラスの実装
    ├── your_device_display_override.cpp   ← ディスプレイオーバーライド（オプション）
    └── utility/                           ← ユーティリティ（必要に応じて）
        ├── pin_define.h                   ← ピン定義
        └── tft_color_define.h             ← 色定義
```

### 最小構成

最小限必要なファイル：

- `hal_your_device.h`
- `hal_your_device.cpp`

---

## 🔧 実装手順

### 🛠️ ステップ1: ディレクトリの作成

```bash
mkdir -p src/hal/hardware/your_device
cd src/hal/hardware/your_device
```

### 🛠️ ステップ2: ヘッダーファイルの作成

`hal_your_device.h` を作成します。

```cpp
// デバイス識別用のプリプロセッサ条件
#if defined(ARDUINO_YOUR_DEVICE_BOARD_NAME)

#ifndef HAL_API_YOUR_DEVICE_H
#define HAL_API_YOUR_DEVICE_H

#include "../hal_interface.h"

// 必要なライブラリをインクルード
#include <M5Unified.h>  // M5Stack系の場合
#include <WiFi.h>
#include <Wire.h>
// ... その他必要なライブラリ

// =============================================================
// ディスプレイ制御クラス
// =============================================================
class HAL_Display_API : public IDisplayControl {
public:
    // IDisplayControlのインターフェースを実装
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillScreen(uint32_t color);

    int16_t width(void);
    int16_t height(void);

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) override;

    bool initializeSemaphoreMutex() override;
    bool takeSemaphoreMutex(uint32_t wait_tick_time) override;
    bool giveSemaphoreMutex() override;

    // その他必要なメソッド
    // ...
};

// =============================================================
// タイマー制御クラス
// =============================================================
class HAL_Timer_API : public ITimerControl {
public:
    using ITimerControl::ITimerControl;

    void begin(uint32_t interval_ms);
    void start(bool enable_first_interrupt = false);
    bool interrupted();

    // サブタイマー管理
    void addSubTimer(std::string key, uint32_t count_limit, uint32_t initial_count = 0);
    bool checkSubTimer(std::string key, bool reset = true);
    // ...
};

// =============================================================
// WiFi制御クラス
// =============================================================
class HAL_WLAN_API : public IWirelessLanControl<WiFiClient> {
public:
    int getStatus();
    int getRSSI();
    MyIPAddress localIP();
    bool connectWlan(String ap_name, String pass, unsigned long timeout_ms = 0);
    void disconnectWlan(bool exit_sta_mode = true, bool erase_ap_settings = true);
    // ...
};

// ... その他のHALクラス（必要に応じて実装）

// =============================================================
// HAL統合クラス
// =============================================================
class IHardwareAbstractionLayer_Impl : public IHardwareAbstractionLayer {
public:
    HAL_Display_API Display;
    HAL_Timer_API Timer;
    HAL_WLAN_API Wlan;
    // ... その他のHAL APIインスタンス

    // 基本メソッド
    void begin() override;
    void update() override;
    void deepSleep(uint32_t sleep_sec) override;
    void restart() override;

    void printf(const char *format, ...);
    void delay(uint32_t ms);
    uint32_t millis();

    // ... その他の必須メソッド
};

extern IHardwareAbstractionLayer_Impl HAL;

#endif /* HAL_API_YOUR_DEVICE_H */
#endif /* ARDUINO_YOUR_DEVICE_BOARD_NAME */
```

### 🛠️ ステップ3: 実装ファイルの作成

`hal_your_device.cpp` を作成します。

```cpp
#if defined(ARDUINO_YOUR_DEVICE_BOARD_NAME)

#include "hal_your_device.h"

// =============================================================
// グローバルインスタンス
// =============================================================
IHardwareAbstractionLayer_Impl HAL;

// =============================================================
// HAL統合クラスの実装
// =============================================================

void IHardwareAbstractionLayer_Impl::begin() {
    // デバイスの初期化
    // M5Stackの場合: auto cfg = M5.config(); M5.begin(cfg);

    // ディスプレイ初期化
    Display.initializeSemaphoreMutex();

    // タイマー初期化
    Timer.begin(100);  // 100ms間隔

    // その他の初期化処理
}

void IHardwareAbstractionLayer_Impl::update() {
    // デバイスの更新処理
    // M5Stackの場合: M5.update();
}

void IHardwareAbstractionLayer_Impl::deepSleep(uint32_t sleep_sec) {
    // ディープスリープの実装
    esp_sleep_enable_timer_wakeup(sleep_sec * 1000000ULL);
    esp_deep_sleep_start();
}

void IHardwareAbstractionLayer_Impl::restart() {
    // リスタートの実装
    ESP.restart();
}

void IHardwareAbstractionLayer_Impl::printf(const char *format, ...) {
    // シリアル出力の実装
    va_list args;
    va_start(args, format);
    Serial.printf(format, args);
    va_end(args);
}

void IHardwareAbstractionLayer_Impl::delay(uint32_t ms) {
    ::delay(ms);
}

uint32_t IHardwareAbstractionLayer_Impl::millis() {
    return ::millis();
}

// =============================================================
// ディスプレイ制御クラスの実装
// =============================================================

void HAL_Display_API::drawPixel(int32_t x, int32_t y, uint32_t color) {
    // ディスプレイライブラリを使用して実装
    M5.Display.drawPixel(x, y, color);
}

void HAL_Display_API::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    M5.Display.fillRect(x, y, w, h, color);
}

void HAL_Display_API::fillScreen(uint32_t color) {
    M5.Display.fillScreen(color);
}

int16_t HAL_Display_API::width(void) {
    return M5.Display.width();
}

int16_t HAL_Display_API::height(void) {
    return M5.Display.height();
}

void HAL_Display_API::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {
    M5.Display.pushImage(x, y, w, h, data);
}

// ミューテックス管理
static SemaphoreHandle_t _display_mutex = NULL;

bool HAL_Display_API::initializeSemaphoreMutex() {
    if (_display_mutex == NULL) {
        _display_mutex = xSemaphoreCreateMutex();
    }
    return (_display_mutex != NULL);
}

bool HAL_Display_API::takeSemaphoreMutex(uint32_t wait_tick_time) {
    if (_display_mutex != NULL) {
        return (xSemaphoreTake(_display_mutex, wait_tick_time) == pdTRUE);
    }
    return false;
}

bool HAL_Display_API::giveSemaphoreMutex() {
    if (_display_mutex != NULL) {
        return (xSemaphoreGive(_display_mutex) == pdTRUE);
    }
    return false;
}

// =============================================================
// タイマー制御クラスの実装
// =============================================================

static hw_timer_t *_timer = NULL;
static volatile bool _timer_interrupted = false;

void IRAM_ATTR onTimer() {
    _timer_interrupted = true;
}

void HAL_Timer_API::begin(uint32_t interval_ms) {
    _interval_ms = interval_ms;

    // ESP32のハードウェアタイマーを設定
    _timer = timerBegin(0, 80, true);  // 80MHz / 80 = 1MHz
    timerAttachInterrupt(_timer, &onTimer, true);
    timerAlarmWrite(_timer, interval_ms * 1000, true);  // マイクロ秒単位
}

void HAL_Timer_API::start(bool enable_first_interrupt) {
    if (enable_first_interrupt) {
        _timer_interrupted = true;
    }
    timerAlarmEnable(_timer);
}

bool HAL_Timer_API::interrupted() {
    if (_timer_interrupted) {
        _timer_interrupted = false;
        return true;
    }
    return false;
}

// =============================================================
// WiFi制御クラスの実装
// =============================================================

int HAL_WLAN_API::getStatus() {
    return WiFi.status();
}

int HAL_WLAN_API::getRSSI() {
    return WiFi.RSSI();
}

MyIPAddress HAL_WLAN_API::localIP() {
    IPAddress ip = WiFi.localIP();
    return MyIPAddress(ip[0], ip[1], ip[2], ip[3]);
}

bool HAL_WLAN_API::connectWlan(String ap_name, String pass, unsigned long timeout_ms) {
    WiFi.begin(ap_name.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (timeout_ms > 0 && (millis() - start) > timeout_ms) {
            return false;
        }
        delay(100);
    }
    return true;
}

void HAL_WLAN_API::disconnectWlan(bool exit_sta_mode, bool erase_ap_settings) {
    WiFi.disconnect(exit_sta_mode, erase_ap_settings);
}

// ... その他のメソッド実装

#endif /* ARDUINO_YOUR_DEVICE_BOARD_NAME */
```

---

## 🗃️ HALインターフェース一覧

実装が必要なインターフェースクラス：

### 必須インターフェース

| クラス名                    | 説明             | 主な実装内容                      |
| --------------------------- | ---------------- | --------------------------------- |
| `IHardwareAbstractionLayer` | HAL統合クラス    | begin, update, deepSleep, restart |
| `IDisplayControl`           | ディスプレイ制御 | 描画メソッド、ミューテックス管理  |
| `ITimerControl`             | タイマー制御     | タイマー開始、サブタイマー管理    |
| `IWirelessLanControl`       | WiFi制御         | 接続、切断、ステータス取得        |

### 推奨インターフェース

| クラス名                         | 説明            | 備考                    |
| -------------------------------- | --------------- | ----------------------- |
| `INtpRtcControl`                 | NTP/RTC制御     | 時刻同期が必要な場合    |
| `ILedControl`                    | LED制御         | ステータスLEDがある場合 |
| `ISettingFileInputOutputControl` | 設定ファイルI/O | SPIFFSやSDカード使用時  |
| `ITfCardControl`                 | SDカード制御    | SDカードがある場合      |
| `IWireI2cControl`                | I2C制御         | I2Cセンサを使用する場合 |
| `IGpioControl`                   | GPIO制御        | GPIOを使用する場合      |

### オプションインターフェース

| クラス名                     | 説明           | 備考                         |
| ---------------------------- | -------------- | ---------------------------- |
| `IWiredLanControl`           | 有線LAN制御    | Ethernetモジュールがある場合 |
| `IBluetoothLowEnergyControl` | BLE制御        | BLE機能を使用する場合        |
| `IModbusControl`             | Modbus制御     | Modbusセンサを使用する場合   |
| `ICameraControl`             | カメラ制御     | カメラモジュールがある場合   |
| `IDebugLogControl`           | デバッグログ   | デバッグ機能が必要な場合     |
| `IMetadataControl`           | メタデータ制御 | デバイス情報管理用           |

---

## 📝 hal.h への登録

新しいデバイスを `src/hal/hal.h` に登録します。

### 登録方法

```cpp
// src/hal/hal.h

#if defined (ESP32)
    // ... 既存のデバイス定義 ...

    #elif defined (ARDUINO_YOUR_DEVICE_BOARD_NAME)
        #include "hardware/your_device/hal_your_device.h"
        #define TARGET__YOUR_DEVICE
        #define DISPLAY_320X240  // または適切な画面サイズ
        #define HAL_MQTT_MAX_PACKET_SIZE (65534)
        #define HAL_ENABLE_EXCEPTIONAL_BUFFER

    #else
        // デフォルトデバイス
        #include "hardware/esp32_devkit/hal_esp32_devkit.h"
        // ...
    #endif

#elif defined (SAMD_SERIES)
    // SAMD系の定義
    // ...

#endif
```

### プリプロセッサ定義

| 定義                            | 説明               | 例                       |
| ------------------------------- | ------------------ | ------------------------ |
| `TARGET__YOUR_DEVICE`           | デバイス識別子     | `TARGET__M5STACK_BASIC`  |
| `DISPLAY_*`                     | ディスプレイサイズ | `DISPLAY_320X240`        |
| `HAL_MQTT_MAX_PACKET_SIZE`      | MQTTパケットサイズ | `65534`                  |
| `HAL_ENABLE_EXCEPTIONAL_BUFFER` | 例外バッファ有効化 | -                        |
| `BTN_LAYOUT_*`                  | ボタンレイアウト   | `BTN_LAYOUT_STACK_BASIC` |

### ディスプレイサイズ定義

```cpp
#define DISPLAY_320X240           // 320×240ピクセル
#define DISPLAY_240X135           // 240×135ピクセル
#define DISPLAY_128X128           // 128×128ピクセル
#define DISPLAY_222X480           // 222×480ピクセル
#define DISPLAY_CHARACTER_8X2     // キャラクタLCD 8文字×2行
```

---

## 🖼️ ディスプレイオーバーライド

デフォルトの画面表示をカスタマイズする場合、ディスプレイオーバーライドを実装します。

### ファイル作成

`your_device_display_override.cpp` を作成：

```cpp
#if defined(ARDUINO_YOUR_DEVICE_BOARD_NAME)

#include "../../../display/display.h"

void Display::drawSystemStatusBar(std::unique_ptr<OpenFontRender> &render) {
    // デバイス固有のステータスバー描画
    takeSemaphoreMutex(portMAX_DELAY);

    HAL.Display.fillRect(0, 0, 320, 20, TFT_NAVY);
    render->setFontSize(12);
    render->setCursor(5, 5);
    render->setFontColor(TFT_WHITE, TFT_NAVY);
    render->printf("Your Device");

    giveSemaphoreMutex();
}

// その他のオーバーライドメソッド
// ...

#endif
```

---

## 📂 参考実装

### シンプルなデバイス（参考）

ESP32 DevKit のような最小構成：

```
src/hal/hardware/esp32_devkit/
├── hal_esp32_devkit.h
├── hal_esp32_devkit.cpp
├── esp32_devkit_display_override.cpp
└── utility/
    ├── pin_define.h
    └── button.h
```

### フル機能デバイス（参考）

M5Stack Basic のような完全な実装：

```
src/hal/hardware/m5stack_basic/
├── hal_m5stack_basic.h
├── hal_m5stack_basic.cpp
└── m5stack_basic_display_override.cpp
```

### 実装の参考にすべきファイル

**最小構成の参考:**

- `src/hal/hardware/esp32_devkit/`
- キャラクタLCD使用
- 基本機能のみ実装

**標準的な実装の参考:**

- `src/hal/hardware/m5stack_basic/`
- TFTディスプレイ使用
- ほぼ全機能実装

**最新デバイスの参考:**

- `src/hal/hardware/m5stack_core_s3/`
- ESP32-S3使用
- 最新の機能に対応

---
