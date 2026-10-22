# Diagnóstico inicial de ESP

Este protótipo não desenha linhas ou caixas e não conta todos os inimigos da missão.
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
Não chamamos consultas adicionais, não escrevemos na entidade e não voltamos a acessar os endereços armazenados.
A interface recebe contadores protegidos por mutex; apenas a thread do jogo lê a memória do Xbox.

Próximas etapas: validar as detecções em uma missão, encontrar uma enumeração completa
com filtro de equipe/vida e confirmar a projeção da câmera antes de desenhar pontos.

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
