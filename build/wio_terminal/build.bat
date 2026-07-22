@echo off
REM 遅延展開を有効化
SETLOCAL ENABLEDELAYEDEXPANSION
REM build_all.bat 等、別ディレクトリから呼ばれた場合でも自分の場所を起点にする
cd /d "%~dp0"

set TARGET=wio_terminal
set VERSION_FILE=..\..\src\firmware_version.h
set PROJECT_ROOT=..\..\
set FQBN=Seeeduino:samd:seeed_wio_terminal:role=master,cache=off,speed=120,opt=small,maxqspi=50,usbstack=arduino,debug=off,txrxled=on
set LIB_DIR=wio_terminal
set SKIP_BOOT_APP0=1

call ..\_common.bat run_build

REM 不要ファイルの削除
del "!OUTPUT_DIR!\*.map" 2>nul
del "!OUTPUT_DIR!\*.elf" 2>nul
del "!OUTPUT_DIR!\*.hex" 2>nul

REM エラーステータスで終了
if "!BUILD_STATUS!"=="FAILED" exit /b 1
exit /b 0
