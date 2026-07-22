#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="m5stack_core_s3"
SUB_TARGET="m5stack_core_s3_se"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="m5stack:esp32:m5stack_cores3:JTAGAdapter=default,PSRAM=enabled,FlashMode=qio,FlashSize=16M,LoopCore=1,EventsCore=1,USBMode=hwcdc,CDCOnBoot=cdc,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,PartitionScheme=app3M_fat9M_16MB,CPUFreq=240,UploadSpeed=921600,DebugLevel=none"
LIB_DIR="m5stack"

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*merged.bin
rm -f "${SUB_OUTPUT_DIR}"/*.map "${SUB_OUTPUT_DIR}"/*.elf "${SUB_OUTPUT_DIR}"/*merged.bin

exit "$status"
