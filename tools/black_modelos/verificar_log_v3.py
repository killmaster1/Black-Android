#!/usr/bin/env python3
"""Confere fonte selecionada e CRC32 de uma leitura completa no APK diagnóstico."""
import argparse
from pathlib import Path
import re
import zlib


def verificar(texto,db,iso,caminho):
    marcador='MODELO fonte_selecionada='
    if marcador not in texto:raise ValueError('Inconclusivo: sem marcador da instrumentação; confira APK, ativação e início do log')
    trecho=texto[texto.rfind(marcador):]
    fonte=re.search(r'MODELO fonte_selecionada=(.*?) diagnostico=(\w+)',trecho)
    if not fonte or fonte[2]!='ativo':raise ValueError('Diagnóstico não estava ativo')
    if fonte[1].replace('\\','/').rsplit('/',1)[-1]!=iso:raise ValueError('Outra fonte foi selecionada: '+fonte[1])
    crc=f'{zlib.crc32(db):08x}';alvo=caminho.replace('/','\\').upper()
    linhas=re.findall(r'MODELO recurso_completo caminho=(.*?) tamanho=(\d+) crc32=([0-9a-fA-F]{8})',trecho)
    encontrados=[(p,n,c) for p,n,c in linhas if p.replace('/', '\\').upper().endswith(alvo)]
    if not encontrados:raise ValueError('Inconclusivo: não houve cobertura completa do caminho solicitado no log')
    if not any(int(n)==len(db) and c.lower()==crc for p,n,c in encontrados):raise ValueError('Recurso lido diverge do DB local (tamanho/CRC32)')
    return f'Fonte confirmada; recurso completo lido: {caminho}, {len(db)} bytes, CRC32 {crc}. Efeito visual ainda precisa ser confirmado.'


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--log',type=Path,required=True);p.add_argument('--db',type=Path,required=True)
    p.add_argument('--iso',required=True);p.add_argument('--caminho',default='levels/level_00/fpguns/bg1_pst.db')
    a=p.parse_args()
    try:print(verificar(a.log.read_text(errors='replace'),a.db.read_bytes(),a.iso,a.caminho))
    except (OSError,ValueError) as e:p.exit(1,f'{e}\n')
