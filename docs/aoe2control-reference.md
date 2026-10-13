# AoE2Control como referência conceitual

Última atualização: 2026-10-12.

Este documento usa o projeto **AoE2Control para Age of Empires II: Definitive Edition** apenas como **referência conceitual e arquitetural** para orientar a engenharia reversa do **Age of Empires II HD (AoK HD.exe 5.8.INT, x86)**.

## Regra principal

Nenhum offset, endereço, layout de classe, stride, vtable ou estrutura do AoE2:DE deve ser transplantado para o AoE2 HD.

O uso permitido aqui é:

~~~text
AoE2Control/DE
    ↓
conceitos de alto nível
    ↓
lista de alvos semânticos
    ↓
validação independente no HD com Ghidra + RTTI + x32dbg
~~~

Portanto:

~~~text
referência conceitual ≠ evidência estrutural
~~~

Só um campo descoberto diretamente no binário/runtime do HD pode ser tratado como parte da implementação do AoE2HDControl.

## Fontes conceituais

Projetos oficiais usados como referência:

- https://github.com/aoe2control/AoE2Control
- https://github.com/aoe2control/aoe2control.github.io
- https://github.com/aoe2control/aoe2-control-lua-vscode-extension

Essas fontes mostram quais abstrações uma API madura de controle do jogo tende a expor.

## Modelo de camadas

A arquitetura conceitual de interesse é:

~~~text
Jogo
  ↓
estado interno
  ↓
camada de adaptação
  ↓
API de leitura
  ↓
API de comandos
  ↓
IPC / agente externo
~~~

Para o AoE2HDControl, a implementação correspondente deve ser reconstruída de baixo para cima:

~~~text
AoE2 HD 5.8.INT
  ↓
Ghidra / RTTI / x32dbg
  ↓
estruturas e funções confirmadas
  ↓
HDAdapter
  ↓
API estável
~~~

## Mapa de conceitos prioritários

| Conceito | Referência conceitual no AoE2Control | Estado no AoE2HDControl |
|---|---|---|
| jogador local | assigned player / player id | ✅ `World+0x174` estruturalmente forte |
| lista de jogadores | player collection | ✅ `World.mPlayers` |
| classe/tipo do jogador | human / computer / Gaia | ✅ RTTI + `humanity` parcial |
| número/cor visual do jogador | player id / color | ✅ `ResolvedPlayerConfig+0x4C = playerNumberIndex` (Jog.1→0 etc.) |
| recursos | player attributes | 🔴 |
| população | player attributes / facts | 🔴 |
| objetos por jogador/classe | object queries | 🔴 |
| object id | object id | 🔴 |
| owner | owner/player id | 🔴 |
| unit type | object/unit type | 🔴 |
| posição | x/y/z | 🔴 |
| HP | hitpoints | 🔴 |
| alvo atual | target object / target position | 🔴 |
| estado idle/moving | object state | 🔴 |
| mapa | tile grid | 🔴 |
| terreno | terrain | 🔴 |
| elevação | elevation | 🔴 |
| passabilidade | walkable/passable | 🔴 |
| pathfinding | path query | 🔴 |
| placement | placement query | 🔴 |
| comandos | move/target/build/train/research | 🔴 |
| IPC | named pipe / external agent | 🔴 futuro |

## Referência de objetos

O AoE2Control expõe conceitualmente objetos com atributos como:

~~~text
id
type
class
owner
position
hitpoints
alive
idle
moving
target
action
training/research progress
~~~

Esses nomes devem ser usados como **checklist de investigação** no HD.

Não assumir que:

- os campos estão na mesma ordem;
- os tipos têm o mesmo tamanho;
- os ids usam a mesma enumeração;
- existe uma única estrutura equivalente;
- o DE e o HD compartilham offsets.

## Referência de jogador

Conceitos de maior retorno a procurar no HD:

~~~text
player id
color / slot visual
civilization
human/computer/Gaia
resources
population
diplomacy
object collections
research state
~~~

No estado atual do projeto, já temos:

~~~text
World.mPlayers
WorldPlayerGaia
WorldPlayerHumanOrCoop
WorldPlayerComputer
ResolvedPlayerConfig.worldPlayerIndex
ResolvedPlayerConfig.humanity
World.mLocalPlayerIndex
~~~

A coluna visual **"Jog." 1–8** já foi mapeada de forma independente como `ResolvedPlayerConfig+0x4C = playerNumberIndex`; o próximo foco é consolidar readiness estrutural e leitura dos recursos.

## Referência de mapa

Uma API madura tende a expor por tile:

~~~text
x
y
terrain
elevation
visibility
walkable/passable
~~~

Para o HD, isso sugere procurar:

1. dimensões do mapa;
2. base/array de tiles;
3. stride do tile;
4. terreno;
5. elevação;
6. flags de colisão/pathing;
7. estruturas auxiliares de pathfinding.

## Referência de leitura em lote

O AoE2Control usa operações em lote para evitar milhares de chamadas individuais.

No HD, isso sugere preferir arquiteturas como:

~~~text
ResolveWorld()
  ↓
ResolvePlayers()
  ↓
enumerar coleção de objetos
  ↓
copiar snapshot próprio
  ↓
consumidor externo
~~~

em vez de expor ponteiros internos de longa duração.

## Referência de lifecycle

A documentação do AoE2Control reforça um princípio já confirmado no HD: o estado do jogo pode mudar e estruturas podem ser recriadas.

Política do AoE2HDControl:

~~~text
não cachear WorldPlayer* indefinidamente
resolver a cadeia atual
validar readiness
copiar estado necessário
falhar com segurança
~~~

## Referência de comandos

Uma futura API do HD pode mirar, em ordem de prioridade:

~~~text
Move
Target / right-click
Build
Train
Research
Patrol
AttackMove
Garrison
Stance
Formation
Delete
~~~

A investigação de comandos deve continuar depois que leitura de jogadores, recursos e objetos estiver sólida.

## Critério de promoção de hipótese

Um conceito inspirado pelo AoE2Control só entra em `structures.md` ou `functions.md` como campo real do HD quando houver evidência independente suficiente, por exemplo:

- acesso consistente em código;
- escrita/leitura observada em runtime;
- correlação controlada com mudança visual;
- RTTI/vtable;
- aritmética de coleção coerente;
- repetição em múltiplas partidas.

## Objetivo final

A meta não é clonar internamente o AoE2Control.

A meta é reconstruir, para o HD, uma API de alto nível com conceitos semelhantes:

~~~text
HDAdapter
  ├── World
  ├── Players
  ├── Resources
  ├── Objects
  ├── Map
  ├── Queries
  └── Commands
~~~

com toda a camada inferior validada especificamente no **AoK HD.exe 5.8.INT x86**.
