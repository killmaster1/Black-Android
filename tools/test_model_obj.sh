#!/usr/bin/env bash
set -euo pipefail
raiz="$(cd -- "$(dirname -- "$0")/.." && pwd)"
sdk="$(realpath -- "$1")"
amostra="$(realpath -- "$2")"
temporario="$(mktemp -d)"
trap 'rm -rf -- "$temporario"' EXIT
g++ -std=c++20 -O2 -ffp-contract=off -I "$raiz/native" -I "$sdk/runtime" \
 "$raiz/native/model_obj.cpp" "$sdk/runtime/kernel/crypto.cpp" "$raiz/tools/test_model_obj.cpp" -o "$temporario/testar"
"$temporario/testar" "$amostra" "$raiz/tools/black_modelos/exemplos/cubo_v3.obj" "$temporario/nativo.db"
python3 - "$raiz" "$amostra" "$temporario" <<'PY'
from pathlib import Path
import sys
raiz,original,pasta=map(Path,sys.argv[1:])
sys.path.insert(0,str(raiz/'tools/black_modelos'))
import black_obj_v3
black_obj_v3.substituir(original,raiz/'tools/black_modelos/exemplos/cubo_v3.obj',pasta/'python.db',pasta/'python.json',True)
assert (pasta/'nativo.db').read_bytes()==(pasta/'python.db').read_bytes()
print('Conversor nativo e V3 Python: cubo gera DB idêntico byte por byte.')
PY
