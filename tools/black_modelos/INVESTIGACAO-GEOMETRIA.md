# BLACK Xbox — investigação da geometria não visível

## Resultado e escopo desta entrega

A ausência do cubo não demonstra um defeito do importador nem confirma seu
funcionamento em runtime. O teste V2 demonstrou inclusão do arquivo na XISO, segundo
os hashes informados; ainda não demonstrou que a malha 0 é a pistola visível.

Não alterei V1/V2, APK, ISO ou DB original. Acrescentei apenas diagnóstico de leitura,
exportação isolada dos grupos e testes. Não gerei novos DBs de deformação.

## Evidências verificadas localmente

O pacote recebido contém os originais e a banana. Não contém os arquivos
`bg1_pst_cubo_limpo.db`, `bg1_pst_teste_malha1.db` ou `bg1_pst_teste_malha2.db`.
O cubo gerado aqui na etapa anterior não é necessariamente o cubo limpo do usuário:
este tem 19 vértices por separação de UVs, enquanto o exemplo local tinha 8.
Não substituí a auditoria dos três arquivos ausentes pela de arquivos recriados.
Os hashes desses arquivos no texto são evidência relatada, não hashes recalculados.

Reconfirmei na banana 77857 bytes diferentes e 10614 vértices alterados, todos em
XYZ/caixas. Ela é reproduzível byte a byte pelo OBJ enviado. O JSON acompanhante
registra a comparação local. Essa evidência confirma a edição; a associação à
imagem observada ainda merece o controle A/B solicitado.

Descoberta nova na pistola original examinada:

| Grupo | Vértices reservados | Referenciados pelas faces | Faixa dos índices usados | Triângulos |
|---|---:|---:|---|---:|
| 0 | 322 | 322 | 0–321 | 330 |
| 1 | 5152 | 3687 | 1465–5151 | 3415 |
| 2 | 5152 | 1465 | 0–1464 | 2310 |

Os buffers dos grupos 1 e 2 são **idênticos nos 36 bytes de cada um dos 5152
registros**, mas ocupam intervalos físicos diferentes. Os conjuntos de índices são
disjuntos e, unidos, cobrem os 5152 vértices. Não há sobreposição física dos buffers
reconhecidos. As caixas dos três grupos também são separadas dos buffers.

Isso sustenta a interpretação de duas partes/draws distintos que carregam cópias
dos mesmos atributos e desenham subconjuntos diferentes. Ainda não estabelece
material, LOD, arma, mãos ou identidade dos ossos. Exportar todo o buffer de cada
grupo exibia vértices soltos da outra parte: o diagnóstico novo exporta **somente os
vértices referenciados**, com índices compactados, para eliminar essa ambiguidade.
Não use esses OBJs compactados com o importador V1.

A malha 0 mede aproximadamente 0,03277 × 0,03277 × 0,17269 nas coordenadas locais.
Isso é compatível com um componente estreito, mas não prova que seja clarão de tiro,
silenciador ou outra peça. As partes articuladas ainda são exportadas sem pose de
ossos; sobreposição no Blender não demonstra corrupção do DB.

## Hipóteses, em ordem de prioridade investigativa

| Prioridade | Hipótese | Evidência e limite |
|---|---|---|
| Alta | Malha 0 não é a parte principal visível, ou só aparece em certo estado | Cubo sem alteração perceptível; grupo pequeno e separado. Falta identificá-lo visualmente e durante tiro/recarga. |
| Média | Grupo 1 corresponde a outra parte e/ou +0,03 foi um sinal pouco perceptível | Grupos 1 e 2 usam subconjuntos completamente diferentes. Não há comparação enquadrada nem auditoria da ISO desse teste. |
| Média, fácil de eliminar | A fonte efetivamente montada difere da pretendida | O código prefere dados extraídos e escolhe a primeira ISO compatível. O usuário confirmou a ISO; falta ligar essa confirmação ao log de seleção. |
| Média | O recurso ativo tem outro nome/fase, ou só muda ao trocar/equipar a arma | Nenhum log de abertura do DB foi fornecido. A presença do arquivo na XISO não prova seu consumo. |
| Em aberto | O resultado visual da banana incluiu mudança de pose/FOV ou outro fator | Edição binária confirmada, mas o teste visual não teve controle idêntico. Não invalida o relato. |
| Baixa sem evidência nova | Cache persistente de modelos explica todos os resultados | Não encontrei nesta inspeção um cache persistente de DB no caminho de leitura. Encerrar o processo elimina o cache em memória da sessão. |

Matrizes/animações podem mudar a direção e a escala aparentes de um deslocamento.
Elas não explicam, por si sós, o desaparecimento de toda alteração XYZ: uma
transformação afim não degenerada preserva uma diferença de posição. Precisamos
ver os atributos, o shader e os draws antes de atribuir o resultado ao skinning.
Não há base para percentuais numéricos de probabilidade.

## Caminho observado no código do port

- `native/game_files.cpp`, função `Search`: procura `default.xbe` extraído antes das
  ISOs de cada pasta; ordena nomes de ISOs e aceita a primeira versão compatível.
  `Game: ... in ...` registra candidatos; a linha aceita sem `another release`,
  junto com a sequência completa, indica a escolha normal. Não basta uma linha
  isolada de candidato incompatível. O hash dessa linha é do **XBE**, não do DB.
- O executável europeu é convertido para o executável americano recompilado.
  Isso não traduz nem reconstrói a geometria de todos os arquivos DB.
- No SDK examinado: `runtime/kernel/files.cpp`, `Resolve`, `OpenOnDisc`,
  `NtCreateFile` e `Transfer`/`NtReadFile`: resolvem nomes e leem arquivos do disco
  montado ou diretório host. `OpenOnDisc` guarda a entrada da XISO; `Transfer` chama
  `Disc::Read` para entregar os bytes ao jogo.
- Há logs `open ...` limitados por `LOG_FIRST(400)` em pontos de abertura. A ausência
  de uma linha não prova ausência da leitura: o limite pode ter sido atingido ou
  uma outra rota pode ter sido usada. Não são logs por frame.
- `src/d3d/d3d8.cpp`: `D3DDevice_SetStreamSource`, `SetIndices`, `SetTransform`,
  `SetVertexShaderConstant*`, `DrawIndexedVertices` e `DrawVertices` transportam
  buffers/transformações para o backend. Os registros de recursos/buffers também
  passam pelo runtime XDK/GPU. O nome do DB não acompanha automaticamente cada draw.
- `native/main.cpp`, `LogToLogcat`: encaminha stderr ao Android sob tag `BLACK`.

A semântica de abrir um DB de arma, associá-lo ao identificador da pistola e escolher
um grupo é do código original recompilado. Não localizei nesta inspeção uma função
host que já faça toda essa associação. Instrumentar genericamente cada draw não
resolveria o vínculo e criaria excesso de logs.

Os detalhes de `files.cpp` foram verificados na cópia SDK-current disponível; como
o build busca SDK `main`, o SDK usado no APK do telefone deve ser identificado antes
de aplicar um patch. Não alterei apenas uma dependência em cache fingindo que isso
mudaria o próximo APK.

## Próximo experimento: fechar a cadeia de evidência antes de outra ISO

1. Auditar localmente os cinco DBs e exportar os três grupos separados. Abrir no
   Blender um grupo por vez, isolando os demais. Capturar vistas frontal/lateral e
   informar que partes reconhece. Não assumir que o grupo de maior contagem é arma.
2. Coletar um log desde a inicialização a frio usando a ISO original e, se ainda
   existe, a ISO banana já testada. Conferir a fonte selecionada e os `open` de
   `fpguns`. Evitar deixar dados extraídos/ISOs concorrentes nas pastas pesquisadas
   durante o teste; movê-los temporariamente para uma pasta fora dessa busca,
   preservando os arquivos. Não apague saves ou dados do aplicativo.
3. Fazer A/B usando essas duas ISOs existentes: mesma fase, arma, posição, FOV e
   ajustes do mod, com câmera livre/ajuste desativados. Capturar arma parada e um
   vídeo curto de tiro, recarga e troca. Fechar o processo entre A e B. O objetivo
   é primeiro reproduzir a diferença da banana com fonte registrada.
4. Só após identificar visualmente o grupo e confirmar a fonte, considerar um
   teste isolado de posição. A malha 2 ainda não foi testada no jogo segundo o
   relato; não a considero falha. Não recomendo gerar agora várias ISOs cegamente.

Esse experimento usa os arquivos já existentes. O sinal procurado é a cadeia:
DB editado na XISO → fonte selecionada no processo → abertura do caminho correto
→ mudança visual reprodutível no mesmo estado da arma.

## Comandos Linux Mint

Extraia o pacote de diagnóstico e abra o terminal nessa pasta. Use seus caminhos:

```bash
ferramentas="$HOME/Documentos/Black Modelos d/black-modelos"
cd "$ferramentas"

python3 diagnostico_geometria.py auditar bg1_pst_original.db \
  bg1_pst_banana.db bg1_pst_cubo_limpo.db \
  bg1_pst_teste_malha1.db bg1_pst_teste_malha2.db \
  > auditoria-todos.json

# A pasta de saída precisa não existir, para evitar sobrescrita.
python3 diagnostico_geometria.py separar bg1_pst_original.db partes-original
python3 diagnostico_geometria.py separar bg1_pst_banana.db partes-banana

sha256sum bg1_pst_original.db bg1_pst_banana.db bg1_pst_cubo_limpo.db \
  bg1_pst_teste_malha1.db bg1_pst_teste_malha2.db
```

Arquivos ausentes são marcados como `disponivel: false` pelo diagnóstico. O comando
`sha256sum` informa erro para ausentes. A auditoria classifica XYZ, caixas, índices,
contador, UVs e outros atributos. Campos não classificados são destacados; isso não
é uma declaração automática de que o arquivo é válido no jogo.

Para um diagnóstico de deslocamento isolado da malha 1, espera-se somente XYZ e
caixas no grupo 1; o grupo 2 deve ter zero alterações. Na V2 são esperadas mudanças
adicionais de UV, atributos emprestados, índices e contador no grupo selecionado.
Comparar os hashes após extrair a XISO continua sendo necessário. Use o procedimento
de extração que já funcionou e uma pasta de destino nova; não substitua a original.

Com depuração USB autorizada no telefone e `adb` instalado, terminal 1:

```bash
adb devices
adb shell am force-stop com.racas.black
adb logcat -T 1 -v threadtime 'BLACK:I' '*:S' > log-original.txt
```

Abra o aplicativo manualmente, entre na missão e realize a captura. Depois encerre
`logcat` com Ctrl+C. Repita para banana, usando `log-banana.txt`. Não use limpar os
dados do aplicativo. Filtre depois, preservando o log completo:

```bash
rg -ni 'Game:|open .*fpguns|bg1_pst|missing|game:|build' log-original.txt log-banana.txt
# Se não houver rg no Mint:
grep -Ein 'Game:|open .*fpguns|bg1_pst|missing|game:|build' log-original.txt log-banana.txt
```

O log atual não fornece SHA-256 do DB lido, os descritores por abertura, fase e arma
correlacionados. Não afirmo que esses campos já existam. Se os logs atuais forem
insuficientes, a instrumentação seguinte deve ser filtrada por `fpguns/bg1_pst.db`,
registrando abertura com sucesso, origem, tamanho e offset de cada leitura; para
hash completo, reconstituir o recurso por intervalos (incluindo leituras scatter),
sem confundir hash de um fragmento com hash do arquivo. Só depois correlacionar
ponteiros de buffers com draws da arma. Isso exigirá um APK diagnóstico separado;
não é necessário recompilar para a coleta inicial acima.

## Critérios de sucesso e de falha

- Auditoria: todos os bytes alterados devem pertencer aos campos pretendidos do
  grupo escolhido. Falha: alteração em outro grupo ou intervalo sem explicação.
- Integridade da ISO: hash do DB reextraído igual ao DB de teste. Falha: divergência.
- Seleção: log identifica a fonte pretendida. Se selecionar outra, o teste visual
  fica invalidado; isso não condena o conversor.
- Consumo: abertura do recurso esperado é evidência positiva; ausência no log
  limitado é inconclusiva. Não confundir presença na ISO com leitura pelo jogo.
- A/B: deformação aparece repetidamente apenas com banana, no mesmo estado.
  Ausência de diferença após confirmar fonte/consumo exige rever o recurso/mapeamento.
- Identificação de grupo: a geometria referenciada isolada deve corresponder a
  uma parte reconhecível, confirmada depois por alteração controlada em runtime.
  Só o tamanho da malha não satisfaz esse critério.
- Crash, arma ausente ou renderização corrompida são falhas do teste; conservar o
  log e voltar à cópia original. Teste visual negativo do grupo 0 não valida nem
  invalida grupos 1/2 ou NPCs.

## Arquivos exatos ainda necessários para a próxima auditoria local

1. `bg1_pst_cubo_limpo.db` com hash relatado 02f94484ae86ed54402587eeb4971ebbcd88e1b588f460a4f23bd3130eb99392.
2. `bg1_pst_teste_malha1.db` e `bg1_pst_teste_malha2.db` efetivamente produzidos.
3. `diagnosticar_malhas.py` usado para gerar esses dois arquivos.
4. DB reextraído da ISO realmente usada no teste da malha 1 (com caminho da fase).
5. `log-original.txt`, `log-banana.txt`, build do APK e capturas A/B no mesmo enquadramento.
6. Se os hashes dos originais por fase diferirem, os originais de cada fase também;
   não aplicar automaticamente offsets da level_00 em outro pacote.

Não é necessário enviar outra ISO inteira para essa auditoria. Os OBJs e mapa
originais já estão disponíveis. Os primeiros cinco itens reduzem diretamente as
incertezas atuais, sem mudar o projeto para substituição de NPCs antes da hora.
