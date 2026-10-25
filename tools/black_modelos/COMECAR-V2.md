# V2 experimental — começar pelo relatório

Leia `RELATORIO-V2.md` antes de testar. A V1 permanece disponível nos comandos
`analisar`, `exportar` e `importar`. Esta etapa acrescenta topologia nova em uma
malha de passo 24, dentro da reserva existente. Não implementa realocação ou skinning.

Use Python 3 no Linux Mint. Descompacte o pacote e abra o terminal nessa pasta.
Os comandos abaixo supõem uma cópia do DB original `bg1_pst_original.db` nessa pasta.
Ele precisa ser o original de 741896 bytes examinado no relatório.

```sh
# Conferir a estrutura reconhecida; ordem das malhas começa em zero.
python3 black_obj.py analisar bg1_pst_original.db

# Conferir os testes locais sem modificar o original.
python3 testar_v2.py bg1_pst_original.db

# Gerar uma saída de teste com topologia nova, sem sobrescrever arquivos.
python3 black_obj.py substituir bg1_pst_original.db exemplos/cubo_pst_malha0.obj bg1_pst_cubo.db --malha 0
```

O resultado esperado é 8 vértices usados, 12 triângulos e 58 índices. As reservas
continuam no DB e seu tamanho permanece 741896 bytes. Apenas a malha 0 muda.
Os UVs do cubo são intencionalmente constantes: o teste não avalia texturização.
O material antigo permanece, e a iluminação usa atributos emprestados do original.

Para o teste visual, substitua o recurso correspondente por `bg1_pst_cubo.db`,
renomeado como `bg1_pst.db`, **numa cópia** da estrutura extraída. Mantenha o WDD
original. Reconstrua outra XISO pelo procedimento que já funcionou. Não altere a
ISO original. Teste a arma parada, tiro, recarga e troca, observando quando o cubo
aparece. A malha 0 ainda não foi identificada como sendo a pistola inteira.
Não é necessário recompilar o APK para esse teste dos dados.

Para reproduzir a auditoria da banana enviada:

```sh
python3 auditar_db.py bg1_pst_original.db bg1_pst_banana.db
```

A auditoria classifica alterações V1; uma topologia V2 altera também índices e
contador, portanto aparecerá como alteração fora de XYZ/caixas nesse relatório.

O pacote contém somente código, documentação, resultados da auditoria e o cubo
sintético. Não inclui os DBs nem a ISO. A V2 ainda não foi executada no jogo.
