#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="lilygo_t_display_s3_pro"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="esp32:esp32:esp32s3:JTAGAdapter=default,PSRAM=opi,FlashMode=qio,FlashSize=16M,LoopCore=1,EventsCore=1,USBMode=hwcdc,CDCOnBoot=cdc,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,PartitionScheme=app3M_fat9M_16MB,CPUFreq=240,UploadSpeed=921600,DebugLevel=none,EraseFlash=none"
LIB_DIR="lilygo_t-display_s3_pro"

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*merged.bin

exit "$status"
