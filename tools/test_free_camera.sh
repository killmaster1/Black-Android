#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
if [ ! -f "$sdk/runtime/core/input.h" ]; then
    echo 'Usage: bash tools/test_free_camera.sh /path/to/XboxXBE-Recomp' >&2
    exit 1
fi
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
# These host tests use simulated memory, no game symbols. Never use this header for an APK build.
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread \
    -I "$repo/src" -I "$repo/native" -I "$sdk/runtime" -I "$scratch" \
    "$repo/tests/free_camera_test.cpp" "$repo/src/game/free_camera.cpp" -o "$scratch/free_camera_test"
"$scratch/free_camera_test"
