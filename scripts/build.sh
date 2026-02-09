#!/usr/bin/env bash
set -euo pipefail

if [[ "${VERBOSE:-0}" == "1" ]]; then
  set -x
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CONFIG="debug"
BUILD_TYPE="Debug"
DO_CLEAN=0

for arg in "$@"; do
  case "$arg" in
    clean)
      DO_CLEAN=1
      ;;
    release)
      CONFIG="release"
      BUILD_TYPE="Release"
      ;;
    debug)
      CONFIG="debug"
      BUILD_TYPE="Debug"
      ;;
  esac
done

BUILD_DIR="$ROOT_DIR/build/$CONFIG"

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
fi

if ! command -v timeout >/dev/null 2>&1; then
  echo "timeout command not found; install coreutils to enable configure timeout." >&2
  exit 1
fi

if [[ "$DO_CLEAN" -eq 1 ]]; then
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

CMAKE_ARGS=(
  -S "$ROOT_DIR"
  -B "$BUILD_DIR"
  -G "$GENERATOR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

if [[ "${USE_SYSTEM_GLFW:-0}" == "1" ]]; then
  CMAKE_ARGS+=(-DUSE_SYSTEM_GLFW=ON)
fi

echo "Configuring ($CONFIG) in $BUILD_DIR..."
if ! timeout 120s cmake "${CMAKE_ARGS[@]}"; then
  rc=$?
  if [[ "$rc" -eq 124 ]]; then
    echo "Configure timed out. Likely FetchContent download or dependency probe." >&2
    echo "Next steps:" >&2
    echo "  - offline mode: init submodules (git submodule update --init --recursive)" >&2
    echo "  - or rerun with USE_SYSTEM_GLFW=1" >&2
    echo "  - check logs: $BUILD_DIR/CMakeFiles/CMakeOutput.log and CMakeError.log" >&2
    echo "  - run with VERBOSE=1 to print commands" >&2
  fi
  exit "$rc"
fi
echo "Configure complete."

cmake --build "$BUILD_DIR"
