# Cheats e painel lateral de câmera

## Uso durante a missão

1. Abra o menu de pausa pelo botão Voltar do Android ou Start do controle.
2. Selecione **CHEATS**, na própria lista da pausa.
3. Escolha **Camera livre** ou **Ajuste de camera**.
4. Toque em **Voltar a pausa** e selecione a opção normal de continuar a missão.

Ao ativar **Ajuste de camera**, um painel pequeno do ImGui aparece na lateral assim que a partida é retomada. Os ajustes têm efeito durante o jogo, sem precisar abrir e fechar a pausa a cada mudança.

Ativar uma opção por esse painel desativa a outra: a câmera livre usa movimento independente, enquanto o ajuste de câmera acompanha o jogador. O estado de ativação vale para a sessão; o aplicativo não inicia com um painel sobre a partida.

## Controles do painel

Na aba **Camera**:

- **Lateral** desloca o ponto de vista para os lados.
- **Altura** sobe ou desce o ponto de vista.
- **Distancia** desloca o ponto de vista para frente ou para trás, conforme o eixo da visão do jogador.
- **Campo de visao** ajusta o FOV de 45 a 140 graus.
- **Restaurar camera** retorna os deslocamentos a zero e o FOV a 70.

Os deslocamentos são visuais e acompanham a orientação do jogador. Não teletransportam o personagem nem mudam sua colisão física. Deslocamentos grandes podem colocar a visão dentro de objetos do cenário.

Na aba **AK47 (ADS)**, é possível ligar o ADS, registrar a arma equipada e ajustar a posição e a rotação do modelo. Equipe a AK-47 antes de registrá-la. Use o botão normal de mirar para observar os ajustes; os controles de jogo fora do painel continuam disponíveis. Consulte também [Mira de ferro da AK-47](ads-ak47.md).

- **Recolher** reduz o painel a um botão pequeno. **Ajustar camera** expande novamente.
- **Mover p/ direita/esquerda** troca a borda ocupada pelo painel.
- **Salvar perfil** salva o perfil de câmera e os ajustes do ADS em `black.toml`.
- **Desativar** desliga o ajuste e remove o painel. Para reativar, abra novamente **Pausa → Cheats**.

Os controles de atirar, mirar e movimentar o jogador continuam funcionando fora da área ocupada pelo painel. Um toque iniciado no painel permanece nele até ser solto, mesmo que o dedo saia dessa área. Outro dedo pode continuar usando os controles do jogo. Para mouse, o cursor fica disponível enquanto o painel está visível; o movimento fora do painel continua sendo encaminhado ao jogo.

O painel de ajustes fica oculto durante a pausa, nas opções tradicionais e quando não há uma missão em andamento. A seção Cheats é exibida sobre a pausa e pode ser fechada pelo botão de voltar ou por Escape/Voltar do Android.

## Implementação

A opção **CHEATS** reutiliza o botão já inserido na lista original de pausa. A seção e o editor são desenhados com o ImGui já usado pelo projeto; não há dependência nova de interface.

O ajuste de posição usa os eixos da matriz do jogador em `+0xD0`, `+0xE0` e `+0xF0` e o campo de olhos em `+0x100`, identificados no executável americano. A posição original é restaurada antes da atualização seguinte, evitando acumular o deslocamento. O FOV usa o hook existente. A câmera livre recebe solicitações de ativação pela interface, aplicadas na thread do jogo.

Os eventos são separados antes dos controles de toque. A escolha do destino permanece válida até soltar o dedo ou o botão, evitando disparos e giros involuntários durante o ajuste.

A implementação segue o fluxo solicitado pelo usuário. O site de referência da MixMods respondeu HTTP 403 durante a consulta; não foi possível conferir os detalhes daquele mod.

## Validação

```sh
bash tools/test_camera_tools.sh /caminho/para/XboxXBE-Recomp /caminho/para/imgui /caminho/para/SDL
bash tools/test_free_camera.sh /caminho/para/XboxXBE-Recomp
bash tools/test_ads_ak47.sh /caminho/para/XboxXBE-Recomp
```

O teste do painel usa o ImGui real, sem GPU, e memória simulada. Verifica o fluxo pausa/missão, a área lateral, o isolamento de toques e mouse, a atualização/restauração da câmera e a persistência do perfil.

Ainda é necessário conferir no APK o enquadramento dos ajustes, a usabilidade dos controles no dispositivo e o retorno após pausa, morte, troca de missão e segundo plano. Os testes locais não substituem essa validação dentro do jogo.
