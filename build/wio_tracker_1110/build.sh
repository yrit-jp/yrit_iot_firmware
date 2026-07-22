#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
source ../_common.sh

TARGET="wio_tracker_1110"
VERSION_FILE="../../src/firmware_version.h"
PROJECT_ROOT="../../"
FQBN="Seeeduino:nrf52:wio_tracker_1110:power_supply_grove=off"
LIB_DIR="wio_tracker_1110"
SKIP_BOOT_APP0=1

run_build
status=$?

# 不要ファイルの削除
rm -f "${OUTPUT_DIR}"/*.map "${OUTPUT_DIR}"/*.elf "${OUTPUT_DIR}"/*.hex

exit "$status"
