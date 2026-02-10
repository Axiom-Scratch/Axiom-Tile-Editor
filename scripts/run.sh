#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CONFIG="debug"
case "${1:-}" in
  release)
    CONFIG="release"
    ;;
  debug|"")
    CONFIG="debug"
    ;;
esac

BUILD_DIR="$ROOT/build/$CONFIG"
BIN_PATH="$BUILD_DIR/tile_editor"

if [[ ! -x "$BIN_PATH" ]]; then
  echo "Build first: ./scripts/build.sh <debug|release>"
  exit 1
fi

exec "$BIN_PATH"
