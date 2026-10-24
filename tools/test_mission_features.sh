#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
sdk=${1:-"$repo/sdk"}
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
printf '#pragma once\n' > "$scratch/image_symbols.h"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread -I "$repo/src" "$repo/tests/mission_lifecycle_test.cpp" -o "$scratch/lifecycle"
"$scratch/lifecycle"
"${CXX:-c++}" -std=c++20 -Wall -Wextra -pthread -I "$repo/src" -I "$sdk/runtime" -I "$scratch" \
    "$repo/tests/extreme_test.cpp" "$repo/src/game/extreme.cpp" -o "$scratch/extreme"
"$scratch/extreme"
echo "Missao: passaram inicio, morte, reinicio, multiplicadores, restauracao e dano sem hitkill artificial."
