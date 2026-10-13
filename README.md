# AoE2HDControl

Projeto de engenharia reversa e reconstrução de uma API de controle para **Age of Empires II HD (AoK HD.exe 5.8.INT, x86)**. O **AoE2Control para Definitive Edition** é usado apenas como referência conceitual para definir alvos semânticos; offsets, layouts e estruturas do DE não são assumidos válidos no HD.

## Uso local (Windows / Visual Studio 2026)

Pasta de trabalho padrão: `C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl`.

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_probe.exe
```

O `aoe2hd_probe.exe` apresenta `World`, jogadores e, quando disponíveis, comida, madeira, pedra e ouro. Para acompanhar transições de `World` entre partidas: `.\build2026\Release\aoe2hd_monitor.exe --interval-ms 250`. A leitura é somente leitura; os resultados ainda precisam ser validados em outras partidas.

**Nota:** `git pull` requer um clone com `.git`. Se a pasta veio de um ZIP, preserve arquivos próprios e clone o repositório corretamente. Ao renomear/mover o projeto, recrie `build2026` para eliminar os caminhos absolutos armazenados pelo CMake.

## Estado atual

A cadeia central até `World`, o vetor `mPlayers`, a ponte entre configuração/lobby e `WorldPlayer` e parte relevante do lifecycle de construção/reset foram validadas com Ghidra, RTTI e x32dbg.

Principais descobertas confirmadas:

- ImageBase estático: `0x00400000`;
- RTTI confirma `World : BaseWorld`;
- vtable estática de `World`: `0x009D329C`;
- `FUN_00697800` é o slot virtual 33 (`+0x84`);
- `FUN_006889B0` instala a vtable de `World`;
- tamanho alocado observado de `World`: `0x2DC` bytes;
- `DAT_00AF2C58` fornece o `Owner*` usado na cadeia validada;
- `Owner + 0xC8 → World*`;
- `FUN_0062E5D0` publica o novo `World*` em `Owner+0xC8` em `0x0062E68B` e possui caminho de rollback que zera o mesmo campo em `0x0062E73A`;
- portanto `Owner->world != nullptr` isoladamente não prova readiness;
- `World + 0x174 → mLocalPlayerIndex`;
- `World + 0x184/+0x188/+0x18C` formam um `std::vector` x86 confirmado;
- `mPlayers.size() = (end - begin) / 8`;
- cada entry de `mPlayers` tem 8 bytes e o destrutor confirma semântica shared_ptr-like;
- `FUN_00430160` destrói entries em stride 8 e decrementa strong/weak refs;
- `FUN_00430DD0` destrói/libera o vetor e zera begin/end/capacity;
- `FUN_0072C6E0` limpa `mPlayers` mantendo a capacidade (`end = begin`);
- `FUN_0072F7D0` limpa/redimensiona `mPlayers` e mexe em `mLocalPlayerIndex`;
- `FUN_007288B0` é uma rotina `resize`-like do vetor, com crescimento de capacidade ~1,5x;
- RTTI confirma `WorldPlayerGaia`, `WorldPlayerHumanOrCoop` e `WorldPlayerComputer`;
- `SlotRecord` tem stride `0x28`;
- `SlotRecord+0x18` seleciona um `ResolvedPlayerConfig`;
- `ResolvedPlayerConfig` tem stride `0x68`;
- `ResolvedPlayerConfig+0x4C = playerNumberIndex` (zero-based: `Jog.1→0`, `Jog.2→1`, `Jog.3→2`; esperado `0..7` para `Jog.1..8`);
- `ResolvedPlayerConfig+0x50 = worldPlayerIndex`;
- `ResolvedPlayerConfig+0x54 = humanity`;
- observado: `humanity=2` para humano, `humanity=4` para computador e `humanity=1` em configs sem `WorldPlayer`;
- `FUN_00598700` resolve `SlotRecord → World.mPlayers[worldPlayerIndex]`;
- `FUN_00598880` aceita `humanity ∈ {2,3,4}`;
- Gaia ocupa `World.mPlayers[0]`, mas não há evidência de um `SlotRecord` normal correspondente no lobby;
- a UI do lobby mantém 8 linhas/configurações com stride `0x70` e `configIndex=0..7`;
- `FUN_00615A30` escreve `ResolvedPlayerConfig+0x4C` e funciona como setter do `playerNumberIndex`;
- `FUN_00658E40(configIndex)` aplica a seleção de “Jog.” da linha correspondente;
- callbacks de 12 bytes usam vtable `0x009D1128`, `+0x04=target` e `+0x08=configIndex`; o thunk `00653CD0` chama `target->FUN_00658E40(configIndex)`.

## Cadeia runtime validada

~~~text
Owner global RVA = 0x006F2C58
World vtable RVA = 0x005D329C

[moduleBase + 0x006F2C58]
        ↓
      Owner*
        ↓ +0xC8
      World*
        ├── +0x000 → vtable = moduleBase + 0x005D329C
        ├── +0x174 → mLocalPlayerIndex
        ├── +0x184 → mPlayers.begin
        ├── +0x188 → mPlayers.end
        └── +0x18C → mPlayers.end_of_storage
~~~

Em uma execução com `moduleBase = 0x00410000`, os endereços runtime relevantes incluíram:

~~~text
Owner global = 0x00B02C58
World vtable = 0x009E329C

FUN_00598700 runtime = 0x005A8700
FUN_00598750 runtime = 0x005A8750
FUN_00598310 runtime = 0x005A8310
FUN_00598380 runtime = 0x005A8380
FUN_00598880 runtime = 0x005A8880
~~~

## World.mPlayers: partida com 8 jogadores

Em uma partida com 1 humano + 7 IAs:

~~~text
World*                 = 0x18839030
mPlayers.begin         = 0x1865C968
mPlayers.end           = 0x1865C9B0
mPlayers.end_of_storage= 0x1865C9B0
size                   = 9
~~~

Logo:

~~~text
mPlayers[0] = Gaia
mPlayers[1] = HumanOrCoop
mPlayers[2] = Computer
mPlayers[3] = Computer
mPlayers[4] = Computer
mPlayers[5] = Computer
mPlayers[6] = Computer
mPlayers[7] = Computer
mPlayers[8] = Computer
~~~

Vtables runtime confirmadas:

~~~text
Gaia        = 0x009EC8A0
HumanOrCoop = 0x009ECAE4
Computer    = 0x009EC618
~~~

Nas 9 entries observadas, `object = controlBlock + 0x0C`, reforçando fortemente o modelo `std::shared_ptr`.

## Configuração/lobby → WorldPlayer

A cadeia confirmada é:

~~~text
SlotRecord
  +0x18 → configIndex
        ↓
ResolveSlotConfig
        ↓
ResolvedPlayerConfig (stride 0x68)
  +0x50 → worldPlayerIndex
  +0x54 → humanity
        ↓
World.mPlayers[worldPlayerIndex]
~~~

Exemplos confirmados em runtime:

~~~text
SlotRecord 0x00B01AF8
  configIndex      = 0
  worldPlayerIndex = 1
  humanity         = 2
  → WorldPlayerHumanOrCoop

SlotRecord 0x00B01B20
  configIndex      = 1
  worldPlayerIndex = 2
  humanity         = 4
  → WorldPlayerComputer
~~~

Nos configs seguintes da amostra:

~~~text
configIndex 2..7
worldPlayerIndex = -1
humanity = 1
~~~

## Campo player+0x08

Valores repetidamente observados:

~~~text
Gaia        = 2
HumanOrCoop = 1
Computer    = 3
~~~

Isso confirma que o campo **não é a coluna “Jog.” 1–8 do lobby**. A semântica exata ainda permanece aberta; tratá-lo apenas como discriminador/tipo observado.

## Ciclo de vida

O jogo pode recriar `World`, `mPlayers` e os objetos de player durante mudanças de estado. Não armazenar ponteiros de player indefinidamente.

Regra operacional:

~~~text
resolver Owner → World → mPlayers novamente
após criação/troca de partida ou mudanças relevantes de setup
~~~

Uma vtable válida, isoladamente, não garante que o `World` esteja gameplay-ready.

## Campo “Jog.” 1–8 fechado

A coluna visual do lobby foi localizada em:

~~~text
ResolvedPlayerConfig + 0x4C = playerNumberIndex
~~~

Validação dinâmica:

~~~text
Jog.1 → 0
Jog.2 → 1
Jog.3 → 2
~~~

O valor é independente de `configIndex`, `worldPlayerIndex`, `player+0x08` e `mLocalPlayerIndex`.

Cadeia estrutural confirmada:

~~~text
configIndex 0..7
  ↓
linha da UI (stride 0x70)
  ↓
PlayerNumberCallback { target, configIndex }
  ↓
FUN_00658E40(configIndex)
  ↓
FUN_00615A30
  ↓
ResolvedPlayerConfig+0x4C = playerNumberIndex
~~~

## Readiness/lifecycle — progresso atual

O estado do vetor pode passar por:

~~~text
buffer alocado, vetor vazio:
begin != nullptr
end == begin
capacityEnd != nullptr
~~~

Logo, readiness deve usar o tamanho real e validar objetos concretos, não apenas ponteiros do vetor.

`FUN_0072F7D0` faz:

~~~text
mPlayers.clear()
→ FUN_007288B0(&mPlayers, newSize, ...)
→ continua inicialização
~~~

`FUN_007288B0` calcula explicitamente size/capacity por divisão por 8 e implementa os três caminhos típicos de `std::vector::resize`.

### Snapshot runtime adicional — `moduleBase = 0x00D80000`

Em uma partida com Gaia + humano + IA:

~~~text
Owner global runtime     = 0x01472C58
[Owner global]           = 0x03DFE440
Owner+0xC8               = 0x03DFE508
[Owner+0xC8]             = 0x18E14488 = World*

World+0x174              = 1
World+0x184 begin        = 0x14932B78
World+0x188 end          = 0x14932B90
World+0x18C capacityEnd  = 0x14932B90
size                     = 3
~~~

As três entries foram validadas por vtable e back-pointer:

~~~text
mPlayers[0] = 0x14D7070C → vtable 0x0135C8A0 → Gaia
mPlayers[1] = 0x147D601C → vtable 0x0135CAE4 → HumanOrCoop
mPlayers[2] = 0x14C8D01C → vtable 0x0135C618 → Computer

[player+0x04] = 0x18E14488 para os três
[player+0x08] = 2 / 1 / 3 respectivamente
~~~

Isso valida dinamicamente o critério semântico de readiness em gameplay:

~~~text
mPlayers[0] → Gaia
mPlayers[mLocalPlayerIndex] → HumanOrCoop
back-pointer player+0x04 → World atual
~~~

Os breakpoints nos três call sites conhecidos de `FUN_007288B0` (`0x0072F9FF`, `0x00731917`, `0x00732E27`) não dispararam na transição de partida testada; portanto a fronteira temporal exata que popula `mPlayers` continua aberta.

## Próximo passo de maior retorno

Capturar por hardware write o `World+0x188` da instância recém-criada em uma nova transição de partida, identificando a instrução que faz `mPlayers.end` crescer e, a partir dela, fechar a fronteira temporal `slots vazios → WorldPlayer* válidos`. Depois disso, consolidar `IsWorldReady()` como API e iniciar `Food/Wood/Gold/Stone`.

## Documentação

- `docs/reverse-engineering.md`
- `docs/functions.md`
- `docs/structures.md`
- `docs/roadmap.md`
- `docs/aoe2control-reference.md` — referência conceitual do AoE2Control/DE, sem transplante de offsets

## Captura temporal de mPlayers — 2026-10-12 (x32dbg)

Em **AoK HD.exe 5.8.INT x86**, com `moduleBase=0x003B0000`, foi acompanhada uma nova instância desde a publicação de `World*` até a validação dos jogadores em uma partida jogável. Endereços abaixo são **runtime da amostra**, não offsets portáveis.

- `OwnerGlobal=0x00AA2C58`; `Owner*=0x049E9F58`; `Owner+0xC8=0x049EA020`.
- Em `0x005DE68B` (`mov [eax],ecx`), foi publicado `World*=0x0F5428D0`.
- `World+0x174=0x0F542A44` continha `mLocalPlayerIndex=1`.
- `World+0x184=0x0F542A54`: `begin=0x17E24EE8`, `end=capacityEnd=0x17E24F00`; tamanho e capacidade = 3.

A rotina `FUN_007288B0` (runtime `0x006D88B0`) instalou os limites do vetor:
```asm
006D8A0C  mov [edi],esi           ; begin
006D8A0E  lea eax,[esi+ebx*8]   ; end = begin + 3*8
006D8A11  mov [edi+4],eax        ; end
006D8A19  mov [edi+8],eax        ; capacityEnd
```
**Importante:** logo após o resize, as **três entradas existiam, mas seus 24 bytes estavam zerados**. Assim, `size=3` não implica objetos criados. O primeiro `object` foi observado sendo gravado em `0x17E24EE8` (`0x13A78734`), enquanto `controlBlock` ainda era zero. Depois, `0x00648F75: mov [ebx+4],edx` publicou `controlBlock=0x13A78728`, completando o par.

Snapshot final validado por vtable, back-pointer e campo `+0x08`:

| Índice | Classe | object | controlBlock | vtable runtime | +0x08 |
|---:|---|---|---|---|---:|
| 0 | Gaia | `0x13A78734` | `0x13A78728` | `0x0098C8A0` | 2 |
| 1 | HumanOrCoop | `0x1B11A85C` | `0x1B11A850` | `0x0098CAE4` | 1 |
| 2 | Computer | `0x17F11854` | `0x17F11848` | `0x0098C618` | 3 |

Os três objetos apontavam em `+0x04` para `World*=0x0F5428D0`; todos obedeciam a `object=controlBlock+0x0C`. O índice local **1** selecionava HumanOrCoop. O usuário confirmou **mapa jogável, unidades selecionáveis e comandos disponíveis** nesse estado.

**Limite da evidência:** os breakpoints de escrita nas entradas 1 e 2 não dispararam apesar de seus campos aparecerem preenchidos posteriormente, sem realocação do buffer. Isso não estabelece a ordem nem o instante exato da publicação dessas entradas; a causa da ausência de disparo permanece indeterminada. A coincidência de prontidão estrutural com gameplay jogável foi observada nesse snapshot, **não** a primeira transição para gameplay-ready. Próximas prioridades: `IsWorldStructurallyReady()` com leituras seguras, instrumentar primeira transição e então implementar leitura de recursos.

## Protótipo de leitura (C++17)

Há agora um **protótipo experimental e somente leitura** com `ReadWorld()`, `GetLocalPlayer()` e diagnóstico `aoe2hd_probe`, além de testes unitários com memória simulada. Para instruções de compilação Win32, limitações de prontidão e estados suportados, consulte [docs/implementation.md](docs/implementation.md). Não há ainda API de comandos nem garantia de prontidão operacional.

## Monitor de ciclo de vida (experimental)

`aoe2hd_monitor.exe` registra mudanças observadas em `World` e no jogador local, incluindo o desaparecimento e a reaparição do processo. É somente leitura e não equivale a uma detecção comprovada de `GameplayReady`. Consulte [docs/implementation.md](docs/implementation.md) para compilar e testar.

## Leitura de recursos econômicos (experimental)

A cadeia `[LocalPlayer + 0x3C] -> ResourceTable` foi confirmada em uma partida de **AoK HD.exe 5.8.INT (x86)**, por instruções Assembly e mudanças individuais na interface. Os quatro primeiros `float32` são **comida (+0x00)**, **madeira (+0x04)**, **pedra (+0x08)** e **ouro (+0x0C)**. O programa `aoe2hd_probe.exe` agora utiliza `ReadLocalResources()` para exibir os valores em modo somente leitura. Não armazena endereços absolutos entre partidas. Consulte [docs/implementation.md](docs/implementation.md) para executar e testar. Ainda não foi validado em todas as configurações ou versões do jogo.

## Monitor econômico contínuo (experimental)

Execute `aoe2hd_economy_monitor.exe --interval-ms 250` para registrar somente mudanças de comida, madeira, pedra e ouro em tempo real. Na troca de World/jogador/tabela, o monitor emite um novo snapshot inicial em vez de somar valores de partidas diferentes; estados indisponíveis são informados separadamente. Não escreve na memória. Consulte [docs/implementation.md](docs/implementation.md).


## Telemetria econômica CSV (2026-10-12)

O `aoe2hd_economy_monitor.exe` agora salva eventos em `aoe2hd_economy.csv` na pasta corrente, enquanto mantém as mensagens no terminal. A opção `--csv` permite escolher outro arquivo. O arquivo é **sobrescrito a cada execução** (use outro nome para preservar sessões anteriores), com cabeçalho e colunas `elapsed_ms,session,event,world,local_player,resource_table,resource,previous,current,delta,details`. Os endereços são expressos como inteiros decimais no CSV; `initial` tem quatro linhas, uma por recurso, com `previous` e `delta` vazios; `change` tem valores anteriores, atuais e delta; `unavailable` descreve a perda de leitura. `session` aumenta quando o World, jogador, tabela ou geração do processo mudam, e também após indisponibilidade. Essas sessões são identificadores **dentro da execução atual**, não entre arquivos de execuções diferentes. Exemplo:

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_economy_monitor.exe --interval-ms 250 --csv aoe2hd_economy.csv
```

A observação por polling (250 ms) não distingue automaticamente coleta de outras fontes de ganho nem capta necessariamente todos os eventos intermediários. Não é uma medição exata de coleta/gasto brutos. O programa continua somente leitura em relação ao jogo; escreve apenas o CSV no disco local.


## Analisador econômico por sessão

Execute `python scripts/analyze_economy.py aoe2hd_economy.csv --output aoe2hd_summary.csv`. O analisador (Python, sem dependências externas) agrupa por `run_id` e `session`, e calcula saldos iniciais/finais, aumentos e reduções observados, líquido e variação líquida por minuto. Lê arquivos anteriores sem `run_id` como `legacy`. Os registros iniciais agora deixam `previous` e `delta` vazios. O monitor sobrescreve o CSV a cada execução; use nomes distintos para preservar capturas.


## Leitura experimental da população

O `aoe2hd_probe.exe` agora tenta ler população utilizada (`ResourceTable+0x2C`), contador espelho (`+0x4C`), vagas disponíveis (`+0x10`) e aldeões (`+0x94`), todos `float32`. Calcula capacidade = população + vagas. Os offsets foram observados em **quatro estados controlados de uma única partida**; a identificação semântica ainda é experimental e deve ser validada em outra partida e com unidades militares. O probe retorna código 4 se a população não puder ser validada.


## População no monitor contínuo e CSV (2026-10-13)

O `aoe2hd_economy_monitor.exe` agora também consulta o `PopulationReader` no mesmo ciclo de amostragem. Além dos recursos, registra no **mesmo CSV** os indicadores `PopulationUsed`, `FreePopulationSlots`, `PopulationLimit` e `VillagerCount` nas linhas `initial` e `change`, sem mudar as colunas existentes. Falhas temporárias de leitura de população são indicadas por `population_unavailable` e não interrompem a coleta dos recursos. A primeira leitura válida depois de indisponibilidade registra novos valores iniciais, não uma diferença através da falha. As sessões continuam separadas quando muda `World`, jogador, tabela ou processo. O analisador `scripts/analyze_economy.py` mantém compatibilidade: resume somente `Food/Wood/Stone/Gold` e ignora os indicadores populacionais.

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull --ff-only origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_economy_monitor.exe --interval-ms 250 --csv aoe2hd_population_economy.csv
```

A semântica da contagem de aldeões e dos dois contadores populacionais ainda requer verificação em outra partida e com unidades militares. A captura é de leitura não atômica, portanto pode retornar `PopulationUnavailable` temporariamente.


## Readiness gate for new matches

The continuous monitor uses `GameplayReadinessMonitor` to withhold telemetry until two consecutive coherent, non-all-zero snapshots of the same World/player/table have been observed. The initial all-zero transitional frame is no longer recorded as fake collection/population changes. Once activated, zero balances are permitted. This is an observational heuristic, not proof of game-engine readiness. Tests cover zero-to-initial transition, World change, and invalid intermediate population. Run `ctest --test-dir build2026 -C Release --output-on-failure`.


## Eventos econômicos derivados (experimental)

O monitor emite `derived` para aumentos observados de `VillagerCount` (`VillagerProduced`) e de `PopulationLimit` (`PopulationCapacityIncreased`). Esses fatos são derivados dos contadores, não da identificação de unidades nem de edifícios. Quando observa gasto compatível de 50 Food ou 25 Wood nos até 120 segundos anteriores, emite opcionalmente `hypothesis` (`PossibleVillagerFoodCost` ou `PossibleHouseWoodCost`). A associação é apenas plausível, nunca causalmente demonstrada. Não associa entre sessões e descarta candidatos após perda de leitura. As linhas usam o esquema CSV existente; o analisador de recursos continua ignorando `derived` e `hypothesis`. Teste: `derived_event_tests`.


## Descoberta experimental de vetores de unidades

Execute `build2026\\Release\\aoe2hd_unit_discovery.exe --csv aoe2hd_unit_candidates.csv` com uma partida aberta e treine um aldeão. A ferramenta **não identifica unidades ainda**: examina somente estruturas candidatas a vetores dentro dos primeiros 0x400 bytes do jogador local, limita tamanho/leituras e registra mudanças de contagem no CSV. Compare candidatos antes/depois do treinamento; muitos serão falsos positivos e vetores vazios nao sao descobertos. A leitura é externa e somente leitura. Não use os endereços de uma sessão em outra. Teste `unit_discovery_tests`.


### Unit discovery v2: diagnostico de candidatos

A varredura agora inspeciona `LocalPlayer` **e** `World` (ate 0x400 bytes por raiz, com limite rigido de 0x800), sem afirmar que um candidato seja uma lista de unidades. O CSV agrega `source` e `reason` (`Accepted`, `Empty` ou `UnreadableEntries`), alem de contadores de rejeicao: `read_failures`, `nonvector`, `invalid_bounds`, `oversized`, `misaligned` e `unreadable`. Linhas `Summary` sao emitidas a cada cerca de 5 segundos durante snapshots estruturais validos, mesmo sem candidatos. Compare capturas antes/depois de treinar um aldeao (populacao 4/5 para 5/5, limite constante). O detector nao identifica unidades diretamente e pode produzir falsos positivos.


### Rastreio de conteudo dos vetores candidatos (v3)

O executavel `aoe2hd_unit_discovery` agora gera tambem `aoe2hd_unit_changes.csv`, com as colunas `elapsed_ms,world,player,source,offset,stride,index,kind,before,after,count`. Sao registradas mudancas de palavras de 32 bits das entradas de vetores candidatos aceitos, mesmo quando a contagem e constante. `changed`, `added`, `removed` e `relocated` sao diferencas de memoria, **nao identificacoes confirmadas de unidades**. O primeiro snapshot estabelece baseline; leituras parciais sao ignoradas; trocas de World/player e indisponibilidade resetam a comparacao. Nao interpretar mudanca em `World+0x184` (vetor de jogadores) como unidade. Compare uma partida com populacao 4/5 e depois 5/5; envie ambos CSVs. Ha limite de 512 entradas por candidato por amostra. Teste novo: `unit_content_tracker_tests`.


### Validacao semantica de ponteiros (v4, experimental)

`aoe2hd_unit_discovery` tambem cria `aoe2hd_unit_pointer_evidence.csv` a cada cerca de cinco segundos. Para cada candidato aceito, limita a verificacao a 32 entradas e le somente as primeiras quatro bytes de cada alvo plausivel. Colunas: `source,offset,stride,count,sampled,nonzero,readable_targets,distinct_targets,plausible_vtables,modal_vtable,matching_vtables` (alem de tempo e identidade do jogo). Interpretacoes de stride sobrepostas sao deduplicadas por raiz, offset e inicio do array; apenas uma representacao e amostrada. `plausible_vtables` indica primeiras palavras com endereco alinhado dentro da imagem principal do jogo: **heuristica**, nao comprovacao de objeto C++ ou unidade. A ferramenta continua somente leitura e nao segue cadeias arbitrarias de ponteiros. Compare o CSV antes e depois de produzir um aldeao, mantendo o limite populacional 5.


### Object field sampling (experimental)

The unit discovery executable also creates `aoe2hd_object_fields.csv`. It samples 16 objects (maximum) from each of the previously examined World vectors at offsets `0x88`, `0xA0`, `0xB0` using eight-byte entries, inspecting the first 64 bytes (16 32-bit words) of each readable target. The first observation becomes the baseline; subsequent samples record `field_changed` or `object_changed`, without assigning semantics to field offsets. A change of World/player or an unavailable world resets the baseline. Invalid/incomplete object reads do not update it. These are structural probes, NOT established unit identifiers, types, or coordinates. Test `object_field_sampler_tests`.


### ObjectFieldSampler v2 (expanded sampling)

The read-only discovery executable now samples up to 24 evenly spaced objects per candidate at `World+0x88`, `World+0xA0`, and `World+0xB0`, reading a maximum of 128 bytes per readable object. `World+0xA0` supports stride 4; other candidates prefer stride 8. New `aoe2hd_object_field_summary.csv` reports periodic objects/fields/changes/read_failures, even when no field changes occur. `aoe2hd_localplayer_raw.csv` isolates observed changes at `LocalPlayer+0x18` with stride 8, without inferring unit semantics. `aoe2hd_object_fields.csv` still reports detailed field changes. The first sample is a baseline; partial reads are discarded. These probes are not a validated unit reader.


### ProductionProgressProbe (experimental, read-only)

A new executable `aoe2hd_production_progress.exe --csv aoe2hd_production_progress.csv --phase-file production_phase.txt` scans **only existing candidate object collections** at `World+0x88`, `World+0xA0` and `World+0xB0`. It examines up to 24 distributed objects per vector and their first 256 bytes, recording candidate float32 values in (0,1) or [1,100], and integer values in [1,100]. These **are not established production percentages**: they are raw candidate values and the UI may derive its percentage from timers elsewhere. The CSV includes timestamps, World/player, current population and villagers (when readable), object/field addresses, candidate encoding, phase and initial/change markers. Phase labels are provided by changing the contents of `production_phase.txt` (e.g. `idle`, `training`, `paused`, `complete`) from another PowerShell window; do not edit game memory. Labels are user annotations only, not engine signals. Samples are taken every ~250 ms; the file may grow rapidly. A failed read is ignored, and a World/player transition resets the field baseline. No direct memory writing or unit commands are performed.

Suggested experiment: start with a stable town center; write `idle` in the phase file; start the executable; write `training` when ordering a villager; pause the game and set `paused`; resume and set `training`; after completion set `complete`. Compare candidate fields that rise during training, remain still during pause, and reset on completion. Do not assume a specific field is progress until independently validated across multiple runs. Run `production_progress_probe_tests` with CTest.


### Absolute progress address experiment

`aoe2hd_production_progress.exe` now writes a separate `aoe2hd_absolute_progress.csv` with **read-only** 16-bit and 32-bit reads at the three Cheat Engine addresses `0x142E8E54`, `0x005AD5EC`, `0x005AFC30`. The main exploratory CSV remains unchanged. Rows include phase, World/player, population used, villager count, address, readability flags, values, and initial/change/status/phase events. Sample interval is approximately 250 ms. Missing data is explicitly marked unreadable instead of being interpreted as zero. A changed World/player resets the comparison baseline. This does not use a debugger, breakpoint or any memory-writing operation. **Addresses are hardcoded only for this specific diagnostic experiment and may become stale on game restart; do not treat them as stable offsets.** Inspect the recorded values against the in-game progress bar before assigning meaning. CTest includes `absolute_progress_monitor_tests`.


### Configurable absolute progress address (2026-10-13)

Use `aoe2hd_production_progress.exe --watch 0x07E9A9BC --csv aoe2hd_production_progress_v3.csv --phase-file production_phase.txt`. Repeat `--watch` to monitor several distinct 32-bit addresses; duplicates are ignored, and invalid/out-of-range addresses cause a usage error. Only explicitly provided addresses are read. The old addresses from previous sessions are **no longer hardcoded**. With no `--watch`, the absolute-progress CSV has only a header, while the existing structural scanner continues normally. Results are logged to `aoe2hd_absolute_progress.csv` with 16-bit and 32-bit interpretations, including read failures. These are volatile absolute addresses, **not** stable offsets; revalidate them after restarting the game. Never use breakpoints or write memory for this probe.


### Construction progress probe (experimental)

The standalone `aoe2hd_construction_progress.exe` monitors the observed Cheat Engine construction percentage field at **object base + 0xC1C**, interpreted as IEEE-754 `float32`. Example:

```powershell
Set-Content .\construction_phase.txt "idle"
.\build2026\Release\aoe2hd_construction_progress.exe --base 0x07E99D78 --csv aoe2hd_construction_progress.csv --phase-file construction_phase.txt
```

Repeat `--base 0x...` to compare two structures. The CSV logs elapsed time, phase, world/player identity, base, field offset, absolute field address, read success, float32 value, raw u32 bits, whether it lies in [0,100], and the event kind (`initial`, `change`, `status`, `phase`). Samples run every ~250 ms; changes are logged as they occur. Mark experimental phases by rewriting `construction_phase.txt` to `building`, `paused`, `resumed`, `complete`. The process uses only `ReadProcessMemory`; no breakpoints or writes are involved.

**Important**: the base `0x07E99D78` was observed in one game session and must be revalidated after restart. The relative field `+0xC1C` is still a hypothesis about construction-state ownership: it could represent a UI object. No automatic building enumeration is claimed. See CTest `construction_progress_monitor_tests`.


### Experimental construction pointer discovery

Run the known base probe with `--discover-pointers`:

```powershell
.\build2026\Release\aoe2hd_construction_progress.exe --base 0x145AD1F0 --discover-pointers --csv aoe2hd_construction_progress_v3.csv --phase-file construction_phase.txt
```

This creates `aoe2hd_construction_pointer_hits.csv` when direct or one-hop references to the supplied base are found in the first 0x400 bytes of the validated World or LocalPlayer objects. The search runs at about 5-second intervals, scans no more than 64 aligned intermediary candidates, and never changes the game process. An **empty CSV (apart from its header) means no reference was detected within this deliberately limited search**, not that there is no stable path. This is *candidate path discovery*, **not automatic building object discovery or a validated stable pointer chain**. Check candidate root offsets across fresh matches and separate sessions before trusting them. In particular, `EDI` may be a reused UI object rather than an actual building.


### Entity health probe: villager/Loom experiment (experimental)

`aoe2hd_entity_health.exe` logs per-address read-only `float32` samples of candidate entity HP fields at `object + 0x34`. Use the **HP addresses** observed in the current session directly, without assuming object identities:

```powershell
Set-Content .\entity_health_phase.txt "after_loom"
.\build2026\Release\aoe2hd_entity_health.exe --hp-address 0x141BD9CC --hp-address 0x141BDBB4 --hp-address 0x141BDD9C --csv aoe2hd_entity_health.csv --phase-file entity_health_phase.txt
```

Or pass base pointers with repeated `--base 0x...`. The two options can be mixed; repeated bases are deduplicated. For a controlled new match, identify live candidate addresses *before* Loom, start the monitor, and change the phase file from `before_loom` to `researching_loom` to `after_loom`. To distinguish individual units, damage one villager and check whether exactly one candidate address changes. CSV fields include phase, World/LocalPlayer, base, source HP address, float and raw values, readability, plausibility, and change kind. Samples are taken approximately every 250 ms and rows are emitted for changes, phase transitions, first observation, and read-status changes.

**No identity is inferred**: three addresses containing 40.0 do not by themselves prove three separate villagers, nor that the field is current rather than maximum HP. Addresses are dynamic and must be revalidated after a game restart; `+0x34` is a candidate offset based on the observed building HP-copy instruction. This probe only reads external process memory and does not set breakpoints or modify the game.


### Entity HP damage validation & pointer-reference experiment (2026-10-13)

A controlled in-game damage experiment provided strong evidence for a *current-HP* field at candidate entity `+0x34`. With three candidate float32 addresses initially reading 40.0, only `0x141BD9CC` changed, in five steps: 40 → 38 → 36 → 34 → 32 → 30. The two other candidates `0x141BDBB4` and `0x141BDD9C` yielded no change events during the capture. Corresponding candidate object bases are `0x141BD998`, `0x141BDB80`, `0x141BDD68`. These addresses are valid only in their original match; individual villager identities and general-purpose enumeration remain unverified.

Optional next experiment: add `--discover-pointers` to `aoe2hd_entity_health.exe` with the validated bases or `--hp-address` values:

```powershell
.\build2026\Release\aoe2hd_entity_health.exe --hp-address 0x141BD9CC --hp-address 0x141BDBB4 --hp-address 0x141BDD9C --discover-pointers --csv aoe2hd_entity_health_discovery.csv --phase-file entity_health_phase.txt
Import-Csv .\aoe2hd_entity_pointer_hits.csv | Format-Table -AutoSize
```

The optional `aoe2hd_entity_pointer_hits.csv` contains candidate direct and one-hop references to each **object base** found only within the first 0x400 bytes of the validated World and LocalPlayer roots, sampled approximately every five seconds and bounded to 64 intermediate addresses. This reuses the existing conservative pointer-reference scanner; it **does not** yet enumerate all units or scan arbitrary process memory. An empty result does not rule out a pointer chain elsewhere. Confirm candidate offsets independently in a new match before calling them stable. All access remains read-only.


### Experimental LocalPlayer selection-region tracker

After identifying direct references to three HP-bearing objects within the local player's first 0x400 bytes, the read-only entity-health executable now supports `--watch-selection`, even without supplying any `--hp-address` or `--base`:

```powershell
Set-Content .\entity_health_phase.txt "none"
.\build2026\Release\aoe2hd_entity_health.exe --watch-selection --phase-file entity_health_phase.txt
Import-Csv .\aoe2hd_selection_pointers.csv | Format-Table elapsed_ms,phase,offset,pointer,hp_float,kind -AutoSize
```

The monitor reads five candidate pointer slots at **LocalPlayer+0x15C, +0x160, +0x164, +0x168 and +0x16C** every ~250 ms. For each aligned candidate pointer, it attempts a read-only float32 HP read at `pointer + 0x34`. The resulting `aoe2hd_selection_pointers.csv` records initial values, pointer changes and clears, read status changes, HP changes and phase transitions. Use phase markers such as `none`, `villager_a`, `villager_b` and `villager_c` while selecting and deselecting units. The original explicit-address HP monitor and `--discover-pointers` remain available and can be combined with `--watch-selection`.

**These offsets are candidates, not a verified unit enumeration table.** The fields were observed to hold changing references and may represent selection/UI/controller state instead of a full roster. Pointers may be stale; failed reads are recorded rather than treated as health, and plausibility of a float value does not establish object identity. Repeat the experiment in a new match to validate any structural inference. Read-only access only.


### EntityTypeDiscovery v1: compare known object layouts

The experimental read-only `aoe2hd_entity_type.exe` captures the first 0x100 bytes (or up to 0x400 via `--bytes`) from each **known object base** and compares 32-bit values at matching offsets. Use at least two labeled object addresses from the **currently running match**:

```powershell
.\build2026\Release\aoe2hd_entity_type.exe --entity villager 0x141BD998 --entity house 0x15756740 --entity barracks 0x157561F0 --entity town_center 0x14091830 --prefix aoe2hd_entity_type
Import-Csv .\aoe2hd_entity_type_comparison.csv | Where-Object differentiates -eq '1' | Format-Table -AutoSize
```

Output files: `aoe2hd_entity_type_fields.csv` (one row per object and offset) and `aoe2hd_entity_type_comparison.csv` (readable/distinct-value statistics). Labels are entered by the operator; **the program does not yet identify types automatically**. Differences may reflect HP, identity, pointers or mutable state. Repeating with multiple examples of the same type and in fresh games is required to isolate candidate type-identification fields. The example absolute addresses are from an earlier game and may no longer be valid. The probe is read-only and does not attach a debugger.
