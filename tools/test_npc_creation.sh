#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
test -f "$sdk/runtime/xdk/hle.h" || { echo "SDK ausente: $sdk" >&2; exit 1; }
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread -I "$repo/src" -I "$sdk/runtime" -I "$scratch" \
    "$repo/tests/npc_creation_test.cpp" "$repo/src/game/npc_creation.cpp" -o "$scratch/npc_creation_test"
"$scratch/npc_creation_test"
echo "NPCs: passaram leitura sem escrita, argumentos, retornos, perfis, aninhamento e mudanca de mundo."
