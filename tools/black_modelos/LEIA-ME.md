# BLACK Xbox — ferramenta experimental OBJ (Sandro Mods)

Python 3, Linux, sem dependências externas. Reconhece geometria nas 16 amostras
`bg1_*.db` fornecidas. Não é um conversor universal de arquivos de BLACK.

## Usar

Coloque o código e os arquivos na mesma pasta, ou adapte os caminhos:

```sh
python3 black_obj.py analisar bg1_ak1.db
python3 black_obj.py exportar bg1_ak1.db personagem.obj
# Edite somente as posições dos vértices no OBJ.
python3 black_obj.py importar bg1_ak1.db personagem.obj personagem.obj.json bg1_ak1_modificado.db
```

A exportação gera OBJ e um mapa `.obj.json`. Guarde ambos e o DB original.
A importação exige exatamente o original usado na exportação, conferido por SHA-256.
As saídas existentes nunca são sobrescritas. Para testar outra vez, use outro nome.

No Blender, mantenha a ordem dos vértices e das faces e as coordenadas originais.
Não una/separe objetos, não subdivida, não remova vértices, não refaça UVs e não
exporte normais. Algumas versões do Blender reordenam faces ao exportar OBJ;
nesse caso a ferramenta rejeita a importação. Não remova a validação para contornar
isso. Para o primeiro teste, altere poucas posições diretamente no OBJ em texto.
Faces usam índices globais correspondentes de posição e UV: `f 1/1 2/2 3/3`.

Substitua, numa cópia dos dados extraídos, o DB pelo modificado mantendo o nome
original. O `.wdd` correspondente permanece original. Isso não recompõe a ISO e
não altera o APK. O carregador do port precisa acessar esses dados modificados;
não foi adicionada uma pasta de substituição automática ao Android.

## O que é suportado

- Exportação das partes reconhecidas em OBJ, com triângulos e UVs.
- Reimportação de posições com a mesma quantidade e ordem dos vértices e faces.
- Preservação dos demais bytes, incluindo dados de ossos, pesos e animações.
- Atualização das duas caixas locais reconhecidas por malha quando há edição.
- Rejeição de topologia, UVs, mapa ou DB incompatíveis.

Os grupos podem conter cópias do mesmo conjunto de vértices para materiais ou
partes diferentes. Edite as cópias correspondentes consistentemente. A identificação
desses grupos ainda é experimental. Nos modelos articulados, as coordenadas ainda estão nos espaços locais originais:
a ferramenta não aplica as matrizes dos ossos para montar uma pose completa.
Por isso, partes podem aparecer sobrepostas no editor. A exportação serve para
inspecionar/editar a geometria local, não entrega um personagem pronto para render.

O OBJ não contém rig, pesos ou animação:
esses dados permanecem no DB original, não são transferidos de outro personagem.

Não há exportação de texturas, MTL nem decodificação das normais compactadas.
As normais originais são preservadas e podem produzir iluminação incorreta após
deformações. Outros limites de visibilidade, colisões e poses não são recalculados;
comece com alterações pequenas. O importador não converte um OBJ arbitrário em
um personagem completo de BLACK. Isso ainda exige reconstruir buffers, referências,
rig, pesos, materiais e limites do formato.

## Estrutura observada

O DB é um pacote, não uma malha simples: nas amostras há também blocos RIFF/WAVE.
Os cabeçalhos de geometria encontrados têm campo 35 em +0x18 e a sequência float
8,16,24 em +0x2C. +0x1C aponta relativamente ao grupo de descritores; +0x50
contém sua quantidade. Cada descritor tem 28 bytes: +4 aponta relativamente ao
buffer de vértices, +0x0C aos limites locais e +0x14 indica passo 24 ou 36.
Os primeiros 12 bytes do vértice são XYZ; os últimos 8 são UV. Após os limites,
em +0x20, há primitiva 6 (triangle strip), quantidade de índices e deslocamento
relativo aos limites para índices uint16. Entre vértices e índices pode haver
alinhamento de até 15 bytes. O código confere intervalos, valores e índices antes
de aceitar uma malha. Outras versões/plataformas podem usar outra estrutura.

A função dos `.wdd` enviados ainda não foi determinada. Todos têm 65536 bytes;
eles não são convertidos por este programa. Não confunda a extensão com formatos
homônimos de outros jogos.

## Validação

```sh
python3 testar_amostras.py /caminho/da/pasta/com/arquivos
```

Busca DBs recursivamente, exporta e reimporta em pasta temporária e compara todos
os bytes. Também testa edição de posição, rejeição de topologia e de mapa alterado.
`relatorio-amostras.json` registra os resultados das 16 amostras recebidas.
Isso verifica integridade binária; aparência, animação e carregamento de uma edição
no jogo ainda precisam ser verificados no dispositivo. Nenhum arquivo do jogo é
incluído na distribuição da ferramenta.
