# BLACK Xbox — V3 experimental para substituir a pistola

## Fluxo recomendado: sem reconstruir a ISO

A atualização mais recente do port prioriza `/sdcard/Black/bg1_pst.db`.
Leia `CARREGAR-DA-PASTA-BLACK.md`: instalar o APK atualizado uma vez e enviar o DB
por ADB substitui a etapa de reconstrução da XISO. O procedimento de reconstrução
abaixo permanece como alternativa para testar o recurso embutido.

## Entrega e estado da validação

Há uma implementação nova, `black_obj_v3.py`, sem sobrescrever V1/V2. Ela aceita
OBJ com topologia diferente e modifica o grupo 1 do `bg1_pst.db` examinado. Atualiza
as duas cópias do buffer de 36 bytes, preserva todos os vértices usados pelas mãos
e deixa os índices do grupo 2 intactos. O objeto inteiro fica ligado a um registro
original de `Pistol_Root`. Não recria peças separadas de ferrolho/carregador.

O exemplo transforma seis quads de um cubo em 12 triângulos, 24 vértices com normais
por face e 58 índices. O DB mantém 741896 bytes. Resultado reproduzido localmente:

- SHA-256: `8d2b43c5707cb54524fd3bc54c7e74b432b2fc71295b31d8ced14515995eb8cc`
- CRC32: `b71f6444`

**Isso é validação binária. Ainda não executei o resultado no Black-Android.**
A leitura efetiva da nova geometria e o efeito visual precisam ser coletados no
telefone. O APK diagnóstico registra fonte, caminho e CRC32 dos bytes realmente
lidos, antes de o jogo modificar/relocalizar o buffer recebido.

## Arquivos disponíveis e ausentes

Inspecionei o código existente e o pacote `black-modelos.7z`. Estão disponíveis o
DB original, a banana, OBJs e mapas. Não estão disponíveis o cubo limpo com hash
02f94484..., os DBs de diagnóstico isolado nem `diagnosticar_malhas.py`. Não afirmei
ter auditado esses arquivos ausentes. Não há ISO ou aparelho Android anexado.

A V3 aceita apenas o SHA-256 original analisado:
`df87876f470d025644e538b64ff1a827bb63ec13e9c251fcabd8797441b7d3ea`.
Ela recusa banana, ps5 e originais de outra fase que tenham conteúdo diferente.
Essa limitação evita aplicar um mapeamento parcial a outro pacote sem análise.

## Registros de 36 bytes: interpretação apoiada por evidências

| Bytes | Leitura implementada | Evidência e grau de certeza |
|---|---|---|
| 0–11 | XYZ float32 LE | V1 e comparação binária; coordenadas e formas coerentes. |
| 12–15 | Quatro bytes, compatíveis com pesos normalizados | Todos os 3687 vértices do grupo 1 têm `FF 00 00 00`. Nas mãos a soma varia de 252 a 255, compatível com quantização. A V3 copia esses bytes do original, sem gerar pesos. |
| 16–23 | Quatro uint16, correlacionados à tabela de ossos | Valores não nulos são múltiplos de 3; divididos por 3 apontam para nomes coerentes com arma/mãos. A V3 copia um registro original inteiro. O fator 3 sugere linhas de matrizes; não foi confirmado no shader de cada draw. |
| 24–27 | Normal compactada assinada 11/11/10 | O codec `NORMPACKED3` do SDK usa esse formato. Aplicado aos vértices, produz comprimentos próximos de 1: grupo 1 entre 0,99772 e 0,99994. A V3 usa esse codec experimental para novas normais; falta correlacionar a declaração do draw em runtime. |
| 28–35 | UV float32 LE | Coordenadas coerentes, correspondentes à exportação anterior. |

Não foi identificado um atributo de cor separado nessa leitura. Não é correto
chamar bytes de cor simplesmente porque cabem em quatro bytes. O layout acima
possui evidência estrutural/estatística forte, mas a declaração e o shader ativos
ainda não foram rastreados até o draw concreto da pistola.

`analise-vertices36.json` registra os resultados medidos. A tabela de 44 nomes
contém `Pistol_Root` no índice 36, Hammer 37, Magazine 38, Slide 39 e Trigger 40.
No grupo 1, os vértices usados se distribuem assim:

| Nome correlacionado | Vértices |
|---|---:|
| Pistol_Slide | 1903 |
| Pistol_Root | 1459 |
| Pistol_Magazine | 138 |
| Pistol_Hammer | 131 |
| Pistol_Trigger | 56 |

No grupo 2 os nomes correlacionados são Root, ombros, cotovelos, pulsos e dedos.
Isso reforça a identificação feita no Blender: grupo 1 é a geometria da pistola;
grupo 2 é a geometria das mãos/braços. Ainda não prova que esse DB é o consumido
pela arma equipada no teste anterior.

## Estratégia de preservação e relação dos grupos

Os 5152 registros são cópias idênticas em dois buffers físicos. O grupo 1 referencia
1465–5151; o grupo 2 referencia 0–1464. Não há interseção nem sobreposição física.

A V3 reserva os slots do conjunto da pistola e grava os novos registros nos mesmos
slots **em ambas as cópias**. Nenhum índice novo pode apontar para os slots das mãos.
Os primeiros 1465 registros continuam iguais aos originais nas duas cópias.
Também preserva índices, contador e caixas do grupo 2 e todo o grupo 0.

Atributos 12–23 vêm integralmente do vértice 2306 do original, associado ao
`Pistol_Root`: `ff0000006c00000000000000`. Nenhum peso ou índice novo foi inventado.
Isso dá ao novo objeto a vinculação geral da pistola. Não transfere rig do OBJ e
não mantém animação independente de ferrolho, gatilho ou carregador. Não tratamos
essa escolha como solução de skinning para personagens.

A V3 troca o contador e os índices do grupo 1. Os registros antigos não usados e
os índices após o novo contador continuam no DB, mas não são referenciados pelas
faces reconstruídas do grupo 1. Isso preserva reservas e offsets; não é compactação.
Pode haver referências externas ainda não identificadas, razão pela qual o teste
em runtime continua indispensável. A igualdade dos buffers é verificada novamente
antes da gravação.

A capacidade é 3687 slots de pistola e 5905 índices. A strip usa degenerados entre
triângulos: 3 + 5 × (triângulos − 1) índices. Assim, o limite prático desse algoritmo
é 1181 triângulos, mesmo que a strip original eficiente desenhasse mais faces.
Não aumentamos buffers nem deslocamos recursos posteriores.

## OBJ e limites do protótipo

Aceita triângulos e polígonos planos simples, inclusive côncavos, triangulados por
corte de orelhas. Recusa polígonos não planos, auto-interseções, faces degeneradas,
índices inválidos, valores não finitos e mais de um material. Índices negativos,
UVs separados e normais OBJ são aceitos. Sem normais, calcula normais por face;
sem UV, usa zeros e registra advertência. Smoothing groups não substituem normais
explícitas: exporte normais do Blender se quiser suavização.

`--ajustar` centraliza e escala uniformemente o modelo para 80% da caixa dos
vértices associados ao Pistol_Root. Sem essa opção, exige que o OBJ já caiba nessa
caixa. A limitação reduz expansão de volumes desconhecidos. O cubo resultante tem
cerca de 2,6 cm nas coordenadas locais; a substituição de toda a pistola por esse
bloco deve ser distinguível, mas a escala aparente depende da câmera/transformação.
Materiais e texturas permanecem originais: não espere um cubo de cor lisa.

## Executar no Linux Mint

Extraia as ferramentas em uma pasta nova, por exemplo `black-modelos-v3`, para
preservar suas cópias V1/V2. Coloque nessa pasta uma cópia do DB original homologado.

```bash
cd "$HOME/Documentos/Black Modelos d/black-modelos-v3"

python3 testar_v3.py bg1_pst_original.db

python3 black_obj_v3.py substituir \
  --original bg1_pst_original.db \
  --obj exemplos/cubo_v3.obj \
  --saida bg1_pst_v3_cubo.db \
  --ajustar

# Modelo próprio, com a mesma restrição de capacidade:
python3 black_obj_v3.py substituir \
  --original bg1_pst_original.db \
  --obj meu_modelo.obj \
  --saida bg1_pst_personalizado.db \
  --ajustar
```

Cada saída gera `nome.db.json` com hashes, CRC32, grupos, capacidades, transformação,
campos alterados/preservados, validações e limitações. Saídas existentes são
recusadas. Nem o original nem a ISO são alterados pelo conversor.

## Reconstruir uma cópia da XISO

Use o `extract-xiso` que já compilou. O exemplo modifica apenas `level_00` e não
presume que outras cópias sejam iguais. A pasta e a ISO de saída precisam ser novas.

```bash
set -e
trabalho="$HOME/Área de trabalho/Black-Android/work"
ferramentas="$HOME/Documentos/Black Modelos d/black-modelos-v3"
origem="$trabalho/Black-Extraido"
copia="$trabalho/Black-Extraido-V3-Cubo"
iso="$trabalho/Black-V3-Cubo.iso"

test ! -e "$copia"
test ! -e "$iso"
cp -a --reflink=auto "$origem" "$copia"

# Conferir o original da fase antes de substituí-lo na cópia.
python3 - "$copia/levels/level_00/fpguns/bg1_pst.db" <<'PYCODE'
import hashlib,sys
from pathlib import Path
assert hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest() == 'df87876f470d025644e538b64ff1a827bb63ec13e9c251fcabd8797441b7d3ea', 'Original da fase difere; pare e audite'
PYCODE

cp -- "$ferramentas/bg1_pst_v3_cubo.db" "$copia/levels/level_00/fpguns/bg1_pst.db"
cmp -- "$ferramentas/bg1_pst_v3_cubo.db" "$copia/levels/level_00/fpguns/bg1_pst.db"
extract-xiso -c "$copia" "$iso"

# Reextrair para comprovar o conteúdo efetivamente empacotado.
verificacao="$trabalho/Conferencia-V3-Cubo"
test ! -e "$verificacao"
extract-xiso -x "$iso" -d "$verificacao"
cmp -- "$ferramentas/bg1_pst_v3_cubo.db" "$verificacao/levels/level_00/fpguns/bg1_pst.db"
sha256sum "$verificacao/levels/level_00/fpguns/bg1_pst.db"
```

Se sua versão do extract-xiso cria um subdiretório adicional na extração, ajuste o
caminho do `cmp` para o arquivo realmente extraído. Não interprete um erro de caminho
como validação. Para outras fases, primeiro compare seus originais com o SHA
homologado e confirme no log qual caminho a missão abre; não copie cegamente.

## APK diagnóstico e transferência por ADB

O CI Android foi configurado para compilar suporte ao rastreamento. O artefato
`Black-Android-APK` dessa compilação é distinto do APK antigo v1.2.0, que não contém
a instrumentação. Não houve publicação de uma nova release nesta etapa.

A opção Gradle é `-PmodelTrace=true` e a opção CMake é `BLACK_TRACE_MODELS=ON`.
O arquivo do SDK não é editado: CMake gera uma cópia instrumentada e recusa a
configuração se não encontrar exatamente o ponto de leitura esperado. Para build
local com código recompilado já disponível: `./gradlew assembleRelease -PmodelTrace=true`.
Para o fluxo Windows do repositório, `GRADLE_ARGS=-PmodelTrace=true` em `build.bat`.

Ative somente no teste, criando um marcador na mesma pasta da ISO escolhida:

```bash
adb devices
adb shell mkdir -p /sdcard/Black
adb push "$HOME/Área de trabalho/Black-Android/work/Black-V3-Cubo.iso" /sdcard/Black/Black-V3-Cubo.iso
printf 'diagnostico-v3\n' > model_trace.txt
adb push model_trace.txt /sdcard/Black/model_trace.txt
```

O port prefere arquivos extraídos antes das ISOs e escolhe a primeira ISO
compatível por nome. Para o teste, deixe só a fonte pretendida nas pastas pesquisadas;
**mova temporariamente as outras fontes para fora da busca, sem apagá-las**.
Não mexa nos saves ou dados do aplicativo.

Instale o APK diagnóstico pelo método que já usa. Se utilizar ADB e a assinatura
permitir atualização: `adb install -r /caminho/app-release.apk`. Não é necessário
recompilar o APK a cada novo OBJ/ISO; o APK diagnóstico pode ser reutilizado.

Terminal de coleta:

```bash
adb shell am force-stop com.racas.black
adb logcat -T 1 -v threadtime 'BLACK:I' '*:S' > log-v3.txt
```

Abra o jogo no telefone, entre na missão correspondente e equipe a pistola. Capture
arma parada, tiro, recarga e troca. Depois encerre o log com Ctrl+C e confira:

```bash
python3 verificar_log_v3.py \
  --log log-v3.txt \
  --db bg1_pst_v3_cubo.db \
  --iso Black-V3-Cubo.iso \
  --caminho levels/level_00/fpguns/bg1_pst.db
```

O verificador considera a última seleção registrada, exige diagnóstico ativo e
confere nome da fonte, caminho, tamanho e CRC32 completo. Ele não declara sucesso
visual. Se o jogo abrir outra fase, a saída mostra o caminho real; audite essa fase
antes de tentar substituí-la. Para desativar: `adb shell rm /sdcard/Black/model_trace.txt`
e encerre/reabra o aplicativo.

## O que a instrumentação demonstra

Mensagens `MODELO` usam nível Report, que permanece em release. O SDK remove Info
em release (`logx::Write`), portanto a falta dos antigos `open` não era uma prova
confiável de que o recurso não foi lido.

- `fonte_selecionada`: apenas a fonte realmente aceita, com caminho da ISO/pasta.
- Aberturas relativas a diretórios mantêm um caminho separado de diagnóstico,
  reconstruído a partir do handle pai, sem alterar a resolução original do jogo.
- `primeira_leitura`: caminho normalizado, tamanho do recurso, offset e bytes de
  uma leitura concluída. O nome `level_XX` identifica a pasta da fase, não um ID
  semântico confirmado da missão. Não foi adicionado ID de arma sem mapeamento.
- `recurso_completo`: todas as posições do arquivo foram vistas, inclusive em
  fragmentos/scatter fora de ordem; registra CRC32 do conteúdo bruto lido.
- `malha`: descritores reconhecidos no recurso completo, stride, offsets e índices.
- `bytes_divergentes`: duas leituras da mesma posição discordaram; o hash é recusado.

Não calcula hash de um fragmento como se fosse o arquivo inteiro. Se o jogo só ler
parte do DB, haverá primeira leitura, mas não `recurso_completo`: resultado
inconclusivo para a identidade integral. CRC32 é um identificador diagnóstico,
não substitui SHA-256 de integridade na comparação local da ISO.

O filtro aceita apenas caminhos terminados em `fpguns/bg1_pst.db`; não instrumenta
cada draw, outros arquivos ou cada frame. Limita-se a oito caminhos de até 2 MiB,
com duas cópias temporárias por recurso para cobertura; desativado por padrão sem
marcador. Não toca nos bytes entregues ao jogo. O teto de armazenamento é 32 MiB
na build de diagnóstico, e não há captura quando ela está inativa.

## Critérios de conclusão

1. Conversor: relatório válido, índices e capacidade conferidos. Comprovado localmente.
2. XISO: DB reextraído idêntico ao gerado. Precisa ser executado no Linux do usuário.
3. Seleção: `fonte_selecionada` aponta para Black-V3-Cubo.iso. Pendente no telefone.
4. Consumo: `recurso_completo` tem caminho, tamanho e CRC32 esperados. Pendente.
5. Renderização: cubo/bloco substitui a geometria original da pistola, mãos continuam
   presentes, e não há peças antigas durante tiro/recarga/troca. Pendente.

Se 3/4 falharem, investigue fonte/fase antes do conversor. Se 3/4 passarem e 5 não,
isso reduz fortemente a dúvida de carregamento e exige rastrear o draw, contador,
declaração e possíveis buffers alternativos. Não será correto declarar V3 funcional
só pelo CRC. Crash/arma invisível também não é sucesso; preserve o log e volte à ISO
original.

## Testes executados e bloqueios

Nove testes Python da V3: original intacto; geração determinística; releitura;
contagem, orientação e limites dos índices; UVs; atributos originais; normal
quantizada; buffers duplicados/mãos; escrita confinada; recusas; triangulação
côncava/índices negativos; relatório e verificador de log. Passaram.

Testes C++ da captura: CRC32 de vetor conhecido, filtro, fragmentos/scatter,
cobertura incompleta, leituras repetidas/divergentes e três descritores: passaram.
Geração CMake testada com duas revisões locais do SDK e recusa de layout desconhecido.
Checagem sintática ARM64 das fontes modificadas e da cópia instrumentada: passou.
Configuração Gradle/compilação Java: passou. O resultado da build APK completa deve
ser conferido no Actions; compilação não demonstra funcionamento no telefone.

Bloqueios restantes: confirmação do recurso em runtime; correlação da declaração
ativa dos vértices; modelo multi-osso arbitrário; normais/tangentes de outros layouts;
realocação de buffers; novos materiais/texturas; volumes externos/colisão. A escala
conservadora e o vínculo rígido ao Pistol_Root são limites deliberados desta etapa.
Não houve trabalho em NPCs, cheats ou interface.
