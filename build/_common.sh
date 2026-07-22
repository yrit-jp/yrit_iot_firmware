#!/bin/bash
# 各 <board>/build.sh から source される共通ビルドロジック。単体では実行しない。

RED='\033[31m'
GREEN='\033[32m'
NC='\033[0m'

ARDUINO_USER_LIBRARIES="${ARDUINO_USER_LIBRARIES:-$HOME/.arduino15/libraries}"

# firmware_version.h からバージョン情報を取得し、V_NAME/PROGRAM_NAME/PROGRAM_DIR を設定する
fetch_firmware_version() {
    echo "Fetch firmware version..."
    local v_main v_minor v_release_tag
    v_main=$(awk '/#define[ \t]+APP_MAIN_VERSION/{print $3}' "$VERSION_FILE")
    v_minor=$(awk '/#define[ \t]+APP_MINOR_VERSION/{print $3}' "$VERSION_FILE")
    v_release_tag=$(awk -F'"' '/#define[ \t]+APP_RELEASE_TAG/{print $2}' "$VERSION_FILE")

    RELEASE_TAG="$v_release_tag"
    if [ -n "$v_release_tag" ]; then
        V_NAME="v${v_main}.${v_minor}_${v_release_tag}"

        # 末尾の数字を削除する処理
        local tag_without_number="$v_release_tag"
        while [[ "$tag_without_number" =~ [0-9]$ ]]; do
            tag_without_number="${tag_without_number%?}"
        done
        echo "$tag_without_number"

        PROGRAM_NAME="IoT_Device_${tag_without_number}"
        PROGRAM_DIR="IoT_Device_${tag_without_number}"
        echo "$PROGRAM_DIR"
    else
        V_NAME="v${v_main}.${v_minor}"
        PROGRAM_NAME="IoT_Device"
        PROGRAM_DIR="IoT_Device"
    fi

    OUTPUT_DIR="../__output/${TARGET}/${PROGRAM_DIR}/${V_NAME}"
    if [ -n "${SUB_TARGET:-}" ]; then
        SUB_OUTPUT_DIR="../__output/${SUB_TARGET}/${PROGRAM_DIR}/${V_NAME}"
    fi

    echo "==> ${V_NAME}"
}

# arduino-cli が出力するスケッチ名 (yrit_iot_firmware.ino*) を PROGRAM_NAME.ino* へ一括リネームする
rename_sketch_outputs() {
    local dir="$1"
    local prefix base
    for prefix in "yrit_iot_firmware.ino" "_yrit_iot_firmware.ino"; do
        for f in "${dir}/${prefix}"*; do
            [ -e "$f" ] || continue
            base=$(basename "$f")
            mv "$f" "${dir}/${PROGRAM_NAME}.ino${base#"$prefix"}"
        done
    done
}

# TARGET / VERSION_FILE / PROJECT_ROOT / FQBN / LIB_DIR を使ってビルドを実行する
# SUB_TARGET / SKIP_BOOT_APP0 は必要な場合のみ呼び出し側で設定する
# 呼び出し元 (<board>/build.sh) は build/<board>/ に置かれている前提で、
# __output/__cache/boot_app0.bin/CHANGELOG*.md は build/ 直下を共有する
run_build() {
    local out_dir build_status

    fetch_firmware_version
    out_dir="../__output/${TARGET}/${PROGRAM_DIR}"

    echo "Clean previous output..."
    echo "${out_dir}"
    rm -rf "${out_dir}/${V_NAME}"
    if [ -n "${SUB_TARGET:-}" ]; then
        local sub_out_dir="../__output/${SUB_TARGET}/${PROGRAM_DIR}"
        echo "${sub_out_dir}"
        rm -rf "${sub_out_dir}/${V_NAME}"
    fi
    mkdir -p "${out_dir}/${V_NAME}"

    if arduino-cli compile \
        --fqbn="${FQBN}" \
        --libraries "${ARDUINO_USER_LIBRARIES}" \
        --libraries "${PROJECT_ROOT}libraries/common" \
        --libraries "${PROJECT_ROOT}libraries/${LIB_DIR}" \
        --output-dir "${out_dir}/${V_NAME}" \
        --build-path "../__cache/${TARGET}" \
        --verbose \
        "${PROJECT_ROOT}"; then

        rename_sketch_outputs "${out_dir}/${V_NAME}"

        if [ -n "$RELEASE_TAG" ]; then
            cp "../../CHANGELOG_BETA.md" "${out_dir}/CHANGELOG.md"
        else
            cp "../../CHANGELOG.md" "${out_dir}/"
        fi

        if [ -z "${SKIP_BOOT_APP0:-}" ]; then
            cp "../boot_app0.bin" "${out_dir}/${V_NAME}/"
        fi

        if [ -n "${SUB_TARGET:-}" ]; then
            mkdir -p "../output/${SUB_TARGET}"
            cp -R "../output/${TARGET}/." "../output/${SUB_TARGET}/"
        fi

        build_status=SUCCESS
    else
        echo
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}  BUILD FAILED!${NC}"
        echo -e "${RED}  arduino-cli compile error occurred.${NC}"
        echo -e "${RED}========================================${NC}"
        echo
        build_status=FAILED
    fi

    echo
    echo "Finish. Target: ${TARGET}"
    if [ -n "${SUB_TARGET:-}" ]; then
        echo "Finish. Target: ${SUB_TARGET}"
    fi
    if [ "$build_status" = SUCCESS ]; then
        echo -e "${GREEN}Build Status: SUCCESS${NC}"
    else
        echo -e "${RED}Build Status: FAILED${NC}"
    fi
    echo

    # build_all.sh から標準入力を /dev/null にリダイレクトして呼ばれた場合は自動的にスキップされる
    read -r -p "Press Enter to continue..." _ || true

    [ "$build_status" = SUCCESS ]
}
