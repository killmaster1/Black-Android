#!/usr/bin/env python3
"""Testes do protótipo V2 usando uma amostra de pistola local, não distribuída."""
from pathlib import Path
import contextlib
import io
import json
import struct
import sys
import tempfile
import black_obj
import black_v2


def cubo(dados, malha):
    m = black_obj.detectar(dados)[malha]
    vs = [struct.unpack_from('<3f', dados, m['inicio']+i*m['passo']) for i in range(m['nvertices'])]
    centro = [(min(v[e] for v in vs)+max(v[e] for v in vs))/2 for e in range(3)]
    raio = min(max(v[e] for v in vs)-min(v[e] for v in vs) for e in range(3))/8
    assert raio > 0
    linhas = ['# Cubo simples ajustado aos limites da malha original', 'o cubo']
    for x, y, z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]:
        linhas.append('v '+' '.join(f'{centro[e]+raio*v:.9g}' for e,v in enumerate((x,y,z))))
        linhas.append('vt 0 0')
    for face in [(1,3,2),(1,4,3),(5,6,7),(5,7,8),(1,2,6),(1,6,5),(4,8,7),(4,7,3),(1,5,8),(1,8,4),(2,3,7),(2,7,6)]:
        linhas.append('f '+' '.join(f'{i}/{i}' for i in face))
    return '\n'.join(linhas)+'\n'


def rejeitar(funcao, path):
    try:
        funcao()
    except ValueError:
        assert not path.exists()
        return
    raise AssertionError('Entrada incompatível aceita')


def testar(original):
    dados = original.read_bytes()
    malhas = black_obj.detectar(dados)
    assert malhas[0]['passo'] == 24
    with tempfile.TemporaryDirectory() as pasta:
        p = Path(pasta)
        obj, saida = p/'cubo.obj', p/'cubo.db'
        obj.write_text(cubo(dados, 0))
        resultado = black_v2.substituir(original, obj, saida, 0)
        assert resultado['vertices_usados'] == 8 and resultado['triangulos'] == 12
        novos = black_obj.detectar(saida.read_bytes())
        assert len(novos[0]['faces']) == 12 and novos[1:] == malhas[1:]
        m = malhas[0]
        ib = m['limites']+black_obj.u32(dados, m['limites']+40)
        permitidos = (set(range(m['inicio'],m['inicio']+8*24)) |
                      set(range(ib,ib+resultado['indices_usados']*2)) |
                      set(range(m['limites'],m['limites']+12)) |
                      set(range(m['limites']+16,m['limites']+28)) |
                      set(range(m['limites']+36,m['limites']+40)))
        assert all(i in permitidos for i,(a,b) in enumerate(zip(dados,saida.read_bytes())) if a != b)
        erro = p/'rejeitado.db'
        rejeitar(lambda: black_v2.substituir(original,obj,erro,1),erro)
        linhas = cubo(dados,0).splitlines()
        primeiro = next(i for i,l in enumerate(linhas) if l.startswith('v '))
        linhas[primeiro] = 'v 999 999 999'
        obj.write_text('\n'.join(linhas)+'\n')
        rejeitar(lambda: black_v2.substituir(original,obj,erro,0),erro)
        texto = cubo(dados,0)
        obj.write_text(texto+('\n'.join(l for l in texto.splitlines() if l.startswith('f '))+'\n')*100)
        rejeitar(lambda: black_v2.substituir(original,obj,erro,0),erro)
        obj.write_text(cubo(dados,0))
        # V1 permanece intacta: ida e volta sem edição, todos os bytes iguais.
        with contextlib.redirect_stdout(io.StringIO()):
            black_obj.exportar(original,p/'v1.obj')
            black_obj.importar(original,p/'v1.obj',p/'v1.obj.json',p/'v1.db')
        assert (p/'v1.db').read_bytes() == dados
    print('V2: cubo, orientação das faces, capacidade, rejeição de skinning, isolamento e regressão V1: OK')


if __name__ == '__main__':
    testar(Path(sys.argv[1]))
