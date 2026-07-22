#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="arduino_nesso_n1"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="m5stack:esp32:arduino_nesso_n1:UploadSpeed=921600,CDCOnBoot=cdc,CPUFreq=160,FlashFreq=80,FlashMode=qio,FlashSize=16M,PartitionScheme=huge_app,DebugLevel=none,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default"
LIB_DIR="m5stack"

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*merged.bin

exit "$status"
