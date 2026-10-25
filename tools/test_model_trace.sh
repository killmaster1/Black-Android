#!/usr/bin/env bash
set -euo pipefail
raiz="$(cd -- "$(dirname -- "$0")/.." && pwd)"
sdk="$(realpath -- "$1")"
amostra="$(realpath -- "$2")"
temporario="$(mktemp -d)"
trap 'rm -rf -- "$temporario"' EXIT
g++ -std=c++20 -DBLACK_TRACE_MODELS=1 -I "$raiz/native" -I "$sdk/runtime" \
 "$raiz/native/model_trace.cpp" "$raiz/tools/test_model_trace.cpp" -pthread -o "$temporario/testar"
"$temporario/testar" "$amostra"
