# Funções mapeadas

Última atualização: 2026-10-07.

## Convenção de endereços

Os endereços principais abaixo são os endereços estáticos do binário com ImageBase `0x00400000`.

ASLR varia entre execuções. Já foram observados `moduleBase = 0x00410000` e `moduleBase = 0x00D80000`; sempre recalcular o runtime por RVA.

## FUN_006889B0

Endereço estático: `0x006889B0`.

Papel: construtor/inicializador estrutural de `World`.

~~~asm
006889D6  MOV ESI,ECX
006889DE  CALL FUN_00729360
006889F1  MOV dword ptr [ESI],009D329C
~~~

Estado: ✅.

## FUN_00697800

- Endereço: `0x00697800`
- Classe: `World`
- Vtable: slot 33 / `+0x84`
- Chama `FUN_00735A00`
- Mesmo `World*` em ECX

Estado: ✅.

## FUN_00735A00

- Endereço: `0x00735A00`
- `this = World*`
- `+0x174 → mLocalPlayerIndex`
- `+0x184 → mPlayers.begin`
- `+0x188 → mPlayers.end`
- serializa dados relacionados a slots, incluindo `worldPlayerIndex`, `humanity` e nome.

Estado: ✅ para os campos acima.

## FUN_0062E5D0

- Endereço: `0x0062E5D0`
- `this = Owner*`
- cria/instala `World*` em `Owner+0xC8`.

Estado: ✅ para a relação estrutural.

## FUN_0062E5D0 — criação/publicação de World

Endereço estático: `0x0062E5D0`.

Fluxo confirmado:

~~~asm
0062E62E  PUSH 2DCh
0062E633  CALL operator_new
...
0062E65A  CALL FUN_006889B0
...
0062E671  MOV EAX,[EBP+local_20] ; Owner*
0062E674  ADD EAX,0C8h
...
0062E689  MOV EDX,[EAX]          ; World* antigo
0062E68B  MOV [EAX],ECX          ; Owner+0xC8 = novo World*
~~~

Depois da publicação há chamada virtual pelo slot `World.vtable+0x04`, resolvido para `FUN_0072D1A0`.

Também existe rollback:

~~~asm
0062E734  MOV ECX,[ESI+0C8h]
0062E73A  MOV [ESI+0C8h],0
...
0062E74C  CALL [EAX]
~~~

Conclusão: `Owner+0xC8 != nullptr` isoladamente não prova gameplay readiness.

Estado: ✅ para publicação/rollback; 🟡 para semântica completa da inicialização virtual.

## FUN_0072D1A0 — World vtable slot +0x04

Endereço: `0x0072D1A0`.
Vtable entry: `0x009D32A0`.

No início instala/solta estado em `World+0x170` e zera `World+0x17C`. Até agora não foi localizada escrita direta em `+0x184/+0x188/+0x18C` nessa função.

Estado: 🟡.

## FUN_00659750

~~~asm
0065979D  MOV ECX,[DAT_00AF2C58]
006597A8  CALL FUN_0062E5D0
~~~

Conclusão:

~~~text
[DAT_00AF2C58] = Owner*
~~~

Estado: ✅.

## FUN_00430160 — DestroySharedPtrRange

Endereço: `0x00430160`.

Percorre `[begin,end)` com stride 8:

~~~asm
00430170  MOV ESI,[EDI+4]
...
0043017A  XADD.LOCK [ESI+4],EAX
...
00430185  CALL [EAX]
...
0043018A  XADD.LOCK [ESI+8],EAX
...
00430195  CALL [EAX+4]
00430198  ADD EDI,8
~~~

Interpretação:

~~~text
entry+0x4 = controlBlock
controlBlock+0x4 = strong refcount
controlBlock+0x8 = weak refcount
~~~

Estado: ✅.

## FUN_00430DD0 — DestroyPlayersVectorLike

Endereço: `0x00430DD0`.

Com `ECX=&mPlayers` destrói `[begin,end)`, libera o buffer e zera:

~~~text
vector+0 = begin
vector+4 = end
vector+8 = capacityEnd
~~~

Estado: ✅.

## FUN_0072C6E0 — reset/clear de World

Endereço: `0x0072C6E0`.

Trecho-chave:

~~~asm
0072C82D  MOV EAX,[ESI+188]
0072C833  MOV EDI,[ESI+184]
...
0072C840  MOV ECX,[EDI]
0072C846  CALL FUN_0074E5C0
0072C84E  ADD EDI,8
...
0072C85C  LEA EDI,[ESI+184]
0072C868  CALL FUN_00430160
0072C86D  MOV EAX,[EDI]
0072C872  MOV [EDI+4],EAX
~~~

Interpretação estrutural: percorre players, destrói shared_ptrs e faz `end = begin`, preservando capacidade.

Estado: ✅ estrutural.

## FUN_0072F7D0 — reconstrução/preparo de mPlayers

Endereço: `0x0072F7D0`.

Fluxo:

~~~asm
0072F8F0  MOV [EDI+174],EAX
...
0072F9A8  LEA EAX,[EDI+184]
0072F9B4  CALL FUN_00430160
0072F9B9  MOV EAX,[EDI+184]
0072F9C2  MOV [EDI+188],EAX
...
0072F9F2  LEA ECX,[EDI+184]
0072F9FF  CALL FUN_007288B0
...
0072FA89  MOV [EDI+174],1
~~~

Confirma clear + resize de `mPlayers` e escrita em `mLocalPlayerIndex`.

Na execução com `moduleBase=0x00D80000`, foram armados breakpoints nos três call sites conhecidos de `FUN_007288B0`:

~~~text
0x0072F9FF → runtime 0x010AF9FF
0x00731917 → runtime 0x010B1917
0x00732E27 → runtime 0x010B2E27
~~~

Nenhum deles disparou ao iniciar a partida testada. Portanto, embora a função e esses callers sejam estruturalmente relevantes, eles não representam necessariamente o caminho de população de `mPlayers` usado por toda transição de jogo.

Estado: ✅ estrutural / 🟡 fronteira temporal de readiness.

## FUN_007288B0 — ResizePlayersVectorLike

Endereço: `0x007288B0`.

No início:

~~~asm
007288E3  MOV EDX,[EDI+4]
007288E6  MOV ESI,[EDI]
007288EA  SUB EAX,ESI
007288EC  SAR EAX,3
...
007288F2  MOV ECX,[EDI+8]
007288F5  SUB ECX,ESI
007288F7  SAR ECX,3
~~~

Logo:

~~~text
size     = (end - begin) / 8
capacity = (capacityEnd - begin) / 8
~~~

A função implementa os três caminhos de resize. Se precisa realocar, cresce aproximadamente `max(newSize, capacity + capacity/2)`, aloca `newCapacity*8` e ao final escreve begin/end/capacityEnd.

Estado: ✅.

## FUN_00583980 — GetSlotRecord28

Endereço estático: `0x00583980`.

Assembly:

~~~asm
MOV EAX,[arg]
LEA EAX,[EAX+EAX*4]
LEA EAX,[EAX*8+DAT_00AF1AD0]
RET 4
~~~

Retorna:

~~~text
DAT_00AF1AD0 + index * 0x28
~~~

Nome `GetSlotRecord28` é provisório; o stride `0x28` é confirmado.

Estado: ✅ estrutural.

## FUN_00598310 — ResolveSlotConfig

Endereço estático: `0x00598310`.
Runtime observado: `0x005A8310`.

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
    const int i = slot->configIndex; // +0x18
    return base + 0xB0 + i * 0x68;
}
~~~

Estado: ✅ para a relação `SlotRecord+0x18 → config com stride 0x68`.

## FUN_00598380 — GetHumanity

Endereço estático: `0x00598380`.
Runtime observado: `0x005A8380`.

~~~asm
005A8380  CALL 005A8310
005A8385  MOV EAX,[EAX+54]
005A8388  RET
~~~

Logo:

~~~text
ResolvedPlayerConfig + 0x54 = humanity
~~~

Valores observados:

~~~text
2 → humano
4 → computador
1 → configs sem WorldPlayer na amostra
~~~

`3` ainda não foi observado diretamente.

Estado: ✅ para o offset; 🟡 para enumeração completa.

## FUN_00598750 — GetWorldPlayerIndex

Endereço estático: `0x00598750`.
Runtime observado: `0x005A8750`.

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

Exemplos:

~~~text
SlotRecord 0x00B01AF8 → 1
SlotRecord 0x00B01B20 → 2
~~~

Estado: ✅.

## FUN_00598880 — predicate de humanity {2,3,4}

Endereço estático: `0x00598880`.
Runtime observado: `0x005A8880`.

A função:

1. chama `FUN_00598380`;
2. compara o resultado contra `2`, `3` e `4`;
3. retorna true se houver match.

Equivalente:

~~~cpp
bool IsResolvablePlayerSlot(SlotRecord* slot) {
    int h = GetHumanity(slot);
    return h == 2 || h == 3 || h == 4;
}
~~~

Nome semântico provisório.

Estado: ✅ para a lógica.

## FUN_00598700 — ResolveWorldPlayerFromSlot

Endereço estático: `0x00598700`.
Runtime observado: `0x005A8700`.

Trecho runtime:

~~~asm
005A8706  MOV EDI,ECX
005A8708  CALL 005A8880
005A870D  TEST AL,AL
005A870F  JE ...
005A8711  MOV EAX,[00B02C58]
005A8716  MOV ESI,[EAX+C8]
...
005A8723  MOV ECX,EDI
005A8726  CALL 005A8750
005A872B  MOV EAX,[EBP-4]
005A872E  CMP [00ABB914],EAX
...
005A8736  MOV ECX,[ESI+184]
005A873C  MOV EAX,[ECX+EAX*8]
~~~

Interpretação consolidada:

~~~cpp
WorldPlayer* ResolveWorldPlayerFromSlot(SlotRecord* slot) {
    if (!IsResolvablePlayerSlot(slot))
        return nullptr;

    World* world = Owner->world;

    int index;
    GetWorldPlayerIndex(slot, &index);

    if (index == invalidSentinel)
        return nullptr;

    return world->mPlayers[index].get();
}
~~~

Validação dinâmica:

~~~text
0x00B01AF8 → worldPlayerIndex 1 → WorldPlayerHumanOrCoop
0x00B01B20 → worldPlayerIndex 2 → WorldPlayerComputer
~~~

Estado: ✅.

## FUN_00615A30 — SetPlayerNumberIndex

Endereço estático: `0x00615A30`.
Runtime observado com `moduleBase=0x00D80000`: `0x00F95A30`.

~~~asm
00615A30  PUSH EBP
00615A31  MOV  EBP,ESP
00615A33  MOV  EAX,[EBP+8]
00615A36  MOV  [ECX+4C],EAX
00615A39  POP  EBP
00615A3A  RET  4
~~~

Interpretação:

~~~cpp
void ResolvedPlayerConfig::SetPlayerNumberIndex(int value) {
    this->playerNumberIndex = value; // +0x4C
}
~~~

Validação dinâmica:

~~~text
Jog.1 → 0
Jog.2 → 1
Jog.3 → 2
~~~

Estado: ✅.

## FUN_00658E40 — ApplyPlayerNumberSelection

Endereço estático: `0x00658E40`.

Recebe o índice da configuração/linha como primeiro argumento. O fluxo confirmado resolve a seleção feita no controle da UI e, no caminho normal observado, resolve `ResolvedPlayerConfig[configIndex]` e chama `FUN_00615A30`.

Trecho-chave:

~~~asm
00658E6D  MOV  EDI,[EBP+8]       ; configIndex
00658E78  IMUL ESI,EDI,70h
...
00658EE4  IMUL EAX,EDI,68h
00658EE7  ADD  EAX,B0h
00658EEC  ADD  ECX,EAX
00658EEE  CALL FUN_00615A30
~~~

Nome semântico provisório, mas o papel na cadeia de “Jog.” está confirmado.

Estado: ✅ estrutural / 🟡 para nomes dos objetos auxiliares.

## FUN_00644C10 — CreatePlayerNumberCallback

Endereço estático: `0x00644C10`.

Aloca `0x0C` bytes e constrói um callback:

~~~asm
00644C3B  PUSH 0Ch
00644C3D  CALL operator_new
...
00644C51  MOV [EDX],009D1128
00644C57  MOV ECX,[EBP+8]
00644C5A  MOV EAX,[ECX]
00644C5C  MOV [EDX+4],EAX
00644C5F  MOV EAX,[ECX+4]
00644C62  MOV [EDX+8],EAX
~~~

Modelo:

~~~cpp
struct PlayerNumberCallback {
    void** vtable;       // +0x00 = 0x009D1128
    void* target;        // +0x04
    int32_t configIndex; // +0x08
};
~~~

Estado: ✅.

## LAB_00653CD0 — callback thunk

Entrada virtual da vtable `0x009D1128`.

~~~asm
00653CD0  MOV EAX,[ECX+8]
00653CD3  PUSH ECX
00653CD4  MOV EDX,ESP
00653CD6  MOV [EDX],EAX
00653CD8  MOV ECX,[ECX+4]
00653CDB  CALL FUN_00658E40
00653CE0  RET
~~~

Equivalente aproximado:

~~~cpp
void PlayerNumberCallback::Invoke() {
    target->ApplyPlayerNumberSelection(configIndex);
}
~~~

Estado: ✅.

## FUN_0064F0E0 — construtor/configurador da UI de 8 linhas

Endereço estático: `0x0064F0E0`.

A função contém um loop explícito de oito entradas:

~~~asm
006500F3  XOR ESI,ESI
00650131  IMUL EAX,ESI,70h
00650139  ADD EAX,EDI
...
0065011F  MOV [EBP+local_1BC],EDI
...
0065091B  MOV [EBP+local_1B8],ESI
00650936  LEA EAX,[EBP+local_1BC]
0065093D  CALL FUN_00644C10
...
00651506  INC ESI
0065150D  CMP ESI,08
00651510  JC 00650131
~~~

Conclusões:

~~~text
ESI = configIndex = 0..7
linha/config UI usa stride 0x70
callback.target = EDI
callback.configIndex = ESI
~~~

Estado: ✅ para o loop e vínculo `configIndex → callback`; 🟡 para o nome semântico exato do objeto em EDI.

## FUN_00593640

Na execução runtime foi chamada em `0x00593640`.

Seu retorno é usado por `FUN_00598310`; o comportamento observado é compatível com fornecer o endereço do ponteiro/base interno localizado a `+0xD54` do objeto de configuração global carregado via `[00B01AF4]`.

Estado: 🟡 para o nome/semântica exata; ✅ para o papel na cadeia observada.

## WorldPlayerGaia

Vtable estática: `0x009DC8A0`.

Runtime com base `0x00410000`: `0x009EC8A0`.

Estado: ✅.

## WorldPlayerHumanOrCoop

Vtable estática: `0x009DCAE4`.

Runtime com base `0x00410000`: `0x009ECAE4`.

Estado: ✅.

## WorldPlayerComputer

Vtable estática: `0x009DC618`.

Runtime com base `0x00410000`: `0x009EC618`.

Estado: ✅.

## FUN_00729360

Chamada no início de `FUN_006889B0` antes da escrita da vtable de `World`.

Hipótese: inicializador/construtor de `BaseWorld`.

Confiança: 🟡.

## Próximo experimento de readiness

Breakpoint estático:

~~~text
0x0072FA04
~~~

É a instrução imediatamente após `0x0072F9FF CALL FUN_007288B0`.

Ao parar, inspecionar:

~~~text
EDI = World*
[EDI+174]
[EDI+184]
[EDI+188]
[EDI+18C]
entries em [[EDI+184]]
~~~

Objetivo: distinguir `size > 0` de `PlayerEntry.object != nullptr` e localizar a fronteira real em que Gaia/jogadores passam a existir.

## Nota de lifecycle

Uma nova instância de `World` pode possuir vtable correta enquanto ponteiros derivados de uma instância anterior já estão obsoletos.

A futura API deve:

~~~text
1. resolver Owner
2. resolver World atual
3. validar readiness
4. resolver mPlayers atual
5. só então resolver WorldPlayer
~~~

Evitar cache longo de `WorldPlayer*`.


## Snapshot dinâmico de gameplay — moduleBase 0x00D80000

Após a partida iniciar, a cadeia foi resolvida como:

~~~text
[0x01472C58] = 0x03DFE440          ; Owner*
[0x03DFE508] = 0x18E14488          ; Owner+0xC8 = World*

World+0x174 = 1
World+0x184 = 0x14932B78
World+0x188 = 0x14932B90
World+0x18C = 0x14932B90
size = 3
~~~

Players:

~~~text
0: 0x14D7070C → vtable 0x0135C8A0 → Gaia
1: 0x147D601C → vtable 0x0135CAE4 → HumanOrCoop
2: 0x14C8D01C → vtable 0x0135C618 → Computer
~~~

Para os três, `player+0x04 = 0x18E14488`, confirmando back-pointer para o mesmo `World`. Os valores `player+0x08` foram `2`, `1` e `3`, respectivamente.

Esse snapshot fecha a validação semântica em estado gameplay-ready; resta localizar a instrução exata que aumenta `World+0x188` durante a criação da partida.

## Observações dinâmicas complementares — 2026-10-12

Base do módulo nesta sessão: `0x003B0000` (base estática `0x00400000`; runtime = base do módulo + endereço estático - base estática).

### Publicação de World

`FUN_0062E5D0`, runtime `0x005DE5D0`: em `0x005DE68B`, `mov [eax],ecx` gravou `World*=0x0F5428D0` em `Owner+0xC8=0x049EA020`. Nesse instante o vetor ainda podia estar nulo.

### Resize de mPlayers

`FUN_007288B0`, runtime `0x006D88B0`, observada com `EDI=&World.mPlayers=0x0F542A54`, `ESI=0x17E24EE8` e `EBX=3`:
```asm
006D8A0C  mov [edi],esi
006D8A0E  lea eax,[esi+ebx*8]
006D8A11  mov [edi+4],eax
006D8A14  mov eax,[ebp-24]
006D8A17  add eax,esi
006D8A19  mov [edi+8],eax
```
Resultou em `begin=0x17E24EE8`, `end=capacityEnd=0x17E24F00`. Os primeiros 24 bytes do buffer estavam **zerados** após a alocação; resize não é sinônimo de criação de `WorldPlayer`.

### Publicação de um PlayerEntry (Gaia)

Um breakpoint hardware write DWORD em `0x17E24EE8` capturou `entry[0].object=0x13A78734` com `entry[0].controlBlock=0`, `EBX=0x17E24EE8` e `EDX=0x13A78728`. O disassembly seguinte:
```asm
00648F69  mov ecx,[ebx+4]  ; bloco anterior (nulo na amostra)
00648F6C  mov [ebp-20],eax
00648F6F  mov [ebp-18],ecx
00648F72  mov [ebp-1C],ecx
00648F75  mov [ebx+4],edx  ; publica novo controlBlock
00648F7C  test ecx,ecx     ; se anterior não nulo, segue release
```
Após `0x00648F75`, a memória mostrou `{object=0x13A78734,controlBlock=0x13A78728}`. O endereço da instrução **exata** que escreveu `object` ainda requer o disassembly imediatamente anterior a `0x00648F69`.

As entradas 1 e 2 estavam depois completas e foram validadas como HumanOrCoop e Computer. Os hardware breakpoints de escrita nessas posições não dispararam; a razão ainda é desconhecida. Os valores observados coincidiam com gameplay interativo, mas a fronteira temporal entre prontidão estrutural e operacional **não foi medida**.
