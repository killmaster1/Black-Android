#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
if [ "$#" -eq 0 ]; then set -- "$repo/sdk"; fi
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
cat > "$scratch/compat.cpp" <<'CPP'
#include "app/app.h"
#include "common/menu.h"
static_assert(app::kAndroidFpsCapRefresh == 1);
CPP
for sdk in "$@"; do
    test -f "$sdk/runtime/app/app.h" || { echo "SDK ausente: $sdk" >&2; exit 1; }
    "${CXX:-c++}" -std=c++20 -fsyntax-only -I "$repo/src" -I "$sdk/runtime" "$scratch/compat.cpp"
    echo "Cabecalhos Android compativeis com: $sdk"
done
