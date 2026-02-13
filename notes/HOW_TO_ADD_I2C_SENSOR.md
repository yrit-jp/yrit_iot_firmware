# I2C センサの追加方法（ユーザーセンサ）

本ドキュメントでは、YRIT IoT Firmware に新しい I2C センサを追加する方法を説明します。

## 📚 前提知識

このドキュメントを読む前に、以下を理解しておくことを推奨します。

- **C++の基本文法**: クラス、ポインタ、関数の定義など
- **I2C 通信の基本**: I2C アドレス、通信フロー、レジスタの概念
- **センサの基本情報**: レジスタマップ、デバイス ID、データ変換式など

## 📁 プロジェクトの構成

YRIT IoT Firmware は、I2C センサを **標準センサ** と **ユーザーセンサ** の 2 つに分けて管理しています。

### 🗂️ ディレクトリ構成

```
src/
├── i2c/                            # ファームウェア標準センサ
│   ├── i2c_manager.h               # 標準I2Cセンサの関数宣言
│   ├── i2c_manager.cpp             # 標準I2Cセンサの処理
│   └── i2c_type_def.h              # 型定義
│
└── user/                           # ユーザー追加領域（ここで作業する）
    ├── user_i2c_sensor.h           # ユーザーI2Cセンサの関数宣言
    ├── user_i2c_sensor.cpp         # ユーザーI2Cセンサの統合処理
    ├── user_modbus_sensor.h/cpp    # ユーザーModbusセンサ（別ドキュメント参照）
    └── user_sensors/
        ├── i2c_sensors/            # ← センサクラスをここに作る
        │   ├── i2c_ct_unit.h/cpp   # （実装例）
        │   └── your_sensor.h/cpp   # ← 新規センサを追加
        └── modbus_sensors/         # Modbusセンサ用（別ドキュメント参照）
```

### ⚖️ 標準センサとユーザーセンサの違い

センサを追加する場合、通常はユーザーセンサ側（`user_i2c_sensor.cpp`）に追加します。
本ドキュメントではユーザーセンサ側（`user_i2c_sensor.cpp`）にセンサを追加する方法について解説します。

|                      | 標準センサ（`i2c_manager.cpp`） | ユーザーセンサ（`user_i2c_sensor.cpp`） |
| -------------------- | ------------------------------- | --------------------------------------- |
| **編集するファイル** | `src/i2c/i2c_manager.cpp`       | `src/user/user_i2c_sensor.cpp`          |
| **処理内容**         | センサ処理 + 周辺処理           | センサ処理のみ                          |
| **実装の複雑さ**     | 複雑                            | シンプル（センサに関する処理のみ）      |
| **処理の優先度**     | 後で処理される                  | 先に処理される                          |
| **使用する場面**     | 標準センサとして組み込む時      | 追加のセンサを使用する時                |

## 📋 プログラム概要

プログラムがどのようにセンサを検出しデータを取得するか、ユーザはどのような処理を書かなければならないのかを説明します。

### 🔄 センサ初期化の流れ

センサは下記フローに従って検出と初期化が行われます。

<!-- For Markdown -->
<div align="center">
    <img src="../assets/i2c_init_flow.svg" alt="" style="max-width: 50vw;">
</div>

<!-- For Doxygen -->
<div align="center">
    <img src="./assets/i2c_init_flow.svg" alt="" style="max-width: 30vw;">
</div>

### 📊 センサデータ取得の流れ

初期化完了後、センサデータは定期的に以下のフローで取得されます。

<!-- For Markdown -->
<div align="center">
    <img src="../assets/i2c_fetch_flow.svg" alt="" style="max-width: 50vw;">
</div>

<!-- For Doxygen -->
<div align="center">
    <img src="./assets/i2c_fetch_flow.svg" alt="" style="max-width: 30vw;">
</div>

### 📝 実装しなければならない処理

センサを追加する場合、ユーザは下記の処理を `user_i2c_sensor.cpp` に実装する必要があります。これらの関数はファームウェアの `i2c_manager.cpp` から呼び出されます。

| 処理             | 概要                             | 関数名                    |
| ---------------- | -------------------------------- | ------------------------- |
| センサ初期化処理 | センサの初期化のための処理       | `initializeUserI2cDevice` |
| センサ名取得処理 | センサ名とデータ種別を返す処理   | `getUserI2cDeviceName`    |
| 単位取得処理     | データの単位を返す処理           | `getUserI2cDeviceUnit`    |
| データ取得処理   | センサから実際の値を取得する処理 | `fetchUserI2cDeviceData`  |

---

### 🗃️ データ構造

ユーザーセンサは、2 次元マップとセンサリストクラスを組み合わせて管理されています。

#### 全体構造

```cpp
std::map<int, std::map<int, I2C_User_Sensor_List>> i2c_user_sensors;
          ↑             ↑             ↑
         port        address     センサリスト
```

#### アクセス方法

```cpp
i2c_user_sensors[port][address].your_sensor
       ↑          ↑       ↑            ↑
  ２次元マップ    ポート  I2Cアドレス   センサインスタンス
```

| パラメータ | 説明           |  値の範囲   | 例                                              |
| :--------: | -------------- | :---------: | ----------------------------------------------- |
|   `port`   | I2C ポート番号 |    0 ~ N    | M5Stack の Grove I2C ポート A1 (0), A2 (1) など |
| `address`  | I2C アドレス   | 0x00 ~ 0x7F | 0x48, 0x76 など                                 |

#### I2C_User_Sensor_List クラス

`i2c_user_sensors[port][address]` でアクセスすると、以下のようなセンサリストクラスのインスタンスが得られます。

```cpp
class I2C_User_Sensor_List {
public:
    std::unique_ptr<Sensor0> sensor0;
    std::unique_ptr<Sensor1> sensor1;
    std::unique_ptr<Sensor2> sensor2;
    ...
};
```

各センサは `std::unique_ptr` で管理され、有効なセンサオブジェクトのみメモリに展開されます。
この構造により、有効なセンサへのアクセスが明快に記述できる他、同じアドレスのセンサでも異なるポートなら区別可能になります。

##### 使用例

```cpp
// ポート0、アドレス0x48のセンサ
i2c_user_sensors[0][0x48].sensor0

// ポート1、アドレス0x48のセンサ（異なるポート → 別のセンサとして扱われる）
i2c_user_sensors[1][0x48].sensor0

// ポート0、アドレス0x76の別のセンサ
i2c_user_sensors[0][0x76].sensor2
```

---

## 🔧 実装の流れ

I2C センサの追加は、大きく分けて **2 つのステップ** で行います。

### 🛠️ ステップ 1: センサ用のライブラリを作る

まず、センサ専用のプログラム（クラス）を作ります。これは「センサとの通信やデータ処理」を定義するもので、ファームウェアから独立した再利用可能なライブラリであり、一般的な Arduino で利用されるセンサライブラリと同じ形式です。

**作成するファイル：**

```
src/user/user_sensors/i2c_sensors/
├── your_sensor.h      ← センサクラスのヘッダ
└── your_sensor.cpp    ← センサクラスの実装
```

ライブラリは `src/user/user_sensors/i2c_sensors/` 以下に作成する他に、Arduino のデフォルトのライブラリフォルダに作成しても構いません。

#### 実装上の注意点

ライブラリを実装する際は、以下の点に注意してください。

##### 1. クラスとして定義する

コードはクラスとして実装し、インスタンス化して利用できるようにしなければなりません。これにより、複数のセンサを同時に扱う場合や、同じセンサを複数個使用する場合にも対応できます。

##### 2. 初期化メソッドで Wire オブジェクトを受け取る

初期化メソッドの引数で Wire オブジェクトを受け取るようにしなければなりません。
受け取り方は `TwoWire` 型のポインタ渡しか参照渡しで受け取ることが望ましいです。
ファームウェアは複数の I2C バス（Wire、Wire1 など）を管理しているため、どの I2C バスを使用するかを外部から指定できるようにする必要があります。より利便性を向上するために、ポインタ渡しの場合はデフォルト引数に `Wire` のポインタを指定しておくと良いでしょう。

```cpp
// ✅実装例（ポインタ渡し）
bool init(TwoWire *wire);          // デフォルト引数なし
bool init(TwoWire *wire = &Wire);  // デフォルト引数あり

// ✅実装例（参照渡し）
bool init(TwoWire& wire);
```

##### 3. 初期化メソッドで受け取った Wire オブジェクトを使用して通信する

ライブラリ内での I2C 通信には、初期化メソッドで受け取った `Wire` オブジェクトを使用します。グローバルな `Wire` オブジェクトを直接使用しないでください。ファームウェアが指定した I2C バスで通信を行う必要があり、誤ったバスで通信するとセンサと正常に通信できなくなります。

```cpp
// ✅実装例（ポインタ渡しの場合）
bool init(TwoWire *wire) {
    _wire = wire;
    _wire->beginTransmission(slave_address);
    _wire->endTransmission();
}

// ✅実装例（参照渡しの場合）
bool init(TwoWire& wire) {
    _wire = wire;
    _wire.beginTransmission(slave_address);
    _wire.endTransmission();
}

// ❌不適切な実装例
bool init() {
    // グローバルな Wire オブジェクトを使用している
    Wire.beginTransmission(slave_address);
    Wire.endTransmission();
}
```

##### 4. 初期化メソッドでセンサを同定する

WHO_AM_I レジスタなどのセンサ固有のレジスタを読み取ってセンサの同定を行い、初期化の成否を `bool` 型の戻り値で返すことが望ましいです（`true`: 該当センサ、`false`: 別のセンサ など）。同じ I2C アドレスを使用するセンサは複数存在するため、デバイス ID などで正しく識別する必要があります。識別を行わないと、間違ったセンサとして処理されてしまう可能性があります。
ただし、一部のセンサにはそのようなレジスタが存在しないものもあるため、難しい場合は省略しても構いません。

```cpp
// ✅実装例
bool init(TwoWire *wire) {
    _wire = wire;

    // センサ固有のレジスタを読み出し
    _wire->beginTransmission(slave_address);
    _wire->write(addr_who_am_i);
    _wire->endTransmission();

    // センサの同定
    uint8_t num = _wire->requestFrom((uint8_t)slave_address, size, (uint8_t)true);
    if (_wire->read() != WHO_AM_I_VALUE) {
        return false;
    }

    // センサ初期化処理
    _wire->beginTransmission(slave_address);
    _wire->write(config);
    _wire->endTransmission();

    return true;
}

// ⚠️推奨されない実装例
bool init(TwoWire *wire) {
    _wire = wire;

    // センサ初期化処理
    _wire->beginTransmission(slave_address);
    _wire->write(config);
    _wire->endTransmission();

    return true;
}
```

その他の実装例については、`src/user/user_sensors/i2c_sensors` や同梱のライブラリを参照してください。

### 🧩 ステップ 2: ファームウェアに組み込む

次に、作ったセンサライブラリを使って、該当の I2C センサをファームウェアに組み込みます。`user_i2c_sensor.cpp` に以下の処理を追加します。

```
(1) インクルード追加     ← センサライブラリを読み込む
(2) 宣言追加           ← センサ用の変数を用意する
(3) 初期化処理         ← センサを検出して初期化と同定を行う
(4) センサ名取得       ← 識別用のセンサ名と種別名を返す
(5) 単位取得          ← センサ値の単位を返す
(6) センサ値取得       ← 実際のセンサデータを取得する
```

#### (1) インクルードファイルの追加

作ったセンサライブラリのヘッダーファイルを読み込みます。
ヘッダファイルのパスは作成場所に応じて適宜合わせてください。

```cpp
/* インクルードファイルを追加する（ここから）*/
#include "./user_sensors/i2c_sensors/i2c_ct_unit.h"
#include "./user_sensors/i2c_sensors/your_sensor.h"  // ← この行を追加
/* インクルードファイルを追加する（ここまで）*/
```

#### (2) センサリストクラスへの宣言追加

使用するセンサの宣言を追加します。
本ファームウェアではメモリ効率化のため、`std::unique_ptr` を使用した宣言となっています。

```cpp
class I2C_User_Sensor_List {
public:
    /* ここに新規センサの宣言を追加する（ここから）*/
    std::unique_ptr<I2C_CT_Unit> i2c_ct_unit;

    // std::unique_ptr<センサクラス名> インスタンス変数名;
    std::unique_ptr<YourSensor> your_sensor;  // ← この行を追加（センサ名とクラス名は適宜合わせる）
    /* ここに新規センサの宣言を追加する（ここまで）*/
};
```

> **💡`std::unique_ptr` とは？**  
> C++ でメモリを自動管理するスマートポインタの一種です。本ファームウェアでは、センサオブジェクトの確保と解放を自動で行うことでメモリリークを防ぎ、実際に接続されているセンサのみメモリを確保することで効率的なメモリ使用を実現しています。

#### (3) 初期化処理の実装

`initializeUserI2cDevice()` 関数の `switch` 文の中にセンサを検出して初期化する処理を追加します。

**例：I2C アドレス 0x48 のセンサの場合**

まずは、該当するアドレスの case 文を追加し、センサの初期化ブロックを実装します。

```cpp
/* ここに新規センサの初期化処理を追加する（ここから）*/

case 0x48:  // ← センサのI2Cアドレス（データシートで確認）
    // まだセンサが作られていない場合のみ実行
    if (!i2c_user_sensors[port][address].your_sensor) {

    }
    break;

/* ここに新規センサの初期化処理を追加する（ここまで）*/
```

次にセンサクラスのインスタンスを作成します。

```cpp
/* ここに新規センサの初期化処理を追加する（ここから）*/

case 0x48:
    if (!i2c_user_sensors[port][address].your_sensor) {
        // センサクラスのインスタンスを作成
        i2c_user_sensors[port][address].your_sensor.reset(new YourSensor());
    }
    break;

/* ここに新規センサの初期化処理を追加する（ここまで）*/
```

最後に、センサの初期化メソッドの呼び出しを行います。
メソッドの呼び出しは `i2c_user_sensors[port][address].your_sensor->init(_wire, address)` のように `i2c_user_sensors[port][address].your_sensor->メソッド名()` で呼び出します。
現在使用中の I2C バスオブジェクトは、`_wire` に格納されており、これは I2C バスオブジェクト（Wire オブジェクト）へのポインタになっています。
初期化が成功した場合は取得可能なデータ数を `I2C_INIT_RESULT` 型の値で返します。
初期化に失敗した場合はメモリを解放し、他のセンサの初期化のためにバスをリセットします。

```cpp
/* ここに新規センサの初期化処理を追加する（ここから）*/

case 0x48:
    if (!i2c_user_sensors[port][address].your_sensor) {
        i2c_user_sensors[port][address].your_sensor.reset(new YourSensor());

        // センサの初期化メソッドを呼んで初期化を行う
        if (i2c_user_sensors[port][address].your_sensor->init(_wire, address)) {
            // 初期化に成功した場合（該当センサであった場合）
            return I2C_2_DATA_DEVICE;  // 取得可能なデータ数を返す（例は2種類の場合）
        } else {
            // 初期化に失敗した場合（別のセンサであった場合）
            i2c_user_sensors[port][address].your_sensor.reset();    // メモリを解放

            // I2Cバスをリセット（次のセンサ検出のため）
            _wire->end();
            HAL.delay(20);
            _wire->begin();
        }
    }
    break;

/* ここに新規センサの初期化処理を追加する（ここまで）*/
```

> **💡I2C_INIT_RESULT**  
> センサを初期化した時の結果を表す値です。「このセンサは何種類のデータを取得できるか」を返します。
>
> ```cpp
> enum I2C_INIT_RESULT {
>     I2C_IGNORE_DEVICE = -1,    // 未知のデバイス（サポート外のデバイス）
>     I2C_0_DATA_DEVICE = 0,     // データは取得しない（マルチプレクサなど）
>     I2C_1_DATA_DEVICE = 1,     // 1種類のデータを取得できる
>     I2C_2_DATA_DEVICE = 2,     // 2種類のデータを取得できる
>     I2C_3_DATA_DEVICE = 3,     // 3種類のデータを取得できる
>     // ... 最大 30 種類まで
> };
> ```
>
> 例えば、温度と湿度など、2 つ値を取得できるセンサの場合は `I2C_2_DATA_DEVICE` を返します。
> 距離しか取得できないセンサの場合は `I2C_1_DATA_DEVICE` を返します。

ここまでで初期化処理の実装は完了です。
最後に省略部分を含めた初期化関数の実装例を下記に示します。

```cpp
I2C_INIT_RESULT initializeUserI2cDevice(uint8_t port, uint8_t address, bool is_on_top_bus) {
    auto _wire = HAL.I2c.getWire(port);  // I2C通信用のオブジェクト取得

    switch (address) {
        /* ここに新規センサの初期化処理を追加する（ここから）*/

    case 0x48:  // ← センサのI2Cアドレス（データシートで確認）
        // まだセンサが作られていない場合のみ実行
        if (!i2c_user_sensors[port][address].your_sensor) {
            // センサクラスのインスタンスを作成
            i2c_user_sensors[port][address].your_sensor.reset(new YourSensor());

            // センサの初期化メソッドを呼んで初期化を行う
            if (i2c_user_sensors[port][address].your_sensor->init(_wire, address)) {
                // 初期化に成功した場合（該当センサであった場合）
                return I2C_2_DATA_DEVICE;  // 取得可能なデータ数を返す（例は2種類の場合）
            } else {
                // 初期化に失敗した場合（別のセンサであった場合）
                i2c_user_sensors[port][address].your_sensor.reset();    // メモリを解放

                // I2Cバスをリセット（次のセンサ検出のため）
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
    return I2C_IGNORE_DEVICE;  // 該当センサではない
}
```

#### (4) センサ名取得処理の実装

`getUserI2cDeviceName()` 関数の `switch` 文の中にセンサ名の取得処理を追加します。

**例：I2C アドレス 0x48 のセンサの場合**

まずは、該当するアドレスの case 文を追加します。

```cpp
/* ここに新規センサの名前取得処理を追加する（ここから）*/

case 0x48:
    if (i2c_user_sensors[port][address].your_sensor) {

    }
    break;

/* ここに新規センサの名前取得処理を追加する（ここまで）*/
```

次に、`sub_id` に応じてセンサ名を返す処理を追加します。
`sub_id` は「何番目のデータか」を示す番号で、`sub_id = 0` が 1 つ目、`sub_id = 1` が 2 つ目のデータに対応します。
`sub_id` は初期化処理の返り値（`I2C_INIT_RESULT`）に応じて自動的に上限が決まります。

センサ名は `sub_id` に応じて `センサ名_データ種別_(I2Cアドレス)` を返すようにし、デフォルトでは `センサ名` だけを返すようにしてください。データを 1 種類しか取得できないデバイスの場合は「データ種別」を省略しても構いません。注意点として、返すセンサ名は必ず ASCII 文字である必要があります。I2C アドレスは関数上部で char 型に変換され、s_addr という変数に格納されているので、これを使用することを推奨します。

また、`sub_id` をどのデータ種別に割り当てるかは自由に決めて良いですが、以降の実装においても一貫性を持たせる必要があります。

```cpp
/* ここに新規センサの名前取得処理を追加する（ここから）*/

case 0x48:
    if (i2c_user_sensors[port][address].your_sensor) {
        // sub_id に応じて名前を変える
        switch (sub_id) {
        case 0:  // 1つ目のデータ（例：温度）
            return "YourSensor_Temperature_(" + String(s_addr) + ")";
        case 1:  // 2つ目のデータ（例：湿度）
            return "YourSensor_Humidity_(" + String(s_addr) + ")";
        default:
            return "YourSensor";
        }
    }
    break;

/* ここに新規センサの名前取得処理を追加する（ここまで）*/
```

> **💡 命名規則**  
> センサ名の形式は、取得するデータ数に応じて以下のように設定することが望ましいです。
>
> | データ数   | 名前の形式                       | 例                         |
> | ---------- | -------------------------------- | -------------------------- |
> | 1 種類のみ | `センサ名_(アドレス)`            | `LM75_(0x48)`              |
> | 複数種類   | `センサ名_データ種別_(アドレス)` | `SHT31_Temperature_(0x44)` |

ここまででセンサ名取得処理の実装は完了です。
最後に省略部分を含めた完全な実装例を下記に示します。

```cpp
String getUserI2cDeviceName(uint8_t port, uint8_t address, int16_t sub_id, bool is_on_top_bus) {
    char s_addr[5] = {0};
    snprintf(s_addr, 5, "0x%02X", address);  // アドレスを文字列化（例："0x48"）

    switch (address) {
        /* ここに新規センサの名前取得処理を追加する（ここから）*/

    case 0x48:
        if (i2c_user_sensors[port][address].your_sensor) {
            // sub_id に応じて名前を変える
            switch (sub_id) {
            case 0:  // 1つ目のデータ
                return "YourSensor_Temperature_(" + String(s_addr) + ")";
            case 1:  // 2つ目のデータ
                return "YourSensor_Humidity_(" + String(s_addr) + ")";
            default:
                return "YourSensor";  // 範囲外（通常は来ない）
            }
        }
        break;

        /* ここに新規センサの名前取得処理を追加する（ここまで）*/

    default:
        break;
    }

    return "";
}
```

#### (5) 単位取得処理の実装

`getUserI2cDeviceUnit()` 関数の `switch` 文の中に単位の取得処理を追加します。

**例：I2C アドレス 0x48 のセンサの場合**

まずは、該当するアドレスの case 文を追加します。

```cpp
/* ここに新規センサの単位取得処理を追加する（ここから）*/

case 0x48:
    if (i2c_user_sensors[port][address].your_sensor) {

    }
    break;

/* ここに新規センサの単位取得処理を追加する（ここまで）*/
```

次に、`sub_id` に応じて単位を返す処理を追加します。
`sub_id` の割り当ては `(4) センサ名取得処理` で決めたものと一致させる必要があります。

単位は測定するデータの種類に応じて適切なものを選んでください。ただし、単位の文字列は ASCII 文字での表記（例：`'C`）を使用する必要があります。

```cpp
/* ここに新規センサの単位取得処理を追加する（ここから）*/

case 0x48:
    if (i2c_user_sensors[port][address].your_sensor) {
        // sub_id に応じて単位を返す
        switch (sub_id) {
        case 0:  // 1つ目のデータ（温度）
            return "\'C";
        case 1:  // 2つ目のデータ（湿度）
            return "%RH";
        default:
            return "";
        }
    }
    break;

/* ここに新規センサの単位取得処理を追加する（ここまで）*/
```

ここまでで単位取得処理の実装は完了です。
最後に省略部分を含めた完全な実装例を下記に示します。

```cpp
String getUserI2cDeviceUnit(uint8_t port, uint8_t address, int16_t sub_id, bool is_on_top_bus) {

    switch (address) {
        /* ここに新規センサの単位取得処理を追加する（ここから）*/

    case 0x48:
        if (i2c_user_sensors[port][address].your_sensor) {
            // sub_id に応じて単位を返す
            switch (sub_id) {
            case 0:  // 1つ目のデータ（温度）
                return "\'C";
            case 1:  // 2つ目のデータ（湿度）
                return "%RH";
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
```

#### (6) センサ値取得処理の実装

`fetchUserI2cDeviceData()` 関数の `switch` 文の中にセンサ値の取得処理を追加します。**これが最後のステップです！**

**例：I2C アドレス 0x48 のセンサの場合**

まずは、該当するアドレスの case 文を追加します。

```cpp
/* ここに新規センサのセンサ値取得処理を追加する（ここから）*/

case 0x48:
    if (i2c_user_sensors[port][address].your_sensor) {

    }

/* ここに新規センサのセンサ値取得処理を追加する（ここまで）*/
```

次に、`sub_id` に応じてセンサクラスの適切なメソッドを呼び出す処理を追加します。
`sub_id` の割り当ては `(4) センサ名取得処理` および `(5) 単位取得処理` で決めたものと必ず一致させる必要があります。

センサクラスのメソッド（例：`fetchTemperature()`）を呼び出し、データ型に応じて値返却用のポインタ変数に取得した値を格納します。取得結果に応じて `I2C_DATA_RESULT` を返却します。取得失敗時や該当する `sub_id` がない場合は `I2C_RES_FAILED` を返します。

| 返却用の変数 | 対応する型      | 返却する `I2C_DATA_RESULT` | 値の例                                         |
| :----------: | --------------- | -------------------------- | ---------------------------------------------- |
|  `*d_data`   | double 型       | `I2C_RES_DOUBLE`           | 実数値（温度、湿度、電圧など）                 |
|  `*i_data`   | int 型 (32 bit) | `I2C_RES_INT`              | 整数値（カウント値、RGB 値、ADC 生データなど） |
|  `*s_data`   | String 型       | `I2C_RES_STRING`           | 文字列値（RFID の ID など）                    |
|      -       | -               | `I2C_RES_FAILED`           | 通信エラーなど取得失敗時                       |

```cpp
/* ここに新規センサのセンサ値取得処理を追加する（ここから）*/

case 0x48:
    if (i2c_user_sensors[port][address].your_sensor) {
        switch (sub_id) {
        case 0:  // 1つ目のデータ（温度）
            // センサクラスの fetchTemperature() を呼ぶ
            result = i2c_user_sensors[port][address].your_sensor->fetchTemperature(&d_tmp);
            if (result) {
                *d_data = d_tmp;           // 取得した値を格納
                return I2C_RES_DOUBLE;     // double型で返す
            }
            return I2C_RES_FAILED;  // 取得失敗

        case 1:  // 2つ目のデータ（湿度）
            // センサクラスの fetchHumidity() を呼ぶ
            result = i2c_user_sensors[port][address].your_sensor->fetchHumidity(&d_tmp);
            if (result) {
                *d_data = d_tmp;           // 取得した値を格納
                return I2C_RES_DOUBLE;     // double型で返す
            }
            return I2C_RES_FAILED;  // 取得失敗

        default:
            return I2C_RES_FAILED;  // 取得失敗
        }
    }

/* ここに新規センサのセンサ値取得処理を追加する（ここまで）*/
```

ここまででセンサ値取得処理の実装は完了です。
最後に省略部分を含めた完全な実装例を下記に示します。

```cpp
I2C_DATA_RESULT fetchUserI2cDeviceData(uint8_t port,
                                       uint8_t address,
                                       int16_t sub_id,
                                       bool is_on_top_bus,
                                       bool tx_buffered,
                                       double *d_data,   // double 型データの格納先
                                       int32_t *i_data,  // int 型データの格納先
                                       String *s_data) { // String 型データの格納先
    bool result;
    double d_tmp = 0.0;    // 一時保存用（double型）
    uint32_t ui_tmp = 0;   // 一時保存用（int型）

    switch (address) {
        /* ここに新規センサのセンサ値取得処理を追加する（ここから）*/

    case 0x48:
        if (i2c_user_sensors[port][address].your_sensor) {
            switch (sub_id) {
            case 0:  // 1つ目のデータ（温度）
                // センサクラスの fetchTemperature() を呼ぶ
                result = i2c_user_sensors[port][address].your_sensor->fetchTemperature(&d_tmp);
                if (result) {
                    *d_data = d_tmp;           // 取得した値を格納
                    return I2C_RES_DOUBLE;     // double型で返す
                }
                return I2C_RES_FAILED;  // 取得失敗

            case 1:  // 2つ目のデータ（湿度）
                // センサクラスの fetchHumidity() を呼ぶ
                result = i2c_user_sensors[port][address].your_sensor->fetchHumidity(&d_tmp);
                if (result) {
                    *d_data = d_tmp;           // 取得した値を格納
                    return I2C_RES_DOUBLE;     // double型で返す
                }
                return I2C_RES_FAILED;  // 取得失敗

            default:
                return I2C_RES_FAILED;  // 取得失敗
            }
        }

        /* ここに新規センサのセンサ値取得処理を追加する（ここまで）*/

    default:
        break;
    }

    return I2C_RES_FAILED;
}
```

これで新規センサの追加は完了です。
