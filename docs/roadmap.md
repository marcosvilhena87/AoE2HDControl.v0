# Roadmap

Última atualização: 2026-10-12.

## Marco 1 — cadeia estável até World

Status: ✅ concluído para o build analisado.

~~~text
DAT_00AF2C58 → Owner*
Owner + 0xC8 → World*
World vtable = 0x009D329C
World size observado = 0x2DC
~~~

RVA operacional:

~~~text
Owner global RVA = 0x006F2C58
World vtable RVA = 0x005D329C
~~~

## Marco 2 — World.mPlayers e tipos concretos

Status: ✅ concluído estruturalmente.

Confirmado:

~~~text
World + 0x184 → mPlayers.begin
World + 0x188 → mPlayers.end
World + 0x18C → mPlayers.end_of_storage
stride = 8
~~~

Partida com 8 jogadores:

~~~text
size = 9

mPlayers[0] → WorldPlayerGaia
mPlayers[1] → WorldPlayerHumanOrCoop
mPlayers[2] → WorldPlayerComputer
...
mPlayers[8] → WorldPlayerComputer
~~~

Todos os tipos foram validados por vtable runtime.

O padrão `object = controlBlock + 0x0C` e o RTTI `_Ref_count_obj` reforçam fortemente `vector<shared_ptr<WorldPlayer>>`.

## Marco 3 — SlotRecord → WorldPlayer

Status: ✅ cadeia principal fechada.

Confirmado:

~~~text
SlotRecord stride = 0x28
SlotRecord+0x18 = configIndex

ResolveSlotConfig:
  base + 0xB0 + configIndex * 0x68

ResolvedPlayerConfig+0x50 = worldPlayerIndex
ResolvedPlayerConfig+0x54 = humanity

FUN_00598700:
  SlotRecord → worldPlayerIndex → World.mPlayers[index]
~~~

Validação runtime:

~~~text
config 0 → worldPlayerIndex 1 → humanity 2 → HumanOrCoop
config 1 → worldPlayerIndex 2 → humanity 4 → Computer
configs 2..7 → worldPlayerIndex -1 → humanity 1 na amostra
~~~

`humanity=3` continua não identificado.

## Marco 4 — localizar “Jog.” 1–8 / número-cor

Status: ✅ concluído estruturalmente.

Confirmado:

~~~text
ResolvedPlayerConfig+0x4C = playerNumberIndex
Jog.1 → 0
Jog.2 → 1
Jog.3 → 2
~~~

O campo é zero-based e distinto de:

~~~text
player+0x08
SlotRecord.configIndex
ResolvedPlayerConfig.worldPlayerIndex
World.mLocalPlayerIndex
~~~

Também foi fechada a cadeia de UI:

~~~text
FUN_0064F0E0: configIndex 0..7, stride de linha 0x70
  ↓
PlayerNumberCallback { target, configIndex }
  ↓
LAB_00653CD0
  ↓
FUN_00658E40(configIndex)
  ↓
FUN_00615A30
  ↓
ResolvedPlayerConfig+0x4C
~~~

Pendente apenas nomear com maior precisão o objeto/contexto de lobby usado como `target`.

## Marco 5 — readiness / lifecycle

Status: 🟡 avançado; a publicação do vetor e a criação parcial/completa de Gaia foram capturadas, e os três jogadores foram validados em gameplay. Falta determinar a primeira transição estrutural e sua relação temporal com gameplay-ready.

Já confirmado:

~~~text
0x0062E68B publica Owner+0xC8 = World*
0x0062E73A pode fazer rollback para nullptr
FUN_0072C6E0 pode esvaziar mPlayers mantendo capacidade
FUN_0072F7D0 limpa e redimensiona mPlayers
FUN_007288B0 implementa resize-like do vetor de stride 8
~~~

Portanto:

~~~text
World pointer válido != gameplay-ready
begin != nullptr != vetor não vazio
size > 0 != necessariamente PlayerEntry.object válido
~~~

Meta:

~~~cpp
bool IsWorldReady(const World*);
~~~

e política de não cachear ponteiros derivados por longo prazo.

Critério estrutural provisório:

~~~text
World != nullptr
vtable correta
mPlayers begin/end coerentes
2 <= size <= 9
0 <= localPlayerIndex < size
~~~

Checks semânticos agora validados em gameplay:

~~~text
mPlayers[0] → Gaia
mPlayers[localPlayerIndex] → HumanOrCoop
player+0x04 → World* atual
~~~

Snapshot com `moduleBase=0x00D80000`:

~~~text
World* = 0x18E14488
localPlayerIndex = 1
begin = 0x14932B78
end   = 0x14932B90
cap   = 0x14932B90
size  = 3

0 → Gaia
1 → HumanOrCoop
2 → Computer
~~~

Em 2026-10-12, hardware watch em `World+0x188` capturou a atualização de `mPlayers.end` em runtime `0x006D8A11`, após `begin` em `0x006D8A0C` e antes de `capacityEnd` em `0x006D8A19`. O vetor passou a size 3 **com os 24 bytes das entradas zerados**. Uma escrita em `mPlayers[0].object` produziu o estado transitório `(object!=0, controlBlock=0)`; `0x00648F75` publicou o `controlBlock` de Gaia. Os objetos Gaia/HumanOrCoop/Computer e `mLocalPlayerIndex=1` foram validados quando o mapa já estava jogável. As escritas nas entradas 1/2 não foram capturadas pelos breakpoints, apesar de aparecerem preenchidas no mesmo buffer. Isso **não** prova a fronteira exata de gameplay readiness.

## Marco 6 — jogador local

Status: 🟢 estruturalmente forte.

Confirmado:

~~~text
World + 0x174 → mLocalPlayerIndex
~~~

Em estados gameplay-ready, o índice resolve para `WorldPlayerHumanOrCoop`.

Meta:

~~~cpp
WorldPlayer* GetLocalPlayer(World*);
~~~

Separar explicitamente esse índice da coluna visual “Jog.” do lobby.

## Marco 7 — recursos do jogador

Depois de fechar o campo “Jog.” e readiness:

~~~text
Food
Wood
Gold
Stone
Population
~~~

Primeiro objetivo: leitura somente.

## Marco 8 — objetos/unidades

Mapear:

~~~text
object id
owner
type
position
HP
coleções
~~~

## Marco 9 — comandos

Investigar `RGE_Command` e `TRIBE_Command` somente após a leitura de estado/jogadores/objetos estar sólida.

## Marco 10 — robustez por versão

- signatures para raízes/funções críticas;
- version/hash gate;
- validação RTTI/vtable;
- falha segura em build desconhecido.

## Referência conceitual — AoE2Control / Definitive Edition

Status: 🟢 incorporada como guia semântico.

O AoE2Control é usado apenas para responder:

~~~text
quais conceitos uma API madura de controle precisa expor?
~~~

Exemplos de alvos inspirados por essa referência:

~~~text
player id / color
resources / population
object id / owner / type / position / HP
map tiles / terrain / elevation / passability
pathfinding / placement
commands
IPC / external agent
~~~

Nenhum offset, stride, vtable ou layout do AoE2:DE deve ser usado como evidência para o HD.

Detalhes: `docs/aoe2control-reference.md`.

## Ordem atual recomendada

~~~text
1. consolidar validador `IsWorldStructurallyReady()` com snapshots e leituras seguras
2. instrumentar primeira transição estrutural (incluindo publicações de entries 1/2) e correlacionar com `GameplayReady`
3. fechar `GetLocalPlayer()` como API de leitura
4. mapear Food/Wood/Gold/Stone
5. mapear objetos/unidades
6. mapear comandos
7. trocar RVAs críticos por signatures
8. implementar HDAdapter
~~~
