#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
if [ ! -f "$sdk/runtime/core/input.h" ]; then
    echo 'Uso: bash tools/test_ads_ak47.sh /caminho/para/XboxXBE-Recomp' >&2
    exit 1
fi
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
# Estes testes usam memória simulada, sem símbolos do jogo. Nunca use este cabeçalho para gerar um APK.
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread \
    -I "$repo/src" -I "$repo/native" -I "$sdk/runtime" -I "$scratch" \
    "$repo/tests/ads_ak47_test.cpp" "$repo/src/game/ads_ak47.cpp" -o "$scratch/ads_ak47_test"
"$scratch/ads_ak47_test"
