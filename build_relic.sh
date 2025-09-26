#!/bin/bash
# RELIC 빌드 스크립트
# Usage: ./build_relic.sh

set -e  # 에러 발생 시 스크립트 중단

# 현재 스크립트가 volepsi 디렉토리에서 실행되는지 확인
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAKEFILE_PATH="$SCRIPT_DIR/thirdparty/build_relic.mk"

echo "=== RELIC Build Script ==="
echo "Working directory: $SCRIPT_DIR"
echo "Makefile path: $MAKEFILE_PATH"

# Makefile 존재 확인
if [ ! -f "$MAKEFILE_PATH" ]; then
    echo "Error: build_relic.mk not found at $MAKEFILE_PATH"
    exit 1
fi

# Makefile 실행
echo "Executing Makefile..."
make -f "$MAKEFILE_PATH"

echo "=== RELIC Build Completed ==="