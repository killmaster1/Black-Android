#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
test -f "$sdk/runtime/xdk/hle.h" || { echo "SDK ausente: $sdk" >&2; exit 1; }
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread -I "$repo/src" -I "$sdk/runtime" -I "$scratch" \
    "$repo/tests/npc_spawn_test.cpp" "$repo/src/game/npc_spawn.cpp" -o "$scratch/npc_spawn_test"
"$scratch/npc_spawn_test"
echo "Spawn: passaram chamada unica, parametros, contexto, reentrada, cancelamento, limite e troca de mundo."
