@echo off
REM 各 <board>\build.bat から call される共通ビルドロジック。単体では実行しない。
REM 呼び出し例: call ..\_common.bat run_build
REM (注意: "call 別ファイル.bat :ラベル" は別ファイル内のラベルへ直接ジャンプしない。
REM  必ず先頭から実行されるため、引数を見て goto で振り分ける必要がある)

if "%~1"=="fetch_firmware_version" goto fetch_firmware_version
if "%~1"=="run_build" goto run_build
goto :eof

:fetch_firmware_version
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
set OUTPUT_DIR=..\__output\!TARGET!\!PROGRAM_DIR!\!vName!
if defined SUB_TARGET set SUB_OUTPUT_DIR=..\__output\!SUB_TARGET!\!PROGRAM_DIR!\!vName!
echo "==> !vName!"
goto :eof

REM TARGET / VERSION_FILE / PROJECT_ROOT / FQBN / LIB_DIR を使ってビルドを実行する
REM SUB_TARGET / SKIP_BOOT_APP0 は必要な場合のみ呼び出し側で設定する
REM 呼び出し元 (<board>\build.bat) は build\<board>\ に置かれている前提で、
REM __output/__cache/boot_app0.bin/CHANGELOG*.md は build\ 直下を共有する
:run_build
call :fetch_firmware_version

REM 前回のファイルを削除
echo Clearn previous output...
echo "..\__output\!TARGET!\!PROGRAM_DIR!"
rd /s /q ..\__output\!TARGET!\!PROGRAM_DIR!\!vName!
if defined SUB_TARGET (
  echo "..\__output\!SUB_TARGET!\!PROGRAM_DIR!"
  rd /s /q ..\__output\!SUB_TARGET!\!PROGRAM_DIR!\!vName!
)

if not exist "..\__output\!TARGET!\!PROGRAM_DIR!\!vName!" (
  mkdir "..\__output\!TARGET!\!PROGRAM_DIR!\!vName!"
)

arduino-cli.exe compile ^
--fqbn=!FQBN! ^
--libraries !USERPROFILE!\AppData\Local\Arduino15\libraries ^
--libraries !PROJECT_ROOT!\libraries\common ^
--libraries !PROJECT_ROOT!\libraries\!LIB_DIR! ^
--output-dir ..\__output\!TARGET!\!PROGRAM_DIR!\!vName! ^
--build-path ..\__cache\!TARGET! ^
--verbose ^
!PROJECT_ROOT!

REM arduino-cliのエラーチェック
if errorlevel 1 (
  echo.
  powershell -Command "Write-Host '========================================' -ForegroundColor Red"
  powershell -Command "Write-Host '  BUILD FAILED!' -ForegroundColor Red"
  powershell -Command "Write-Host '  arduino-cli compile error occurred.' -ForegroundColor Red"
  powershell -Command "Write-Host '========================================' -ForegroundColor Red"
  echo.
  set BUILD_STATUS=FAILED
  goto :run_build_finish
)

REM 一括リネーム
ren ..\__output\!TARGET!\!PROGRAM_DIR!\!vName!\yrit_iot_firmware.ino* !PROGRAM_NAME!.ino* 2>nul
ren ..\__output\!TARGET!\!PROGRAM_DIR!\!vName!\_yrit_iot_firmware.ino* !PROGRAM_NAME!.ino* 2>nul

REM 変更履歴ファイルをコピー
if defined v_release_tag (
  REM v_release_tag is not empty
  copy ..\..\CHANGELOG_BETA.md ..\__output\!TARGET!\!PROGRAM_DIR!\CHANGELOG.md
) else (
  REM v_release_tag is empty
  copy ..\..\CHANGELOG.md ..\__output\!TARGET!\!PROGRAM_DIR!
)

REM boot_app0 ファイルをコピー
if not defined SKIP_BOOT_APP0 (
  copy ..\boot_app0.bin ..\__output\!TARGET!\!PROGRAM_DIR!\!vName!
)

if defined SUB_TARGET (
  REM SUB_TARGET 用のファイルをコピー
  xcopy /y /s /q /i ..\__output\!TARGET! ..\__output\!SUB_TARGET!
)

REM 成功時の処理
set BUILD_STATUS=SUCCESS

:run_build_finish
echo.
echo Finish. Target: !TARGET!
if defined SUB_TARGET (
  echo Finish. Target: !SUB_TARGET!
)
if "!BUILD_STATUS!"=="SUCCESS" (
  powershell -Command "Write-Host 'Build Status: SUCCESS' -ForegroundColor Green"
) else (
  powershell -Command "Write-Host 'Build Status: FAILED' -ForegroundColor Red"
)
echo.

REM 終了時にベルを鳴らす
echo 
pause
goto :eof
