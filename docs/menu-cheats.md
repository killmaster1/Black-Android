# Menu de cheats no Android

Durante a missão, toque no botão **MOD** ao lado da pausa. Ele acompanha a posição
do botão de pausa configurada no layout de toque. A ação fica congelada e o menu
aparece sobre a imagem da missão, sem abrir a página de pausa do jogo.
Use **Continuar missao**, o botão MOD ou Voltar do Android para fechar e retomar.
Também continua disponível o caminho **Pausa → CHEATS**. O menu usa fundo preto, bordas e título em laranja,
categorias na coluna esquerda e opções na direita. Arraste pelo título ou
redimensione pelo canto. As listas têm rolagem quando não cabem na tela.
Ao entrar pelo caminho antigo, o botão **Voltar a pausa** mantém o comportamento anterior.

| Categoria | Opções |
| --- | --- |
| Jogador | Invencibilidade e matar com um tiro |
| Armas | Munição infinita, granadas infinitas, sem recuo, cadência de 1x a 4x, mira de ferro e registro da AK47 |
| Jogo | Velocidade de 25% a 200%, liberar todas as missões e liberar Black Ops |
| Câmera | Câmera livre/noclip, velocidade do voo, ajuste de câmera e ocultar HUD |
| ESP | Ativar leitura, linhas, caixas, vida, contador e direções fora da tela, altura estimada, pontos e desligar ESP |
| Diagnóstico | Ativar leitura de NPCs, exibir contadores na missão e conferir a lista |
| NPCs | Observar criações naturais e conferir os perfis usados pela missão; criação manual ainda em estudo |

Os cheats da página original de opções usam os mesmos valores deste painel.
Salvar perfil registra os cheats persistentes, o perfil de câmera e as opções de linha,
caixa, altura estimada e contadores. A câmera livre sempre inicia desligada.
A leitura isolada do diagnóstico, os pontos e a opção de ocultar o HUD são controles da sessão.
O registro da arma exige que a AK47 esteja equipada. A liberação de missões aparece
na tela de seleção, sem trocar a missão em andamento.

## ESP Linha e Caixa

Marque **ESP Linha** ou **ESP Caixa** e retome a missão. As linhas partem da região
inferior central até os pés do personagem. As caixas usam a posição dos pés e uma
altura estimada no eixo vertical do mundo; ajuste o controle de altura se necessário.
A largura é proporcional à altura projetada. Não são caixas de colisão nem uma
leitura dos ossos, e a altura não se adapta automaticamente ao agachamento.
Pontos fora da tela ou atrás da câmera são descartados. Uma caixa que cruza o limite
da tela pode deixar de ser desenhada.

O desenho usa a lista de NPCs que foi validada pelo usuário no aparelho.
Há filtro experimental de morte/vida, mas ainda não há filtro de equipe; aliados
podem ser marcados. As sobreposições não fazem teste de oclusão e podem aparecer
através do cenário. Se a lista falhar, o diagnóstico informa o motivo; linhas e caixas
não usam as posições antigas da consulta da mira.

Os contadores do teste ficam ocultos por padrão. Ative **Diagnóstico → Exibir contadores
na missão** para voltar a vê-los. Desmarque **ESP → Pontos dos personagens** se preferir
somente linhas e caixas.

## Validação

```sh
bash tools/test_camera_tools.sh out/check-deps/sdk
bash tools/test_esp.sh out/check-deps/sdk
```

Os testes desenham o menu com ImGui real, verificam arraste, isolamento de toques,
restauração das cores, desenho com contadores ocultos, projeção de caixas, leitura da
lista, vida/morte e atualização sem depender da consulta da mira.
O alinhamento das novas caixas e a operação do menu por toque ainda precisam de teste no APK.

## Vida e NPCs fora da tela

Em **ESP**, marque **Mostrar vida** para exibir o valor atual lido do personagem,
sem convertê-lo em porcentagem. **Contador e direcoes fora da tela** mostra um
contador vermelho abaixo do botão MOD e linhas vermelhas com setas nas bordas,
incluindo direções para NPCs atrás da câmera.
As opções são independentes de ESP Linha/Caixa e são salvas pelo perfil.

O contador considera NPCs vivos da lista carregada, podendo incluir aliados.
Ele indica personagens fora do campo de visão, usando pés, tronco e cabeça estimados
para reduzir a contagem de personagens parcialmente visíveis. Não mede oclusão:
um NPC atrás de uma parede, mas dentro da imagem projetada, não entra nesse contador.
Não representa inimigos futuros ainda não carregados ou todos os inimigos da missão.
Se a lista ou a câmera não estiverem disponíveis, mostra `--`.

## Congelamento do menu MOD

A apresentação repete a mesma imagem enquanto o menu direto está aberto, sem
retornar ao processamento do próximo quadro do jogo. O ImGui continua recebendo
eventos e sendo desenhado; a saída de áudio é suspensa durante esse intervalo.
Ao abrir e fechar, os gestos de jogo são limpos para evitar tiro ou movimento preso.
Os toques fora do painel também são consumidos enquanto ele está aberto.
A perda de foco mantém o tratamento de suspensão do Android.

Os testes locais verificam o modal independente da pausa, os toques, as direções
para pontos fora da imagem e atrás da câmera e a cópia do valor da vida.
A apresentação repetida, a retomada do áudio e o ciclo de foco precisam ser
verificados no aparelho com o APK completo.

A investigação de spawn está documentada no [guia de criação de NPCs](criacao-npcs.md).
