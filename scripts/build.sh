#!/usr/bin/env bash
set -euo pipefail

if [[ "${VERBOSE:-0}" == "1" ]]; then
  set -x
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/../build"

BUILD_TYPE="Debug"
DO_CLEAN=0

for arg in "$@"; do
  case "$arg" in
    clean)
      DO_CLEAN=1
      ;;
    debug)
      BUILD_TYPE="Debug"
      ;;
    release)
      BUILD_TYPE="Release"
      ;;
    relwithdebinfo)
      BUILD_TYPE="RelWithDebInfo"
      ;;
    minsizerel)
      BUILD_TYPE="MinSizeRel"
      ;;
  esac
done

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
fi

if ! command -v timeout >/dev/null 2>&1; then
  echo "timeout command not found; install coreutils to enable configure timeout." >&2
  exit 1
fi

clean_build() {
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
}

if [[ "$DO_CLEAN" -eq 1 ]]; then
  clean_build
fi

CMAKE_ARGS=(
  -S "$ROOT_DIR"
  -B "$BUILD_DIR"
  -G "$GENERATOR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

if [[ "${USE_SYSTEM_GLFW:-0}" == "1" ]]; then
  CMAKE_ARGS+=(-DUSE_SYSTEM_GLFW=ON)
fi

if timeout 120s cmake "${CMAKE_ARGS[@]}"; then
  :
else
  rc=$?
  if [[ "$rc" -eq 124 ]]; then
    echo "Likely FetchContent download stalled or dependency check hanging" >&2
    echo "CMake logs: $BUILD_DIR/CMakeFiles/CMakeOutput.log" >&2
    echo "CMake logs: $BUILD_DIR/CMakeFiles/CMakeError.log" >&2
  fi
  echo "CMake configure failed. Cleaning build directory." >&2
  clean_build
  exit "$rc"
fi

cmake --build "$BUILD_DIR"
