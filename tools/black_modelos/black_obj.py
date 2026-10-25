#!/usr/bin/env python3
"""Exportação experimental dos bg1 Xbox e edição de posições sem mudar a malha."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys


def sha(data):
    return hashlib.sha256(data).hexdigest()


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def detectar(data):
    """Reconhece somente o layout observado nas 16 amostras bg1 fornecidas."""
    malhas = []
    for raiz in range(0, len(data) - 128, 16):
        if data[raiz + 44:raiz + 56] != struct.pack('<3f', 8, 16, 24):
            continue
        if u32(data, raiz + 24) != 35:
            continue
        grupo = raiz + u32(data, raiz + 28)
        quantidade = u32(data, raiz + 80)
        if not 1 <= quantidade <= 32:
            continue
        candidatas = []
        try:
            for j in range(quantidade):
                descritor = grupo + j * 28
                inicio = descritor + u32(data, descritor + 4)
                limites = descritor + u32(data, descritor + 12)
                passo = u32(data, descritor + 20)
                primitiva, nindices, relativo, _ = struct.unpack_from('<4I', data, limites + 32)
                indices_inicio = limites + relativo
                if passo not in (24, 36) or primitiva != 6 or not 3 <= nindices <= 1000000:
                    raise ValueError('Layout não suportado')
                if not raiz < inicio < indices_inicio or indices_inicio + nindices * 2 > len(data):
                    raise ValueError('Intervalo inválido')
                nvertices, padding = divmod(indices_inicio - inicio, passo)
                if padding >= 16 or not 3 <= nvertices <= 65536:
                    raise ValueError('Vértices inválidos')
                indices = struct.unpack_from(f'<{nindices}H', data, indices_inicio)
                if max(indices) >= nvertices:
                    raise ValueError('Índice fora da malha')
                vertices = [struct.unpack_from('<3f', data, inicio + k * passo) for k in range(nvertices)]
                uv = [struct.unpack_from('<2f', data, inicio + k * passo + passo - 8) for k in range(nvertices)]
                if not all(math.isfinite(v) and abs(v) < 1e6 for linha in vertices + uv for v in linha):
                    raise ValueError('Coordenadas inválidas')
                # Xbox D3DPT_TRIANGLESTRIP: troca a orientação em passos ímpares.
                faces = []
                for k in range(2, len(indices)):
                    a, b, c = indices[k-2:k+1]
                    if k % 2:
                        a, b = b, a
                    if len({a, b, c}) == 3:
                        faces.append([a, b, c])
                candidatas.append(dict(raiz=raiz, inicio=inicio, passo=passo, limites=limites,
                                       nvertices=nvertices, nindices=nindices, faces=faces))
            malhas.extend(candidatas)
        except (struct.error, ValueError):
            continue
    return malhas


def gravar_novo(path, data):
    # Nunca substitui o original nem uma saída existente.
    with Path(path).open('xb') as arquivo:
        arquivo.write(data)


def exportar(origem, destino):
    dados = origem.read_bytes()
    malhas = detectar(dados)
    if not malhas:
        raise ValueError('Nenhuma malha bg1 compatível reconhecida. O .wdd não foi decodificado.')
    mapa = Path(str(destino) + '.json')
    if destino.exists() or mapa.exists():
        raise ValueError('Saída já existe; escolha outro nome.')
    linhas = ['# Sandro Mods — BLACK Xbox; posições editáveis, topologia fixa',
              '# Manter ordem dos vértices, UVs e faces. Sem materiais/texturas/normais.']
    deslocamento = 1
    for numero, malha in enumerate(malhas):
        linhas.append(f'o malha_{numero:02d}')
        for i in range(malha['nvertices']):
            offset = malha['inicio'] + i * malha['passo']
            xyz = struct.unpack_from('<3f', dados, offset)
            linhas.append('v ' + ' '.join(format(x, '.9g') for x in xyz))
        for i in range(malha['nvertices']):
            offset = malha['inicio'] + i * malha['passo'] + malha['passo'] - 8
            linhas.append('vt ' + ' '.join(format(x, '.9g') for x in struct.unpack_from('<2f', dados, offset)))
        for face in malha['faces']:
            linhas.append('f ' + ' '.join(f'{x+deslocamento}/{x+deslocamento}' for x in face))
        deslocamento += malha['nvertices']
    gravar_novo(destino, ('\n'.join(linhas) + '\n').encode())
    gravar_novo(mapa, (json.dumps(dict(versao=1, original_sha256=sha(dados), malhas=malhas), indent=2)+'\n').encode())
    print(f'{len(malhas)} malhas exportadas: {destino}; mapa: {mapa}')


def ler_obj(path):
    vertices, uv, faces = [], [], []
    for n, linha in enumerate(path.read_text().splitlines(), 1):
        campos = linha.split('#', 1)[0].split()
        if not campos:
            continue
        tipo, *valores = campos
        if tipo in ('v', 'vt'):
            tamanho = 3 if tipo == 'v' else 2
            if len(valores) != tamanho:
                raise ValueError(f'Linha {n}: coordenadas incompatíveis')
            numeros = tuple(float(x) for x in valores)
            if not all(math.isfinite(x) and abs(x) < 1e6 for x in numeros):
                raise ValueError(f'Linha {n}: coordenadas inválidas')
            (vertices if tipo == 'v' else uv).append(numeros)
        elif tipo == 'f':
            if len(valores) != 3:
                raise ValueError('Somente as faces triangulares originais são aceitas')
            face = []
            for valor in valores:
                partes = valor.split('/')
                if len(partes) != 2 or partes[0] != partes[1] or int(partes[0]) <= 0:
                    raise ValueError('Preserve os índices de vértices e UVs do OBJ exportado')
                face.append(int(partes[0])-1)
            faces.append(face)
        elif tipo not in ('o', 'g', 's', 'mtllib', 'usemtl'):
            raise ValueError(f'Linha {n}: elemento {tipo} não suportado; não exporte normais')
    return vertices, uv, faces


def importar(origem, obj, mapa, destino):
    dados = origem.read_bytes()
    meta = json.loads(mapa.read_text())
    if meta.get('versao') != 1 or sha(dados) != meta.get('original_sha256'):
        raise ValueError('O arquivo original não corresponde ao mapa desta exportação')
    malhas = detectar(dados)
    if malhas != meta.get('malhas') or not malhas:
        raise ValueError('Mapa alterado ou layout incompatível')
    vertices, uv, faces = ler_obj(obj)
    total = sum(m['nvertices'] for m in malhas)
    esperadas, base = [], 0
    for m in malhas:
        esperadas.extend([[x+base for x in f] for f in m['faces']])
        base += m['nvertices']
    if len(vertices) != total or len(uv) != total or faces != esperadas:
        raise ValueError('Topologia alterada: mantenha quantidade, ordem, faces e UVs')
    saida = bytearray(dados)
    base, alterados = 0, 0
    for m in malhas:
        mudou = False
        for i in range(m['nvertices']):
            offset = m['inicio'] + i * m['passo']
            uv_original = dados[offset + m['passo']-8:offset + m['passo']]
            if struct.pack('<2f', *uv[base+i]) != uv_original:
                raise ValueError('Edição de UVs não suportada nesta versão')
            novo = struct.pack('<3f', *vertices[base+i])
            if novo != dados[offset:offset+12]:
                mudou = True
                alterados += 1
                saida[offset:offset+12] = novo
        if mudou:
            pontos = vertices[base:base+m['nvertices']]
            # Atualiza somente as duas caixas locais reconhecidas. Mantém os demais metadados.
            struct.pack_into('<3f', saida, m['limites'], *(max(p[e] for p in pontos) for e in range(3)))
            struct.pack_into('<3f', saida, m['limites']+16, *(min(p[e] for p in pontos) for e in range(3)))
        base += m['nvertices']
    gravar_novo(destino, saida)
    print(f'{destino}: {alterados} vértices alterados; tamanho preservado ({len(saida)} bytes)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='acao', required=True)
    analisar = sub.add_parser('analisar', help='Lista malhas compatíveis, sem modificar arquivos')
    analisar.add_argument('arquivos', nargs='+', type=Path)
    exp = sub.add_parser('exportar', help='DB → OBJ e mapa JSON')
    exp.add_argument('original', type=Path)
    exp.add_argument('obj', type=Path)
    imp = sub.add_parser('importar', help='Reinsere posições usando DB original + OBJ + mapa')
    for nome in ('original', 'obj', 'mapa', 'saida'):
        imp.add_argument(nome, type=Path)
    novo = sub.add_parser('substituir', help='V2 experimental: topologia nova em malha rígida, sem realocar')
    for nome in ('original', 'obj', 'saida'):
        novo.add_argument(nome, type=Path)
    novo.add_argument('--malha', required=True, type=int, help='Índice da malha, começando em zero')
    args = parser.parse_args()
    try:
        if args.acao == 'analisar':
            for arquivo in args.arquivos:
                print(json.dumps(dict(arquivo=str(arquivo), tamanho=arquivo.stat().st_size,
                                      malhas=[{k:v for k,v in m.items() if k != 'faces'}
                                              for m in detectar(arquivo.read_bytes())]), ensure_ascii=False))
        elif args.acao == 'exportar':
            exportar(args.original, args.obj)
        elif args.acao == 'substituir':
            from black_v2 import substituir
            print(json.dumps(substituir(args.original, args.obj, args.saida, args.malha), ensure_ascii=False))
        else:
            importar(args.original, args.obj, args.mapa, args.saida)
    except (OSError, ValueError, KeyError, struct.error) as erro:
        parser.exit(1, f'Erro: {erro}\n')


if __name__ == '__main__':
    main()
