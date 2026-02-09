#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CONFIG="debug"
case "${1:-}" in
  release)
    CONFIG="release"
    ;;
  debug|"")
    CONFIG="debug"
    ;;
esac

BUILD_DIR="$ROOT_DIR/build/$CONFIG"
BIN_PATH="$BUILD_DIR/tile_editor"

if [[ ! -x "$BIN_PATH" ]]; then
  echo "Build first: ./scripts/build.sh <debug|release>"
  exit 1
fi

cd "$ROOT_DIR"
exec "$BIN_PATH"
