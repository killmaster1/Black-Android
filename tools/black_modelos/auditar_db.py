#!/usr/bin/env python3
"""Compara o original e uma edição V1 sem executar os scripts recebidos."""
import argparse
import json
from pathlib import Path
import struct
import black_obj


def auditar(original, modificado):
    a, b = Path(original).read_bytes(), Path(modificado).read_bytes()
    if len(a) != len(b):
        raise ValueError('Tamanhos diferentes: esta auditoria cobre somente edições sem realocação')
    diferentes = {i for i, (x, y) in enumerate(zip(a, b)) if x != y}
    classificados = set()
    malhas = []
    for numero, m in enumerate(black_obj.detectar(a)):
        posicoes = {m['inicio']+i*m['passo']+j for i in range(m['nvertices']) for j in range(12)}
        caixas = set(range(m['limites'], m['limites']+12)) | set(range(m['limites']+16, m['limites']+28))
        classificados |= posicoes | caixas
        indices_inicio = m['limites'] + black_obj.u32(a, m['limites']+40)
        malhas.append(dict(numero=numero, raiz=hex(m['raiz']), vertices_inicio=hex(m['inicio']),
                           limites_inicio=hex(m['limites']), indices_inicio=hex(indices_inicio),
                           passo=m['passo'], vertices=m['nvertices'], indices=m['nindices'],
                           bytes_xyz_alterados=len(diferentes & posicoes),
                           bytes_caixas_alterados=len(diferentes & caixas),
                           vertices_alterados=sum(a[m['inicio']+i*m['passo']:m['inicio']+i*m['passo']+12] !=
                                                 b[m['inicio']+i*m['passo']:m['inicio']+i*m['passo']+12]
                                                 for i in range(m['nvertices']))))
    fora = sorted(diferentes-classificados)
    return dict(original_sha256=black_obj.sha(a), modificado_sha256=black_obj.sha(b), tamanho=len(a),
                bytes_alterados=len(diferentes), bytes_fora_xyz_caixas=len(fora),
                primeiros_offsets_fora=[hex(x) for x in fora[:32]], malhas=malhas)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('original', type=Path)
    p.add_argument('modificado', type=Path)
    args = p.parse_args()
    print(json.dumps(auditar(args.original, args.modificado), indent=2, ensure_ascii=False))
