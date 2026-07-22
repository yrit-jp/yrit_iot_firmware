@echo off
setlocal enabledelayedexpansion

:: コピー先フォルダの指定（バッチ実行フォルダからの相対パス）
set DESTINATION=C:\Users\%USERNAME%\Documents\Arduino\libraries

:: コピー元フォルダの指定（後でプロファイル選択）
set SOURCE=

:: プロファイルの選択
:select_profile
echo === コピーするプロファイルを選択してください ===
echo [1] Common Library Only
echo [2] ESP32 Devkit
echo [3] M5Stack
echo [4] LilyGO
echo [5] Wio Terminal
echo [6] Wio Tracker 1110
echo [7] キャンセル
choice /c 1234567 /n /m "プロファイルを選択: "
set PROFILE=%ERRORLEVEL%

if "%PROFILE%" == "1" set SOURCE=%CD%\libraries\common
if "%PROFILE%" == "2" set SOURCE=%CD%\libraries\esp32_yrit_dev_board
if "%PROFILE%" == "3" set SOURCE=%CD%\libraries\m5stack
if "%PROFILE%" == "4" set SOURCE=%CD%\libraries\lilygo_t-display_s3_pro
if "%PROFILE%" == "5" set SOURCE=%CD%\libraries\wio_terminal
if "%PROFILE%" == "6" set SOURCE=%CD%\libraries\wio_tracker_1110
if "%PROFILE%" == "7" exit /b

echo 選択されたプロファイル: %SOURCE%
echo.

:: 確認プロンプト
echo 指定先フォルダ (%DESTINATION%) の内容を全て削除します。
choice /c YN /n /m "続行しますか? (Y/N): "
if errorlevel 2 exit /b

:: フォルダの中身を削除
echo 指定先フォルダをクリア中...
del /q /s %DESTINATION%\* > nul 2>&1
for /d %%D in (%DESTINATION%\*) do rmdir /s /q "%%D" > nul 2>&1

echo コピーを開始します...

:: コピーの進行状況を表示しながら実行
if not "%PROFILE%" == "1" robocopy %CD%\libraries\common "%DESTINATION%" /E /ETA /NS /NC /NJS /NJH
robocopy "%SOURCE%" "%DESTINATION%" /E /ETA /NS /NC /NJS /NJH

:: コピー完了メッセージ
echo コピーが完了しました。
timeout /t 5
endlocal
