#!/usr/bin/env python3
"""V3 experimental para bg1_pst Xbox: topologia nova ligada ao Pistol_Root original."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import struct
import zlib
import black_obj
from black_v2 import montar_strip

ORIGINAL_SHA = 'df87876f470d025644e538b64ff1a827bb63ec13e9c251fcabd8797441b7d3ea'


def normal(a,b,c):
    u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
    n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    return unitario(n)


def unitario(v):
    comprimento=math.sqrt(sum(x*x for x in v))
    if comprimento<1e-12:raise ValueError('Normal nula ou face degenerada')
    return tuple(x/comprimento for x in v)


def triangular(cantos,posicoes):
    if len(cantos)<3:raise ValueError('Face com menos de três cantos')
    vs=[posicoes[c[0]] for c in cantos]
    if len(set(vs))!=len(vs):raise ValueError('Polígono com posições repetidas')
    # Normal de Newell, seguida de projeção no plano dominante e corte de orelhas.
    n=[sum((vs[i][(e+1)%3]-vs[(i+1)%len(vs)][(e+1)%3])*
           (vs[i][(e+2)%3]+vs[(i+1)%len(vs)][(e+2)%3]) for i in range(len(vs))) for e in range(3)]
    n=unitario(n)
    escala=max(1.,max(abs(x) for v in vs for x in v))
    if any(abs(sum((v[e]-vs[0][e])*n[e] for e in range(3)))>1e-6*escala for v in vs):
        raise ValueError('Polígono não plano; triangule no editor')
    excluir=max(range(3),key=lambda e:abs(n[e]));eixos=[e for e in range(3) if e!=excluir]
    xy=[tuple(v[e] for e in eixos) for v in vs]
    def cruz(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    area=sum(xy[i][0]*xy[(i+1)%len(xy)][1]-xy[(i+1)%len(xy)][0]*xy[i][1] for i in range(len(xy)))
    sinal=1 if area>0 else -1
    # Recusa cruzamentos e toques entre arestas não adjacentes.
    for i in range(len(xy)):
        for j in range(i+1,len(xy)):
            if j==i+1 or (i==0 and j==len(xy)-1):continue
            a,b=xy[i],xy[(i+1)%len(xy)];c,d=xy[j],xy[(j+1)%len(xy)]
            if cruz(a,b,c)*cruz(a,b,d)<=0 and cruz(c,d,a)*cruz(c,d,b)<=0:
                if max(min(a[0],b[0]),min(c[0],d[0]))<=min(max(a[0],b[0]),max(c[0],d[0])) and max(min(a[1],b[1]),min(c[1],d[1]))<=min(max(a[1],b[1]),max(c[1],d[1])):
                    raise ValueError('Polígono com auto-interseção ou arestas sobrepostas')
    pendentes=list(range(len(cantos)));triangulos=[]
    while len(pendentes)>3:
        for k,b in enumerate(pendentes):
            a,c=pendentes[k-1],pendentes[(k+1)%len(pendentes)]
            if cruz(xy[a],xy[b],xy[c])*sinal<=1e-14:continue
            if any(all(cruz(xy[x],xy[y],xy[p])*sinal>=-1e-14 for x,y in ((a,b),(b,c),(c,a))) for p in pendentes if p not in (a,b,c)):
                continue
            triangulos.append([cantos[x] for x in (a,b,c)]);pendentes.pop(k);break
        else:raise ValueError('Não foi possível triangular o polígono com segurança')
    triangulos.append([cantos[x] for x in pendentes])
    for t in triangulos:normal(*(posicoes[c[0]] for c in t))
    return triangulos


def ler_obj(path):
    vs=[];uv=[];ns=[];triangulos=[];materiais=set()
    def indice(s,n):
        x=int(s);i=x-1 if x>0 else n+x if x<0 else -1
        if not 0<=i<n:raise ValueError('Índice OBJ inválido')
        return i
    for numero,linha in enumerate(Path(path).read_text().splitlines(),1):
        campos=linha.split('#',1)[0].split()
        if not campos:continue
        t,*val=campos
        if t in ('v','vt','vn'):
            if len(val)!=(2 if t=='vt' else 3):raise ValueError(f'Linha {numero}: quantidade de coordenadas não suportada')
            v=tuple(float(x) for x in val)
            if not all(math.isfinite(x) and abs(x)<1e6 for x in v):raise ValueError('Coordenada não finita ou excessiva')
            (vs if t=='v' else uv if t=='vt' else ns).append(v)
        elif t=='f':
            cantos=[]
            for token in val:
                a=token.split('/')
                if len(a)>3:raise ValueError('Canto OBJ inválido')
                cantos.append((indice(a[0],len(vs)),indice(a[1],len(uv)) if len(a)>1 and a[1] else None,
                               indice(a[2],len(ns)) if len(a)>2 and a[2] else None))
            triangulos.extend(triangular(cantos,vs))
        elif t=='usemtl':materiais.add(' '.join(val))
        elif t not in ('o','g','s','mtllib'):raise ValueError(f'Elemento OBJ não suportado: {t}')
    if not triangulos or len(materiais)>1:raise ValueError('Use um objeto com faces e no máximo um material')
    pontos=[];faces=[];unicos={};sem_uv=False
    for tri in triangulos:
        n=normal(*(vs[c[0]] for c in tri));face=[]
        for pi,ti,ni in tri:
            tex=uv[ti] if ti is not None else (0.,0.)
            sem_uv|=ti is None
            norm=unitario(ns[ni]) if ni is not None else n
            chave=(pi,tex,norm)
            if chave not in unicos:unicos[chave]=len(pontos);pontos.append((vs[pi],tex,norm))
            face.append(unicos[chave])
        faces.append(face)
    return pontos,faces,sem_uv


def decodificar_normal(valor):
    def sinal(x,b):return x-(1<<b) if x&(1<<(b-1)) else x
    return (max(-1,sinal(valor&2047,11)/1023),max(-1,sinal((valor>>11)&2047,11)/1023),max(-1,sinal(valor>>22,10)/511))


def codificar_normal(n):
    x,y,z=unitario(n)
    return (round(x*1023)&2047)|((round(y*1023)&2047)<<11)|((round(z*511)&1023)<<22)


def examinar(dados):
    if black_obj.sha(dados)!=ORIGINAL_SHA:raise ValueError('DB não homologado: esta V3 aceita somente o bg1_pst original examinado, sem edições')
    ms=black_obj.detectar(dados)
    if len(ms)!=3 or [m['passo'] for m in ms]!=[24,36,36]:raise ValueError('Layout inesperado')
    a,b=ms[1:];r=a['raiz'];u=lambda o:black_obj.u32(dados,o)
    if r!=b['raiz'] or a['nvertices']!=b['nvertices']:raise ValueError('Grupos incompatíveis')
    raw=lambda m:dados[m['inicio']:m['inicio']+m['nvertices']*36]
    if raw(a)!=raw(b):raise ValueError('Buffers não são cópias idênticas')
    usados=[set(i for f in m['faces'] for i in f) for m in ms]
    if usados[1]&usados[2] or usados[1]|usados[2]!=set(range(a['nvertices'])):raise ValueError('Partição dos grupos inválida')
    n=u(r+0x5c);table=r+u(r+0x60);nomes=[]
    if not 1<=n<=128 or table+n*4>len(dados):raise ValueError('Tabela de nomes inválida')
    for i in range(n):
        offset=r+u(table+i*4)
        nomes.append(dados[offset:offset+24].split(b'\0')[0].decode('ascii'))
    raiz=nomes.index('Pistol_Root');doadores=[];hist=Counter();comprimentos=[]
    for i in sorted(usados[1]):
        o=a['inicio']+i*36;w=tuple(dados[o+12:o+16]);ids=struct.unpack_from('<4H',dados,o+16)
        if w!=(255,0,0,0) or ids[1:]!=(0,0,0) or ids[0]%3 or ids[0]//3>=n:raise ValueError('Atributos não rígidos ou desconhecidos no grupo 1')
        nome=nomes[ids[0]//3]
        if not nome.startswith('Pistol_'):raise ValueError('Grupo alvo inclui atributos não identificados como pistola')
        hist[nome]+=1
        if ids[0]==raiz*3:doadores.append(i)
        normal=decodificar_normal(u(o+24));comprimentos.append(math.sqrt(sum(v*v for v in normal)))
    if not doadores or any(abs(x-1)>.01 for x in comprimentos):raise ValueError('Doador ou interpretação das normais não validado nas amostras')
    return ms,usados,doadores,dict(hist),nomes


def substituir(original,obj,saida,relatorio,ajustar=False):
    for path in (saida,relatorio):
        if Path(path).exists():raise ValueError(f'Saída já existe: {path}')
    if Path(saida).resolve()==Path(relatorio).resolve():raise ValueError('DB e relatório precisam de caminhos distintos')
    dados=Path(original).read_bytes();ms,usados,doadores,hist,nomes=examinar(dados)
    pontos,faces,sem_uv=ler_obj(obj);strip=montar_strip(faces);slots=sorted(usados[1]);m=ms[1]
    if len(pontos)>len(slots) or len(strip)>m['nindices']:raise ValueError('Capacidade excedida: não há realocação de buffers nesta V3')
    xyz_doador=[struct.unpack_from('<3f',dados,m['inicio']+i*36) for i in doadores]
    minimo=[min(v[e] for v in xyz_doador) for e in range(3)];maximo=[max(v[e] for v in xyz_doador) for e in range(3)]
    baixo=[min(p[0][e] for p in pontos) for e in range(3)];alto=[max(p[0][e] for p in pontos) for e in range(3)]
    escala=1.;translacao=[0.,0.,0.]
    if ajustar:
        escalas=[(maximo[e]-minimo[e])/(alto[e]-baixo[e]) for e in range(3) if alto[e]-baixo[e]>1e-12]
        escala=min(escalas)*.8
        translacao=[(minimo[e]+maximo[e])/2-escala*(baixo[e]+alto[e])/2 for e in range(3)]
        pontos=[(tuple(escala*v[e]+translacao[e] for e in range(3)),uv,n) for v,uv,n in pontos]
    if any(not minimo[e]-1e-7<=v[e]<=maximo[e]+1e-7 for v,uv,n in pontos for e in range(3)):
        raise ValueError('OBJ fora da caixa dos doadores Pistol_Root; use --ajustar ou ajuste no editor')
    resultado=bytearray(dados);permitidos=set();doador=doadores[0];atributos=dados[m['inicio']+doador*36+12:m['inicio']+doador*36+24]
    for grupo in (1,2):
        atual=ms[grupo]
        for k,(v,uv,n) in enumerate(pontos):
            o=atual['inicio']+slots[k]*36
            struct.pack_into('<3f',resultado,o,*v)
            resultado[o+12:o+24]=atributos
            struct.pack_into('<I2f',resultado,o+24,codificar_normal(n),*uv)
            permitidos.update(range(o,o+36))
    indices=m['limites']+black_obj.u32(dados,m['limites']+40)
    struct.pack_into(f'<{len(strip)}H',resultado,indices,*(slots[x] for x in strip))
    struct.pack_into('<I',resultado,m['limites']+36,len(strip))
    permitidos.update(range(indices,indices+len(strip)*2));permitidos.update(range(m['limites']+36,m['limites']+40))
    for ofs,func in ((0,max),(16,min)):
        struct.pack_into('<3f',resultado,m['limites']+ofs,*(func(p[0][e] for p in pontos) for e in range(3)))
        permitidos.update(range(m['limites']+ofs,m['limites']+ofs+12))
    novos=black_obj.detectar(resultado);esperadas=[[slots[x] for x in f] for f in faces]
    if len(novos)!=3 or novos[1]['faces']!=esperadas or novos[0]!=ms[0] or novos[2]!=ms[2]:raise ValueError('Releitura do DB falhou')
    if len(resultado)!=len(dados) or any(i not in permitidos for i,(a,b) in enumerate(zip(dados,resultado)) if a!=b):raise ValueError('Escrita fora dos intervalos permitidos')
    for i in usados[2]:
        for grupo in (1,2):
            o=ms[grupo]['inicio']+i*36
            if resultado[o:o+36]!=dados[o:o+36]:raise ValueError('Atributos usados pelas mãos alterados')
    if resultado[ms[1]['inicio']:ms[1]['inicio']+ms[1]['nvertices']*36]!=resultado[ms[2]['inicio']:ms[2]['inicio']+ms[2]['nvertices']*36]:raise ValueError('Cópias dos buffers divergiram')
    rel=dict(versao=3,original_sha256=black_obj.sha(dados),saida_sha256=black_obj.sha(resultado),
             saida_crc32=f'{zlib.crc32(resultado):08x}',grupos_buffers_alterados=[1,2],grupos_indices_alterados=[1],
             vertices_utilizados=len(pontos),vertices_reservados_alvo=len(slots),vertices_reservados_por_buffer=m['nvertices'],
             indices_utilizados=len(strip),indices_reservados=m['nindices'],triangulos=len(faces),tamanho_db=len(resultado),
             grupos_originais=hist,doador_vertice=doador,atributos_12_23_hex=atributos.hex(),
             escala=escala,translacao=translacao,campos_alterados=['XYZ e UV dos novos vértices','normal interpretada como 11/11/10','atributos copiados de Pistol_Root','índices e contador do grupo 1','caixas locais do grupo 1'],
             campos_preservados=['tamanho, offsets e cabeçalhos externos','grupo 0','índices e limites do grupo 2','todos os registros referenciados pelas mãos','regiões desconhecidas fora dos slots usados'],
             validacoes=['SHA original homologado','partição de índices','doadores rígidos originais','capacidade','faces/orientação por releitura','cópias idênticas','mãos preservadas','escrita confinada'],
             advertencias=['protótipo experimental, sem validação no jogo','objeto inteiro ligado ao Pistol_Root; sem ferrolho/carregador separados','materiais e texturas originais','normais interpretadas por estatística e codec do SDK; declaração do draw ainda não correlacionada','sem realocação, colisões ou rig OBJ']+(['OBJ sem UV: usados zeros'] if sem_uv else []))
    black_obj.gravar_novo(saida,resultado)
    try:black_obj.gravar_novo(relatorio,(json.dumps(rel,indent=2,ensure_ascii=False)+'\n').encode())
    except OSError:
        Path(saida).unlink();raise
    return rel


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='acao',required=True)
    s=sub.add_parser('substituir')
    for nome in ('original','obj','saida'):s.add_argument('--'+nome,required=True,type=Path)
    s.add_argument('--relatorio',type=Path);s.add_argument('--ajustar',action='store_true')
    a=p.parse_args()
    try:
        rel=substituir(a.original,a.obj,a.saida,a.relatorio or Path(str(a.saida)+'.json'),a.ajustar)
        print(json.dumps(rel,ensure_ascii=False,indent=2))
    except (OSError,ValueError,struct.error) as erro:p.exit(1,f'Erro: {erro}\n')


if __name__=='__main__':main()
