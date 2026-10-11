# Phase 3 — Timed bus reads

## Design recorded before production changes

Starting HEAD: `e027f5a23f5a2cd49c7a7366c4a211479b7866dd`, source version 0.2.1. The Phase 2 CMake/tests and both baseline documents are preexisting local changes and must be retained. This contract was written after reviewing both baselines and the emitter/service/bus/PPU/test sources, before changing production code.

### Cause and bounded architecture change

Previously generated memory reads call a two-argument service immediately. Device advancement happens at timed writes or instruction completion. An absolute LDA can therefore read PPUSTATUS at offset 0 instead of 3 despite consuming four cycles. The bus already separates the instruction-boundary CPU clock (`cycles_`) from synchronized device time (`device_cycles_`). Its delta-based `sync_devices` is sufficient; no new CPU core or DMA scheduler is needed.

Change the read service to `read8(context, address, cycle_offset)`. Offsets are **zero-based CPU cycles from the current instruction/interrupt-entry boundary**, just like existing write offsets. Synchronize PPU/APU to `cycles_ + offset` before performing the ordinary read, including all open-bus/register side effects. Keep `cycles_` at the instruction start until `advance(total_cycles)` commits the full instruction duration. Delta synchronization advances only the residual interval; it must not advance devices twice. Access timestamps must be ordered and within the instruction; existing backward-device-time rejection remains active.

`Bus::read8(address)` remains an untimed access at the already synchronized device state for DMA/debug/host clients. The RAM-only `MemoryAccess` adapter is outside this CPU timing contract and remains unchanged. All native full-bus reads must provide an explicit offset; there is no default/fallback offset in the native helper.

### Access offsets

| Operation | Represented reads, zero-based offsets | Writes / total duration |
|---|---|---|
| Zero page | Data 2 | Ordinary store 2 / 3 cycles |
| Absolute | Data 3 | Ordinary store 3 / 4 cycles |
| Zero-page indexed | Unindexed dummy 2, data 3 | Store 3 / 4 cycles |
| `(d,X)` | Unindexed dummy 2, pointer low/high 3/4, data 5 | Store 5 / 6 cycles |
| `(d),Y` | Pointer low/high 2/3; crossing dummy 4, final read 5; noncrossing read 4 | Store always dummy 4, write 5 / 6 cycles |
| Absolute indexed | Crossing dummy 3, final read 4; noncrossing read 3 | Store always dummy 3, write 4 / 5 cycles |
| RMW zp / zpx / abs / abx | Data 2 / 3 / 3 / 4, plus existing indexing dummy | Old/new writes N-2/N-1; N=5/6/6/7 |
| JMP indirect | Target low/high 3/4; preserve NMOS page-wrap address | 5 cycles |
| PLA / PLP | Stack data 3 | 4 cycles |
| RTS | Stack low/high 3/4 | 6 cycles |
| RTI | Stack P/low/high 3/4/5 | 6 cycles |
| BRK and hardware IRQ/NMI entry | Vector low/high 5/6, after stack writes | Existing stack writes 2/3/4; 7 cycles |
| Guarded RAM JMP | Opcode guard 0, operand low/high 1/2 | 3 cycles |

The table describes accesses already represented by the emitter. Pointer byte reads are separately sequenced; indexed dummy reads retain real side effects, even when their values are discarded. Page crossing changes both the final read offset and the total duration. RMW reads precede both existing writes. Stack-pointer updates and B/U handling remain unchanged.

### Explicit remaining approximations

Opcode/operand fetches baked into ROM-native blocks, immediate operand reads, implied/accumulator dummy fetches, branch dummy fetches, stack dummy reads and the final RTS return-address dummy fetch remain omitted. This change timestamps existing accesses; it does not claim one physical bus access per CPU cycle. Interrupt delivery/polling and DMA remain at the existing instruction boundaries; DMA does not interrupt an instruction midway. Vector reads receive offsets, but interrupt hijacking and finer IRQ/NMI/DMC/OAM races remain unresolved. PAL's existing 16/5 accumulator is preserved. PPU/APU register logic is not changed.

### Module compatibility contract

Increment the native service ABI to **version 2**. The generated module exports a simple `nes_recomp_abi_version()` function, checked by the frontend before binding/calling game entry points. Missing or mismatched versions must fail explicitly. Header/emitter changes invalidate generator fingerprints; additionally require `abi_version=2` in the import receipt before reusing a cached module. Older caches regenerate from the user's private compatible ROM. Old frontend/new module combinations are unsupported; the guarded direction is the updated frontend refusing old modules. No commercial content enters source control.

## Implementation and verification

**Completed within the bounded contract above**, 2026-10-10. Both PPUSTATUS divergences are resolved, instruction totals are preserved, and no full scheduler replacement was necessary. Hardware accuracy beyond the represented accesses is still limited as listed below. HEAD remains `e027f5a23f5a2cd49c7a7366c4a211479b7866dd`; changes are local and uncommitted. Application version remains 0.2.1; service ABI is version 2.

### Read inventory and implementation

`emit_address` emits final data reads for ADC/SBC/logical/load/compare/BIT instructions; zero-page indexing dummies; indexed-indirect dummies and pointer bytes; indirect-indexed pointer bytes; and indexed read/store/RMW dummies. `emit_semantics` separately emits RMW data reads, JMP-indirect bytes, stack pulls and BRK vectors. `emit_image` emits the existing guarded RAM JMP opcode/operand reads. Immediate constants and ROM operands are baked into C, with no observable fetch. Every currently emitted native read now supplies an offset. `recomp_read16` and its zero-page variant sequence their two byte reads explicitly, including wrapping rules.

`Bus::read8(address, offset)` first synchronizes devices, then delegates to the existing ordinary read. This preserves the same RAM, cartridge, PPU, APU status, controller shift and open-bus logic. There are no PPU/APU register-specific substitutes. The two service wrappers in ActorView/GodMode merely forward the added argument; their existing game behavior is unchanged. Hardware interrupt entry now also reads its vector at offsets 5/6. Existing timed stack/RMW writes retain their offsets and order.

The service helper requires an explicit offset. A backward device target throws the existing `logic_error`; equal timestamps do not advance devices again. The emitter/oracle tests require strictly ordered represented accesses and offsets below instruction duration. No runtime field for instruction length was added, so that upper bound is a compiler/test invariant rather than a new runtime validation. At completion, `advance(total)` advances the CPU anchor and synchronizes the remaining device interval before existing interrupt/DMA boundary handling.

### Changed files

| Files | Purpose |
|---|---|
| `include/recomp/runtime_services.h`, new `include/recomp/module_abi.h` | Timed read callback/helpers and ABI version 2 |
| `runtime/include/runtime/bus.hpp`, `runtime/src/bus.cpp` | Timed overload and hardware vector read offsets |
| `tools/recompiler/emit_blocks.py` | Offsets for every represented read; exported module ABI function |
| `tools/recompiler/import_user_rom.py` | Require ABI version in cache receipts; preserve fingerprint checks |
| new `runtime/include/runtime/module_abi.hpp`, `runtime/src/user_rom.cpp` | Reject missing/wrong module ABI before binding entry points/resources |
| `game/src/actor_view.cpp`, `game/src/god_mode.cpp` | One read callback signature/forwarding change in each file |
| `tests/opcode_codegen_checks.cpp`, `tests/native_program_checks.cpp` | Independent expected read offsets in the existing matrix/program traces |
| `tests/emit_opcode_fixtures.py`, new `tests/bus_timing_checks.cpp` | Four generated test C units total; 14 additional side-effect schedules |
| `tests/ppustatus_timing_check.cpp` | Both reproductions now require equality; clock and latch diagnostics retained |
| `tests/device_checks.cpp` | One additional mixed read/write clock contract case |
| new `tests/module_abi_fixture.c`, `tests/module_abi_checks.cpp`, `tests/import_cache_checks.py` | Real module loading and synthetic cache decision tests |
| `tests/run_known_divergence.py`, `tests/known_divergence_checks.py` | Preserve diagnostic harness coverage while removing expected-failure/skip handling |
| `CMakeLists.txt`, `.github/workflows/build.yml` | Normal required timing tests, new ABI/device tests, verbose CI with empty-suite rejection |
| `docs/BUS_TIMING_REPORT.md`, `docs/TESTING_BASELINE.md` | This report and pointer from the preserved historical Phase 2 report |

`memory_access.h`, `cpu_state.h`, opcode registry, cartridge/MMC1 rules, PPU/APU implementations, renderers and distribution scripts are unchanged. No preexisting test was deleted. The historical technical audit is unchanged.

### Reproduction commands

From the public repository:

```sh
cmake -S . -B /private/tmp/nes-technical-baseline-build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error

cmake -S . -B /private/tmp/nes-phase3-intel-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build /private/tmp/nes-phase3-intel-build --parallel 4
ctest --test-dir /private/tmp/nes-phase3-intel-build --output-on-failure --verbose --no-tests=error
```

The ARM64 tree was the previously configured Release build, rebuilt completely through the default target after each production change. The Intel tree was newly configured and built. Tests need no game ROM.

### Executed results

Both local architecture configurations produced **12/12 passing CTest entries; zero failed or skipped entries**. The following counts describe one complete suite, avoiding double-counting repeated development runs or the second architecture:

| Entry | Executed result per architecture |
|---|---|
| `public_import_contract` | PASS: 3 preexisting Python cases |
| `diagnostic_harness_contract` | PASS: 5 retained classification cases; failures propagate, with no skip exemption |
| `native_opcode_contract` | PASS: 4,736 executed native instruction cases, all 151 official encodings |
| `native_stack_programs` | PASS: 2 complete programs; 19-cycle JSR/PHA/PLA/RTS and 13-cycle BRK/RTI |
| `ppustatus_read_timing` | PASS: both read/value/side-effect/CPU/total-cycle comparisons |
| `native_bus_side_effects` | PASS: 14 executed native programs, one instruction each |
| `import_cache_abi_contract` | PASS: 4 synthetic cache decisions |
| `module_abi_contract` | PASS: 3 real loadable synthetic modules; missing/old rejected, current accepted |
| `synthetic_ram_contract` | PASS: 3 cases / 10,248 assertions |
| `synthetic_mmc1_contract` | PASS: 4 cases / 45 assertions |
| `synthetic_bus_contract` | PASS: 5 cases / 35 assertions, including NMI entry and the new clock case |
| `synthetic_ppu_contract` | PASS: 5 cases / 20 assertions |

Thus the suite executes 4,736 existing CPU cases, 14 additional register-access cases, two timing regressions, two stack programs, 17 device cases, three module-loader cases and 12 Python cases: **4,786 checks/program cases per architecture**. Assertion counts inside those cases are additional detail, not additional cases. The prior 524,288 ADC/SBC helper cases were not rerun: arithmetic implementation is unchanged, and that historical result is not counted as a Phase 3 execution.

The 14 new cases cover absolute, absolute indexed X/Y, `(d,X)` and `(d),Y` PPUSTATUS reads; crossing dummy reads that clear vblank before the final read; zero-page pointer wrapping; BIT; read/old-write/new-write INC in absolute/indexed forms; an indexed STA dummy read; and PPUDATA buffered reads/increments in absolute and indexed/indirect-indexed forms. A manually authored schedule advances the reference bus explicitly and uses **untimed** accesses, rather than the new timed helper. Tests compare address, value, order, offset, registers/flags/PC/SP, final device effects and total clocks. Native callbacks assert PAL dots and APU cycles immediately after each real access and assert that CPU time remains at the instruction anchor.

| PPUSTATUS start | Generated/reference read cycle | A / P | Status before/after | Total cycles |
|---|---|---|---|---|
| 25,680 | 25,683 / 25,683 | `$80` / `$ED` on both routes | `$80` → `$00` | 4 / 4 |
| 58,927 | 58,930 / 58,930 | `$80` / `$ED` on both routes | `$80` → `$00`; address latch reset checked | 4 / 4 |

Native pre-clear status in diagnostics is inferred from returned high bits, so instrumentation adds no extra read or external device synchronization. The reference directly inspects status before reading.

CTest now invokes `ppustatus_timing_check` directly as a normal mandatory test: there is no `WILL_FAIL`, skip return code or known-divergence label. A temporary generated-C mutation changing the absolute read offset back to 0 reproduced **both** old mismatches (25,680 and 58,927, A=$00, P=$6F, four cycles) and exited 1 with `first=0 warm=0`. Production files were not altered by this negative sensitivity check. Those two intentional mutation failures are not unresolved production failures.

Additional ARM64 checks, outside CTest:

- Existing bundled compiler smoke test: PASS, synthetic C compiled and loaded, returning 42.
- Existing frontend import process checks: PASS, spaced paths, early stdout/stderr and launch failure.
- An isolated copy of an existing private cache with no ABI receipt field was regenerated from the previously configured compatible private ROM. New receipt reports schema 2 / ABI 2; the frontend loaded the new module and its existing probe completed **1,800 frames**, reached the garden and passed GOD/horizontal-map assertions. This is a smoke check, not a full-game or visual certification. The original cache was preserved; module/resources stayed outside Git.
- Initial bundled compiler/cross-compile attempts hit the sandbox restriction on `~/.cache/zig`; rerunning with `ZIG_GLOBAL_CACHE_DIR=/private/tmp/nes-phase3-zig-cache` passed. This was a local environment failure, not a timing test result.

### Real platform verification

| Platform | Evidence | Status |
|---|---|---|
| macOS ARM64 | Full Release build, 12/12 CTest, package-tool checks, fresh private import/1,800-frame probe | Executed and passed on macOS 27.0.1 ARM64 / AppleClang 21 |
| macOS Intel | Full x86_64 Release build, verified Mach-O x86_64 binaries, 12/12 CTest via Rosetta | Executed and passed through Rosetta; not a physical Intel host or private Intel import test |
| Linux x64 | All four synthetic generated C units cross-compiled with bundled Zig to x86_64-linux-gnu | C translation compile passed; frontend/link/runtime tests **not executed** |
| Windows x64 | All four synthetic generated C units cross-compiled with bundled Zig to x86_64-windows-gnu | C translation compile passed; frontend/link/runtime tests **not executed** |

The existing four-host workflow (macos-15, macos-15-intel, Windows, Linux) already builds all default targets and runs CTest. Its commands now use `--verbose --no-tests=error`, exposing native subcase diagnostics and rejecting empty suites. No new workflow dispatch, push, commit, release or packaging change was made. Linux/Windows runtime verification remains pending actual CI execution; cross-compilation alone does not verify it.

### References, remaining limits and risks

The scheduling expectations follow the existing instruction model, checked against [NESdev addressing modes](https://www.nesdev.org/wiki/Addressing_modes), [instruction reference](https://www.nesdev.org/wiki/Instruction_reference), and the preserved transistor-analysis descriptions of the [6502 state machine](https://www.nesdev.org/wiki/Visual6502wiki/6502_State_Machine) and [timing states](https://www.nesdev.org/wiki/Visual6502wiki/6502_Timing_States). The tests use independent expected arithmetic/schedules, but the device reference shares the production PPU/APU implementation. They isolate read scheduling and do not independently validate the entire device model or sub-cycle electrical behavior.

Remaining issues, ordered by impact:

1. **Hardware access coverage:** baked ROM fetches and other omitted dummy accesses remain absent. Open-bus/register observations involving those missing accesses can still differ from hardware. This fix makes existing represented accesses coherent, not every physical 6502 bus transaction.
2. **Interrupt/DMA precision:** polling, OAM/DMC stalls and delivery remain at existing boundaries. Earlier device synchronization can expose pending device events before completion; their delivery still follows existing boundary rules. Within-instruction line sampling, BRK/NMI hijacking, DMA read-cycle contention and cancellation races are not solved. Existing mask/NMI/BRK tests pass; no new hardware-wide DMA accuracy claim is made.
3. **Device oracle limitations:** precise PPU vblank-edge races, PPU/APU register internals and timed APU/controller side-effect coverage beyond the preserved tests remain separate work. The same device model underlies both routes of the clock regression.
4. **ABI deployment:** a matching frontend/importer/header set is required. Updated frontends explicitly reject old modules; receipts without version 2 trigger regeneration. An old frontend cannot validate a future module and is unsupported. ABI/version errors are surfaced without calling incompatible game entry points.
5. **Platform execution:** Windows/Linux C ABI compilation is verified, but full runtime suites need the prepared CI hosts. macOS Intel was exercised using Rosetta.

Next recommended task, requiring a separate instruction: execute the prepared suite on Windows/Linux CI before distribution, then specify synthetic tests for the missing fetch/dummy and interrupt/DMA observations before extending their hardware contract. Headless build, render/widescreen, audio fidelity and specific gameplay issues remain outside this phase.

Evidence logs, starting-state hashes, negative mutation output and source audit are retained outside Git in the surrounding workspace's `outputs/bus-timing-2026-10-10/`. No commercial data or generated game C is placed in that evidence directory or in the public source tree.
