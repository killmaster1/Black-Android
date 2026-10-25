# Viabilidade da V2 — BLACK Xbox / Sandro Mods

## Conclusão

É viável avançar para topologia diferente **sem mudar o tamanho do pacote**, usando
o espaço já reservado para uma malha rígida. Implementei esse passo como comando
separado `substituir`, mantendo as funções V1 existentes.

Ainda não há evidências suficientes para aumentar os buffers e realocar todos os
recursos do DB. Também falta confirmar o skinning para substituir com segurança
as partes de passo 36. Portanto, esta entrega não converte uma banana arbitrária
em uma pistola completa e animada. O teste no Android descrito no texto recebido
comprova a deformação V1, não valida o protótipo V2.

## Material examinado

Li integralmente o `black_obj.py` recebido. Suas funções V1 coincidem, por comparação
de AST, com as funções locais anteriores; há diferenças de espaços no arquivo.
Inspecionei os mapas, OBJ original e banana, DBs original e modificados, os scripts
de lote e o WDD. Nenhum script recebido foi executado para extrair o pacote.

`bg1_pst.db` e `bg1_pst_original.db` são idênticos:

`df87876f470d025644e538b64ff1a827bb63ec13e9c251fcabd8797441b7d3ea`

`bg1_pst_mod.db` e `bg1_pst_banana.db` também são idênticos:

`7db3c25d72bf59ada96650ccfe37cb16c34c451b3b53fe5fe91e0133e17a276e`

Ambos têm 741896 bytes. O SHA-256 do original coincide com o mapa JSON. Reexecutei
o importador V1 local com o OBJ banana recebido e reproduzi **exatamente** o DB
banana enviado. Nenhum dos originais ou da ISO foi modificado.

## Alterações comprovadas na Pistola-Banana

| Malha | Início dos vértices | Passo | Vértices reservados | Vértices alterados | Bytes XYZ alterados | Bytes das caixas alterados |
|---|---|---:|---:|---:|---:|---:|
| 0 | 0x10760 | 24 | 322 | 322 | 2557 | 16 |
| 1 | 0x13C30 | 36 | 5152 | 5146 | 37625 | 21 |
| 2 | 0x43EE0 | 36 | 5152 | 5146 | 37625 | 13 |

Total: 10614 vértices e 77857 bytes alterados. **Zero bytes diferentes fora dos
intervalos XYZ e das duas caixas locais por malha.** Cabeçalho, índices, UVs e
outros atributos permaneceram idênticos. Os offsets exatos e hashes constam em
`auditoria-pistola-banana.json`, reproduzível com `auditar_db.py`.

Isso explica por que o experimento continua limitado a deformação: não mudou uma
única face. A transformação atingiu as três malhas, não uma parte já identificada
com segurança como sendo apenas a arma. O nome `fpguns` é contexto fornecido pelo
usuário; ainda precisamos separar arma, mãos e eventuais acessórios por inspeção.

## Estruturas verificadas nas amostras

Os offsets abaixo descrevem os bytes e as relações observadas, não uma especificação
oficial de todas as versões do jogo.

| Estrutura | Campo relativo | Relação observada |
|---|---|---|
| Cabeçalho geométrico R | +0x18 | uint32 35 nas geometrias reconhecidas |
| R | +0x1C | deslocamento para grupo G, relativo a R |
| R | +0x2C | três float32: 8, 16, 24; significado completo desconhecido |
| R | +0x50 | quantidade de descritores reconhecidos |
| Descritor D = G + j × 28 | +0x04 | V = D + uint32(D+4) |
| D | +0x0C | B = D + uint32(D+0x0C) |
| D | +0x14 | passo do vértice: 24 ou 36 |
| Limites B | +0, +0x10 | dois vetores XYZ, tratados como máximo/mínimo pela V1 |
| B | +0x20 | primitiva 6, interpretada como triangle strip |
| B | +0x24 | quantidade de índices consumidos |
| B | +0x28 | início dos índices = B + uint32(B+0x28) |
| Vértice | +0 | XYZ float32 little-endian |
| Vértice | passo − 8 | UV float32 little-endian |
| Índices | — | uint16 little-endian |

Na pistola, R = 0x10680 para a malha 0; R = 0x12A80 para as malhas 1 e 2.
Os limites B são 0x10730, 0x13BD0 e 0x13C00. Os índices começam em 0x12590,
0x410B0 e 0x71360, com 447, 5905 e 2927 entradas respectivamente.

A contagem de vértices da V1 é **inferida** da distância entre V e os índices,
dividida pelo passo. Não foi localizado um campo explícito universal de contagem.
Nas amostras há até 15 bytes de alinhamento. Os bytes entre XYZ e UV não estão
completamente decodificados; preservar esses bytes não prova que compreendemos
normais, pesos, índices de ossos ou todos os atributos.

No cabeçalho externo da pistola há valor 9 em +0x18 e dez uint32 a partir de +0x1C:
0x80, 0x10580, 0x10200, 0x10400, 0x72C00, 0x95B00, 0x95B80, 0x96D80,
0xAE580, 0xB4D80. Apontam para estruturas internas observáveis; **não é correto
tratá-los automaticamente como uma simples tabela ordenada de nove seções**.
Há RIFF/WAVE em 0x96D80 e 0xAE580. O significado do último bloco e suas referências
ainda precisa ser confirmado pelo carregador do executável.

## Riscos encontrados no parser atual

1. A assinatura é heurística. Os filtros e o teste de ida e volta não comprovam
   que todas as geometrias do pacote foram encontradas.
2. Um candidato incompatível é descartado silenciosamente. Um OBJ pode representar
   apenas a parte reconhecida do DB; isso não deve virar uma reconstrução completa.
3. Contagem inferida e padding não substituem a identificação do alocador/carregador.
4. Copiar posições sem compreender matrizes e pesos pode deformar mãos e acessórios.
5. As duas caixas locais não são todos os possíveis volumes de visibilidade,
   colisões e poses. O significado em runtime ainda requer inspeção do carregador.
6. A exportação OBJ não recupera materiais, texturas, rig nem normais compactadas.
7. Escalar buffers deslocaria estruturas posteriores. Corrigir só o descritor e
   a tabela externa pode deixar outras referências inválidas.

## Protótipo implementado: capacidade fixa

`black_obj.py substituir` recebe OBJ com posições, UVs e triângulos novos. Separa
vértices nos encontros de UV e aceita índices positivos ou negativos. Converte as
faces para uma strip com ligações degeneradas e preservação de orientação.

O protótipo aceita **somente passo 24**. Novos vértices e índices devem caber nas
reservas originais. Preserva os offsets, o comprimento do DB, materiais e outras
malhas. Atualiza o contador de índices e as caixas locais. Os registros não usados
permanecem reservados; não há compactação nem realocação. Os quatro bytes não
posicionais/UV de cada novo vértice vêm do doador espacialmente mais próximo.
Isso não recalcula normais e pode causar iluminação incorreta.

Para limitar alterações de visibilidade, o OBJ deve caber na caixa XYZ dos vértices
originais. Isso reduz a expansão da geometria; não garante correção de todos os
volumes e efeitos no jogo. Modelo maior, vários materiais e passo 36 são rejeitados.
A identidade funcional da malha 0 ainda não foi comprovada: **o cubo pode alterar
um componente ou efeito, e não a pistola inteira**. Este é um teste do formato.

Antes de gravar, o código relê o DB em memória e confere as faces novas. Saída
existente não é sobrescrita. Depois de mudar a topologia, um mapa V1 antigo não
serve para esse DB; para editar posições, gere outra exportação e mapa.

## Testes e limites da evidência

- Reprodução exata da banana enviada a partir do OBJ e DB originais: passou.
- Cubo com 8 vértices e 12 triângulos no lugar da malha rígida: passou em memória.
- Reconversão das strips compara orientação e sequência das faces: passou.
- Nenhum byte fora dos registros, índices, contador e caixas autorizados: passou.
- Rejeição de capacidade excedida, coordenadas externas e passo 36: passou.
- Regressão da V1 com comparação byte por byte: passou.
- Não houve reconstrução de XISO: a ISO não veio no pacote.
- **Não executei a V2 no Black-Android, xemu ou Xbox físico.**

## Próximos passos incrementais

1. Testar o cubo no Android numa cópia da XISO e registrar quando aparece:
   arma equipada, tiro, recarga e troca. Isso identifica a função da malha 0.
2. No executável europeu correspondente à ISO, rastrear as rotinas que leem o
   DB, aplicam relocação e criam buffers. Correlacionar cada campo com o código;
   não transplantar endereços da versão americana sem confirmação.
3. Mapear os registros de passo 36 e as matrizes/pesos. Separar arma e mãos com
   visualização por índices, material e ossos. Só então habilitar nova geometria
   articulada preservando as animações originais.
4. Fazer um mapa de todas as referências que cruzam buffers. Testar primeiro uma
   expansão de um bloco conhecido, incluindo alinhamento e recursos seguintes.
   A V2 com realocação deve recusar o pacote se não puder provar essas referências.
5. Investigar o WDD rastreando sua abertura e consumo. Os 65536 bytes e sua extensão
   não estabelecem que contenha texturas. Não foi modificado.
6. Localizar os recursos de NPCs pelo carregador, antes de prometer troca de corpos,
   hitboxes ou personagens completos. Os arquivos de fase citados não vieram aqui.

O bloqueio de realocação e skinning é de conhecimento do formato. Não se resolve
retirando as verificações do importador V1.
