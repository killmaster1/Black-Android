#!/usr/bin/env bash
set -euo pipefail
raiz="$(cd -- "$(dirname -- "$0")/.." && pwd)"
sdk="$(realpath -- "$1")"
amostra="$(realpath -- "$2")"
temporario="$(mktemp -d)"
trap 'rm -rf -- "$temporario"' EXIT
g++ -std=c++20 -ffp-contract=off -I "$raiz/native" -I "$sdk/runtime" \
 "$raiz/native/model_override.cpp" "$raiz/native/model_obj.cpp" "$sdk/runtime/kernel/crypto.cpp" "$raiz/tools/test_model_override.cpp" -pthread -o "$temporario/testar"
"$temporario/testar" "$temporario/Black" "$amostra" "$raiz/tools/black_modelos/exemplos/cubo_v3.obj"
python3 - "$temporario/Black/cache" <<'PY'
from pathlib import Path
import json,sys
p=list(Path(sys.argv[1]).glob("*.json"));assert p
for f in p:
 r=json.loads(f.read_text());assert r["vertices"]==24 and r["triangulos"]==12 and r["indices"]==58
print("Relatório JSON do cache: válido")
PY
