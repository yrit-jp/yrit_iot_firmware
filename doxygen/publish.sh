#!/bin/sh
cd /doxygen
#rm -r -f /doxygen/documents/html
#rm -r -f /doxygen/documents/latex
(
# ファイルが存在すればそれを表示、なければデフォルトパスを表示
if [ -f "$DOXYFILE_PATH" ]; then
  cat "$DOXYFILE_PATH"
else
  echo "Warning: $DOXYFILE_PATH not found, using default" >&2
  cat /Doxyfile
fi
for x in "$@"
do
 echo $x
done
) | doxygen -


# 後処理スクリプトの実行（環境変数で指定された場合のみ）
if [ -n "$POST_PROCESS_SCRIPT" ]; then
  if [ -f "$POST_PROCESS_SCRIPT" ]; then
    echo "Running post-process script: $POST_PROCESS_SCRIPT"
    chmod +x "$POST_PROCESS_SCRIPT"
    "$POST_PROCESS_SCRIPT"
  else
    echo "Warning: POST_PROCESS_SCRIPT specified but file not found: $POST_PROCESS_SCRIPT" >&2
  fi
fi