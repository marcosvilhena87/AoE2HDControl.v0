# Read-only World probe

This is an **experimental** C++17 proof-of-concept for AoK HD.exe 5.8.INT (32-bit). It does not inject code, write memory or issue game commands. It uses `ReadProcessMemory` to build a detached snapshot and reports **structural readiness only**, not guaranteed gameplay readiness. Values are based on observed x86 RVAs and must not be used against unknown versions.

## Build (Windows, Visual Studio / CMake)

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\aoe2hd_probe.exe
```

Run with the same Windows account as the game. The tool finds `AoK HD.exe` and the process module base. A return code of 0 means the structural checks passed; 1 means not structurally ready; 2 means the process could not be opened. Closing/restarting the game requires restarting the probe.

## Validation and limits

- The resolver uses `OwnerGlobal RVA 0x006F2C58`, `Owner+0xC8`, `World` vtable RVA `0x005D329C`, `World+0x174` and `World+0x184..0x18C`.
- Accepts 2–9 entries (Gaia + up to eight players), checks pointer pairs, per-player vtables, back-pointers, observed discriminator and local human type.
- The initial version assumes **one local HumanOrCoop and other entries Computer**; cooperative/multiplayer/scenario-editor configurations may be rejected intentionally until independently mapped.
- `object=controlBlock+0x0C` is validated for the observed `_Ref_count_obj` pattern. Other storage variants are not supported.
- A bounded two-pass anchor check mitigates—but **does not eliminate**—races while the game updates its state. Do not use this as an injection/synchronization guarantee.
- The synthetic tests cover the confirmed good snapshot, a null control block, null object, empty vector, invalid local index and invalid World vtable.
- The x86 target's address space is assumed; building the probe as Win32 is recommended. No version/hash gate exists yet.

Next: expand dynamic transition tests and identify player resources by read-only inspection.
