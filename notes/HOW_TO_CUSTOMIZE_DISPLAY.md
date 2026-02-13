# 画面表示のカスタマイズ方法

本ドキュメントでは、YRIT IoT Firmwareの画面表示をカスタマイズする方法を説明します。

## 目次

1. [はじめに](#はじめに)
2. [カスタマイズの仕組み](#カスタマイズの仕組み)
3. [コールバック関数一覧](#コールバック関数一覧)
4. [基本的なカスタマイズ手順](#基本的なカスタマイズ手順)
5. [実装例](#実装例)
6. [OpenFontRenderの使い方](#openfontrender の使い方)
7. [画面サイズ別の実装](#画面サイズ別の実装)
8. [よくあるカスタマイズ](#よくあるカスタマイズ)

---

## はじめに

YRIT IoT Firmwareは、コールバック関数を使用した柔軟な画面表示カスタマイズ機能を提供しています。

### カスタマイズ可能な要素

- センサ値の表示フォーマット
- システム起動画面
- 待機時間表示
- データ送信バッファ状態
- 画面レイアウト全体

### 必要なファイル

```
src/user/
├── user_display_design.h      ← コールバック関数の宣言（変更不要）
└── user_display_design.cpp    ← カスタマイズを記述するファイル ⭐
```

**user_display_design.cpp** にカスタマイズコードを記述します。

---

## カスタマイズの仕組み

### コールバック関数の構造

```
ファームウェア本体
    ↓
コールバック関数を呼び出し (user_display_design.cpp)
    ↓
デフォルト実装を呼び出す or カスタム実装を実行
    ↓
画面に表示
```

### 実装パターン

#### パターン1: デフォルトのまま使用

```cpp
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render,
                          const char *sensor_name,
                          int32_t value,
                          const char *unit,
                          bool selected) {
    // デフォルト実装をそのまま使用
    cb_UpdateI2cValueInt_Default(render, sensor_name, value, unit, selected);
}
```

#### パターン2: 完全にカスタマイズ

```cpp
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render,
                          const char *sensor_name,
                          int32_t value,
                          const char *unit,
                          bool selected) {
    // 完全にカスタム実装
    HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
    HAL.Display.fillRect(0, 0, 320, 240, TFT_BLACK);  // 画面クリア

    render->setFontSize(20);
    render->setCursor(10, 10);
    render->setFontColor(TFT_CYAN, TFT_BLACK);
    render->printf("%s: %d %s", sensor_name, value, unit);

    HAL.Display.returnSemaphoreMutex();
}
```

#### パターン3: デフォルト + 追加処理

```cpp
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render,
                          const char *sensor_name,
                          int32_t value,
                          const char *unit,
                          bool selected) {
    // デフォルト表示を実行
    cb_UpdateI2cValueInt_Default(render, sensor_name, value, unit, selected);

    // 追加のカスタム処理
    if (value > 1000) {
        HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
        render->setFontSize(12);
        render->setCursor(10, 200);
        render->setFontColor(TFT_RED, TFT_BLACK);
        render->printf("WARNING: High value!");
        HAL.Display.returnSemaphoreMutex();
    }
}
```

---

## コールバック関数一覧

### システム関連

| 関数名 | 呼び出しタイミング | パラメータ |
|--------|-------------------|-----------|
| `cb_BeforeSystemDisplayUpdate` | 画面更新の直前 | `DisplayTiming` 列挙型 |
| `cb_AfterSystemDisplayUpdate` | 画面更新の直後 | `DisplayTiming` 列挙型 |
| `cb_SystemReady` | システム起動完了時 | デバイスID、センサ数 |
| `cb_WaitTiming` | 待機時間中 | 経過秒数 |

### センサ値更新関連

#### GPIO センサ

| 関数名 | データ型 | パラメータ |
|--------|---------|-----------|
| `cb_UpdateGpioValue` | int32_t | センサ名、値、単位、選択状態 |

#### I2C センサ

| 関数名 | データ型 | パラメータ |
|--------|---------|-----------|
| `cb_UpdateI2cValueInt` | int32_t | センサ名、値、単位、選択状態 |
| `cb_UpdateI2cValueDouble` | double | センサ名、値、単位、選択状態 |
| `cb_UpdateI2cValueString` | String | センサ名、値、単位、選択状態 |
| `cb_UpdateI2cValueError` | エラー | センサ名、選択状態 |

#### Modbus センサ

| 関数名 | データ型 | パラメータ |
|--------|---------|-----------|
| `cb_UpdateModbusValueInt` | int32_t | センサ名、値、単位、選択状態 |
| `cb_UpdateModbusValueDouble` | double | センサ名、値、単位、選択状態 |
| `cb_UpdateModbusValueString` | String | センサ名、値、単位、選択状態 |
| `cb_UpdateModbusValueError` | エラー | センサ名、選択状態 |

#### カメラ

| 関数名 | データ型 | パラメータ |
|--------|---------|-----------|
| `cb_UpdateCameraValue` | - | センサ名、選択状態 |

### サンプリング完了関連

| 関数名 | 呼び出しタイミング | パラメータ |
|--------|-------------------|-----------|
| `cb_SamplingGpioSensors` | GPIOセンサ値取得完了時 | 時刻情報 |
| `cb_SamplingI2cSensors` | I2Cセンサ値取得完了時 | 時刻情報 |
| `cb_SamplingModbusSensors` | Modbusセンサ値取得完了時 | 時刻情報 |
| `cb_SamplingCameraSensor` | カメラ撮影完了時 | 時刻情報 |

### データ送信バッファ関連

| 関数名 | 対象サービス | パラメータ |
|--------|-------------|-----------|
| `cb_AmbientBufferUpdate` | Ambient | 残データ数 |
| `cb_FtpBufferUpdate` | FTP | 残データ数 |
| `cb_HttpBufferUpdate` | HTTP | 残データ数 |
| `cb_MqttBufferUpdate` | MQTT | 残データ数 |

### その他

| 関数名 | 呼び出しタイミング | パラメータ |
|--------|-------------------|-----------|
| `cb_ReadMqttStatus` | MQTT状態変化時 | ステータス値 |

---

## 基本的なカスタマイズ手順

### ステップ1: カスタマイズする関数を選択

例: I2Cセンサの整数値表示をカスタマイズしたい場合

```cpp
void cb_UpdateI2cValueInt(...)
```

### ステップ2: user_display_design.cpp を編集

`src/user/user_display_design.cpp` を開き、該当する関数を編集します。

```cpp
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render,
                          const char *sensor_name,
                          int32_t value,
                          const char *unit,
                          bool selected) {
    // ここにカスタマイズコードを記述
}
```

### ステップ3: 画面描画コードを実装

OpenFontRender を使用して画面に描画します。

---

## 実装例

### 例1: 温度センサの値を大きく表示

```cpp
void cb_UpdateI2cValueDouble(std::unique_ptr<OpenFontRender> &render,
                             const char *sensor_name,
                             double value,
                             const char *unit,
                             bool selected) {
    // 温度センサのみ大きく表示
    if (String(sensor_name).indexOf("Temperature") >= 0) {
        HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

        // 画面をクリア
        HAL.Display.fillRect(0, 60, 320, 100, TFT_BLACK);

        // 大きなフォントサイズで表示
        render->setFontSize(48);
        render->setCursor(60, 80);
        render->setFontColor(TFT_YELLOW, TFT_BLACK);
        render->printf("%.1f%s", value, unit);

        HAL.Display.returnSemaphoreMutex();
    } else {
        // その他のセンサはデフォルト表示
        cb_UpdateI2cValueDouble_Default(render, sensor_name, value, unit, selected);
    }
}
```

### 例2: 警告メッセージの表示

```cpp
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render,
                          const char *sensor_name,
                          int32_t value,
                          const char *unit,
                          bool selected) {
    // デフォルト表示
    cb_UpdateI2cValueInt_Default(render, sensor_name, value, unit, selected);

    // しきい値チェック
    if (value > 1000) {
        HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

        // 警告メッセージを赤色で表示
        render->setFontSize(14);
        render->setCursor(10, 210);
        render->setFontColor(TFT_RED, TFT_BLACK);
        render->printf("⚠ WARNING: Value exceeds limit!");

        HAL.Display.returnSemaphoreMutex();
    }
}
```

### 例3: カスタム起動画面

```cpp
void cb_SystemReady(std::unique_ptr<OpenFontRender> &render,
                    const char *dev_id,
                    int sensor_num) {
    HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

    // 画面全体をクリア
    HAL.Display.fillRect(0, 0, 320, 240, TFT_NAVY);

    // タイトル表示
    render->setFontSize(24);
    render->setCursor(60, 60);
    render->setFontColor(TFT_WHITE, TFT_NAVY);
    render->printf("My IoT Device");

    // デバイスID表示
    render->setFontSize(14);
    render->setCursor(80, 100);
    render->setFontColor(TFT_CYAN, TFT_NAVY);
    render->printf("ID: %s", dev_id);

    // センサ数表示
    render->setCursor(60, 130);
    render->setFontColor(TFT_GREENYELLOW, TFT_NAVY);
    render->printf("Sensors: %d", sensor_num);

    HAL.Display.returnSemaphoreMutex();

    // 3秒間表示
    HAL.delay(3000);
}
```

### 例4: データ送信状態の可視化

```cpp
void cb_MqttBufferUpdate(std::unique_ptr<OpenFontRender> &render,
                         int remain_data_quantity) {
    HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

    // バッファ状態を色で表現
    uint16_t color;
    if (remain_data_quantity == 0) {
        color = TFT_GREEN;     // 正常
    } else if (remain_data_quantity < 10) {
        color = TFT_YELLOW;    // 注意
    } else {
        color = TFT_RED;       // 警告
    }

    // 状態表示エリア
    HAL.Display.fillRect(250, 200, 60, 30, color);

    // 残データ数を表示
    render->setFontSize(12);
    render->setCursor(255, 210);
    render->setFontColor(TFT_BLACK, color);
    render->printf("MQTT:%d", remain_data_quantity);

    HAL.Display.returnSemaphoreMutex();
}
```

---

## OpenFontRender の使い方

### 基本的なメソッド

#### フォント設定

```cpp
// フォントサイズを設定
render->setFontSize(20);

// フォント色を設定（文字色、背景色）
render->setFontColor(TFT_WHITE, TFT_BLACK);
```

#### カーソル位置

```cpp
// カーソル位置を設定
render->setCursor(x, y);

// 現在のカーソル位置を取得
int x = render->getCursorX();
int y = render->getCursorY();
```

#### テキスト描画

```cpp
// printf形式で描画
render->printf("Temperature: %.1f℃", temp);

// 文字列描画
render->print("Hello World");
render->println("With newline");
```

### HAL.Display のメソッド

#### ミューテックス管理

```cpp
// 画面操作開始（必須）
HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

// ... 描画処理 ...

// 画面操作終了（必須）
HAL.Display.returnSemaphoreMutex();
```

**⚠️ 重要:** 画面描画時は必ずミューテックスを取得・解放してください。

#### 図形描画

```cpp
// 矩形塗りつぶし
HAL.Display.fillRect(x, y, width, height, color);

// 矩形枠線
HAL.Display.drawRect(x, y, width, height, color);

// 円塗りつぶし
HAL.Display.fillCircle(x, y, radius, color);

// 線
HAL.Display.drawLine(x1, y1, x2, y2, color);
```

#### 色定義

```cpp
// 基本色
TFT_BLACK       // 黒
TFT_WHITE       // 白
TFT_RED         // 赤
TFT_GREEN       // 緑
TFT_BLUE        // 青
TFT_CYAN        // シアン
TFT_MAGENTA     // マゼンタ
TFT_YELLOW      // 黄色
TFT_ORANGE      // オレンジ
TFT_GREENYELLOW // 黄緑
TFT_PINK        // ピンク
TFT_NAVY        // ネイビー
TFT_DARKGREEN   // ダークグリーン
TFT_MAROON      // マルーン

// カスタム色（RGB565形式）
uint16_t custom_color = 0xF800;  // 16ビットRGB
```

---

## 画面サイズ別の実装

### 対応画面サイズ

YRIT IoT Firmwareは、以下の画面サイズに対応しています：

| サイズ | 解像度 | プリプロセッサ定義 | デフォルト実装ファイル |
|-------|--------|------------------|---------------------|
| 小 | 128×128 | `DISPLAY_128X128` | `user_display_default_128x128.cpp` |
| 中1 | 240×135 | `DISPLAY_240X135` | `user_display_default_240x135.cpp` |
| 中2 | 222×480 | `DISPLAY_222X480` | `user_display_default_222x480.cpp` |
| 大 | 320×240 | `DISPLAY_320X240` | `user_display_default_320x240.cpp` |

### 画面サイズに応じた実装

#### パターン1: すべてのサイズで共通の処理

```cpp
void cb_SystemReady(std::unique_ptr<OpenFontRender> &render,
                    const char *dev_id,
                    int sensor_num) {
    // 画面サイズに関わらず同じ処理
    cb_SystemReady_Default(render, dev_id, sensor_num);
}
```

#### パターン2: 画面サイズごとに異なる処理

```cpp
void cb_UpdateI2cValueDouble(std::unique_ptr<OpenFontRender> &render,
                             const char *sensor_name,
                             double value,
                             const char *unit,
                             bool selected) {
#ifdef DISPLAY_320X240
    // 320×240 用の実装
    render->setFontSize(24);
    render->setCursor(50, 100);

#elif defined(DISPLAY_240X135)
    // 240×135 用の実装
    render->setFontSize(18);
    render->setCursor(30, 60);

#elif defined(DISPLAY_128X128)
    // 128×128 用の実装
    render->setFontSize(12);
    render->setCursor(10, 50);

#else
    // その他のサイズ
    cb_UpdateI2cValueDouble_Default(render, sensor_name, value, unit, selected);
    return;
#endif

    HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
    render->setFontColor(TFT_GREEN, TFT_BLACK);
    render->printf("%.1f%s", value, unit);
    HAL.Display.returnSemaphoreMutex();
}
```

---

## よくあるカスタマイズ

### 1. 特定のセンサだけ表示をカスタマイズ

```cpp
void cb_UpdateI2cValueDouble(std::unique_ptr<OpenFontRender> &render,
                             const char *sensor_name,
                             double value,
                             const char *unit,
                             bool selected) {
    // センサ名で判定
    String name = String(sensor_name);

    if (name.indexOf("BME280_Temperature") >= 0) {
        // BME280の温度だけカスタム表示
        HAL.Display.takeSemaphoreMutex(portMAX_DELAY);
        render->setFontSize(30);
        render->setCursor(100, 100);
        render->setFontColor(TFT_RED, TFT_BLACK);
        render->printf("%.1f℃", value);
        HAL.Display.returnSemaphoreMutex();
    } else {
        // その他はデフォルト
        cb_UpdateI2cValueDouble_Default(render, sensor_name, value, unit, selected);
    }
}
```

### 2. 値の範囲に応じて色を変える

```cpp
void cb_UpdateI2cValueInt(std::unique_ptr<OpenFontRender> &render,
                          const char *sensor_name,
                          int32_t value,
                          const char *unit,
                          bool selected) {
    HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

    // 値によって色を変更
    uint16_t color;
    if (value < 500) {
        color = TFT_BLUE;      // 低い
    } else if (value < 1000) {
        color = TFT_GREEN;     // 正常
    } else if (value < 1500) {
        color = TFT_YELLOW;    // 注意
    } else {
        color = TFT_RED;       // 警告
    }

    render->setFontSize(20);
    render->setCursor(50, 100);
    render->setFontColor(color, TFT_BLACK);
    render->printf("%s: %d %s", sensor_name, value, unit);

    HAL.Display.returnSemaphoreMutex();
}
```

### 3. グラフ表示（簡易版）

```cpp
// グローバル変数で履歴を保持
static int temperature_history[50] = {0};
static int history_index = 0;

void cb_UpdateI2cValueDouble(std::unique_ptr<OpenFontRender> &render,
                             const char *sensor_name,
                             double value,
                             const char *unit,
                             bool selected) {
    if (String(sensor_name).indexOf("Temperature") >= 0) {
        // 履歴に追加
        temperature_history[history_index] = (int)value;
        history_index = (history_index + 1) % 50;

        HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

        // グラフエリアをクリア
        HAL.Display.fillRect(10, 150, 300, 80, TFT_BLACK);

        // 簡易グラフ描画
        for (int i = 0; i < 49; i++) {
            int h1 = temperature_history[i] * 2;  // スケール調整
            int h2 = temperature_history[i + 1] * 2;
            HAL.Display.drawLine(10 + i * 6, 230 - h1,
                                10 + (i + 1) * 6, 230 - h2,
                                TFT_CYAN);
        }

        HAL.Display.returnSemaphoreMutex();
    }

    // 通常の値表示
    cb_UpdateI2cValueDouble_Default(render, sensor_name, value, unit, selected);
}
```

### 4. アイコン表示

```cpp
void cb_SystemReady(std::unique_ptr<OpenFontRender> &render,
                    const char *dev_id,
                    int sensor_num) {
    HAL.Display.takeSemaphoreMutex(portMAX_DELAY);

    // アイコン画像を表示（icon.h で定義されている場合）
    // HAL.Display.pushImage(x, y, width, height, icon_data);

    // テキスト情報
    render->setFontSize(16);
    render->setCursor(10, 10);
    render->setFontColor(TFT_WHITE, TFT_BLACK);
    render->printf("Device: %s", dev_id);
    render->printf("\nSensors: %d", sensor_num);

    HAL.Display.returnSemaphoreMutex();
}
```

---

## まとめ

### 画面表示カスタマイズの手順

1. **カスタマイズする関数を選択**
   - センサ値更新、起動画面、バッファ状態など

2. **user_display_design.cpp を編集**
   - デフォルト実装を呼び出す
   - 完全にカスタマイズする
   - デフォルト + 追加処理

3. **OpenFontRender で描画**
   - フォント設定
   - カーソル位置設定
   - テキスト描画

4. **HAL.Display で図形描画**
   - ミューテックス管理（重要）
   - 矩形、円、線など

### 重要なポイント

- ✅ 画面操作時は必ずミューテックスを取得・解放
- ✅ 画面サイズに応じた実装を検討
- ✅ デフォルト実装を活用して効率的に開発
- ✅ センサ名で条件分岐して個別対応可能

### 参考ファイル

```
src/user/
├── user_display_design.cpp              ← カスタマイズを記述 ⭐
└── user_display_default/
    ├── user_display_default.h
    ├── user_display_default_128x128.cpp  ← デフォルト実装（参考用）
    ├── user_display_default_240x135.cpp
    ├── user_display_default_320x240.cpp
    └── user_display_default_222x480.cpp
```

この手順に従うことで、デバイスの画面表示を柔軟にカスタマイズできます。
