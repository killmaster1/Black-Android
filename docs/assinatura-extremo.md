# Assinatura, créditos e Modo Extremo

## Abertura do aplicativo

A confirmação de créditos aparece na atividade Android antes de a thread do jogo
inicializar SDL, procurar os arquivos ou executar o jogo. A busca por atualização
e os pedidos de acesso aos arquivos também aguardam **Continuar**.
A tela credita Rangga Casper, TGP482, XboxXBE-Recomp e a comunidade dos ports,
além de Criterion Games e Electronic Arts pelo jogo original.
A autoria desta modificação aparece separadamente como **Sandro Mods**.
Os créditos e a licença originais do repositório são preservados.

O bloqueio usa uma sinalização, sem espera ocupada na interface. Se o aplicativo
for destruído antes da confirmação, a sinalização é liberada e a thread do jogo
retorna sem iniciar o jogo. A confirmação é referente à instância da atividade,
não é uma preferência salva para ocultar os créditos em aberturas futuras.

## Mensagem ao entrar na missão

Quando o personagem fica vivo em uma nova tentativa, a missão apresenta:

> Mod Menu criado por Sandro Mods
> Inscreva-se no canal

- **Ir ao canal** abre `https://youtube.com/@sandromods2022?si=n_ompvindW-w_GC1` pelo aplicativo/navegador do Android.
- **Fechar** dispensa somente a mensagem e continua a missão, sem abrir o menu de pausa ou o de cheats.

A mensagem aguarda a saída de páginas do jogo, como instruções iniciais, para não
substituir uma confirmação do próprio jogo. A apresentação é congelada enquanto
ela está aberta. Abrir o canal não a fecha automaticamente; ao retornar, toque em **Fechar**.
O botão Voltar do Android também dispensa somente a mensagem.

## Morte, reinício e contadores

O acompanhamento da tentativa reconhece entrada na missão, troca do endereço do
mundo, morte do jogador e retorno do relógio do mundo. A pausa comum não inicia
uma nova tentativa. Na morte, saída ou retorno do relógio, são descartadas as
solicitações e estatísticas antigas de duplicação, observação de criação e ESP.
A geração das solicitações muda, invalidando chamadas pendentes anteriores.
Ao reviver depois da morte, não se apagam novamente as criações que o carregamento
já observou para a nova tentativa.

O texto agora informa **Extras nesta tentativa**. É um acumulado de criações
confirmadas nessa tentativa, e não o número de extras ainda vivos. **Zerar contadores**
na seção de observação continua zerando apenas aquele monitor.
A reutilização de memória sem morte ou retorno do relógio pode exigir investigação adicional.

## Modo Extremo

Abra **MOD → Jogo → Modo Extremo**. A opção começa desligada.

- A dificuldade consultada pelo jogo fica em **Black Ops** durante a missão.
- **Dano de tiros recebido:** 150% a 400%, padrão 250%.
- **Vida dos NPCs:** 2x a 6x, padrão 3x.

O aumento de um tiro comum é limitado a 75% dos 750 pontos de vida cheia usados
na dificuldade mais alta, para não tornar o aumento uma morte automática com um
tiro a partir da vida cheia. Um golpe que já seria letal no jogo original não é
reduzido. Dano acumulado pode matar normalmente. Esse ajuste atua na rota de dano
de tiros identificada em `0x1EDA7E`; outras rotas, como quedas e explosões específicas,
conservam seu tratamento original.

A vida restante dos NPCs é multiplicada uma vez, sem reaplicação a cada quadro.
Ao alterar o multiplicador ou desligar o modo, preserva-se a proporção restante.
O acompanhamento identifica novos personagens e remove registros que saíram da
lista atual. A dificuldade anterior é restaurada ao desligar quando o campo ainda
contém o valor aplicado pelo mod. Os ajustes são salvos em **Salvar perfil**.

O filtro atual ainda pode incluir aliados. Invencibilidade e matar com um tiro
podem anular o desafio. O campo de dificuldade é alterado em tempo de execução;
configurações já fixadas pelo carregamento não são garantidamente recalculadas.
Não há ajuste próprio de precisão da IA nesta versão: esse cálculo ainda precisa
ser mapeado. O modo usa a base Black Ops que o jogo consultar e os aumentos explícitos
acima, sem modificar a mira do jogador.

## Verificação

```sh
bash tools/test_mission_features.sh out/check-deps/sdk
bash tools/test_camera_tools.sh out/check-deps/sdk
bash tools/test_npc_creation.sh out/check-deps/sdk
bash tools/test_npc_spawn.sh out/check-deps/sdk
```

Também foi feita a compilação Java com `:app:compileDebugJavaWithJavac`, sem gerar
um APK local completo. O APK continua sendo compilado pelo GitHub Actions.
No aparelho, confira a ordem dos créditos, a abertura do canal, o fechamento da
mensagem, a repetição após morrer/reiniciar e a vida/dano com o Modo Extremo ligado.
