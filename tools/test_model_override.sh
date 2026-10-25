#!/usr/bin/env bash
set -euo pipefail
raiz="$(cd -- "$(dirname -- "$0")/.." && pwd)"
sdk="$(realpath -- "$1")"
temporario="$(mktemp -d)"
trap 'rm -rf -- "$temporario"' EXIT
g++ -std=c++20 -I "$raiz/native" -I "$sdk/runtime" \
 "$raiz/native/model_override.cpp" "$raiz/tools/test_model_override.cpp" -pthread -o "$temporario/testar"
"$temporario/testar" "$temporario/Black"
