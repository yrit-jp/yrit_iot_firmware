#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="m5stack_tough"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="m5stack:esp32:m5stack_tough:PSRAM=disabled,PartitionScheme=default,CPUFreq=240,FlashMode=qio,FlashFreq=80,FlashSize=16M,UploadSpeed=1500000,LoopCore=1,EventsCore=1,DebugLevel=none,EraseFlash=none"
LIB_DIR="m5stack"

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*merged.bin

exit "$status"
