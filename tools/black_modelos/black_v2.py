#!/usr/bin/env python3
"""Protótipo V2: nova topologia em malha rígida, sem realocar o pacote DB."""
import math
import struct
from pathlib import Path
import black_obj


def ler_nova_malha(path):
    posicoes, uvs, pontos, faces, unicos = [], [], [], [], {}
    materiais = set()

    def indice(texto, tamanho):
        numero = int(texto)
        numero = numero - 1 if numero > 0 else tamanho + numero if numero < 0 else -1
        if not 0 <= numero < tamanho:
            raise ValueError('Índice OBJ fora do intervalo')
        return numero

    for linha in Path(path).read_text().splitlines():
        campos = linha.split('#', 1)[0].split()
        if not campos:
            continue
        tipo, *valores = campos
        if tipo in ('v', 'vt', 'vn'):
            tamanho = 2 if tipo == 'vt' else 3
            if len(valores) != tamanho:
                raise ValueError('OBJ requer XYZ, UV de dois componentes e faces triangulares')
            v = tuple(float(x) for x in valores)
            if not all(math.isfinite(x) and abs(x) < 1e6 for x in v):
                raise ValueError('Coordenada inválida no OBJ')
            if tipo == 'v':
                posicoes.append(v)
            elif tipo == 'vt':
                uvs.append(v)
            # Normais OBJ não são codificadas: mantemos normais compactadas do doador.
        elif tipo == 'usemtl':
            materiais.add(' '.join(valores))
        elif tipo == 'f':
            if len(valores) != 3:
                raise ValueError('Triangule o OBJ antes da importação')
            face = []
            for token in valores:
                partes = token.split('/')
                if len(partes) not in (2, 3) or not partes[1]:
                    raise ValueError('Cada canto precisa de posição e UV: f v/vt ...')
                chave = (indice(partes[0], len(posicoes)), indice(partes[1], len(uvs)))
                if chave not in unicos:
                    unicos[chave] = len(pontos)
                    pontos.append((posicoes[chave[0]], uvs[chave[1]]))
                face.append(unicos[chave])
            if len(set(face)) != 3:
                raise ValueError('Face com índices repetidos')
            faces.append(face)
        elif tipo not in ('o', 'g', 's', 'mtllib'):
            raise ValueError(f'Elemento OBJ não suportado: {tipo}')
    if len(materiais) > 1 or not faces:
        raise ValueError('Use uma única malha triangular com no máximo um material')
    return pontos, faces


def montar_strip(faces):
    """Une triângulos por degenerados sem mudar a orientação das faces úteis."""
    strip = list(faces[0])
    for a, b, c in faces[1:]:
        # Os três primeiros triângulos da ligação têm um índice repetido.
        if (len(strip) + 4) % 2:
            a, b = b, a
        strip.extend((strip[-1], a, a, b, c))
    return strip


def substituir(original, obj, saida, numero):
    dados = Path(original).read_bytes()
    malhas = black_obj.detectar(dados)
    if not 0 <= numero < len(malhas):
        raise ValueError('Número de malha inexistente; use analisar para conferir a ordem')
    m = malhas[numero]
    if m['passo'] != 24:
        raise ValueError('V2 limitada a passo 24: passo 36 exige mapear skinning antes de substituir')
    pontos, faces = ler_nova_malha(obj)
    strip = montar_strip(faces)
    if len(pontos) > m['nvertices'] or len(strip) > m['nindices']:
        raise ValueError(f'Capacidade excedida: {len(pontos)}/{m["nvertices"]} vértices e '
                         f'{len(strip)}/{m["nindices"]} índices; realocação ainda não implementada')
    antigos = [struct.unpack_from('<3f', dados, m['inicio']+i*24) for i in range(m['nvertices'])]
    minimo = tuple(min(v[e] for v in antigos) for e in range(3))
    maximo = tuple(max(v[e] for v in antigos) for e in range(3))
    if any(not minimo[e] <= xyz[e] <= maximo[e] for xyz, uv in pontos for e in range(3)):
        raise ValueError('Modelo fora dos limites antigos: ajuste escala/posição no editor')
    resultado = bytearray(dados)
    for i, (xyz, uv) in enumerate(pontos):
        doador = min(range(len(antigos)), key=lambda k: sum((antigos[k][e]-xyz[e])**2 for e in range(3)))
        offset = m['inicio']+i*24
        # Preserva os 4 bytes desconhecidos de um vértice próximo. Iluminação é experimental.
        resultado[offset:offset+24] = dados[m['inicio']+doador*24:m['inicio']+doador*24+24]
        struct.pack_into('<3f', resultado, offset, *xyz)
        struct.pack_into('<2f', resultado, offset+16, *uv)
    indices = m['limites'] + black_obj.u32(dados, m['limites']+40)
    struct.pack_into(f'<{len(strip)}H', resultado, indices, *strip)
    struct.pack_into('<I', resultado, m['limites']+36, len(strip))
    for deslocamento, funcao in ((0, max), (16, min)):
        struct.pack_into('<3f', resultado, m['limites']+deslocamento,
                         *(funcao(p[0][e] for p in pontos) for e in range(3)))
    verificadas = black_obj.detectar(resultado)
    if len(verificadas) != len(malhas) or verificadas[numero]['faces'] != faces:
        raise ValueError('Validação do DB reconstruído falhou; nenhum arquivo foi gravado')
    if len(resultado) != len(dados):
        raise ValueError('Tamanho do DB alterado inesperadamente')
    black_obj.gravar_novo(saida, resultado)
    return dict(malha=numero, vertices_usados=len(pontos), vertices_reservados=m['nvertices'],
                triangulos=len(faces), indices_usados=len(strip), indices_reservados=m['nindices'],
                tamanho=len(resultado), validacao='binária; teste no jogo pendente')
