#!/bin/bash
# ============================================================
#  build_all.sh
#  全ビルドスクリプトを指定数ずつ並列実行する
# ============================================================
set -uo pipefail
cd "$(dirname "$0")"

RED='\033[31m'
GREEN='\033[32m'
YELLOW='\033[33m'
NC='\033[0m'

echo
echo "========================================================"
echo "  Build All Targets"
echo "========================================================"
echo

# ----------------------------------------------------------
#  */build.sh を収集
# ----------------------------------------------------------
scripts=()
for f in */build.sh; do
    [ -e "$f" ] || continue
    scripts+=("$f")
done

TOTAL=${#scripts[@]}
if [ "$TOTAL" -eq 0 ]; then
    echo "  No build scripts found in current directory."
    read -r -p "Press Enter to continue..." _ || true
    exit 1
fi

echo "  Found ${TOTAL} build script(s):"
echo
for i in "${!scripts[@]}"; do
    echo "    [$((i + 1))] ${scripts[$i]}"
done
echo

# ----------------------------------------------------------
#  同時実行数をユーザに確認
# ----------------------------------------------------------
while true; do
    read -r -p "  Enter number of simultaneous builds (1-${TOTAL}): " PARALLEL
    if ! [[ "$PARALLEL" =~ ^[0-9]+$ ]]; then
        echo "  Invalid input. Please enter a number."
        continue
    fi
    if [ "$PARALLEL" -lt 1 ]; then
        echo "  Please enter a number from 1 to ${TOTAL}."
        continue
    fi
    [ "$PARALLEL" -gt "$TOTAL" ] && PARALLEL=$TOTAL
    break
done

echo
echo "  Will run ${TOTAL} build(s), ${PARALLEL} at a time."
echo

# ----------------------------------------------------------
#  追跡用の一時ディレクトリ (各ビルドのログと結果を記録)
# ----------------------------------------------------------
TRACKDIR=$(mktemp -d "${TMPDIR:-/tmp}/build_all.XXXXXX")

START_TIME=$(date +%T)

PASS=0
FAIL=0
FAIL_LIST=()

# ----------------------------------------------------------
#  空いた枠に即座に次のビルドを詰める常時並列実行
# ----------------------------------------------------------
declare -A pid_to_idx
idx=0
running=0

# 1件ビルドを起動して実行枠を1つ消費する
launch_one() {
    idx=$((idx + 1))
    local cur="${scripts[$((idx - 1))]}"
    echo "  [${idx}/${TOTAL}] Starting: ${cur}"
    (
        # 標準入力を /dev/null にすることで各スクリプト末尾の pause をスキップする
        if bash "$cur" </dev/null >"${TRACKDIR}/${idx}.log" 2>&1; then
            echo SUCCESS >"${TRACKDIR}/${idx}.result"
        else
            echo FAILED >"${TRACKDIR}/${idx}.result"
        fi
    ) &
    pid_to_idx[$!]=$idx
    running=$((running + 1))
}

echo
echo "  Running builds (max ${PARALLEL} at a time)..."
echo

# 初回起動: 並列数分をまとめて起動
while [ "$running" -lt "$PARALLEL" ] && [ "$idx" -lt "$TOTAL" ]; do
    launch_one
done

# 完了したビルドの結果を表示し、空いた枠に次のビルドを即座に起動
while [ "$running" -gt 0 ]; do
    wait -n 2>/dev/null || true

    for pid in "${!pid_to_idx[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            continue
        fi

        n="${pid_to_idx[$pid]}"
        sname="${scripts[$((n - 1))]}"
        result=$(cat "${TRACKDIR}/${n}.result" 2>/dev/null || echo UNKNOWN)
        if [ "$result" = SUCCESS ]; then
            PASS=$((PASS + 1))
            echo -e "    [${n}/${TOTAL}] ${GREEN}[PASS] ${sname}${NC}"
        else
            FAIL=$((FAIL + 1))
            FAIL_LIST+=("$sname")
            echo -e "    [${n}/${TOTAL}] ${RED}[FAIL] ${sname}${NC}  (log: ${TRACKDIR}/${n}.log)"
        fi

        unset 'pid_to_idx[$pid]'
        running=$((running - 1))

        # 枠が空いたので次のビルドを即座に起動
        if [ "$idx" -lt "$TOTAL" ]; then
            launch_one
        fi
    done
done

echo
echo "  All builds complete."
echo

# ----------------------------------------------------------
#  結果サマリー
# ----------------------------------------------------------
echo
echo "========================================================"
echo "  BUILD RESULTS"
echo "========================================================"
echo

for i in "${!scripts[@]}"; do
    n=$((i + 1))
    result=$(cat "${TRACKDIR}/${n}.result" 2>/dev/null || echo UNKNOWN)
    if [ "$result" = SUCCESS ]; then
        echo -e "    ${GREEN}[PASS] ${scripts[$i]}${NC}"
    else
        echo -e "    ${RED}[FAIL] ${scripts[$i]}${NC}"
    fi
done

echo
echo "  Total: ${TOTAL}   Passed: ${PASS}   Failed: ${FAIL}"
echo "  Start: ${START_TIME}   End: $(date +%T)"

if [ "$FAIL" -gt 0 ]; then
    echo
    echo -e "  ${YELLOW}Failed targets:${FAIL_LIST[*]/#/ }${NC}"
    echo "  Logs kept at: ${TRACKDIR}"
else
    rm -rf "${TRACKDIR}"
fi

echo
echo "========================================================"
echo

read -r -p "Press Enter to continue..." _ || true

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
