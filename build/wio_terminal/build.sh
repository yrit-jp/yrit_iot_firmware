#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="wio_terminal"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="Seeeduino:samd:seeed_wio_terminal:role=master,cache=off,speed=120,opt=small,maxqspi=50,usbstack=arduino,debug=off,txrxled=on"
LIB_DIR="wio_terminal"
SKIP_BOOT_APP0=1

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*.hex

exit "$status"
