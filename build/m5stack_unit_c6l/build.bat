@echo off
REM 遅延展開を有効化
SETLOCAL ENABLEDELAYEDEXPANSION
REM build_all.bat 等、別ディレクトリから呼ばれた場合でも自分の場所を起点にする
cd /d "%~dp0"

set TARGET=m5stack_unit_c6l
set VERSION_FILE=..\..\src\firmware_version.h
set PROJECT_ROOT=..\..\
set FQBN=m5stack:esp32:m5stack_unit_c6l:UploadSpeed=921600,CDCOnBoot=cdc,CPUFreq=160,FlashFreq=80,FlashMode=qio,FlashSize=16M,PartitionScheme=huge_app,DebugLevel=none,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default
set LIB_DIR=m5stack

call ..\_common.bat run_build

REM 不要ファイルの削除
del "!OUTPUT_DIR!\*.map" 2>nul
del "!OUTPUT_DIR!\*.elf" 2>nul
del "!OUTPUT_DIR!\*merged.bin" 2>nul

REM エラーステータスで終了
if "!BUILD_STATUS!"=="FAILED" exit /b 1
exit /b 0
