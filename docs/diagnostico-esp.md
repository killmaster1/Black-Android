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
Ainda não há classificação por equipe nem teste de oclusão nesta etapa. A atualização pela lista aplica um filtro experimental de vida descrito abaixo.
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

## Atualização contínua pela lista de personagens

O primeiro teste visual confirmou pontos próximos à base dos NPCs, mas as amostras
ficavam laranja ao parar a consulta de mira. A versão seguinte lê a lista de personagens
na atualização da câmera, mantendo a consulta antiga apenas como alternativa de diagnóstico.

Referências conferidas no executável americano:

- `0x1CDA30`: lê o gerenciador em `0x2D1980`, a capacidade em `+0x79B4` e o vetor em `+0x79BC`.
- `0x1E6860`: compara a capacidade à ocupação em `+0x79B8`.
- `0x1CDDE0`: identifica tipo de personagem `1` em `+0xC4` e consulta morte em `+0x384`.
- Os hooks de dano já usam vida em `+0x2F0`; o candidato deve ter vida positiva, morte zero e IA em `+0x39C`.

A leitura limita a capacidade a 128 e verifica os intervalos antes de ler o vetor e os objetos.
Somente os endereços da lista atual são lidos; não se percorrem os endereços guardados pela consulta antiga.
Os pontos da lista são substituídos a cada captura e não ficam guardados por 120 quadros.
A validade também vence se a captura deixar de atualizar. Nenhum dado do jogo é escrito.

O HUD mostra **Fonte: lista de NPCs** e **Vivos (teste)** quando a estrutura é válida.
Esses marcadores têm prefixo `N` e devem permanecer amarelos com o NPC parado,
mesmo sem mover a mira. Os números seguem as posições do vetor e podem mudar ao remover personagens.
Se a estrutura estiver indisponível, aparece **Fonte: consulta da mira** e os pontos
`C` antigos mantêm a distinção amarelo/laranja e sua expiração de 120 quadros.
Os contadores **Recentes/Registrados/Deteccoes da mira** continuam exclusivos da consulta antiga.

Teste no aparelho: mantenha um NPC parado em vista por alguns segundos sem mover a mira;
confira o marcador `N`, mova a câmera e depois elimine o NPC para verificar sua remoção.
Envie uma captura incluindo a linha **Fonte**. O filtro de vida e a cobertura da lista
precisam de validação no jogo real; aliados ainda podem ser incluídos.

## Correção da interpretação do vetor (Lista v2)

A primeira leitura contínua tinha capacidade e ocupação invertidas, e rejeitava
listas válidas. Os testes anteriores também reproduziam essa hipótese incorreta.
A inicialização real em `0x1E6BAB` escreve **16** em `+0x79B4` e **zero** em
`+0x79B8`, depois zera os 16 slots apontados por `+0x79BC`.
Assim, `+0x79B4` é a capacidade e `+0x79B8` é a ocupação.
A comparação usada antes não permitia deduzir qual campo representava cada valor.

A leitura corrigida percorre a capacidade, pulando slots nulos, como faz a consulta
original. Um NPC pode ocupar um slot posterior mesmo com ocupação igual a um.
Os testes agora incluem o estado inicial real e um NPC no último slot do vetor,
além da atualização sem novas consultas de mira e da remoção por morte.

O HUD mostra **Lista v2**, o estado da leitura e **Uso: ocupação/capacidade**.
Se aparecer **Fonte: consulta da mira**, a linha seguinte identifica o motivo:
gerenciador indisponível, cabeçalho ilegível, limites inválidos, vetor ilegível ou personagem ilegível.
Quando a leitura funcionar, deve aparecer **Fonte: lista de NPCs** e **Lista v2: OK**.
O teste no aparelho continua necessário; a correção dos campos não comprova a cobertura
nem a classificação por equipe de todos os personagens.
