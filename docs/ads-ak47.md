# Mira de ferro da AK-47

Esta versão experimental acrescenta posição e rotação 3D da arma durante a mira, com entrada e saída suaves. Os deslocamentos iniciais são uma calibração provisória: o alinhamento da alça e da massa de mira ainda precisa ser conferido no celular.

## Como ativar

1. Entre em uma missão e equipe a AK-47.
2. Abra as opções da missão e vá para a página **Graphics**, usando a linha **Page** se necessário.
3. Na seção **MIRA DE FERRO AK47**, confirme **REGISTRAR AK47 EQUIPADA**. Essa ação registra o tipo da arma equipada e liga **ATIVAR ADS AK47**.
4. Retome a missão e use o botão normal de mirar.

O registro é manual: ele não reconhece o nome da arma. Se registrar outra arma, ela receberá o perfil. O tipo registrado e a calibração ficam salvos em `black.toml`; não é preciso repetir o registro ao trocar de missão. Se atualizar para outra base do jogo, registre novamente a AK-47.

Para desligar, desative **ATIVAR ADS AK47**. O ajuste não atua nas outras armas e fica suspenso com a câmera livre. A recarga faz a arma sair suavemente da posição de mira.

## Calibração

- **ADS POSICAO HORIZONTAL**: desloca a arma para os lados.
- **ADS ALTURA**: sobe ou desce a arma.
- **ADS DISTANCIA**: aproxima ou afasta a arma.
- **ADS ROTACAO HORIZONTAL**: gira o conjunto lateralmente.
- **ADS INCLINACAO**: altera a inclinação vertical.

Comece pela posição horizontal e altura. Depois alinhe a massa de mira com a alça usando rotação e inclinação, e confira os disparos contra uma parede. Os ajustes preservam o modelo e as animações originais; não acrescentam peças de mira que não existam no modelo.

O zoom e a sensibilidade permanecem os do jogo. Esta etapa trata do posicionamento 3D da arma; não modifica a trajetória das balas, a dispersão nem o dano. Para calibrar, teste inicialmente parado, sem disparar, e depois andando, atirando, recarregando e trocando de arma.

## Evidências da análise

Executável americano fornecido pelo usuário: SHA1 `c5bfed25449c8212d7a976f15246e87ead57713f`.

- `0x1E634C`: consulta a arma atual em `jogador + 0x29C`.
- `0x1E6356`: lê o índice do tipo no primeiro byte da arma.
- `0x1E6317–0x1E6738`: monta a matriz local em `jogador + 0x420`, incluindo balanço e inclinações.
- `0x1D2651–0x1D2694`: compõe essa matriz com a matriz do jogador e grava o resultado usado para desenhar a arma.
- `0x1BB11E`: consulta o indicador de mira em `arma + 0x105`.
- `0x1BB385`: seleciona o estado 5 na rotina de recarga.

O mod ajusta a matriz local após a atualização original do jogador e a restaura antes da próxima atualização, evitando acumular deslocamentos. Reutiliza o hook existente, sem acrescentar endereços ao arquivo de recompilação. Os executáveis fornecidos não são incluídos no commit.

## Validação

```sh
bash tools/test_ads_ak47.sh /caminho/para/XboxXBE-Recomp
bash tools/test_free_camera.sh /caminho/para/XboxXBE-Recomp
```

Os testes locais exercitam transformação 3D, transição por tempo, registro, restauração da matriz e isolamento por tipo de arma. Não confirmam o alinhamento visual nem o comportamento dentro do jogo. O APK é gerado pelo fluxo existente do GitHub Actions.
