# Diagnóstico inicial de ESP

Este protótipo desenha pontos experimentais, não desenha linhas ou caixas e não conta todos os inimigos da missão.
Ele registra os candidatos com IA encontrados pela consulta original da assistência de mira.
Aliados também podem aparecer; a classificação por equipe e a identificação de mortos ainda não foram validadas.

## Como testar no Android

1. Entre em uma missão, pause e abra **Cheats**.
2. Ative **Diagnostico ESP** e retome a missão.
3. Movimente-se e olhe na direção de inimigos próximos. A consulta original depende das entradas de movimento/mira e tem alcance limitado.
4. Observe os contadores em tempo real no topo da tela, abaixo da fileira de controles perto da pausa no layout padrão. Envie uma captura com inimigos à vista. Os valores também continuam disponíveis em **Cheats**.
5. Repita após eliminar um inimigo ou olhar para uma área vazia por alguns segundos.

O conjunto mantém até 128 endereços distintos, com validade de 120 quadros do jogo.
O contador de detecções inclui repetições do mesmo personagem. Não indica inimigos únicos.
Os contadores são limpos ao mudar de mundo, sair da missão ou ligar/desligar o diagnóstico.
O diagnóstico começa desligado e não é salvo no perfil.

## Referência técnica

A análise de `loader/src/input/autoaim.cpp` no projeto
[Spark198rus/black-pc](https://github.com/Spark198rus/black-pc/blob/d6a86e5571e8bc4b7b3f8fd483ba2f122e888113/loader/src/input/autoaim.cpp)
orientou a investigação. Conferimos o trecho no executável americano fornecido:
`0x1BD3B3` lê entidade+`0x39C`, testa a presença de IA e desvia quando ausente.
O hook em `0x1BD3BD` observa EDX apenas no caminho com IA.
Não chamamos consultas adicionais, não escrevemos na entidade e não voltamos a acessar os endereços armazenados. A posição é copiada no próprio callback original.
A interface recebe contadores protegidos por mutex; apenas a thread do jogo lê a memória do Xbox.

Próximas etapas: validar o alinhamento dos pontos em uma missão e encontrar uma enumeração completa
com filtro de equipe/vida antes de implementar caixas e linhas.

Validação local do contador:

```sh
c++ -std=c++20 -pthread -Isrc tests/esp_diagnostic_test.cpp -o /tmp/esp_diagnostic_test
/tmp/esp_diagnostic_test
```

## Correção após o primeiro teste no celular

As capturas confirmaram chamadas da consulta, mas mostraram zero candidatos recentes.
O contador de quadros usado pela renderização não garantia a suspensão da expiração no menu.
Agora o contexto da pausa suspende explicitamente o relógio do diagnóstico.
A interface também mostra quantos endereços estão registrados no conjunto limitado a 128,
mesmo quando já expiraram. Esse número pode incluir endereços reutilizados e personagens
que morreram; não é uma contagem de inimigos vivos nem um total único da missão.

O texto durante a missão não recebe toques nem abre o cursor. Ele aparece enquanto
**Diagnostico ESP** estiver ligado e desaparece durante a pausa, fora da missão ou
ao desligar a opção. A posição é fixa e não acompanha personalizações do botão de pausa.

## Teste visual com pontos

1. Em **Pausa → Cheats**, ative **Diagnostico ESP** e mantenha **Pontos de teste** marcado.
2. Retome a missão e aproxime-se de um personagem. Mova-se e olhe para ele.
3. Procure um ponto com identificador `C1`, `C2` etc. na base do personagem.
4. Envie uma captura com o ponto e o personagem visíveis; depois repita olhando para cima/baixo e para os lados.
5. Repita com câmera livre ou FOV modificado para verificar se o ponto continua alinhado.

Amarelo indica uma amostra de até dois quadros. Laranja indica uma posição mais antiga,
que pode ficar parada até expirar em 120 quadros. O ponto não acompanha automaticamente
um candidato que saiu da consulta, e pode permanecer brevemente após sua morte.
O identificador representa uma posição no conjunto do diagnóstico e pode ser reutilizado.
Não há classificação por equipe, vida ou visibilidade através de paredes nesta etapa.
A opção **Pontos de teste** permite ocultar os marcadores mantendo os contadores.
O HUD informa **Pontos na tela** e se a matriz de câmera está disponível.

O callback lê a translação da transformação do personagem em `+0xA0` (transformação em
`+0x70`, também inicializada em `0x1E03D0`). O hook `0x20359F`, imediatamente após
`0xD2F40`, copia a matriz de projeção normalizada em `display+0xD450`.
`0x206A0` compõe suas três primeiras colunas como X, Y e profundidade homogêneos:
`X/profundidade` e `Y/profundidade` são coordenadas de imagem normalizadas.
As quatro componentes não utilizadas não participam da validação nem da projeção.
A matriz incorpora a câmera atual, sem reconstruir a projeção por um FOV presumido.
A correspondência visual ainda precisa ser validada no aparelho.

A thread de interface recebe somente cópias de posições e matriz, protegidas por mutex.
As páginas de memória são verificadas antes das novas leituras e nenhum personagem é alterado.
Posições não finitas, pontos atrás da câmera e pontos fora da imagem são descartados.

```sh
bash tools/test_esp.sh out/check-deps/sdk
```
