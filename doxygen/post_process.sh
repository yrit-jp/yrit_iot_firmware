#!/bin/sh
# post-process.sh - Doxygen実行後の後処理スクリプト

echo "Post-processing: Copying assets to HTML output..."

# assetsディレクトリが存在する場合、html出力にコピー
if [ -d "/doxygen/assets" ]; then
  if [ -d "/doxygen/output/html" ]; then
    cp -r /doxygen/assets /doxygen/output/html/
    echo "Assets copied to /doxygen/html/assets"
  else
    echo "Warning: HTML output directory not found"
  fi
else
  echo "Warning: assets directory not found"
fi

echo "Post-processing completed"
