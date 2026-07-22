#!/bin/bash

# 文字コードを UTF-8 に設定
export LANG=en_US.UTF-8

# コピー先フォルダの指定（ホームディレクトリからの相対パス）
DESTINATION="$HOME/Arduino/libraries"

# コピー元フォルダの指定（後でプロファイル選択）
SOURCE=""

# プロファイルの選択
echo "=== コピーするプロファイルを選択してください ==="
echo "[1] Common Library Only"
echo "[2] ESP32 Devkit"
echo "[3] M5Stack"
echo "[4] LilyGO"
echo "[5] Wio Terminal"
echo "[6] Wio Tracker 1110"
echo "[7] キャンセル"
echo -n "プロファイルを選択: "
read -n 1 choice
echo ""

case $choice in
    1) SOURCE="$(pwd)/libraries/common" ;;
    2) SOURCE="$(pwd)/libraries/esp32_yrit_dev_board" ;;
    3) SOURCE="$(pwd)/libraries/m5stack" ;;
    4) SOURCE="$(pwd)/libraries/lilygo_t-display_s3_pro" ;;
    5) SOURCE="$(pwd)/libraries/wio_terminal" ;;
    6) SOURCE="$(pwd)/libraries/wio_tracker_1110" ;;
    7) echo "キャンセルされました"; exit 0 ;;
    *) echo "無効な選択です"; exit 1 ;;
esac

echo "選択されたプロファイル: $SOURCE"
echo ""

# 確認プロンプト
echo "指定先フォルダ ($DESTINATION) の内容を全て削除します。"
echo -n "続行しますか? (y/n): "
read -n 1 confirm
echo ""

if [[ ! "$confirm" =~ [yY] ]]; then
    echo "キャンセルされました"
    exit 0
fi

# フォルダの中身を削除
echo "指定先フォルダをクリア中..."
if [ -d "$DESTINATION" ]; then
    rm -rf "$DESTINATION"/*
else
    mkdir -p "$DESTINATION"
fi

echo "コピーを開始します..."

# コピーの実行
if [ "$choice" != "1" ]; then
    echo "共通ライブラリをコピー中..."
    cp -R "$(pwd)/libraries/common/"* "$DESTINATION/"
fi

echo "選択されたプロファイルをコピー中..."
cp -R "$SOURCE/"* "$DESTINATION/"

# コピー完了メッセージ
echo "コピーが完了しました。"
# sleep 5