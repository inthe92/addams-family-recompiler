# NES Native Lab — Technical baseline

Audit date: **2026-10-10**. This document records inspected code and executed checks; it does not certify a complete port or hardware-exact NES implementation.

## 1. Reference and scope

| Item | Verified reference |
|---|---|
| Repository | <https://github.com/inthe92/addams-family-recompiler> |
| Local audited HEAD | `e027f5a23f5a2cd49c7a7366c4a211479b7866dd` |
| Local declared version | **0.2.1**, `CMakeLists.txt:2,47–48` |
| Public `main` at audit time | `1588f31e3c3375adfbb4f75266d44eff1fdb25e5` |
| Published release | `v0.2.0-alpha`, published 2026-10-10 15:59:25 UTC |
| Local/public difference | Local HEAD is one existing commit ahead; the importer startup fix is local |
| Source inventory | **80 tracked files** before adding this document |
| Input scope | One SHA-256-authenticated European En/Fr/De image; MMC1B, PRG-ROM and CHR-ROM |
| Instruction registry | **15,303 physical instruction starts** across eight banks |
| Registered public CTest suite | **1 CTest entry**, containing **3 Python unittest cases** |

The directory name containing this checkout still says `public-alpha-0.2.0`; it is not the current source version. The existing local commit predates this audit. **No commit, push, engine change, game behavior change, packaging change or feature removal was performed during the audit.** The only new repository file is this document.

The public source tree is the primary subject. Older local development tests were inspected only to identify reusable verification work; one synthetic ISA contract was compiled against the current public emitter and executed, as explicitly recorded below. Results from old builds are not substituted for results from this checkout.

Evidence labels used below:

- **Executed**: run during this audit, with retained output.
- **Code-confirmed**: directly supported by inspected source; not necessarily a reproduced gameplay failure.
- **Risk**: plausible consequence needing a targeted test.
- **Historical CI**: GitHub API results verified during this audit, for the older public commit.

## 2. Architecture and implementation status

The pipeline is user ROM → identity validation → address-profile-based native C generation → local shared-module compilation → SDL module loading → native block dispatch through a C service ABI. The public frontend contains no translated game module. The runtime reproduces NES memory and devices; there is no general 6502 instruction interpreter or fallback core.

| Area | Implemented scope | Remaining limits / verification |
|---|---|---|
| `tools/recompiler/import_user_rom.py` | Size/SHA/header validation before generation, local code/resources/maps, parallel C compilation, hashed cache receipt, module replacement before receipt publication | Single revision; cached import was tested, fresh game compilation was not repeated; concurrency and cache recovery lack public tests |
| `tools/recompiler/emit_blocks.py` | 151 official opcode encodings, addressing arithmetic, indexed page penalties/dummy accesses, RMW double writes, stack/control flow, both CPU address windows | Only registered addresses; no runtime code discovery; read/fetch timing and interrupt sampling are approximate |
| `include/recomp/` | Shared C CPU state and service ABI, binary NES ADC/SBC, normalized status bits | No public ABI/version handshake with loaded modules; reset is a deterministic bootstrap |
| `runtime/src/ram.cpp` | 2 KiB RAM mirroring and byte stack operations | Generic RAM/stack behavior is not tested by the public CTest suite |
| `runtime/src/cartridge.cpp` | Standard MMC1B serial loading/reset, PRG/CHR bank modes, nametable mirroring, PRG-RAM enable/open-bus behavior | Explicitly excludes CHR-RAM and other MMC1 wiring variants; public mapper behavior coverage is absent |
| `runtime/src/bus.cpp` | Memory dispatch, controller shifting, timed writes, PAL device advancement, OAM/DMC DMA and interrupt entry | Reads have no cycle offset; fine DMA/interrupt races are unverified |
| `runtime/src/video_state.cpp` | PAL dots/scanlines, register latches, buffered VRAM reads, palettes, scrolling, sprite priority, sprite-zero hit, frame/line snapshots and NMI edge flag | Simplified sprite evaluation/fetch pipeline and overflow; register race/OAM access details are incomplete |
| `runtime/src/audio_state.cpp` | Pulse/triangle/noise/DMC, envelopes/sweep/length/linear counters, frame sequencer/IRQ, nonlinear mixing, 48 kHz filtering/resampling | No public waveform/timing regression tests; DMA corner cases and analog fidelity remain open |
| `game/src/world_view.cpp` | 427×240 canvas, exact central raster copy, horizontal map extension, one explicit family of live map edits | Nonhorizontal rooms disable extension; scene metadata comes from live RAM; other map edits remain unverified |
| `game/src/actor_view.cpp` | Captures active actor poses at known native addresses, validates them against actual OAM uploads, resolves OAM ownership | Observes actors; does not widen spawning, despawning or AI activation; depends on game-specific bank/PC/field contracts |
| `game/src/god_mode.cpp` | Damage/life guards, item/progression writes and a room-reload rescue policy | Cheats alter game state intentionally; they cannot serve as an original-game correctness oracle |
| Desktop / GL / SDL audio / Lua | Input, presentation, audio queue, restricted Lua RAM/controller API, import progress/error UI | No desktop visual QA in this audit; actual Windows/Linux execution of local 0.2.1 was not performed |
| CMake / dependency setup | Default content-free Release build, pinned local source archives, C++17 and platform linkage | `NES_ENABLE_DESKTOP=OFF` fails to generate; public CTest coverage is much smaller than the runtime |

Completed work is bounded: identity rejection, default local build, the synthetic toolchain contract and local importer process handling passed their stated checks. CPU/mapper/video/audio/gameplay as a whole remain **partially verified**, even where implementations exist.

## 3. Checks actually executed

Host: **macOS 27.0.1, ARM64**; Apple Clang **21.0.0**, CMake **4.4.4**; system Python **3.9.6**, bundled Python **3.11.17**, bundled Zig **0.17.0**. The fresh build used `Release` and C++17. Its output was outside the repository, in `/private/tmp/nes-technical-baseline-build`.

| Check | Result | Scope and limitation |
|---|---|---|
| Fresh default configure + build | **PASS**, exit 0 | Builds the public frontend and probe without a ROM; does not exercise OpenGL display |
| `ctest --output-on-failure --verbose` | **PASS**, 1/1 entries, 3/3 Python cases | Profile fields/address shape, wrong-size/hash rejection, nonempty generated semantics for all 151 opcode encodings |
| `tests/public_toolchain_smoke.py` | **PASS**, exit 0 | Bundled Python/Zig compile and load a synthetic module whose function returns 42 |
| `tests/public_import_process_checks.py` | **PASS**, exit 0 | Fresh native probe launches bundled Python through spaced paths/trailing separator; synthetic image rejection, startup stdout/stderr and launch-failure logging |
| `tests/import_probe.cpp`, optional private input | **PASS**, exit 0 | Fresh public probe loads an existing validated private module, executes **1,800 frames**, and reaches the garden with GOD enabled and horizontal rendering |
| Clean source export audit | **PASS**, exit 0 | Exact 80-file export matched the audited tracked tree byte-for-byte; dependency hashes and private-reference byte-marker checks passed |
| Additional exhaustive arithmetic diagnostic | **PASS**, **524,288 cases** | ADC/SBC helpers, all byte operands/carry states and both D states; A/C/V/N/Z checked with an independent integer/signed-range calculation |
| Reused local `opcode_codegen_checks.cpp` | **PASS**, **668 cases** | Existing local test driver; freshly generated **167 synthetic fixtures** from the current public emitter: 151 opcodes plus 16 branch-page cases, four flag seeds; checks registers/PC, flags, write order/offsets, total cycles and IRQ mask selection |
| Additional read-timing diagnostic | **Discrepancy reproduced**, diagnostic exit 0 | Synthetic generated `LDA $2002` differs from a same-runtime read placed at the instruction's read-cycle offset; not a hardware-accuracy PASS |
| `NES_ENABLE_DESKTOP=OFF`, mods also OFF | **FAIL**, configure exit 1 | CMake generation reports missing `SDL2::SDL2-static` targets |

The reused ISA test is **not present or registered in this public repository**. Its older fixture generator was used with a temporary table-import shim pointing to the public opcode registry, rather than a recovery tree. Neither fixture generation nor that test requires commercial bytes. The test checks instruction-level contracts; it does not record every read/fetch cycle or establish full hardware accuracy.

Private boot scope is deliberately narrow: the import log reports `Importacion local existente validada.` No fresh ROM translation/compilation, complete playthrough, all-room traversal, death recovery matrix, visual gameplay review or waveform comparison was executed in this audit. GOD makes this a boot/integration smoke check, not an unmodified-game fidelity test.

Logs, temporary diagnostic sources and the machine-readable result ledger are retained **outside this Git repository**, in the surrounding workspace's `outputs/technical-baseline-2026-10-10/`. Important files: `results.json`, `ctest.log`, `toolchain.log`, `import-process.log`, `private-boot.log`, `legacy-isa-contract.log`, `diagnostic.log`, `headless-configure.log`, `source-audit.log`, `remote.json` and `remote-jobs.json`. No ROM or translated commercial module was added to the repository.

### Public CI reference

GitHub run [38065822713](https://github.com/inthe92/addams-family-recompiler/actions/runs/38065822713) completed successfully for **Windows, Linux, macOS ARM64 and macOS Intel**, at public commit `1588f31…`. This is verified **historical CI**, not execution of the local `e027f5a…` change. That older workflow runs the source contracts and synthetic compiler/module check, but does not contain the newer frontend process regression. Green older CI therefore does not validate the Windows startup fix or actual game execution.

## 4. Problems ordered by impact

Priorities describe development impact, not an assertion that every risk has already caused a game failure.

| Priority / ID | Finding and evidence | Consequence | Smallest useful next step |
|---|---|---|---|
| **P0 / B01** | Public verification gap: the registered suite has three Python cases and does not execute generated instructions. **Executed / code-confirmed**, `tests/public_import_checks.py:12–35`, `CMakeLists.txt:54–62` | A green public build gives insufficient confidence for CPU/device or widescreen changes | Reuse the synthetic ISA fixture contract, then add small RAM/mapper/bus/device fixtures; avoid duplicating test logic |
| **P1 / B02** | Read/fetch timing is not represented in the ABI. **Executed discrepancy**, `runtime_services.h:9–14`, `bus.cpp:21–35`, `emit_blocks.py:55–83,105–124` | Total instruction cycles can be correct while PPU/APU status reads occur too early; polling/IRQ-sensitive code can differ | Preserve the tiny `$2002` reproducer as a regression before designing a targeted read-timestamp extension |
| **P1 / B03** | Published Windows launcher passes unquoted arguments to `_wspawnvp` and lets Python create the log after parsing/imports. **Code-confirmed** in `1588f31`; local fix exists in `e027f5a`, but was not run on Windows in this audit | Paths with spaces/startup errors can prevent import and leave an empty log | Execute the existing new frontend-process regression on Windows and test the portable package; publication requires a separate authorized step |
| **P1 / B04** | Widescreen is rendering-only for already active entities; nonhorizontal/no-column rooms set `extended_` false. **Code-confirmed**, `game/src/world_view.cpp:19–30,65`, `game/src/actor_view.cpp:15–32` | Enemy pop-in persists at original activation limits; rooms appear to return to a narrow view even though the canvas remains 427 pixels | First capture spawn/despawn and room-transition regressions; implement game-owned changes only after their original contracts are protected |
| **P1 / B05** | Finite native address registry; dispatcher rejects other ROM entries/RAM code. **Code-confirmed**, `compatible_profile.json`, `emit_blocks.py:187–208`, `app/desktop.cpp:129–130` | Uncovered routes or writable-code cases can stop execution; 15,303 starts do not prove complete reachable-code coverage | Record missing bank/PC entries and a route/transition coverage ledger using private user imports; retain explicit failure rather than hiding missing code |
| **P1 / B06** | Simplified PPU sprite evaluation/overflow and incomplete NMI/status race handling. **Code-confirmed approximation / hardware risk**, `video_state.cpp:22–28,50–65,96–105,156–175` | Sprite counts/status timing and midframe effects can differ; observed game impact is unproven | Synthetic sprite-zero/overflow, `$2002` near-vblank and NMI-enable/status-read tests before changing implementation |
| **P2 / B07** | IRQ sampling is at instruction completion with selected old-I handling, rather than all hardware polling points; no BRK/IRQ/NMI hijack model. **Code-confirmed / risk**, `emit_blocks.py:78–79,105–125`, `runtime_services.h:58–75`, `bus.cpp:77–95` | IRQ/NMI ordering can diverge near instruction/branch boundaries despite correct basic flags and stack writes | Add synthetic CLI/SEI/PLP/RTI, taken-branch and overlapping BRK/NMI timing tests; distinguish PAL behavior |
| **P2 / B08** | DMA/APU corner cases and audio fidelity lack public regressions. **Code-confirmed limitation**, `bus.cpp:55–75`, `audio_state.cpp:57–90,116–134,160–185` | DMC cancellation/end-of-OAM overlaps, frame-counter races or output drift may survive smoke tests | Test OAM parity and DMC overlap/cancellation, frame/status IRQ timing, PAL sample counts and deterministic channel traces |
| **P2 / B09** | Live RAM scene ID/layout is combined with a completed video frame; line style captures only the start of each scanline. **Code-confirmed architecture / transition risk**, `world_view.cpp:21–35`, `video_state.cpp:108–114,167–170` | Old raster and newer scene metadata, or midline palette/CHR changes, may disagree at side seams | Add frame-associated scene/camera transition assertions and a raster seam fixture; do not infer new geometry from a narrow room |
| **P2 / B10** | Headless CMake option fails. **Executed FAIL**, `cmake/dependencies.cmake:36–45`, `CMakeLists.txt:31–43` | A declared option cannot produce a minimal test build; unnecessary desktop dependencies complicate verification | Make target creation consistent with the option in a separate change, retaining the default desktop build |
| **P2 / B11** | Import cache/module contract has no lock or explicit engine ABI handshake. **Code-confirmed / risk**, `import_user_rom.py:67–87,121–128`, `user_rom.cpp:214–225` | Simultaneous imports/replacement of a loaded Windows DLL can fail; external compiler selection is not part of cache identity | Add two-process and interrupted/corrupt-cache tests; decide a small ABI/version identity contract before extending exports |
| **P3 / B12** | Game addresses/field contracts are spread across observer, renderer and cheats; auxiliary RAM adapters overlap ABI stack/memory helpers. **Code-confirmed**, `actor_view.cpp`, `world_view.cpp`, `god_mode.cpp`, `ram.cpp:22–42`, `memory_access.h` | Maintenance can drift when game-specific assumptions change | Document shared contracts and consumers; consolidate only after tests, with no feature deletion |

### Read-timing reproducer: exact observation

Two synthetic buses start at CPU cycle **25,680** with identical zero-filled, noncommercial PRG/CHR arrays. One executes a freshly emitted `LDA $2002`; the other advances to offset **3**, reads `$2002`, then advances the last cycle. Both finish at **25,684**.

```text
generated A=0
same-runtime read at offset 3=128
final cycles=25684/25684
```

The generated read runs before `recomp_finish` advances the devices. Timed writes already synchronize through `Bus::write8`; ordinary reads do not. This establishes an internal timing discrepancy around PAL vblank. It is not a console trace, and it does not prove a specific observed gameplay failure.

## 5. CPU, MMC1, PPU and APU precision notes

**6502/NES CPU.** ADC/SBC are correctly binary even when D is set; treating this as a generic MOS decimal-mode CPU would introduce a regression. B/U status normalization, BRK/software status pushes and the indirect-JMP page-wrap behavior have explicit implementations. The additional helper and ISA checks passed their scopes. The official opcode set does not cover undocumented instructions. Operand/instruction fetches, several dummy reads and precise read-cycle timing are not fully modeled; instruction-cycle totals alone cannot certify the bus behavior. The deterministic zero-register/`SP=FD` bootstrap is not a physical reset sequence.

**MMC1.** The inspected implementation includes 32/16 KiB PRG modes, 8/4 KiB CHR modes, four mirroring selections, PRG-RAM gating, consecutive-write suppression and the D7 reset exception. Tracking the previous write even outside the serial-port address range is intentional; it must not be removed as an apparent simplification. The constructor explicitly accepts standard MMC1B PRG/CHR-ROM wiring, not every board using mapper number 1. No mapper defect was reproduced in this audit; missing public mapper tests remain a material verification gap.

**PPU.** PAL uses a 16/5 dot-per-CPU-cycle accumulator, 341 dots and 312 scanlines; this is not an NTSC 3:1/262-line renderer. Scrolling increments/copies, palette aliases, buffered data reads and mask/grayscale handling are present. Sprite selection is a whole-line scan at dot 0, not secondary-OAM evaluation/fetch over hardware cycles, and overflow is a simple ninth-sprite condition. `$2004` access during rendering, PAL forced OAM refresh/write restrictions, detailed vblank races and color emphasis are not fully modeled. The GL palette consumes six-bit indices, so emphasis is not represented by its current frame format.

**APU/DMA/interrupts.** PAL channel tables, nonlinear mixer, frame IRQ, DMC IRQ, address wrap and resampling exist. IRQ/NMI entry preserves the software/hardware status-push distinction and prioritizes NMI; CLI/SEI/PLP retain the old I mask, while RTI uses restored I. These sensible implementations still need device-level timing tests. The code itself marks rare end-of-OAM DMC overlaps/cancellation races unverified. Audio queue bounding protects host latency but cannot establish sound fidelity.

Technical reference checks used original NESdev documentation: [MMC1 consecutive writes/reset](https://www.nesdev.org/wiki/INES_Mapper_001), [CPU interrupt polling](https://www.nesdev.org/wiki/Interrupts), [PAL/NTSC cycle ratios](https://www.nesdev.org/wiki/Cycle_reference_chart), [PPU registers and vblank races](https://www.nesdev.org/wiki/PPU_registers), [sprite evaluation](https://www.nesdev.org/wiki/PPU_sprite_evaluation). Hardware expectations from these references are distinguished from tests executed above.

## 6. Duplication and complexity: retain the architecture

There is one public opcode registry and one generic emitter, plus one shared C CPU-state implementation. The C++ CPU-state header aliases that ABI; it is not a second CPU implementation. Generating both low/high CPU-window variants is deliberate because mapper modes can expose physical banks in either window; removing one variant would require mapping evidence.

Useful cleanup candidates are modest: repeated compatibility constants, dispersed game-field addresses, duplicated observer/cheat service-forwarding lambdas, and the older `MemoryAccess`/`main_ram_access` adapter alongside `RuntimeServices`. These are maintainability concerns, not justification for rewriting the motor. Bank-specific rendering knowledge belongs in `game/`, not the generic bus. The emitter and device renderer have hot per-instruction/per-dot paths, but **no performance profile was collected**; a bottleneck or speedup is not claimed.

The public importer test currently checks successful code-string generation for every opcode. That is necessary but weaker than compiling and executing those strings. The reusable local ISA contract provides the most direct way to close this gap. Its current fixture generator imports the opcode table through a recovery-directory interface even though its bytes are wholly synthetic; removing that dependency in a future test-only change would avoid shipping recovery code or fixtures from a commercial ROM.

Cache identity already includes generator/header inputs and `target.json`, which records pinned Python and Zig archive hashes. Bundled toolchain versions therefore are represented; the missing contracts are an explicit engine ABI version, serialization of concurrent imports and identity for the external `NES_NATIVE_CC` compiler override. These are risks requiring tests, not reproduced cache failures.

## 7. Efficient next steps and acceptance criteria

1. **Protect the baseline before fixes.** Register reusable synthetic native instruction tests and small RAM/MMC1/bus/PPU/APU tests in the public suite. Keep commercial-input integration separate and private. Require them to run on the four CI hosts; retain exact commit/build results.
2. **Close current startup verification.** Run the already written frontend importer regression on Windows for local 0.2.1. Exercise Unicode/spaced paths and real packaged startup as separate acceptance cases. Do not treat the older successful CI run as this check.
3. **Address timing with a small contract.** Keep the `$2002` synthetic case, then specify read offsets, read side effects and interrupt polling expectations before touching the ABI/emitter. Ensure existing central CPU/device tests remain stable; avoid a motor rewrite.
4. **Treat 16:9 as explicit gameplay and rendering work.** Verify early activation, persistence, no duplicate descriptors, pool exhaustion, signed X movement and absence of wrapped-coordinate contacts. Test horizontal → vertical/fixed → horizontal transitions, center-pixel preservation and frame-associated scene metadata. A wider GL viewport alone cannot satisfy these cases.
5. **Build a private route ledger.** Record visited scenes, transitions, missing native entries, deaths/reloads and deterministic inputs. Run both unmodified-game behavior and GOD/item testing with separate expectations. A garden smoke check is not full-game coverage.
6. **Optimize after evidence.** Profile import and frame costs only after correctness fixtures exist. Preserve mapper-window duplication and native block polling unless measurements and behavior tests justify a bounded change.

### Public behavior tests still missing

| Area | Minimum acceptance cases |
|---|---|
| Native instruction execution | All official encodings, registers/status/PC, zero-page/index wrapping, page penalties, JMP-indirect wrap, stack edges, RMW write order and reads with side effects |
| Interrupts / bus | CLI/SEI/PLP/RTI old/restored-I selection, pending NMI/IRQ order, branch polling, BRK/NMI overlap, controller strobe/shift/open bus |
| MMC1 | Serial commits across addresses, D0 adjacent-write suppression, D7 reset exception, every PRG/CHR mode, mirroring, disabled PRG-RAM and timestamp/order rejection |
| PPU | Register mirrors, latch resets, palette aliases/buffer, scroll wraps/copies, sprite priority/hit/overflow, vblank/NMI edges and rendering-time OAM accesses |
| APU / DMA | Envelope/sweep/length/linear traces, PAL frame events/status clearing, DMC wrap/loop/IRQ and cancel, OAM 513/514 parity and overlap, sample count/queue behavior |
| Import / ABI | Cached/fresh import, corrupt receipt/module, interrupted and concurrent imports, missing tools, spaces/Unicode paths, complete module symbol/version contract |
| Game view / cheats | Central raster invariance, lateral actor activation, room transitions, dynamic map edits, GOD-off behavior and item/rescue cases |

### Reproduce the public baseline

From this checkout, on a supported desktop build host:

```sh
cmake -S . -B /tmp/nes-native-baseline-build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/nes-native-baseline-build --parallel 4
ctest --test-dir /tmp/nes-native-baseline-build --output-on-failure --verbose
ctest --test-dir /tmp/nes-native-baseline-build --show-only=json-v1
```

With `TASK_RESOURCES` set to an unpacked package resource root containing `tools/recompiler` and `toolchain`:

```sh
python3 -B tests/public_toolchain_smoke.py "$TASK_RESOURCES"
python3 -B tests/public_import_process_checks.py "$TASK_RESOURCES" /tmp/nes-native-baseline-build/import_probe
```

On Windows, use the corresponding Python command and `import_probe.exe`. A clean source-export audit should run on the exact intended source staging tree; auditing `.git` metadata or a build directory is not the same as auditing the source ZIP. Private input/receipt/module paths and rendered commercial content must remain outside Git.

## Resumen en español

La base local 0.2.1 compila y pasan las pruebas públicas de importación, herramientas y arranque acotado. Además pasan 668 casos nativos sintéticos de los 151 opcodes y 524.288 casos de ADC/SBC. Esas comprobaciones no acreditan una NES exacta ni una partida completa. Se reprodujo una discrepancia en el momento de lectura de `$2002` y un fallo de configuración sin escritorio. La prioridad es llevar pruebas de comportamiento al repositorio público, validar la corrección de Windows y proteger los cambios de temporización y 16:9 con regresiones concretas. No se modificó el motor ni se realizaron commits o pushes en esta auditoría.
