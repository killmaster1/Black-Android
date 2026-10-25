#!/usr/bin/env python3
"""Regressões do diagnóstico somente leitura, sem depender da instalação Android."""
from pathlib import Path
import sys
import tempfile
import black_obj
import diagnostico_geometria as d


def main(original,banana):
    dados,editado=original.read_bytes(),banana.read_bytes()
    hashes=[black_obj.sha(dados),black_obj.sha(editado)]
    info=d.inventario(dados)
    assert [m['vertices_referenciados'] for m in info['malhas']]==[322,3687,1465]
    par=info['pares'][-1]
    assert par['registros_identicos'] and par['bytes_sobrepostos']==0 and par['indices_comuns']==0
    auditoria=d.comparar(dados,editado)
    assert auditoria['bytes_diferentes']==77857 and auditoria['bytes_nao_classificados']==0
    assert [m['vertices_xyz_alterados'] for m in auditoria['malhas']]==[322,5146,5146]
    extra=bytearray(dados);extra[0]^=1
    assert d.comparar(dados,extra)['bytes_nao_classificados']==1
    assert not d.comparar(dados,dados[:-1])['tamanho_igual']
    with tempfile.TemporaryDirectory() as pasta:
        saida=Path(pasta)/'partes';d.exportar(original,saida)
        for i,n in enumerate([322,3687,1465]):
            v,uv,faces=black_obj.ler_obj(saida/f'malha_{i}.obj')
            assert len(v)==len(uv)==n
            assert len(faces)==info['malhas'][i]['triangulos']
            assert all(0<=j<n for f in faces for j in f)
        try:d.exportar(original,saida);raise AssertionError('Sobrescrita aceita')
        except FileExistsError:pass
    assert hashes==[black_obj.sha(original.read_bytes()),black_obj.sha(banana.read_bytes())]
    print('Diagnóstico: auditoria, partes disjuntas, OBJ compacto, diferenças externas e originais preservados: OK')


if __name__=='__main__':main(Path(sys.argv[1]),Path(sys.argv[2]))
