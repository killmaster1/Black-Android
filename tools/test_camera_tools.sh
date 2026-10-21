#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
imgui=${2:-"$repo/out/check-deps/imgui"}
sdl=${3:-"$repo/third_party/SDL"}
for required in "$sdk/runtime/core/input.h" "$imgui/imgui.cpp" "$sdl/include/SDL3/SDL.h"; do
    test -f "$required" || { echo "Dependencia ausente: $required" >&2; exit 1; }
done
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread \
    -I "$repo/src" -I "$repo/native" -I "$sdk/runtime" -I "$scratch" -I "$imgui" -I "$sdl/include" \
    "$repo/tests/camera_tools_test.cpp" "$repo/native/camera_tools.cpp" \
    "$repo/src/game/camera_adjust.cpp" "$repo/src/game/free_camera.cpp" "$repo/src/game/ads_ak47.cpp" \
    "$imgui/imgui.cpp" "$imgui/imgui_draw.cpp" "$imgui/imgui_tables.cpp" "$imgui/imgui_widgets.cpp" \
    -o "$scratch/camera_tools_test"
"$scratch/camera_tools_test"
