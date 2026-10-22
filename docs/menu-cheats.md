# Menu de cheats no Android

Abra **Pausa → CHEATS**. O menu usa fundo preto, bordas e título em laranja,
categorias na coluna esquerda e opções na direita. Arraste pelo título ou
redimensione pelo canto. As listas têm rolagem quando não cabem na tela.
O botão **Voltar a pausa** fecha o painel; depois retome a missão no menu do jogo.

| Categoria | Opções |
| --- | --- |
| Jogador | Invencibilidade e matar com um tiro |
| Armas | Munição infinita, granadas infinitas, sem recuo, cadência de 1x a 4x, mira de ferro e registro da AK47 |
| Jogo | Velocidade de 25% a 200%, liberar todas as missões e liberar Black Ops |
| Câmera | Câmera livre/noclip, velocidade do voo, ajuste de câmera e ocultar HUD |
| ESP | Ativar leitura, linhas, caixas, altura estimada da caixa, pontos e desligar ESP |
| Diagnóstico | Ativar leitura de NPCs, exibir contadores na missão e conferir a lista |

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
