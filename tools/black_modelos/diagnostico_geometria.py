#!/usr/bin/env python3
"""Diagnóstico somente leitura dos DBs; exporta apenas vértices referenciados."""
import argparse
import json
from pathlib import Path
import struct
import black_obj


def intervalos(numeros):
    resultado = []
    for x in sorted(numeros):
        if resultado and resultado[-1][1]+1 == x:
            resultado[-1][1] = x
        else:
            resultado.append([x,x])
    return [[hex(a),hex(b)] for a,b in resultado]


def inventario(dados):
    malhas = black_obj.detectar(dados)
    info, usados, registros = [], [], []
    for numero,m in enumerate(malhas):
        s = set(i for f in m['faces'] for i in f)
        usados.append(s)
        registros.append(dados[m['inicio']:m['inicio']+m['nvertices']*m['passo']])
        vs = [struct.unpack_from('<3f',dados,m['inicio']+i*m['passo']) for i in s]
        info.append(dict(numero=numero, raiz=hex(m['raiz']), inicio=hex(m['inicio']),
                         passo=m['passo'], vertices_reservados=m['nvertices'], vertices_referenciados=len(s),
                         faixas_referenciadas=intervalos(s), triangulos=len(m['faces']),
                         minimo=[min(v[e] for v in vs) for e in range(3)],
                         maximo=[max(v[e] for v in vs) for e in range(3)]))
    pares=[]
    for a in range(len(malhas)):
        for b in range(a+1,len(malhas)):
            x,y=malhas[a],malhas[b]
            pares.append(dict(malhas=[a,b], registros_identicos=registros[a]==registros[b],
                              bytes_sobrepostos=max(0,min(x['inicio']+len(registros[a]),y['inicio']+len(registros[b]))-max(x['inicio'],y['inicio'])),
                              indices_comuns=len(usados[a]&usados[b]) if registros[a]==registros[b] else None))
    return dict(sha256=black_obj.sha(dados), tamanho=len(dados), malhas=info, pares=pares)


def comparar(original, variante):
    if len(original)!=len(variante):
        return dict(tamanho_igual=False, inventario=inventario(variante))
    diferentes={i for i,(a,b) in enumerate(zip(original,variante)) if a!=b}
    autorizados=set()
    detalhes=[]
    for numero,m in enumerate(black_obj.detectar(original)):
        xyz={m['inicio']+i*m['passo']+j for i in range(m['nvertices']) for j in range(12)}
        caixas=set(range(m['limites'],m['limites']+12))|set(range(m['limites']+16,m['limites']+28))
        ib=m['limites']+black_obj.u32(original,m['limites']+40)
        indices=set(range(ib,ib+2*m['nindices']))
        contador=set(range(m['limites']+36,m['limites']+40))
        uv={m['inicio']+i*m['passo']+j for i in range(m['nvertices']) for j in range(m['passo']-8,m['passo'])}
        outros={m['inicio']+i*m['passo']+j for i in range(m['nvertices']) for j in range(12,m['passo']-8)}
        campos=dict(xyz=xyz,caixas=caixas,indices=indices,contador=contador,uv=uv,outros_atributos=outros)
        for conjunto in campos.values():autorizados|=conjunto
        detalhes.append(dict(numero=numero,bytes_por_campo={k:len(v&diferentes) for k,v in campos.items()},
                             vertices_xyz_alterados=sum(original[m['inicio']+i*m['passo']:m['inicio']+i*m['passo']+12]!=
                                                        variante[m['inicio']+i*m['passo']:m['inicio']+i*m['passo']+12]
                                                        for i in range(m['nvertices']))))
    fora=diferentes-autorizados
    return dict(tamanho_igual=True,sha256=black_obj.sha(variante),bytes_diferentes=len(diferentes),
                malhas=detalhes,bytes_nao_classificados=len(fora),faixas_nao_classificadas=intervalos(fora))


def exportar(original,pasta):
    dados=original.read_bytes()
    malhas=black_obj.detectar(dados)
    if not malhas:raise ValueError('Nenhuma malha reconhecida')
    pasta.mkdir(parents=True,exist_ok=False)
    for numero,m in enumerate(malhas):
        usados=sorted(set(i for f in m['faces'] for i in f))
        remap={i:j+1 for j,i in enumerate(usados)}
        linhas=['# Diagnóstico: coordenadas locais, sem aplicar ossos; somente vértices usados',f'o malha_{numero}']
        for i in usados:
            xyz=struct.unpack_from('<3f',dados,m['inicio']+i*m['passo'])
            linhas.append('v '+' '.join(format(x,'.9g') for x in xyz))
        for i in usados:
            uv=struct.unpack_from('<2f',dados,m['inicio']+i*m['passo']+m['passo']-8)
            linhas.append('vt '+' '.join(format(x,'.9g') for x in uv))
        for face in m['faces']:
            linhas.append('f '+' '.join(f'{remap[i]}/{remap[i]}' for i in face))
        black_obj.gravar_novo(pasta/f'malha_{numero}.obj',('\n'.join(linhas)+'\n').encode())
    black_obj.gravar_novo(pasta/'inventario.json',(json.dumps(inventario(dados),indent=2,ensure_ascii=False)+'\n').encode())


def main():
    p=argparse.ArgumentParser(description=__doc__)
    sub=p.add_subparsers(dest='acao',required=True)
    a=sub.add_parser('auditar');a.add_argument('original',type=Path);a.add_argument('variantes',nargs='*',type=Path)
    e=sub.add_parser('separar');e.add_argument('original',type=Path);e.add_argument('pasta',type=Path)
    args=p.parse_args()
    try:
        if args.acao=='separar':
            exportar(args.original,args.pasta)
            print(f'OBJs separados em {args.pasta}. São arquivos de diagnóstico; não use com o importador V1.')
        else:
            dados=args.original.read_bytes()
            resultado=dict(original=str(args.original),inventario=inventario(dados),variantes={})
            for path in args.variantes:
                resultado['variantes'][str(path)]=comparar(dados,path.read_bytes()) if path.is_file() else dict(disponivel=False)
            print(json.dumps(resultado,indent=2,ensure_ascii=False))
    except (OSError,ValueError,struct.error) as erro:p.exit(1,f'Erro: {erro}\n')


if __name__=='__main__':main()
