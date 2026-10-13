# Read-only World probe

This is an **experimental** C++17 proof-of-concept for AoK HD.exe 5.8.INT (32-bit). It does not inject code, write memory or issue game commands. It uses `ReadProcessMemory` to build a detached snapshot and reports **structural readiness only**, not guaranteed gameplay readiness. Values are based on observed x86 RVAs and must not be used against unknown versions.

## Build (Windows, Visual Studio / CMake)

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_probe.exe
```

Run with the same Windows account as the game. The tool finds `AoK HD.exe` and the process module base. A return code of 0 means both the World structural checks and resource read passed; 1 means World is not structurally ready; 2 means the process could not be opened; 3 means World is ready but the resource read failed. Closing/restarting the game requires restarting the probe.

## Validation and limits

- The resolver uses `OwnerGlobal RVA 0x006F2C58`, `Owner+0xC8`, `World` vtable RVA `0x005D329C`, `World+0x174` and `World+0x184..0x18C`.
- Accepts 2–9 entries (Gaia + up to eight players), checks pointer pairs, per-player vtables, back-pointers, observed discriminator and local human type.
- The initial version assumes **one local HumanOrCoop and other entries Computer**; cooperative/multiplayer/scenario-editor configurations may be rejected intentionally until independently mapped.
- `object=controlBlock+0x0C` is validated for the observed `_Ref_count_obj` pattern. Other storage variants are not supported.
- A bounded two-pass anchor check mitigates—but **does not eliminate**—races while the game updates its state. Do not use this as an injection/synchronization guarantee.
- The synthetic tests cover the confirmed good snapshot, a null control block, null object, empty vector, invalid local index and invalid World vtable.
- The x86 target's address space is assumed; building the probe as Win32 is recommended. No version/hash gate exists yet.

Next: validate resource readings across new matches and investigate more player attributes.

## Monitor continuo de ciclo de vida (2026-10-12)

O novo executavel `aoe2hd_monitor` consulta periodicamente o estado de `World`, em modo somente leitura, e imprime **apenas quando o estado observado muda** (inclusive troca de `World*` e de jogador local). Abre novamente o processo a cada amostra para tolerar fechamento/reabertura do jogo. Um estado muito breve entre duas amostras pode não ser observado.

Compilacao com Visual Studio 2026:

```powershell
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_monitor.exe --interval-ms 250
```

Aparecera `ProcessNotFound` enquanto o jogo nao estiver aberto; estados como `MissingOwner`, `MissingWorld`, `InvalidVector`, `InvalidPlayer`, `ReadFailed` e `Ready` quando houver processo. Apos `Ready`, a saida inclui World, LocalObject, localIndex e players. Para interromper, `Ctrl+C`. Para fazer um ensaio finito: `--samples 40` (amostras, nao segundos). `--interval-ms` aceita 1..60000.

**Cuidado:** status transitorios podem ser pulados devido ao intervalo de polling, e o monitor nao estabelece prontidao operacional. Fechamento/reinicio do executavel e mudanca de mapa devem ser verificados na pratica. Nao use como sincronizacao para emissao de comandos.

## Leitura de recursos do jogador local (experimental)

Confirmado por alteracoes independentes de recursos e inspecao da memoria da partida AoK HD.exe 5.8.INT x86:

```text
World -> mPlayers[mLocalPlayerIndex] -> LocalPlayer
[LocalPlayer + 0x3C] = ResourceTable*
ResourceTable + 0x00 = Food  (float32)
ResourceTable + 0x04 = Wood  (float32)
ResourceTable + 0x08 = Stone (float32)
ResourceTable + 0x0C = Gold  (float32)
```

O `aoe2hd_probe.exe` passa a exibir os quatro recursos e o endereco da tabela, obtidos apenas por `ReadProcessMemory`. Os valores sao floats brutos; o HUD pode exibir apenas sua parte inteira. A leitura valida o ponteiro, campos finitos nao negativos e relê os campos de referencia do jogador e da tabela para detectar parte das transicoes. **Nao e uma fotografia atomica do jogo**. Os offsets sao especificos da versao investigada; validar em outras partidas e configuracoes.

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_probe.exe
```

`ResourcesStatus=Ready` significa que o leitor conseguiu recuperar os quatro atributos com checagens basicas. Se o World estiver pronto mas a tabela nao estiver legivel, o probe exibe um status de falha especifico e retorna codigo 3; codigo 1 = World nao pronto; codigo 2 = processo indisponivel. O novo teste `resource_reader_tests` verifica leitura, alteracao de ouro, tabela nula, falha de leitura, valor invalido e mudanca do back-pointer.

### Pasta de trabalho no Windows

A pasta padrão para clonar, atualizar, compilar e executar o projeto é `C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl`. Se a pasta já possuir `.git`, use `git pull origin main`. Se o Git responder `not a git repository`, a pasta não é um clone válido: preserve quaisquer arquivos locais e utilize um clone Git antes de tentar atualizar. Após mover ou renomear uma árvore já compilada, remova somente o diretório gerado `build2026` e reconfigure o CMake para evitar caminhos antigos em `CMakeCache.txt`.

## Monitor econômico contínuo

O programa `aoe2hd_economy_monitor.exe` usa `ReadWorld` e `ReadLocalResources` em consultas periódicas, identifica mudanças no World, jogador local e endereço da tabela e registra apenas alterações observadas de recursos. Se houver perda de leitura ou mudança de partida, emite `Unavailable` e reinicializa sua linha de base. O processo é aberto novamente após fechamento/reinício. Não confundir variações observadas com estatísticas totais de coleta/gasto (mudanças intermediárias podem ser perdidas entre amostras).

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_economy_monitor.exe --interval-ms 250
```

Aceita `--interval-ms` de 50 a 60000, padrão 250. Interrompa com Ctrl+C. A saída usa floats com duas casas e deltas assinados. Leituras são somente leitura, não atômicas; a medição ainda requer validação em partidas reais. O teste `economy_monitor_tests` cobre variação de recursos, recuperação de falha, troca de World e troca de geração de processo.


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


## Análise de sessões CSV

O analisador `scripts/analyze_economy.py` sumariza por `run_id`, `session` e recurso; `--output aoe2hd_summary.csv` exporta um CSV com saldos, aumentos/reduções observados, duração e variação líquida por minuto. O monitor agora inclui `run_id` gerado a partir do horário epoch em microssegundos na inicialização, diferenciando execuções; mantém o número de sessão por partida observada. CSVs antigos são lidos com `run_id=legacy`. Para eventos `initial`, `previous` e `delta` ficam vazios. Aumentos/reduções observados representam mudanças de **saldo**, não coleta/gasto bruto.

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
python scripts/analyze_economy.py aoe2hd_economy.csv --output aoe2hd_summary.csv
```

O CTest adiciona `economy_analyzer_tests` quando encontra Python 3 instalado.


## PopulationReader experimental (quatro estados, uma partida)

Foi observado em AoK HD.exe 5.8.INT (x86): `ResourceTable+0x10` vagas livres (1,0,5,4), `+0x2C` população utilizada (4,5,5,6), `+0x4C` contador semelhante (4,5,5,6), `+0x94` aldeões (3,4,4,5). O limite estimado é população + vagas (5,5,10,10). O antigo candidato `+0x104` permaneceu em 5 após a casa e **não** deve ser usado como limite.

`PopulationReader` exige um World e ResourceResult válidos, lê os quatro floats por ponteiros relativos e verifica valores inteiros finitos não negativos, dois contadores iguais, aldeões <= população e estabilidade de ponteiros antes/depois. Leituras externas **não são atômicas**; valores transientes podem retornar `InconsistentCounters`. População ainda **não** é monitorada continuamente nem adicionada ao CSV; por enquanto aparece apenas no `aoe2hd_probe.exe` com `PopulationStatus=Ready`. Código de saída 4 significa falha nessa etapa. A validação em uma segunda partida e com tropas militares continua pendente.

```powershell
cd C:\Minha_Pasta\Jogos\Age_of_Empires_II_HD\AoE2HDControl
git pull origin main
cmake -S . -B build2026 -G "Visual Studio 18 2026" -A Win32
cmake --build build2026 --config Release
ctest --test-dir build2026 -C Release --output-on-failure
.\build2026\Release\aoe2hd_probe.exe
```


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
