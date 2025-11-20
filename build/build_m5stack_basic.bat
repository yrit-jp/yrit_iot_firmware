@echo off
REM 遅延展開を有効化
SETLOCAL ENABLEDELAYEDEXPANSION

set TARGET=m5stack_basic
set VERSION_FILE=..\src\firmware_version.h
set PROJECT_ROOT=..\

REM バージョン情報を取得
echo "Fetch firmware version..."
for /f "tokens=3 delims= " %%a in ('type "%VERSION_FILE%" ^| findstr /c:"#define APP_MAIN_VERSION"') do (
  set "v_main=%%a"
)

for /f "tokens=3 delims= " %%a in ('type "%VERSION_FILE%" ^| findstr /c:"#define APP_MINOR_VERSION"') do (
  set "v_minor=%%a"
)

for /f "tokens=3 delims= " %%a in ('type "%VERSION_FILE%" ^| findstr /c:"#define APP_RELEASE_TAG"') do (
  set "v_release_tag=%%~a"
)

if defined v_release_tag (
  REM v_release_tag is not empty
  set vName=v!v_main!.!v_minor!_!v_release_tag!

  REM 末尾の数字を削除する処理
  set v_release_tag_without_number=!v_release_tag!
  :remove_digits
  set "last_char=!v_release_tag_without_number:~-1!"
  echo !v_release_tag_without_number! | findstr /r "[0-9]" >NUL
  if not errorlevel 1 (
      set "v_release_tag_without_number=!v_release_tag_without_number:~0,-1!"
      goto remove_digits
  )
  echo "!v_release_tag_without_number!"

  set PROGRAM_NAME=IoT_Device_!v_release_tag_without_number!
  set PROGRAM_DIR=IoT_Device_!v_release_tag_without_number!
  echo !PROGRAM_DIR!
  REM goto を使うと if-else が正しく解析されなくなるので goto で移動
  goto skip_else
) else (
  REM v_release_tag is empty
  set vName=v!v_main!.!v_minor!
  set PROGRAM_NAME=IoT_Device
  set PROGRAM_DIR=IoT_Device
)
:skip_else
echo "==> !vName!"

REM 前回のファイルを削除
echo Clearn previous output...
echo ".\output\!TARGET!\!PROGRAM_DIR!"
rd /s /q .\output\!TARGET!\!PROGRAM_DIR!\!vName!

if not exist ".\output\!TARGET!\!PROGRAM_DIR!\!vName!" (
  mkdir ".\output\!TARGET!\!PROGRAM_DIR!\!vName!"
)

arduino-cli.exe compile ^
--fqbn=m5stack:esp32:m5stack_core:PartitionScheme=min_spiffs,CPUFreq=240,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=1500000,LoopCore=1,EventsCore=1,DebugLevel=none,EraseFlash=none ^
--libraries !USERPROFILE!\AppData\Local\Arduino15\libraries ^
--libraries !PROJECT_ROOT!\libraries\common ^
--libraries !PROJECT_ROOT!\libraries\m5stack ^
--output-dir ./output/!TARGET!/!PROGRAM_DIR!/!vName! ^
--build-path ./cache/!TARGET! ^
--verbose ^
../

REM 一括リネーム
ren .\output\!TARGET!\!PROGRAM_DIR!\!vName!\yrit_iot_device* !PROGRAM_NAME!*

REM 変更履歴ファイルをコピー
if defined v_release_tag (
  REM v_release_tag is not empty
  copy ..\CHANGELOG_BETA.md .\output\!TARGET!\!PROGRAM_DIR!\CHANGELOG.md
) else (
  REM v_release_tag is empty
  copy ..\CHANGELOG.md .\output\!TARGET!\!PROGRAM_DIR! 
)

REM boot_app0 ファイルをコピー
copy boot_app0.bin .\output\!TARGET!\!PROGRAM_DIR!\!vName!

REM 不要ファイルの削除
del .\output\!TARGET!\!PROGRAM_DIR!\!vName!\*.map
del .\output\!TARGET!\!PROGRAM_DIR!\!vName!\*.elf

REM 終了時にベルを鳴らす
echo 
pause