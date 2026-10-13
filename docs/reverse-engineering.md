# Engenharia reversa — estado consolidado

Última atualização: 2026-10-07.

## Alvo

- Executável: `AoK HD.exe`
- Versão: `5.8.INT`
- Plataforma: Windows x86 / PE32
- ImageBase estático: `0x00400000`
- Arquitetura: i386
- Timestamp PE registrado: 22/08/2018
- SHA-256: `CBD10D81B93601FFB26773D250B7478969DA92D70792A40AE423294202E14650`

## Relocação usada no x32dbg

Na execução analisada em 04/10/2026:

~~~text
moduleBase runtime = 0x00410000
deslocamento       = +0x00010000
~~~

Exemplos:

~~~text
static FUN_00598700 → runtime 0x005A8700
static FUN_00598750 → runtime 0x005A8750
static DAT_00AF2C58 → runtime 0x00B02C58
static DAT_00AF1AD0 → runtime 0x00B01AD0
~~~

Sempre converter endereço estático para runtime pelo RVA da execução atual.

## World

RTTI confirma:

~~~text
World
  ↓
BaseWorld
~~~

Vtable estática: `0x009D329C`.

Tamanho alocado observado: `0x2DC` bytes.

`FUN_006889B0` recebe `this` em ECX e instala explicitamente a vtable:

~~~asm
006889D6  MOV ESI,ECX
006889DE  CALL FUN_00729360
006889F1  MOV dword ptr [ESI],009D329C
~~~

## Cadeia global

~~~text
[DAT_00AF2C58] = Owner*
Owner + 0xC8   = World*
~~~

RVA do global: `0x006F2C58`.

Na execução com base `0x00410000`:

~~~text
Owner global runtime = 0x00B02C58
~~~

## Publicação e rollback de World

`FUN_0062E5D0` aloca `0x2DC`, chama `FUN_006889B0` e publica o novo ponteiro em:

~~~text
0x0062E68B → Owner+0xC8 = novo World*
~~~

Depois da publicação ocorre chamada virtual pelo slot `World.vtable+0x04`, cujo alvo é `FUN_0072D1A0`.

Existe também caminho de rollback:

~~~text
0x0062E73A → Owner+0xC8 = nullptr
~~~

Logo:

~~~text
Owner->world != nullptr
≠
World gameplay-ready
~~~

## World + 0x174

Associado diretamente à string `mLocalPlayerIndex.raw()`.

Estado: ✅ `mLocalPlayerIndex`.

Importante: esse índice interno não deve ser confundido automaticamente com a coluna visual **“Jog.”** do lobby.

## World + 0x184/+0x188/+0x18C

Interpretação confirmada:

~~~text
+0x184 → mPlayers.begin
+0x188 → mPlayers.end
+0x18C → mPlayers.end_of_storage
stride = 8
~~~

Logo:

~~~text
size = (end - begin) / 8
~~~

### Partida com 8 jogadores

Runtime observado:

~~~text
World*                  = 0x18839030
World+0x184             = 0x1865C968
World+0x188             = 0x1865C9B0
World+0x18C             = 0x1865C9B0
size                    = 9
capacity                = 9
~~~

A presença de 9 entries com 8 jogadores reais é explicada por:

~~~text
mPlayers[0] = Gaia
mPlayers[1] = Jogador real 1
...
mPlayers[8] = Jogador real 8
~~~

## Layout das entries de mPlayers

Cada entry tem 8 bytes:

~~~cpp
struct PlayerEntry {
    WorldPlayer* object;  // +0x0
    void* controlBlock;   // +0x4
};
~~~

Na partida de 8 jogadores:

~~~text
index  object      controlBlock
0      0x0F588734  0x0F588728
1      0x1469485C  0x14694850
2      0x184A501C  0x184A5010
3      0x151FA41C  0x151FA410
4      0x13B4941C  0x13B49410
5      0x185E901C  0x185E9010
6      0x138E564C  0x138E5640
7      0x143F701C  0x143F7010
8      0x18C7584C  0x18C75840
~~~

Em todas:

~~~text
object = controlBlock + 0x0C
~~~

RTTI contém `_Ref_count_obj<...>` para os tipos concretos.

O código fecha ainda mais a interpretação:

~~~text
FUN_00430160
  entry stride = 8
  entry+0x4 = controlBlock
  controlBlock+0x4 = strong count
  controlBlock+0x8 = weak count
~~~

Logo:

~~~text
mPlayers = std::vector<std::shared_ptr<WorldPlayer>>
~~~

## Operações internas de mPlayers

### FUN_00430DD0 — destruição do vetor

Com `ECX=&mPlayers`:

~~~text
destroy [begin,end)
free buffer
begin = 0
end = 0
capacityEnd = 0
~~~

### FUN_0072C6E0 — clear/reset preservando capacidade

A rotina percorre os `WorldPlayer*`, chama cleanup por entry, destrói os shared_ptrs e faz:

~~~text
end = begin
~~~

Portanto pode existir:

~~~text
begin != nullptr
end == begin
capacityEnd != nullptr
~~~

### FUN_007288B0 — resize-like

Confirma:

~~~text
size     = (end - begin) / 8
capacity = (capacityEnd - begin) / 8
~~~

e implementa:

~~~text
newSize > capacity
  → realoca
  → capacity nova ≈ max(newSize, capacity + capacity/2)

size < newSize <= capacity
  → constrói cauda

newSize < size
  → destrói cauda
~~~

### FUN_0072F7D0 — clear + resize

Fluxo relevante:

~~~text
mPlayers.clear()
→ calcula newSize
→ FUN_007288B0(&mPlayers, newSize, ...)
→ continua inicialização
~~~

Também manipula `World+0x174`.

Ainda não está fechado se, logo após o resize, as entries já contêm `WorldPlayer*` válidos ou apenas slots inicializados.

## Hierarquia WorldPlayer

RTTI confirma:

~~~text
WorldPlayer
WorldPlayerBase
WorldPlayerGaia
WorldPlayerHumanOrCoop
WorldPlayerComputer
WorldPlayerScenarioEditorPhantom
~~~

### Vtables estáticas

~~~text
WorldPlayerGaia        = 0x009DC8A0
WorldPlayerHumanOrCoop = 0x009DCAE4
WorldPlayerComputer    = 0x009DC618
~~~

Com base runtime `0x00410000`:

~~~text
WorldPlayerGaia        = 0x009EC8A0
WorldPlayerHumanOrCoop = 0x009ECAE4
WorldPlayerComputer    = 0x009EC618
~~~

### Confirmação da partida com 8 jogadores

Primeiros DWORDs:

~~~text
mPlayers[0] @ 0x0F588734 → 0x009EC8A0 → WorldPlayerGaia
mPlayers[1] @ 0x1469485C → 0x009ECAE4 → WorldPlayerHumanOrCoop
mPlayers[2] @ 0x184A501C → 0x009EC618 → WorldPlayerComputer
mPlayers[3] @ 0x151FA41C → 0x009EC618 → WorldPlayerComputer
mPlayers[4] @ 0x13B4941C → 0x009EC618 → WorldPlayerComputer
mPlayers[5] @ 0x185E901C → 0x009EC618 → WorldPlayerComputer
mPlayers[6] @ 0x138E564C → 0x009EC618 → WorldPlayerComputer
mPlayers[7] @ 0x143F701C → 0x009EC618 → WorldPlayerComputer
mPlayers[8] @ 0x18C7584C → 0x009EC618 → WorldPlayerComputer
~~~

Estado: ✅.

## Prefixo comum dos player objects

Nas amostras:

~~~text
[player+0x04] = World*
~~~

Valores observados em `player+0x08`:

~~~text
Gaia        = 2
HumanOrCoop = 1
Computer    = 3
~~~

Em múltiplas mudanças da coluna visual **“Jog.”**, o Human continuou com `+0x08 = 1`.

Conclusão:

~~~text
player+0x08 != coluna “Jog.” / número-cor do lobby
~~~

A semântica exata do campo ainda é aberta.

## SlotRecord

A tabela usada pelos fluxos de configuração tem stride `0x28`.

Exemplos runtime:

~~~text
SlotRecord jogador 1 = 0x00B01AF8
SlotRecord jogador 2 = 0x00B01B20
próximo              = 0x00B01B48
...
~~~

O endereço `0x00B01AD0` não deve ser tratado como “SlotRecord da Gaia”. O dump mostrou que `0x00B01AF4` contém um ponteiro global usado pelas rotinas auxiliares, e os registros normais começam em `0x00B01AF8`.

### SlotRecord + 0x18

Confirmado como índice usado para resolver o config correspondente:

~~~text
0x00B01AF8 + 0x18 = 0
0x00B01B20 + 0x18 = 1
0x00B01B48 + 0x18 = 2
...
~~~

Nome provisório:

~~~text
configIndex
~~~

Não confundir com `worldPlayerIndex`.

## FUN_00598310 / runtime 0x005A8310

Assembly runtime:

~~~asm
005A8310  PUSH ESI
005A8311  MOV ESI,[ECX+18]
005A8314  MOV ECX,[00B01AF4]
005A831A  CALL 00593640
005A831F  IMUL ECX,ESI,68
005A8323  MOV EAX,[EAX]
005A8325  ADD ECX,B0
005A832B  ADD EAX,ECX
005A832D  RET
~~~

Interpretação:

~~~cpp
ResolvedPlayerConfig* ResolveSlotConfig(SlotRecord* slot) {
    int i = slot->configIndex;          // +0x18
    auto base = *GetConfigBase(...);    // via global em 0x00B01AF4
    return base + 0xB0 + i * 0x68;
}
~~~

Stride de `ResolvedPlayerConfig`: `0x68`.

## FUN_00598750 / runtime 0x005A8750

Assembly runtime:

~~~asm
005A8750  PUSH EBP
005A8751  MOV EBP,ESP
005A8753  CALL 005A8310
005A8758  MOV ECX,[EAX+50]
005A875B  MOV EAX,[EBP+8]
005A875E  MOV [EAX],ECX
005A8760  POP EBP
005A8761  RET 4
~~~

Logo:

~~~text
ResolvedPlayerConfig + 0x50 = worldPlayerIndex
~~~

Estado: ✅.

## FUN_00598380 / runtime 0x005A8380

~~~asm
005A8380  CALL 005A8310
005A8385  MOV EAX,[EAX+54]
005A8388  RET
~~~

Logo:

~~~text
ResolvedPlayerConfig + 0x54 = humanity
~~~

Estado: ✅.

## FUN_00598880 / runtime 0x005A8880

Essa função chama `FUN_00598380` e testa o resultado contra:

~~~text
{2, 3, 4}
~~~

Interpretação:

~~~cpp
bool IsResolvablePlayerSlot(SlotRecord* slot) {
    int humanity = GetHumanity(slot);
    return humanity == 2 || humanity == 3 || humanity == 4;
}
~~~

O nome é provisório; a lógica do conjunto é confirmada.

## FUN_00598700 / runtime 0x005A8700

Fluxo confirmado:

~~~asm
005A8706  MOV EDI,ECX
005A8708  CALL 005A8880
...
005A8711  MOV EAX,[00B02C58]
005A8716  MOV ESI,[EAX+C8]
...
005A8723  MOV ECX,EDI
005A8726  CALL 005A8750
005A872B  MOV EAX,[EBP-4]
...
005A8736  MOV ECX,[ESI+184]
005A873C  MOV EAX,[ECX+EAX*8]
~~~

Interpretação:

~~~cpp
WorldPlayer* ResolveWorldPlayerFromSlot(SlotRecord* slot) {
    if (!IsResolvablePlayerSlot(slot))
        return nullptr;

    World* world = Owner->world;

    int i = GetWorldPlayerIndex(slot);
    if (i == INVALID)
        return nullptr;

    return world->mPlayers[i].get();
}
~~~

### Validação dinâmica

Humano:

~~~text
SlotRecord*       = 0x00B01AF8
configIndex       = 0
worldPlayerIndex  = 1
→ mPlayers[1]     = WorldPlayerHumanOrCoop
~~~

IA:

~~~text
SlotRecord*       = 0x00B01B20
configIndex       = 1
worldPlayerIndex  = 2
→ mPlayers[2]     = WorldPlayerComputer
~~~

Estado: ✅.

## ResolvedPlayerConfig: amostra de 8 configs

Base resolvida na amostra:

~~~text
base = 0x18637148
~~~

Campos observados:

| configIndex | worldPlayerIndex | humanity | interpretação observada |
|---:|---:|---:|---|
| 0 | 1 | 2 | humano |
| 1 | 2 | 4 | computador |
| 2 | -1 | 1 | sem WorldPlayer |
| 3 | -1 | 1 | sem WorldPlayer |
| 4 | -1 | 1 | sem WorldPlayer |
| 5 | -1 | 1 | sem WorldPlayer |
| 6 | -1 | 1 | sem WorldPlayer |
| 7 | -1 | 1 | sem WorldPlayer |

`humanity=3` ainda não foi observado diretamente e não deve ser nomeado.

## Coluna “Jog.” / playerNumberIndex

A coluna visual “Jog.” 1–8 foi localizada em:

~~~text
ResolvedPlayerConfig + 0x4C = playerNumberIndex
~~~

Validação dinâmica mantendo a mesma linha/configuração:

~~~text
Jog.1 → 0
Jog.2 → 1
Jog.3 → 2
~~~

Logo o campo é zero-based; para `Jog.1..8`, o domínio esperado é `0..7`.

O setter foi capturado por hardware breakpoint:

~~~asm
00615A30  PUSH EBP
00615A31  MOV  EBP,ESP
00615A33  MOV  EAX,[EBP+8]
00615A36  MOV  [ECX+4C],EAX
00615A39  POP  EBP
00615A3A  RET  4
~~~

### UI → configuração

A rotina `FUN_0064F0E0` inicializa `ESI=0`, calcula cada linha como `EDI + ESI*0x70` e encerra após `ESI==8`:

~~~asm
006500F3  XOR ESI,ESI
00650131  IMUL EAX,ESI,70h
00650139  ADD EAX,EDI
...
00651506  INC ESI
0065150D  CMP ESI,08
00651510  JC 00650131
~~~

Portanto `ESI = configIndex = 0..7` nesse fluxo.

A mesma rotina prepara:

~~~text
local_1BC = EDI
local_1B8 = ESI
~~~

e chama `FUN_00644C10`, que cria um objeto de 12 bytes:

~~~cpp
struct PlayerNumberCallback {
    void** vtable;       // +0x00 = 0x009D1128
    void* target;        // +0x04
    int32_t configIndex; // +0x08
};
~~~

O thunk virtual `00653CD0` transforma esse objeto em uma chamada:

~~~text
target->FUN_00658E40(configIndex)
~~~

`FUN_00658E40` resolve a seleção da UI e chama `FUN_00615A30` sobre o `ResolvedPlayerConfig` correspondente.

Cadeia consolidada:

~~~text
configIndex 0..7
  ↓
linha UI (stride 0x70)
  ↓
PlayerNumberCallback
  ↓
FUN_00658E40(configIndex)
  ↓
FUN_00615A30
  ↓
ResolvedPlayerConfig+0x4C = playerNumberIndex
~~~

Isso separa definitivamente:

~~~text
configIndex       = qual configuração/linha está sendo editada
playerNumberIndex = qual “Jog.” 1–8 está atribuído a ela
worldPlayerIndex  = índice correspondente em World.mPlayers quando existe
~~~

## Gaia

Confirmado:

~~~text
World.mPlayers[0] → WorldPlayerGaia
~~~

Não há evidência de que Gaia tenha um `SlotRecord` normal na tabela dos jogadores configuráveis do lobby.

Isso é coerente com o fato de Gaia não ser um jogador configurável normal; ela ocupa uma entry interna reservada em `World.mPlayers[0]`.

## Ciclo de vida

`World*`, `mPlayers` e objetos de player podem ser recriados.

Exemplo: um `World*` que antes era válido deixou de ter um `mPlayers.begin` plausível após mudança de estado, enquanto uma nova execução de `FUN_00598700` revelou um novo `World*`.

Conclusão operacional:

~~~text
não cachear WorldPlayer* indefinidamente
resolver a cadeia novamente após mudança de partida/setup
~~~

## Readiness — estado atual

Critério estrutural conservador:

~~~text
World != nullptr
vtable correta
mPlayers begin/end coerentes
2 <= size <= 9
0 <= mLocalPlayerIndex < size
~~~

Checks semânticos desejados:

~~~text
mPlayers[0] → Gaia
mPlayers[mLocalPlayerIndex] → HumanOrCoop
~~~

Mas `size > 0` ainda não garante que os `PlayerEntry.object` já estejam preenchidos.

## Snapshot dinâmico adicional — moduleBase 0x00D80000

Antes da partida, `0x01472C58` foi observado como nulo. Já em gameplay:

~~~text
[0x01472C58] = 0x03DFE440          ; Owner*
[0x03DFE508] = 0x18E14488          ; World*

World+0x174 = 1
World+0x184 = 0x14932B78
World+0x188 = 0x14932B90
World+0x18C = 0x14932B90
size = 3
~~~

Conteúdo do vetor:

~~~text
entry 0 object = 0x14D7070C, controlBlock = 0x14D70700
  vtable = 0x0135C8A0 → WorldPlayerGaia
  +0x04  = 0x18E14488
  +0x08  = 2

entry 1 object = 0x147D601C, controlBlock = 0x147D6010
  vtable = 0x0135CAE4 → WorldPlayerHumanOrCoop
  +0x04  = 0x18E14488
  +0x08  = 1

entry 2 object = 0x14C8D01C, controlBlock = 0x14C8D010
  vtable = 0x0135C618 → WorldPlayerComputer
  +0x04  = 0x18E14488
  +0x08  = 3
~~~

Assim, em gameplay-ready, foram confirmados de uma vez:

~~~text
mLocalPlayerIndex = 1
mPlayers[0] = Gaia
mPlayers[mLocalPlayerIndex] = HumanOrCoop
player+0x04 = World* nos três tipos
object = controlBlock + 0x0C nos três entries
~~~

### Teste dos call sites de FUN_007288B0

Com `moduleBase=0x00D80000`:

~~~text
0x0072F9FF → 0x010AF9FF
0x00731917 → 0x010B1917
0x00732E27 → 0x010B2E27
~~~

Os três breakpoints não dispararam na transição de partida testada. O breakpoint em `FUN_007288B0` propriamente dita disparava antes de “Iniciar o Jogo”, confirmando que é uma helper reutilizada e que não deve ser tratada como exclusiva de `mPlayers`.

## Próximo experimento

Em uma nova transição, resolver o `World*` atual e colocar hardware write em:

~~~text
World + 0x188   ; mPlayers.end
~~~

Objetivo: capturar a instrução que faz o tamanho do vetor crescer e então observar em que ponto as entries passam de slots vazios para `WorldPlayer*` semanticamente válidos.

Depois:

~~~text
1. consolidar IsWorldReady() como API
2. fechar GetLocalPlayer() como API pública
3. mapear Food/Wood/Gold/Stone
~~~

## Captura de inicialização até gameplay — 2026-10-12

Sessão x32dbg: `AoK HD.exe 5.8.INT`, `moduleBase=0x003B0000`, novo `World*=0x0F5428D0`.

Linha do tempo observada:

1. `0x005DE68B` publica o novo `World*` em `Owner+0xC8`.
2. `mPlayers` inicialmente apresenta `begin=end=capacityEnd=0`.
3. Em `FUN_007288B0`, runtime `0x006D8A0C/11/19`, são publicados `begin=0x17E24EE8`, `end=0x17E24F00` e `capacityEnd=0x17E24F00` (três slots de 8 bytes).
4. **Os 24 bytes dos três slots estavam zerados mesmo após o resize:** size 3 não comprova `WorldPlayer*`.
5. Hardware breakpoint em `0x17E24EE8` capturou a primeira entry parcialmente preenchida: `object=0x13A78734`, `controlBlock=0`.
6. `0x00648F75: mov [ebx+4],edx` publicou `controlBlock=0x13A78728`. `object-controlBlock=0x0C`.
7. Sem disparo dos breakpoints nas posições `0x17E24EF0` e `0x17E24EF8`, ambas foram posteriormente vistas preenchidas, com o vetor no mesmo buffer. Ainda não se conhece a causa nem a ordem dessas escritas.
8. Snapshot final: Gaia `0x13A78734` / cb `0x13A78728`, Humano `0x1B11A85C` / cb `0x1B11A850`, IA `0x17F11854` / cb `0x17F11848`.
9. Vtables, back-pointers para `World*=0x0F5428D0` e campo `+0x08=2/1/3` coincidiram; `mLocalPlayerIndex=1` resolve para HumanOrCoop.
10. A partida foi confirmada no mapa jogável com unidades selecionáveis e comandos disponíveis.

**Evidência forte:** há diferença real entre `World*` publicado, vetor dimensionado, par shared_ptr-like parcialmente publicado e jogadores estruturalmente válidos. **Não demonstrado:** instante exato em que ambos os últimos jogadores foram gravados, primeira transição `IsWorldStructurallyReady=false→true` ou suficiência do predicado estrutural para qualquer operação em todos os estados.

Critério proposto para um validador de leitura: resolver a cadeia atual, verificar vtable de World, validar ordem/alinhamento/limites de `begin/end/cap` e tamanho plausível, conferir `localPlayerIndex` dentro do vetor e objetos/controles necessários não nulos, confirmar vtables e back-pointers. Fazer leituras com falha segura; não desreferenciar ponteiros externos sem proteção, nem cacheá-los indefinidamente. `GameplayReady` requer verificação adicional e ainda não está formalizado.
