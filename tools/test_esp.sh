#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
test -f "$sdk/runtime/xdk/hle.h" || { echo "SDK ausente: $sdk" >&2; exit 1; }
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread -I "$repo/src" \
    "$repo/tests/esp_diagnostic_test.cpp" -o "$scratch/contador"
"$scratch/contador"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread -I "$repo/src" -I "$sdk/runtime" -I "$scratch" \
    "$repo/tests/esp_hooks_test.cpp" "$repo/src/game/esp_diagnostic.cpp" -o "$scratch/hooks"
"$scratch/hooks"
echo "ESP: passaram contadores, pausa, expiracao, projecao, copias e hooks sem escrita."
