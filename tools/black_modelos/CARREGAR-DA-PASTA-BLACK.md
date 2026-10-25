# Carregar o DB convertido diretamente de /sdcard/Black

O port agora prioriza o DB externo da pistola. É necessário instalar o APK com
esta alteração uma vez. Depois, **não precisa reconstruir a ISO nem recompilar o
APK para cada modelo**. A ISO original pode continuar sendo usada.

A substituição usa o arquivo binário `.db` produzido pelo conversor, não o `.obj`.
A funcionalidade atual é restrita a `fpguns/bg1_pst.db`; não altera ps5 nem NPCs.

## Primeiro teste

Na pasta das ferramentas, gere o exemplo com uma cópia do DB original homologado:

```bash
python3 black_obj_v3.py substituir \
  --original bg1_pst_original.db \
  --obj exemplos/cubo_v3.obj \
  --saida bg1_pst_v3_cubo.db \
  --ajustar
```

Instale o APK atualizado do CI Android. Com depuração USB autorizada:

```bash
adb devices
adb shell am force-stop com.racas.black
adb shell mkdir -p /sdcard/Black
adb push bg1_pst_v3_cubo.db /sdcard/Black/bg1_pst.db
```

O nome externo precisa ser **bg1_pst.db**, mesmo que a saída no computador tenha
outro nome. Abra o jogo normalmente com a ISO que já usa e equipe a pistola.
Para outro modelo, repita a conversão e a transferência, fechando o processo antes.
Não há recarregamento garantido de modelos durante uma missão já em execução.

## Ordem de prioridade

1. `/sdcard/Black/modelos/levels/level_00/fpguns/bg1_pst.db` para a fase correspondente.
2. `/sdcard/Black/bg1_pst.db` como substituição geral de cada recurso de pistola solicitado.
3. Recurso original na ISO ou na pasta extraída selecionada pelo port.

O caminho específico acompanha o caminho solicitado pelo jogo, como `level_01`,
`level_03`, etc. Para o primeiro teste use só o arquivo geral. A detecção do recurso
aceita maiúsculas/minúsculas do pedido, mas as pastas externas específicas devem
usar os nomes minúsculos mostrados acima.

O arquivo externo deve existir, ser regular e ter o mesmo tamanho do recurso
original que o jogo solicitou. Arquivo ausente ou com tamanho diferente não é usado;
o carregamento volta à próxima opção. Não cria recursos que não existam na fonte
original. Igualdade de tamanho não valida toda a estrutura: use um DB produzido
pelo conversor e conserve seu relatório JSON. Os offsets e atributos do protótipo
V3 continuam sujeitos às limitações descritas em `LEIA-PRIMEIRO.md`.

Para desativar a substituição geral preservando o arquivo:

```bash
adb shell am force-stop com.racas.black
adb shell mv /sdcard/Black/bg1_pst.db /sdcard/Black/bg1_pst.db.desativado
```

Se colocou arquivos específicos por fase, desative-os também. Remover somente
`model_trace.txt` desativa a captura de leituras, **não** a substituição externa.

## Confirmar o que foi carregado

O APK do CI também contém o diagnóstico optativo. Ative o marcador:

```bash
printf 'diagnostico-v3\n' > model_trace.txt
adb push model_trace.txt /sdcard/Black/model_trace.txt
adb shell am force-stop com.racas.black
adb logcat -T 1 -v threadtime 'BLACK:I' '*:S' > log-v3-externo.txt
```

Abra o jogo, entre na missão e equipe a pistola. Depois Ctrl+C no terminal.

Procure:

```text
MODELO fonte_selecionada=... diagnostico=ativo
MODELO override_ativo recurso=levels/.../fpguns/bg1_pst.db externo=/sdcard/Black/bg1_pst.db tamanho=741896
MODELO recurso_completo caminho=... tamanho=741896 crc32=b71f6444
```

O CRC32 acima é o exemplo do cubo distribuído com estas ferramentas. Para outro
OBJ, use o valor do JSON gerado. `override_ativo` comprova a escolha do arquivo
externo durante a resolução; `recurso_completo` comprova a cobertura integral das
leituras bem-sucedidas desse conteúdo. Não confunda a seleção com o efeito visual.
O nome em `fonte_selecionada` continua sendo o da ISO original, porque só o DB foi
substituído. O caminho lógico revela qual pasta de fase foi pedida.

Verificação automática, ajustando o nome real da ISO e o caminho mostrado no log:

```bash
python3 verificar_log_v3.py \
  --log log-v3-externo.txt \
  --db bg1_pst_v3_cubo.db \
  --iso 'Black (Europe) (En,Fr,De,Es,It).iso' \
  --caminho levels/level_00/fpguns/bg1_pst.db
```

Se houver apenas leitura parcial, não será impresso CRC32 de arquivo completo.
Isso é inconclusivo para o hash integral e não significa automaticamente ausência
da substituição. O relatório da V3 explica o restante da cadeia de evidências.

## Implementação e testes

A resolução do arquivo externo ocorre no ponto comum `Resolve` do runtime, antes
de retornar a entrada da ISO. Assim, abertura, consulta de tamanho e leituras
através dessa resolução enxergam o mesmo recurso externo. Abrange também a seleção
de diretório extraído. Não modifica os bytes da ISO nem aplica uma conversão OBJ
em cada frame. Só verifica candidatos para o nome de pistola permitido.

O SDK baixado permanece intacto: CMake gera uma cópia do seu arquivo de resolução,
com pontos de alteração validados. A opção `BLACK_MODEL_OVERRIDES` é ON por padrão;
a captura de leituras é independente (`BLACK_TRACE_MODELS`). As quatro combinações
foram testadas na configuração CMake. Não é preciso marcador para usar o override.

Testes C++ passaram para prioridade específica/geral, retorno ao original,
recusa de tamanho incompatível, nomes com diferença de caixa, recusa de traversal,
filtro de recursos, logs limitados e troca da pasta raiz. A compilação sintática
ARM64 do resolvedor gerado e das fontes novas passou. Confirmação visual no Android
continua pendente; o carregador externo não resolve sozinho as dúvidas do shader
e das transformações do modelo.
