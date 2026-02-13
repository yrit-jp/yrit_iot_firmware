# スタートガイド

このドキュメントでは、YRIT IoT Firmware の開発を始めるための手順を説明します。

## 1. 環境構築

### 必要なソフトウェア

| ソフトウェア | バージョン            | 備考                   |
| ------------ | --------------------- | ---------------------- |
| Arduino IDE  | v2.3.6 または v1.8.19 |                        |
| Git          | 任意                  | リポジトリのクローン用 |

### ボードマネージャのインストール

Arduino IDE の「ボードマネージャ」から以下をインストールしてください。

| ボード            | バージョン | 対象デバイス     |
| ----------------- | ---------- | ---------------- |
| esp32             | v3.2.1     | ESP32 DevKit     |
| M5Stack           | v3.2.5     | M5Stack シリーズ |
| Seeed SAMD Boards | v1.8.5     | Wio Terminal     |

### ライブラリのセットアップ

本プロジェクトには多数の依存ライブラリがあります。
依存ライブラリは本リポジトリに同梱されており、セットアップスクリプトを使用することで対象のデバイスに合わせた環境を自動的に構築します。

```bash
# Windows
.\arduino_library_setter.bat

# Linux / macOS
bash arduino_library_setter.sh
```

プロンプトに従い、ターゲットデバイスに応じたプロファイルを選択します。

```
=== コピーするプロファイルを選択してください ===
[1] Common Library Only
[2] ESP32 Devkit
[3] M5Stack
[4] LilyGO
[5] Wio Terminal
[6] キャンセル
プロファイルを選択: 3   # M5Stack がターゲットの場合
```

> **注意**: このスクリプトは Arduino ライブラリフォルダの内容を削除します。

---

## 2. プロジェクト構成

```
/
├── main.ino                    # メインスケッチ
├── src/
│   ├── config/                 # 設定パラメータ管理
│   ├── display/                # 画面表示制御
│   ├── hal/                    # ハードウェア抽象化層
│   │   └── hardware/           # デバイス固有の実装
│   ├── i2c/                    # I2C センサ管理
│   ├── modbus/                 # Modbus センサ管理
│   ├── transmit/               # データ送信関係
│   └── user/                   # ユーザーカスタマイズ領域
├── libraries/                  # 依存ライブラリ
└── notes/                      # ドキュメント
```

## 3. ビルドと書き込み

### 手順

1. Arduino IDE で `main.ino` を開く
2. ボード設定を行う（下記参照）
3. ビルド（検証）を実行
4. デバイスに書き込み

### ボード設定例（M5Stack Basic）

| 設定項目         | 設定値                           |
| ---------------- | -------------------------------- |
| ボード           | M5Core                           |
| Upload Speed     | 1500000                          |
| CPU Frequency    | 240MHz (WiFi/BT)                 |
| Flash Frequency  | 80MHz                            |
| Flash Mode       | QIO                              |
| Flash Size       | 4MB (32Mb)                       |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |

他のデバイスの設定は `notes/ARDUINO_CONFIG_MEMO.asciidoc` を参照してください。

---

## 4. デバッグ

### シリアルモニタ

Arduino IDE のシリアルモニタでログを確認できます。
WioTerminal は USB シリアルを使用していないため、別途 USB シリアル変換モジュールを使用する必要があります。

- **ボーレート**: 115200
- **出力内容**: 初期化状態、センサ値、送信状態など
