# OBJ direto na pasta Black

O APK atualizado inclui uma porta nativa do conversor V3. Você pode enviar o OBJ
para `/sdcard/Black/bg1_pst.obj`: o port lê o DB original da ISO, converte o modelo
uma vez, valida o resultado e carrega o DB gerado em cache. Não requer Python no
Android, reconstrução de ISO ou conversão em cada frame.

## Usar

Instale o APK mais recente do CI que inclui **OBJ nativo**. Builds anteriores
podem conter somente o carregamento de DB externo. Mantenha a ISO original e use
um OBJ com faces simples e geometria dentro da capacidade da V3.

Para o cubo incluído:

```bash
adb devices
adb shell am force-stop com.racas.black
adb shell mkdir -p /sdcard/Black
adb push exemplos/cubo_v3.obj /sdcard/Black/bg1_pst.obj
```

Abra o jogo normalmente e equipe a pistola. Para um OBJ próprio:

```bash
adb shell am force-stop com.racas.black
adb push meu_modelo.obj /sdcard/Black/bg1_pst.obj
```

O nome no telefone precisa ser `bg1_pst.obj`. Feche o processo antes de substituir;
não há atualização garantida de uma arma já carregada em uma missão em andamento.
O port ajusta automaticamente posição/escala para a região Pistol_Root, como
`--ajustar` no conversor Python. Não lê texturas/MTL novos; conserva o material do jogo.

## Prioridade de carregamento

1. OBJ específico: `/sdcard/Black/modelos/levels/level_00/fpguns/bg1_pst.obj`.
2. OBJ geral: `/sdcard/Black/bg1_pst.obj`.
3. DB específico: `/sdcard/Black/modelos/levels/level_00/fpguns/bg1_pst.db`.
4. DB geral: `/sdcard/Black/bg1_pst.db`.
5. Recurso original da ISO/pasta selecionada.

O caminho específico acompanha a fase realmente solicitada. Cada OBJ precisa ser
aceito pela conversão para ganhar prioridade. OBJ inválido ou conversão recusada
permite tentar o próximo candidato e, depois, DB manual/original. Se você mantiver
um DB manual antigo, ele poderá aparecer nesse fallback; confira o log antes de
concluir que o OBJ foi usado. Nada sobrescreve a ISO ou o OBJ original.

Para desativar o OBJ geral preservando uma cópia:

```bash
adb shell am force-stop com.racas.black
adb shell mv /sdcard/Black/bg1_pst.obj /sdcard/Black/bg1_pst.obj.desativado
```

Desative também variantes específicas e DBs manuais se quiser o modelo original.

## Limites atuais

- Somente a pistola `bg1_pst.db` cujo DB original tenha SHA-1
  `5cbdb886f7f18306d815af5364d824b10bfeb1f0`. É o mesmo original da V3 Python com
  SHA-256 `df87876f470d025644e538b64ff1a827bb63ec13e9c251fcabd8797441b7d3ea`.
- O original vem da ISO/pasta fonte, não de um DB externo já modificado. Uma ISO
  com a banana embutida não satisfaz esse requisito para conversão automática.
- Até 3687 vértices efetivos e 5905 índices; strips de ligação limitam a 1181
  triângulos. UVs e normais podem dividir vértices; contagem do Blender não é
  necessariamente a contagem final. OBJ de até 4 MiB, até 50000 posições/UVs/normais
  de entrada e no máximo 128 cantos por face.
- Polígonos planos simples, inclusive côncavos, são triangulados. Faces degeneradas,
  auto-interseções, índices inválidos e múltiplos materiais são recusados.
- Conserva mãos/braços e vincula o objeto inteiro ao Pistol_Root original. Não
  importa rig nem recria peças animadas de carregador/ferrolho.
- Sem UV usa zeros; sem normais calcula normais por face. O codec de normais segue
  a interpretação experimental descrita no relatório V3.

Portanto, isto acrescenta a conveniência de entrada OBJ à V3 existente; não remove
seus bloqueios de formato nem habilita personagens arbitrários.

## Cache e evidência do carregamento

O cache fica em `modelos-cache` na pasta própria do aplicativo (`app::ExeDir`),
separado do OBJ e da ISO. Sua chave inclui revisão do conversor e hashes SHA-1 do
DB original e do conteúdo OBJ. Arquivo alterado gera outra chave; na mesma sessão
há memoização por recurso, tamanho e data do OBJ. Reiniciar o processo força a
conferência do conteúdo. O mesmo OBJ global não autoriza outra fase sem conferir
o DB original dessa fase.

Cada cache contém `.db`, `.sha1` e `.json` com hashes, contagens e limitações.
Antes de reutilizar um cache persistente, seu tamanho e hash são conferidos; cache
corrompido é regenerado. Se a leitura/conversão/gravação falhar, o erro é registrado
e o recurso original ou o DB manual continua como fallback. Não há promessa de
recarga a quente enquanto a missão já estiver usando o modelo.

No log com tag BLACK:

```text
MODELO obj_convertido arquivo=/sdcard/Black/bg1_pst.obj cache=... vertices=24 triangulos=12 indices=58 ...
MODELO obj_cache_validado arquivo=... cache=...
MODELO obj_recusado arquivo=... motivo=...
MODELO override_ativo recurso=levels/.../fpguns/bg1_pst.db externo=.../modelos-cache/...db tamanho=741896
```

`obj_convertido` informa a geração; `obj_cache_validado`, a reutilização validada.
`override_ativo` informa qual DB foi selecionado. Para comprovar também as leituras,
ative `model_trace.txt` como no guia da pasta Black e colete desde o início:

```bash
printf 'diagnostico-v3\n' > model_trace.txt
adb push model_trace.txt /sdcard/Black/model_trace.txt
adb shell am force-stop com.racas.black
adb logcat -T 1 -v threadtime 'BLACK:I' '*:S' > log-obj-direto.txt
```

Abra o jogo, entre na missão, equipe a arma e depois Ctrl+C no terminal. O cubo
produz o mesmo DB do desktop, CRC32 `b71f6444`. Procure `recurso_completo` com esse
CRC32 para confirmar leitura integral. Uma leitura parcial não gera falso hash de
arquivo completo. Se a conversão for recusada, envie o trecho `obj_recusado`.

## Validação realizada

O conversor C++ gerou DB **idêntico byte por byte à V3 Python** no exemplo de cubo,
com 24 vértices, 12 triângulos e 58 índices. Testes nativos cobrem determinismo,
triangulação côncava, índices negativos, UV/normais, rejeição de OBJ malformado,
capacidade, original não homologado, prioridade OBJ/DB, cache persistente,
corrupção do cache, memoização, original diferente em outra fase e fallback.
O JSON do cache foi lido e validado por Python nos testes.

A compilação sintática ARM64 e a configuração do resolvedor foram verificadas.
A build completa do APK deve ser conferida no Actions. **Ainda não foi confirmado
no telefone que o objeto substitui visualmente a pistola.** Essa confirmação
continua sendo o último elo da cadeia de evidências, mesmo com conversão e leitura
binária corretas.
