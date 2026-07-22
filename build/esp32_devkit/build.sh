#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="esp32_devkit"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="esp32:esp32:esp32:JTAGAdapter=default,PSRAM=disabled,PartitionScheme=min_spiffs,CPUFreq=240,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=921600,LoopCore=1,EventsCore=1,DebugLevel=none,EraseFlash=none"
LIB_DIR="esp32_yrit_dev_board"

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*merged.bin

exit "$status"
