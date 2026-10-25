#!/usr/bin/env python3
"""Verifica ida e volta e rejeições do importador usando amostras locais."""
import contextlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import black_obj as conversor


def rejeitar(funcao):
    try:
        funcao()
    except ValueError:
        return
    raise AssertionError('Entrada inválida foi aceita')


def main():
    arquivos = sorted(Path(sys.argv[1]).rglob('bg1_*.db'))
    assert arquivos, 'Nenhuma amostra bg1 encontrada'
    with tempfile.TemporaryDirectory() as pasta:
        pasta = Path(pasta)
        for numero, origem in enumerate(arquivos):
            obj = pasta / f'{numero}.obj'
            mapa = Path(str(obj)+'.json')
            saida = pasta / f'{numero}.db'
            with contextlib.redirect_stdout(io.StringIO()):
                conversor.exportar(origem, obj)
                conversor.importar(origem, obj, mapa, saida)
            assert saida.read_bytes() == origem.read_bytes(), origem
            original_obj = obj.read_text()
            linhas = original_obj.splitlines()
            k = next(k for k, l in enumerate(linhas) if l.startswith('v '))
            linhas[k] = 'v 0.1 0.2 0.3'
            obj.write_text('\n'.join(linhas)+'\n')
            editado = pasta / f'{numero}-editado.db'
            with contextlib.redirect_stdout(io.StringIO()):
                conversor.importar(origem, obj, mapa, editado)
            meta = json.loads(mapa.read_text())
            inicio = meta['malhas'][0]['inicio']
            assert editado.read_bytes()[inicio:inicio+12] == struct.pack('<3f', .1, .2, .3)
            assert editado.stat().st_size == origem.stat().st_size
            obj.write_text(original_obj+'v 0 0 0\n')
            rejeitar(lambda: conversor.importar(origem, obj, mapa, pasta/'invalido.db'))
            obj.write_text(original_obj)
            meta['malhas'][0]['inicio'] += 4
            mapa.write_text(json.dumps(meta))
            rejeitar(lambda: conversor.importar(origem, obj, mapa, pasta/'invalido.db'))
            print(f'{origem.name}: ida e volta idêntica, edição e rejeições OK')
    print(f'{len(arquivos)} amostras verificadas')


if __name__ == '__main__':
    main()
