# Investigação da criação de NPCs

O menu **MOD → NPCs** observa as criações da missão e inclui um primeiro teste
de duplicação da próxima criação natural. Ainda não há spawn instantâneo nem escolha
de aliado ou inimigo. O monitor começa ligado e também conta a criação extra quando ela usa a fábrica observada.

## Teste no aparelho

1. Instale o APK e entre em uma missão desde o início.
2. Abra **MOD → NPCs** e confira **Chamadas observadas**, **NPCs criados e reconhecidos**
   e **Perfis observados**. Envie uma captura dessa tela.
3. Retome a missão, avance até aparecerem novos personagens e confira novamente.
4. Inclua na captura as linhas **Configuração** e **Transformação** do último perfil.

O diagnóstico é independente do ESP. **Zerar contadores** descarta as amostras anteriores;
**Observar criacoes naturais** liga ou desliga a coleta. Os dados são da sessão e
são limpos quando o endereço do mundo muda. Reutilização do mesmo endereço pelo jogo
pode exigir zerar os contadores manualmente.

**NPCs criados e reconhecidos** significa que a função retornou um controlador ligado
a um personagem de tipo 1 com vida positiva. Não é o número de inimigos vivos atualmente.
**Retornos sem NPC** registra retornos nulos. **Leituras incompletas** significa que
não foi possível validar a amostra como um NPC reconhecido; não prova falha de criação do jogo.
Perfis são índices internos de configuração, não nomes de modelos, equipes ou armas.
Configuração/transformação **lida** confirma acesso aos dados, sem comprovar que possam ser reutilizados.
Este monitor cobre uma rota de criação, não garante capturar todas as variantes do jogo.

## Evidências no executável americano

Trechos conferidos no `default_us.xbe` fornecido, sem incluir o binário no repositório:

| Endereço | Evidência |
| --- | --- |
| `0x1E6860` | Verifica espaço no gerenciador de personagens e disponibilidade em uma reserva associada à IA |
| `0x1AF880` | Entrada de uma fábrica de controladores; 12 argumentos na pilha, ECX e EDI também usados |
| `0x1AF889` | Lê o gerenciador em `0x2D1988` |
| `0x1AF893` | Lê a reserva em gerenciador+`0x22CDC`; retorno nulo quando indisponível |
| `0x1AF8A4` | Retorno antecipado `ret 0x30` sem controlador |
| `0x1AF8A7` | Retira o primeiro controlador da reserva usando sua ligação em `+0x1F00` |
| `0x1AF8B3` | Primeiro argumento seleciona uma configuração em gerenciador `0x2D1980` + `0x7A20`, passo `0x40` |
| `0x1AF8C6` | Quinto argumento é um ponteiro; a fábrica lê o vetor em `+0x10` |
| `0x1AF8CD` | Lê identificadores em ECX+`0x8` e ECX+`0xC` |
| `0x1AF967` | Chama `0x1DF440` para criar/registrar o personagem |
| `0x1AF96C` | Incrementa o contador apontado por EDI; seu papel completo ainda precisa ser mapeado |
| `0x1AF985` | Guarda o quinto argumento no personagem+`0x31C`; sua duração importa após a chamada |
| `0x1AF99D` | Retorno de sucesso `ret 0x30`; EAX contém o controlador |
| `0x1B26B3` | O chamador obtém o personagem em controlador+`0x8C` |
| `0x1B26B0–0x1B2746` | O chamador continua configurando comportamento e granadas depois da fábrica |

A inicialização do gerenciador em `0x1E69B0` prepara 44 configurações com passo `0x40`;
o diagnóstico limita os índices de perfil a 0–43. O índice não identifica por si só
um arquivo de modelo nem a equipe do personagem.
`0x1DF340`, encontrada inicialmente pela lista de personagens, também cria/registra
objetos, mas não equivale à cadeia completa de IA de `0x1AF880`.

## Implementação do monitor

Hooks na entrada e nos dois retornos de `0x1AF880` pareiam chamadas por thread,
com até oito níveis aninhados. Copiam o índice do perfil e verificam a leitura dos
parâmetros conhecidos. No retorno, validam a ligação do controlador ao personagem,
o tipo e a vida. As páginas são conferidas antes de cada nova leitura.
Os hooks não escrevem no jogo, não chamam funções convidadas e não alteram registradores.
A interface recebe apenas contadores e estados protegidos por mutex.
Reiniciar a coleta, desativá-la ou mudar de mundo invalida chamadas pendentes anteriores.

## O que falta para o spawn manual

- Confirmar os perfis e os retornos dessa rota no Android.
- Mapear os demais parâmetros, a configuração visual e as armas.
- Determinar a propriedade e duração dos ponteiros que a fábrica mantém.
- Mapear o contador em EDI e o tratamento das reservas cheias.
- Reproduzir a configuração posterior feita pelo chamador original.
- Confirmar posição válida, colisão, navegação e definição de aliado/inimigo.

Copiar uma estrutura de NPC ou repetir uma chamada com ponteiros de uma pilha antiga
não resolve essas etapas. A criação instantânea de um perfil escolhido depende desse mapeamento.

## Validação local

```sh
bash tools/test_npc_creation.sh out/check-deps/sdk
bash tools/test_camera_tools.sh out/check-deps/sdk
```

Os testes simulam entrada e retorno, confirmam ausência de escrita em memória e
registradores, pareamento de chamadas aninhadas, retorno nulo, limites de perfil,
reinício dos contadores e troca de mundo. Não substituem o teste da fábrica real no aparelho.

## Primeiro protótipo: duplicar a próxima criação natural

As capturas do usuário confirmaram 6, 12 e 67 chamadas reconhecidas, com zero leituras
incompletas, até cinco perfis e leitura da configuração/transformação nessa missão.

1. Abra **MOD → NPCs** e toque em **Duplicar proximo NPC (teste)** uma vez.
2. Confira **Aguardando uma criacao natural** e toque em **Continuar missao**.
3. Avance até o jogo criar outro NPC. Um extra é solicitado no mesmo ponto, com o perfil original.
4. Confira **NPC extra criado** e **Extras confirmados: 1** no painel; use ESP para localizar os dois.
5. Teste se o extra se move, usa a arma e pode morrer. Envie uma captura do resultado.

O botão não cria imediatamente ao lado do jogador. Se o trecho não gerar mais NPCs,
a solicitação permanece aguardando. **Cancelar duplicacao** cancela uma solicitação pendente.
Cada acionamento autoriza no máximo uma tentativa extra; não há repetição automática.
O estado é limpo ao mudar o endereço do mundo. A escolha de equipe ainda não existe:
o extra herda os parâmetros da criação original. O mesmo ponto de nascimento pode
causar sobreposição inicial; posição alternativa e integração com objetivos ainda precisam de teste.

### Chamada completa e duração dos parâmetros

A função `0x1B2640` recebe a definição em ESI e 11 argumentos na pilha (`ret 0x2C`).
Ela verifica disponibilidade em `0x1E6860`, chama `0x1AF880` e executa o restante da
configuração até `0x1B2746`. Seus retornos antecipados ficam em `0x1B2657` e `0x1B26AD`.
Os quatro pontos foram conferidos no executável americano fornecido.

Os argumentos só são copiados enquanto essa chamada está ativa. Depois de uma criação
original reconhecida, antes do retorno ao chamador, a rotina completa é chamada mais
uma vez com os mesmos argumentos e definição, preservando o contexto original em uma cópia.
Isso inclui o contador usado pelo código original, sem criar um identificador arbitrário.
Não há cache de ponteiros para chamar a fábrica mais tarde pelo ImGui.
Ainda é necessário validar no aparelho o comportamento desses recursos compartilhados após a criação.

A rotina é chamada pela própria thread que está criando o personagem; a interface apenas
arma a solicitação. Há proteção contra reentrada, mudança de mundo, cancelamento e
falta de espaço. O protótipo exige uma vaga existente e uma reserva de IA não vazia;
a verificação original de disponibilidade também continua sendo executada.
A tentativa deixa a imagem e os registradores de retorno da criação original intactos;
a única alteração intencional é o trabalho feito pela rotina original para criar o extra.

```sh
bash tools/test_npc_spawn.sh out/check-deps/sdk
```

O teste usa uma chamada convidada simulada para verificar argumentos, uma única tentativa,
reentrada, contexto preservado, criação não confirmada, cancelamento, reservas cheias e
mudança de mundo. O teste não executa a rotina real do jogo; comportamento e estabilidade
do NPC extra precisam ser avaliados no APK.
