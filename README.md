# Black para Android

Adaptação nativa não oficial da versão original de *Black* para Xbox, com controles por toque, mira por giroscópio, suporte a controles e opções gráficas adicionais.

O código x86 do jogo é recompilado antecipadamente para C++ com o [XboxXBE-Recomp](https://github.com/TGP482/XboxXBE-Recomp) e depois compilado para arm64. A renderização usa Vulkan; janela, controles e áudio usam SDL3.

> [!NOTE]
> O projeto não inclui os arquivos do jogo. Para instalar ou compilar, use os arquivos da sua própria cópia adquirida legalmente.

<p align="center">
  <a href="https://youtu.be/x5bczwTuyhw">
    <img src="https://img.youtube.com/vi/x5bczwTuyhw/maxresdefault.jpg" alt="Black para Android em execução no POCO F6" width="720">
  </a>
  <br>
  <sub>▶ Jogo no POCO F6, configurações máximas e mira por giroscópio</sub>
</p>

## Instalação

1. Instale o APK.
2. Abra **Black Android** e autorize o acesso a todos os arquivos. No Android 10, autorize o acesso ao armazenamento.
3. Copie a versão americana ou europeia do jogo para `/storage/emulated/0/Black` ou para uma pasta `Black` na raiz do cartão SD. São aceitos uma imagem de disco (`.iso`) ou `default.xbe` acompanhado das pastas de dados. O aplicativo cria a pasta no armazenamento interno após receber a permissão e inicia o jogo quando encontra os arquivos.

Sem essa permissão, também é possível usar `Android/data/com.racas.black/files/assets`.

As configurações (`black.toml`, `android.toml`, `layout.toml`), os jogos salvos (`hdd`), os registros e o cache de shaders ficam em `Android/data/com.racas.black/files`.

## Recursos

As configurações ficam no menu de opções do jogo, nas páginas **Graphics** (gráficos), **Touch Controls** (controles por toque), **General** (geral), **Mouse and Keyboard** (mouse e teclado) e **Cheats** (trapaças). As setas acima e abaixo da lista permitem rolar pelas opções. Nas opções acessadas durante uma missão, a linha **Page** troca de página. O botão Voltar do Android abre o menu de pausa.

Os nomes originais dos menus são mantidos nesta documentação para facilitar sua localização no aplicativo. As opções adicionadas por este mod aparecem em português.

### Gráficos

- **Render Resolution**: resolução de 20% a 200% da altura da tela, com 67% como padrão, em vez dos 640×480 originais.
- **Field of View**: campo de visão horizontal entre 45 e 140 graus; o padrão do jogo é 70.
- **Viewmodel Field of View**: campo de visão da arma e dos braços em primeira pessoa.
- **FMV Scale**: a opção **Fill** preenche a tela com os vídeos 4:3, cortando as bordas superior e inferior; **4:3** preserva a proporção original.
- **V-Sync**: sincronização vertical, desativada por padrão.
- **Max Frame Rate**: taxa de quadros ilimitada, limitada à frequência da tela ou a um valor personalizado, substituindo o limite original de 30 FPS.
- **FPS Counter**: contador de quadros por segundo.
- **Anisotropic Filtering**: filtragem anisotrópica em todas as texturas.
- **Draw Distance**: distância de renderização e níveis de detalhe multiplicados por 2, 4 ou 8.
- **Reload Blur**: ativa ou desativa o desfoque ao recarregar.

### Controles por toque

- O analógico à esquerda movimenta o jogador; arrastar à direita gira a visão.
- Os botões na tela permitem atirar, mirar, recarregar, interagir, atacar, lançar granadas, agachar, trocar de arma e usar o silenciador. Arrastar a partir de um botão de tiro permite girar a visão enquanto atira.
- Os controles somem durante carregamentos e cenas cinematográficas.
- Nos menus, as setas e o botão Voltar permitem navegar e selecionar opções. Toque em uma seta para avançar uma linha ou segure para continuar.
- **Touch Sensitivity** ajusta a sensibilidade do toque separadamente da sensibilidade do mouse.
- **Controls Layout** oferece disposições para dois, três ou quatro dedos, além de ajustes de opacidade e tamanho.
- **Edit Controls Position** permite arrastar e redimensionar os controles usando os botões **− / +** ou o gesto de pinça.
- **Motion Controls** permite mirar com o giroscópio do celular ou de um DualShock 4/DualSense no Android 12 ou superior. O giroscópio pode atuar sempre ou apenas ao atirar, com sensibilidade ajustável.

### Outros dispositivos de entrada

- **Controle**: compatível com os controles reconhecidos pelo SDL3, incluindo vibração. Os botões de toque somem durante o uso do controle. Também são aceitos dispositivos que alguns celulares apresentam como controles virtuais.
- **Stick Deadzone and Sensitivity**, em **Touch Controls → Controller**, ajusta a zona morta radial entre 0% e 50% e a sensibilidade entre 0,1 e 3,0 para cada analógico. O valor 0% preserva a zona morta do jogo.
- **Teclado e mouse**: permitem remapear as teclas, ajustar a sensibilidade e inverter a visão. Durante a partida, o mouse fica capturado e pode girar a câmera continuamente.

### Opções gerais

- **Language**: inglês, francês, alemão, italiano ou espanhol, conforme os arquivos do disco, e indonésio para os textos próprios da adaptação.
- **Skip Intros**: pula as apresentações iniciais.
- **Audio Output**: saída automática, mono, estéreo ou surround 5.1.
- **Check for Updates**: verifica novas versões no GitHub. A verificação também acontece ao abrir o aplicativo.
- **Background Pause**: pausa o jogo e o áudio quando o aplicativo fica em segundo plano.

### Trapaças

- **Armas**: munição e granadas infinitas, disparo de duas a quatro vezes mais rápido e ausência de recuo.
- **Jogador**: invulnerabilidade e eliminação com um único acerto.
- **Jogo**: velocidade entre 25% e 200%.

### Mod de câmera livre

Durante uma missão, abra o menu de pausa pelo botão Voltar do Android ou Start do controle. Na própria lista, selecione **CHEATS** e ative **Camera livre**. Use **Voltar a pausa** e continue a missão. Para desligar, volte à mesma seção e desmarque a opção.
Também é possível acessar **Cheats → Camera → CAMERA LIVRE**. A opção **VELOCIDADE DA CAMERA** ajusta a velocidade entre 25 e 1000 unidades do mundo por segundo; o padrão é 200. O aplicativo sempre inicia com a câmera livre desligada. Apenas a velocidade é salva.

| Dispositivo | Movimento | Girar a visão | Subir | Descer |
| --- | --- | --- | --- | --- |
| Tela do celular | Analógico esquerdo | Arrastar à direita | Botão de atirar | Botão de granada |
| Controle | Analógico esquerdo | Analógico direito | RT | LT |
| Teclado e mouse | WASD por padrão | Mouse ou teclas de visão | Ação de atirar, botão esquerdo por padrão | Ação de granada, tecla G por padrão |

O botão de granada envia o comando LT e o de atirar envia RT; por isso eles controlam a altura da câmera. O botão de mirar não é usado para descer.

A câmera começa na posição dos olhos do jogador. Avançar acompanha a direção da visão; o movimento lateral permanece horizontal. Enquanto o mod está ligado, os comandos de andar e usar armas são redirecionados para a câmera. Os menus continuam acessíveis.

Abrir o menu suspende o movimento e preserva a posição da câmera. Desligar o mod pela seção Cheats solicita o retorno à visão original na thread do jogo, inclusive durante a pausa. O personagem não é teletransportado; o mundo continua funcionando e a invulnerabilidade é uma opção separada. O termo noclip aqui descreve o deslocamento da câmera através dos cenários.

O funcionamento da câmera livre foi confirmado pelo usuário em um dispositivo Android. A nova seção Cheats e o painel lateral precisam ser verificados no APK atualizado. A renderização de áreas distantes continua dependendo das regras do jogo.

#### Testes locais

Os testes usam memória simulada e verificam movimento, taxa de quadros, isolamento dos controles, restauração dos olhos, pausa, ativação/desativação e troca de mundo. É necessário um compilador C++20 e os cabeçalhos do XboxXBE-Recomp:

```sh
bash tools/test_free_camera.sh /caminho/para/XboxXBE-Recomp
```

No celular, confirme que o novo botão aparece no menu principal da pausa, ligue o mod, retome a missão, teste os controles e volte ao menu para desligá-lo. Verifique também o comportamento após morrer, reiniciar e carregar outra missão.

### Mira de ferro da AK-47 (ADS experimental)

Equipe a AK-47 em uma missão e abra **Graphics → MIRA DE FERRO AK47 → REGISTRAR AK47 EQUIPADA**. A partir desse registro manual, o botão de mirar passa a aplicar o perfil 3D somente ao tipo de arma registrado.

O perfil tem transição suave e ajustes de posição, distância e rotação. Os valores iniciais ainda precisam de calibração visual no celular. Consulte o [guia de ativação, calibração e testes](docs/ads-ak47.md).

### Painel lateral de ajustes em tempo real

Abra **Pausa → CHEATS → Camera → Ajuste de camera**, volte à missão e ajuste altura, posição lateral, distância e FOV pelo painel do ImGui. A aba **AK47 (ADS)** também permite calibrar a arma sem reabrir as opções. O painel começa maior, pode ser arrastado pela barra de título, redimensionado pelo canto, recolhido ou encaixado em uma das laterais. O botão de salvar registra o perfil. A câmera livre e o ajuste usam a câmera de renderização sem alterar a matriz do jogador.

Os toques dentro do painel são separados dos controles de tiro e visão. Consulte o [guia do painel e seus testes](docs/painel-camera.md).

## Requisitos do sistema

- **Android** 10 ou superior, arm64.
- **GPU** com Vulkan 1.1, como Adreno 6xx ou superior, Mali-G7x ou superior ou Xclipse. São aceitos drivers sem renderização dinâmica ou semáforos de linha do tempo.
- **Memória**: 4 GB.
- **Armazenamento**: 2 GB para o jogo e os dados salvos.

## Compilação

A compilação completa utiliza um computador Windows. Neste repositório, o GitHub Actions também gera o APK quando alterações são enviadas para `main`.

### 1. Clonar o repositório

```sh
git clone https://github.com/killmaster1/Black-Android.git
```

### 2. Adicionar os arquivos do jogo

Coloque o `default.xbe` da versão americana em `assets/` ou uma imagem americana (`.iso`) em `assets/` ou `iso/`. O processo de compilação extrai o `default.xbe` da imagem quando necessário.

### 3. Instalar as dependências

- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) com a carga de trabalho de desenvolvimento para desktop com C++. O recompilador é executado no computador.
- [Android Studio](https://developer.android.com/studio). No gerenciador de SDKs, instale Android SDK Platform 34, NDK `28.2.13676358` e CMake `3.31.6`.
- [Python](https://www.python.org/), necessário apenas para extrair o `default.xbe` de uma imagem de disco.

### 4. Gerar o APK

Execute `build.bat`. Ele compila o recompilador, gera o código em `generated/game` e usa o Gradle para produzir `app/build/outputs/apk/release/app-release.apk`. A primeira compilação arm64 completa pode levar de 30 a 90 minutos.

Para instalar o APK pelo computador:

```sh
adb install -r app/build/outputs/apk/release/app-release.apk
```

No GitHub, acompanhe a execução **Android CI** na aba **Actions**. Após a conclusão bem-sucedida, baixe o artefato **Black-Android-APK**.

## Créditos

- [XboxXBE-Recomp](https://github.com/TGP482/XboxXBE-Recomp), de TGP482: recompilador estático e ambiente de execução portável, incluindo kernel do Xbox, bibliotecas XDK e Direct3D 8.
- [SDL](https://github.com/libsdl-org/SDL), [glslang](https://github.com/KhronosGroup/glslang), [toml++](https://github.com/marzer/tomlplusplus) e [Dear ImGui](https://github.com/ocornut/imgui).
- [xemu](https://github.com/xemu-project/xemu) e [Cxbx-Reloaded](https://github.com/Cxbx-Reloaded), usados como referência de hardware e APIs do Xbox pelo XboxXBE-Recomp.

*Black* é uma marca de seus respectivos proprietários. Este projeto não é afiliado nem endossado por eles.

### Menu de cheats e ESP

O menu **Pausa → CHEATS** reúne os cheats existentes em categorias laterais, com fundo preto e detalhes em laranja. Inclui câmera livre, ajustes, ESP Linha e ESP Caixa com altura estimada ajustável. Os contadores do diagnóstico são opcionais. Consulte o [guia do menu](docs/menu-cheats.md) para controles e limitações.

O botão **MOD**, ao lado da pausa durante a missão, abre o painel e congela a ação sobre a imagem atual. **Continuar missao** retoma o jogo. A categoria **ESP** inclui vida numérica e um contador vermelho com direções de NPCs vivos fora da tela. A lista ainda pode incluir aliados.

A categoria **MOD → NPCs** mostra um diagnóstico das criações naturais para investigar spawn manual. Veja o [guia de criação de NPCs](docs/criacao-npcs.md); a criação manual ainda não está implementada.
