@echo off
REM ============================================================
REM  build_all.bat
REM  全ビルドスクリプトを指定数ずつ並列実行する
REM ============================================================
SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo ========================================================
echo   Build All Targets
echo ========================================================
echo.

REM ----------------------------------------------------------
REM  */build.bat を収集
REM ----------------------------------------------------------
set TOTAL=0
for /d %%d in (*) do (
    if exist "%%d\build.bat" (
        set /a TOTAL+=1
        set "SCRIPT_!TOTAL!=%%d\build.bat"
    )
)

if !TOTAL! EQU 0 (
    echo   No build scripts found in current directory.
    pause
    exit /b 1
)

echo   Found !TOTAL! build script(s):
echo.
for /L %%i in (1,1,!TOTAL!) do (
    echo     [%%i] !SCRIPT_%%i!
)
echo.

REM ----------------------------------------------------------
REM  同時実行数をユーザに確認
REM ----------------------------------------------------------
:input_loop
set "PARALLEL="
set /p "PARALLEL=  Enter number of simultaneous builds (1-!TOTAL!): "
if not defined PARALLEL goto :input_loop

REM 数値バリデーション (数字以外の文字が含まれていたら再入力)
for /f "delims=0123456789" %%a in ("!PARALLEL!") do (
    echo   Invalid input. Please enter a number.
    goto :input_loop
)
if !PARALLEL! LSS 1 (
    echo   Please enter a number from 1 to !TOTAL!.
    goto :input_loop
)
if !PARALLEL! GTR !TOTAL! set PARALLEL=!TOTAL!

echo.
echo   Will run !TOTAL! build(s), !PARALLEL! at a time.
echo.

REM ----------------------------------------------------------
REM  追跡用の一時ディレクトリとラッパースクリプトを作成
REM ----------------------------------------------------------
set "TRACKDIR=%TEMP%\build_all_%RANDOM%"
if exist "!TRACKDIR!" rd /s /q "!TRACKDIR!"
mkdir "!TRACKDIR!"

REM ラッパースクリプト: ビルド実行 → 結果記録 → ロック解除
(
    echo @echo off
    echo SETLOCAL
    echo call %%1 ^< nul
    echo if errorlevel 1 ^(
    echo     echo FAILED^> "%%2\%%3.result"
    echo ^) else ^(
    echo     echo SUCCESS^> "%%2\%%3.result"
    echo ^)
    echo del "%%2\%%3.lock" 2^>nul
) > "!TRACKDIR!\_run.bat"

REM 開始時刻を記録
set "START_TIME=!TIME!"

REM ----------------------------------------------------------
REM  バッチごとに並列実行
REM ----------------------------------------------------------
set IDX=0
set RUNNING=0
set PASS=0
set FAIL=0
set "FAIL_LIST="

echo.
echo   Running builds (max !PARALLEL! at a time)...
echo.

REM 初回起動: 並列数分をまとめて起動
:fill_initial
if !RUNNING! GEQ !PARALLEL! goto :poll
if !IDX! GEQ !TOTAL! goto :poll
set /a IDX+=1

REM 二重展開のための for トリック
for %%j in (!IDX!) do set "CUR=!SCRIPT_%%j!"

echo   [!IDX!/!TOTAL!] Starting: !CUR!

REM ロックファイルを先に作成してから起動
type nul > "!TRACKDIR!\!IDX!.lock"

REM 新しいウィンドウでビルド実行
start "[!IDX!/!TOTAL!] !CUR!" cmd /c ""!TRACKDIR!\_run.bat" "!CUR!" "!TRACKDIR!" "!IDX!""

set /a RUNNING+=1
goto :fill_initial

:poll
REM 完了したビルドの結果を表示し、空いた枠に次のビルドを即座に起動
for /L %%i in (1,1,!IDX!) do (
    if exist "!TRACKDIR!\%%i.result" (
        if not exist "!TRACKDIR!\%%i.shown" (
            for %%j in (%%i) do set "SNAME=!SCRIPT_%%j!"
            set "RESULT=UNKNOWN"
            for /f "usebackq" %%r in ("!TRACKDIR!\%%i.result") do set "RESULT=%%r"
            if "!RESULT!"=="SUCCESS" (
                set /a PASS+=1
                powershell -Command "Write-Host '    [%%i/!TOTAL!] [PASS] !SNAME!' -ForegroundColor Green"
            ) else (
                set /a FAIL+=1
                set "FAIL_LIST=!FAIL_LIST! !SNAME!"
                powershell -Command "Write-Host '    [%%i/!TOTAL!] [FAIL] !SNAME!' -ForegroundColor Red"
            )
            type nul > "!TRACKDIR!\%%i.shown"
            set /a RUNNING-=1

            REM 枠が空いたので次のビルドを即座に起動
            if !IDX! LSS !TOTAL! (
                set /a IDX+=1
                for %%k in (!IDX!) do set "CUR=!SCRIPT_%%k!"
                echo   [!IDX!/!TOTAL!] Starting: !CUR!
                type nul > "!TRACKDIR!\!IDX!.lock"
                start "[!IDX!/!TOTAL!] !CUR!" cmd /c ""!TRACKDIR!\_run.bat" "!CUR!" "!TRACKDIR!" "!IDX!""
                set /a RUNNING+=1
            )
        )
    )
)

if !RUNNING! GTR 0 (
    timeout /t 3 /nobreak >nul
    goto :poll
)

echo.
echo   All builds complete.
echo.

REM ----------------------------------------------------------
REM  結果サマリー
REM ----------------------------------------------------------
:show_summary
echo.
echo ========================================================
echo   BUILD RESULTS
echo ========================================================
echo.

for /L %%i in (1,1,!TOTAL!) do (
    for %%j in (%%i) do set "SNAME=!SCRIPT_%%j!"
    set "RESULT=UNKNOWN"
    if exist "!TRACKDIR!\%%i.result" (
        for /f "usebackq" %%r in ("!TRACKDIR!\%%i.result") do set "RESULT=%%r"
    )
    if "!RESULT!"=="SUCCESS" (
        powershell -Command "Write-Host '    [PASS] !SNAME!' -ForegroundColor Green"
    ) else (
        powershell -Command "Write-Host '    [FAIL] !SNAME!' -ForegroundColor Red"
    )
)

echo.
echo   Total: !TOTAL!   Passed: !PASS!   Failed: !FAIL!
echo   Start: !START_TIME!   End: !TIME!

if !FAIL! GTR 0 (
    echo.
    powershell -Command "Write-Host '  Failed targets:!FAIL_LIST!' -ForegroundColor Yellow"
)

echo.
echo ========================================================

REM 一時ファイルをクリーンアップ
rd /s /q "!TRACKDIR!" 2>nul

echo.
pause

if !FAIL! GTR 0 exit /b 1
exit /b 0
