@echo off
REM 遅延展開を有効化
SETLOCAL ENABLEDELAYEDEXPANSION
REM build_all.bat 等、別ディレクトリから呼ばれた場合でも自分の場所を起点にする
cd /d "%~dp0"

set TARGET=m5stack_basic
set VERSION_FILE=..\..\src\firmware_version.h
set PROJECT_ROOT=..\..\
set FQBN=m5stack:esp32:m5stack_core:PartitionScheme=huge_app,CPUFreq=240,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=1500000,LoopCore=1,EventsCore=1,DebugLevel=none,EraseFlash=none
set LIB_DIR=m5stack

call ..\_common.bat run_build

REM 不要ファイルの削除
del "!OUTPUT_DIR!\*.map" 2>nul
del "!OUTPUT_DIR!\*.elf" 2>nul
del "!OUTPUT_DIR!\*merged.bin" 2>nul

REM エラーステータスで終了
if "!BUILD_STATUS!"=="FAILED" exit /b 1
exit /b 0
