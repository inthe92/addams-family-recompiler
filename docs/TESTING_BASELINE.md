# Phase 2 — Automated testing baseline

> Historical Phase 2 snapshot. Phase 3 updates are documented in [BUS_TIMING_REPORT.md](BUS_TIMING_REPORT.md): both PPUSTATUS cases are now mandatory passing regressions, ABI v2 adds timed reads, and the current suite has 12 passing entries on local ARM64 and x86_64/Rosetta builds. The Phase 2 results below remain unchanged as audit history.

Date: **2026-10-10**. Read alongside [TECHNICAL_BASELINE.md](TECHNICAL_BASELINE.md), which remains the unchanged historical audit.

## Source reference and change boundaries

The starting and ending HEAD is **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**, declared version **0.2.1**. At the start, the only uncommitted file was `docs/TECHNICAL_BASELINE.md`; it was preserved byte-for-byte. This phase adds tests and their CMake registration only. No production ABI, emitter, opcode registry, runtime, game logic, renderer, widescreen, dependency/distribution workflow or packaging source was changed. No commits or pushes were made.

The reusable driver was found in the older local development checkout at `nes-static-recomp/tests/opcode_codegen_checks.cpp`; its companion was `tools/recompiler/emit_opcode_fixtures.py`. The public test generator now imports **only this repository's** `tools/recompiler/emit_blocks.py` and `opcodes_6502.py`. There is no recovery-directory shim, ROM, manifest, commercial module or game resource dependency.

## Files added or modified

| File | Purpose |
|---|---|
| `CMakeLists.txt` | Adds targets and CTest entries under existing `BUILD_TESTING`; retains ordinary desktop configuration |
| `tests/emit_opcode_fixtures.py` | Generates 176 instruction fixtures, two authored stack programs and one LDA/PPUSTATUS fixture |
| `tests/opcode_codegen_checks.cpp` | Reused independent instruction oracle, extended addressing/stack/status/bus checks |
| `tests/native_program_checks.cpp` | Executes JSR/PHA/PLA/RTS and BRK/RTI sequences with stack wrap |
| `tests/device_checks.cpp` | Deterministic RAM/MMC1/bus/PPU cases using existing runtime interfaces |
| `tests/ppustatus_timing_check.cpp` | Raw, intentionally failing known timing regression; two vblank phases |
| `tests/run_known_divergence.py` | Converts only the recognized diagnostic failure into an explicit CTest skip |
| `tests/known_divergence_checks.py` | Guards classification of known/unrelated failures, crashes and unexpected resolution |
| `docs/TESTING_BASELINE.md` | This report |

Generated files live exclusively in `<build>/tests/generated/`: **three C translation units and one header**. The C compiler compiles the actual current emitter output as C99; C++17 drivers call its exported functions. CMake regeneration depends on the generator, emitter and opcode table. Generated fixtures are synthetic and are not committed. Tests do not execute a general CPU interpreter.

## Executed results

Host: **macOS 27.0.1 ARM64**, Apple Clang **21.0.0**, CMake **4.4.4**, system Python **3.9.6**. Ordinary Release configuration was built with desktop and mods enabled. The build reused unchanged production/dependency objects from the audit and compiled the new generated C and test drivers. All default build targets completed successfully.

| Check | Executed result |
|---|---|
| Default configure + full build target | PASS, exit 0 |
| `public_import_contract` | PASS: all **3** original Python cases |
| `known_divergence_harness_contract` | PASS: **5** classification cases |
| `native_opcode_contract` | PASS: **4,736** real native instruction executions |
| `native_stack_programs` | PASS: **2** synthetic native programs |
| `synthetic_ram_contract` | PASS: **3** named cases / **10,248** assertions |
| `synthetic_mmc1_contract` | PASS: **4** named cases / **45** assertions |
| `synthetic_bus_contract` | PASS: **4** named cases / **29** assertions |
| `synthetic_ppu_contract` | PASS: **5** named cases / **20** assertions |
| `known_ppustatus_read_timing` | **KNOWN DIVERGENCE / SKIPPED**, not PASS; underlying diagnostic executes and returns 1 |
| Direct `ppustatus_timing_check` | **FAIL as documented**, exit 1: two read-timing fidelity comparisons differ |
| Existing `public_toolchain_smoke.py` | PASS: bundled Python/Zig compile and load a synthetic function returning 42 |
| Existing `public_import_process_checks.py` | PASS: spaced paths, early stdout/stderr capture and executable launch failure |

**CTest summary: 9 registered entries, 8 passed, 1 explicitly skipped, zero unexpected failures.** The ordinary entries contain 4,736 instruction cases, two programs, 16 named device cases and eight Python cases. The two external package-tool checks above are additional checks, not additional CTest entries. The two known timing comparisons failed fidelity expectations; they are not included in passing case counts. CTest's aggregate `100% tests passed` excludes the skipped failure and must not be interpreted as device accuracy.

Four temporary mutation probes were also rejected by the native oracle: a wrong LDA result, removal of an RMW old-value write, a wrong total cycle count, and removal of zero-page pointer high-byte wrap. Only temporary copies of generated C or headers were modified; production files were untouched. These probes check test sensitivity and are not regular CI entries.

The earlier **524,288 ADC/SBC helper cases** were not rerun: production arithmetic headers and runtime configuration are unchanged. Their evidence remains in the original audit; they are not counted as executions in this phase. No commercial-input test, fresh game import, playthrough, visual gameplay test or cross-platform CI job was run in this phase. The existing package-tool checks use only synthetic data and a previously configured local toolchain.

## Native CPU coverage

The instruction matrix preserves **668 original cases** (167 fixtures × four flag seeds). It adds nine boundary fixtures and five additional register/memory/stack scenarios: **3,556 additional cases**, for 4,224 matrix executions. A separate **512-case** sweep executes generated PLP and RTI for every saved status byte, including B/U normalization and stack wrap. Total: **4,736**.

The oracle computes expected results without calling `cpu_adc`, `cpu_sbc`, `cpu_set_flag`, `recomp_pull`, `recomp_push` or the emitter's timing/address helpers. Arithmetic uses integer addition/subtraction and signed-range overflow; cycles use a separate literal opcode timing table. It compares A/X/Y/SP/PC and complete P, final memory, ordered reads/writes and values, timed write offsets, total cycles, completion count and old/restored-I poll selection.

Coverage includes all **151 official encodings**, immediate/accumulator/implied and memory addressing modes; zero-page indexing and pointer wrap; indexed crossing/noncrossing and 16-bit address wrap; taken/not-taken branches, forward/backward page crossing and PC wrap; JSR/RTS/RTI/BRK, push/pull and SP=$00/$FF boundaries; JMP-indirect page wrap; RMW old/new write order; binary ADC/SBC with D set; all saved status bit combinations and unchanged flags/registers where appropriate. Distinct pointer/stack bytes expose incorrect addresses that uniform fill data can hide.

The two multi-instruction programs check complete round trips, stack contents and bus traces. JSR/PHA/PLA/RTS consumes **19 cycles** and returns to `$8003`; BRK/RTI consumes **13 cycles** and restores `$8102`, with software B=1 in saved status and B=0/U=1 after restore.

### Reference limitations

- The functional oracle is independent code, not independently measured silicon. Fixture instruction names/modes come from the public opcode registry, so registry metadata is shared. Manual program results and the separate cycle table reduce that coupling but do not independently certify every encoding-table entry.
- Traces cover accesses exposed by the current service ABI. Fetches baked into native code and absent dummy accesses are not required to appear. Reads have no timestamp in that ABI; `-1` in the CPU trace means **unknown offset**, not cycle zero. Correct trace comparison does not establish a complete per-cycle hardware bus trace.
- IRQ mask selection is checked for both initial I states, but within-instruction sampling, hijacking and device-line races are not modeled by the functional oracle.
- The test memory is a flat synthetic 64 KiB array. Device tests separately use the real bus. Neither is a complete independent NES implementation.
- Only this macOS ARM64 configuration was executed. Windows/Linux/macOS Intel portability of the new tests remains unexecuted, even though they use the existing build configuration and portable interfaces.

Reference expectations were checked against [NESdev instruction reference](https://www.nesdev.org/wiki/Instruction_reference), [stack behavior](https://www.nesdev.org/wiki/Stack), [status flags](https://www.nesdev.org/wiki/Status_flag), [PPU registers](https://www.nesdev.org/wiki/PPU_registers) and the MMC1/interrupt references in the technical audit. The oracle is NES binary arithmetic, not MOS decimal-mode arithmetic.

## Known PPUSTATUS divergence

The test generates `LDA $2002` using the current emitter. Two buses with artificial zero-filled PRG/CHR start identically. The scheduling reference advances three CPU cycles, reads PPUSTATUS, and advances the final cycle. The generated block consumes four cycles but reads before device advancement.

| Observation | Generated execution | Scheduling reference |
|---|---|---|
| First start / read cycle | 25,680 / **25,680** | 25,680 / **25,683** |
| Warm start / read cycle | 58,927 / **58,927** | 58,927 / **58,930** |
| Returned A | `$00` | `$80` |
| Final P, initial `$6D` | `$6F` (Z set) | `$ED` (N set) |
| Status before → after read | `$00 → $00` | `$80 → $00` |
| Status after all four cycles | `$80` | `$00` |
| PC / X / Y / SP | `$8003 / $17 / $81 / $FE` | Same expected values |
| Total cycles | 4 | 4 |

The warm fixture also leaves PPUADDR half-written before the status read and verifies that reading status resets the shared write toggle; both executions pass that side-effect invariant. Low open-bus bits are explicitly initialized to zero. This is a **same-runtime scheduling comparison**, not a console trace; it isolates the read contract and does not validate all PAL race behavior.

The raw executable exits **1** only after recognizing both exact discrepancy signatures and checking invariant state/cycles/side effects. Other setup/state failures exit **2**. The wrapper accepts only exit 1 with the named marker and returns **77**, registered as CTest `SKIP_RETURN_CODE`; it never uses `WILL_FAIL` to turn a failure into PASS. Unexpected resolution (raw exit 0), crashes or unrelated failures make the wrapper fail CI, requiring explicit baseline review. CTest's “did not run” skip wording does not mean the raw diagnostic was omitted: its output is recorded by the wrapper.

## Device coverage and remaining gaps

RAM tests check deterministic initialization, every location across four mirrors, write aliases, clear, range rejection and stack wrap. MMC1 checks all four PRG modes, 4/8 KiB CHR modes, all mirroring modes, serial destination selection/reset, adjacent D0 suppression, the D7 exception, a preceding nonserial write, invalid timestamps and PRG-RAM disable/open bus. Bus tests cover timed writes/device advancement, RAM/PRG routing, controller strobe/bit order, register aliases and basic NMI vector/status/stack/cycle entry. PPU tests cover startup gating, control/mask, address/scroll toggle reset, data writes/increments, buffered/palette reads, grayscale, CHR-ROM write rejection, OAM wrap, status/vblank/NMI and frame notifications.

Pending coverage: exhaustive arithmetic through generated instruction paths rather than helpers; independently validated opcode metadata; complete fetch/dummy-read timelines; interrupt overlap/poll races; OAM/DMC DMA overlaps and cancellation; APU waveform/sequencer regressions; rendering-time scroll/sprite/overflow/OAM behavior; palette/CHR changes within a line; mapper board variants outside the supported wiring; import cache concurrency/ABI compatibility; game-route and widescreen regressions. `NES_ENABLE_DESKTOP=OFF` remains broken as previously documented and was not retested or changed.

## Run the tests

No ROM or network is needed for the ordinary build and CTest suite:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
ctest --test-dir build -L synthetic --output-on-failure --verbose
ctest --test-dir build --show-only=json-v1
```

Inspect the known divergence independently (expected exit **1**):

```sh
./build/ppustatus_timing_check
ctest --test-dir build -L known-divergence --verbose
```

On Windows use `build/ppustatus_timing_check.exe`; for multi-configuration generators use the configured output subdirectory and `ctest -C Release`. Ninja/ordinary Makefiles place the executables directly in `build`.

Existing package-tool checks, with `TASK_RESOURCES` pointing to a configured resource root containing `tools/recompiler` and `toolchain`:

```sh
python3 -B tests/public_toolchain_smoke.py "$TASK_RESOURCES"
python3 -B tests/public_import_process_checks.py "$TASK_RESOURCES" build/import_probe
```

Use the corresponding Python/executable commands on Windows. Package-tool checks retain their existing separate workflow steps; they are not registered unconditionally because ordinary source builds do not require a downloaded bundled toolchain.

Local evidence is retained outside Git in the surrounding workspace's `outputs/testing-baseline-2026-10-10/`: configure/build/CTest logs, CTest inventory, direct PPUSTATUS output with exit code, package-tool logs, mutation probe output, source safety audit and a machine-readable result ledger. No imported game data is retained there by this phase.

## Next phase recommendation

Run these tests on the four existing CI hosts when publication is separately authorized. Then specify a minimal timed-read contract using the visible PPUSTATUS regression and protect additional side-effecting reads/interrupt polling before changing production ABI or runtime. Treat the headless-build fix as a separate bounded change. No work on that next phase was performed here.

## Resumen

Fase 2 incorpora los 668 casos originales y amplía la suite a 4.736 ejecuciones nativas, dos programas y 16 casos de dispositivos. CTest tiene ocho entradas aprobadas y una divergencia conocida explícita. `$2002` continúa fallando sus dos comparaciones de fidelidad; no se ha ocultado ni corregido. El motor y el documento de auditoría previo permanecen intactos.
