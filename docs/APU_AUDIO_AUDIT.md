# APU audio audit — tests only

Date: **2026-10-10**. Outcome: the basic PAL channel contracts pass; **three reference discrepancies reproduce and remain unfixed**. Original-game audio fidelity is **not certified**. Production code, ABI, game logic, graphics, widescreen and distribution are unchanged. No commits, pushes, branch changes or new dependencies.

## 1. Checkout and prior evidence

Repository: `/Users/sebas/Documents/Codex/2026-10-03/referenced-chatgpt-conversation-this-is-an/outputs/public-alpha-0.2.0/addams-family-recompiler`.

- Branch: **`verify/phase-3-1-bus-timing`**.
- HEAD: **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**; CMake application version **0.2.1**, ABI **2**.
- The directory name `public-alpha-0.2.0` is historical. This Git checkout contains the subsequent uncommitted bus-timing, headless and gameplay-diagnostic work. Existing build caches point to this exact source directory.
- `outputs/public-alpha-0.2.1/addams-family-recompiler`, `nes-static-recomp` and its Linux validation source are non-Git exports/development artifacts, not competing active checkouts. None was restored over the current source. The old development tree was read only to recover synthetic test ideas.
- Located: **TECHNICAL_BASELINE.md, TESTING_BASELINE.md, BUS_TIMING_REPORT.md, PLATFORM_VERIFICATION.md, HEADLESS_BUILD_REPORT.md, GAMEPLAY_VERIFICATION.md** under this checkout's `docs/`. **INTERRUPT_DMA_TESTING.md is absent**. Earlier reports are historical snapshots; none was overwritten.

Starting hashes cover **105 public files**. After this task **104 remain byte-identical**, only test integration in `CMakeLists.txt` changed, and three files were added. Existing tracked and untracked work is preserved. Evidence outside public Git: surrounding workspace `outputs/apu-audit-2026-10-10/` (starting/final hashes and Git status, build logs, CTest inventories/JUnit, source-preservation checks).

## 2. Current architecture

Generated C uses ABI-v2 timed reads/writes. `Bus::sync_devices` advances PPU and APU to the instruction anchor plus access offset; final instruction advancement supplies only residual time. Each CPU tick runs the frame sequencer and triangle timer; every second tick also clocks pulses, noise and DMC. DMA stalls advance the same clocks. IRQ delivery uses existing instruction-boundary polling (`runtime/src/bus.cpp:13,32,59,71,90`).

`AudioState` owns digital channel state, precomputed nonlinear pulse/TND tables, a rational **48,000 Hz** accumulator, per-sample cycle averaging and host filters. Its queue holds at most 48,000 samples and counts discarded overflow (`runtime/src/audio_state.cpp:19,145,148,160`). At each completed video frame the frontend drains samples, submits them to SDL and optionally writes WAV (`app/desktop.cpp:154`). SDL uses mono float32, a 1,024-sample requested buffer, delayed startup, a 250 ms queue cap and counted batch rejection; this is host presentation, not APU timing.

The European target uses **PAL/Ricoh 2A07**, rather than NTSC 2A03 timings. CPU frequency is the rational `53203425/32` Hz; noise/DMC rate tables and frame events are PAL. There is no NTSC-region selector. Host architecture does not select a different NES clock. The PAL/NTSC distinction follows the [NESdev clock reference](https://www.nesdev.org/wiki/Cycle_reference_chart).

## 3. Implemented functions and remaining scope

| Component | Implemented and bounded by the new checks | Remaining limits |
|---|---|---|
| Pulse 1 and 2 | Four duties; period `16*(timer+1)`; high-write phase restart without divider restart; length/envelope/constant volume; sweep divider, channel-specific negate and mute thresholds | Register/frame-clock collision details and complete changing-note traces are not covered |
| Triangle | CPU-rate timer, length/linear reload and control, period `32*(timer+1)`, held DAC when halted | Every phase, ultrasonic output and hardware startup/reset alignment remain unverified |
| Noise | All 16 PAL periods; 15-bit LFSR long/short taps, output gate, length/envelope | Startup phase on physical hardware and long statistical/spectral traces are not certified |
| DMC | All 16 PAL periods; seven-bit DAC, LSB-first deltas/saturation; sample address/length, buffering, wrap/loop, end-fetch IRQ and IRQ acknowledgement | PAL get/put alignment, mid-DMA cancellation, reload versus initial-load stalls and OAM end overlaps lack complete coverage |
| Frame counter | PAL four/five-step events, lengths/envelopes/linear clocks, repeated four-step IRQ assertion, inhibit, basic three/four-cycle reset delay | Delayed mode application is wrong in a reproduced edge case; adjacent frame-clock suppression and read-clear races are incomplete |
| Sampling/output | Repeatable PCM across batch/drain schedules, exact rational sample counts, reported queue overflow, nonlinear TND step, approximate filters, dummy SDL submission/drain | No hardware waveform, original soundtrack, full five-channel analog mix, spectral/aliasing or real audio-device comparison |
| Initialization | Deterministic fresh construction and channel disable/status behavior | **No soft-reset API or full hardware reset sequence**. Reconstructing an object is not a passing reset-fidelity test |

Registers `$4000–$4007`, `$4008/$400A/$400B`, `$400C/$400E/$400F`, `$4010–$4013`, `$4015` and writes to `$4017` are implemented. `$4009/$400D` are unused. `$4014` belongs to OAM DMA; `$4016` to controllers; reads of `$4017` belong to controller port 2. Write-only registers read as open bus through the bus. Status bits reflect length/DMC bytes/IRQ; triangle's linear count does not determine its status bit. These contracts were checked against [NESdev APU/status](https://www.nesdev.org/wiki/APU_Status).

## 4. Tests added and independent references

The old `nes-static-recomp/tests/audio_checks.cpp` mixed useful synthetic channel checks with private resource arrays and an original IRQ vector. The reusable cases now use an authored, zero-filled cartridge plus synthetic byte/vector values and the **current public generated-C fixture library**. There is no recovery-tree dependency. Its WAV file-size check was not copied; that is container testing rather than channel fidelity. The legacy SDL check was adapted to the dummy device.

`apu_audio_contract` executes **17 named native groups**, continuing after any failure so all group results remain visible:

1. Fresh construction, status/open-bus bit, DAC masking and ignored non-APU registers.
2. All 32 length-table entries, four channels and 16 channel masks.
3. PAL four-step envelopes/lengths/IRQ assertion/inhibition/wrap.
4. Five-step reset parity and event schedule away from collisions.
5. Three envelope restart/divider/loop/constant-volume traces and length halt.
6. Both pulse duties/timers/mutes and phase-versus-divider restart.
7. Negative sweep asymmetry and divider interval.
8. Triangle linear reload/countdown/control, period and held DAC.
9. Both noise taps, all PAL periods and output gates.
10. All PAL DMC bit rates, LSB-first deltas, saturation and status/IRQ persistence.
11. DMC address wrap and 65-byte loop with IRQ enabled but suppressed by looping.
12. Request cancellation, invalid completion rejection and DMC IRQ disable.
13. Synthetic mapped DMC fetch, ordinary boundary stall parity and IRQ stack/vector.
14. Existing generated C CLI/SEI/PLP/RTI against real APU IRQ state.
15. Existing generated C STA `(zp,X)` to `$4017` and LDA `(zp),Y` from `$4015`, including access/finish cycles and final CPU state.
16. Batch-independent PCM, rational sample count, finite bounds, energy and queue overflow.
17. Analytical nonlinear TND/filter response to a direct DAC step.

The sample check produces **47,999 samples in 1,662,607 CPU cycles**. Exact same-platform samples match despite 997-cycle stepping and repeated drains. This is a determinism contract, not independent original-waveform validation. The TND step uses the [NESdev mixer formula](https://www.nesdev.org/wiki/APU_Mixer) and analytically derived filters, not production lookup/filter helpers. Its **2e-6 absolute PCM tolerance** accommodates float tables and the declared 440 Hz coefficient's approximately **440.015342 Hz** effective pole; the first sample is `0.286143004894` versus `0.286143598425`. This negligible filter approximation is recorded, not hidden as exact analog agreement.

Expected channel values/tables come from [pulse](https://www.nesdev.org/wiki/APU_Pulse), [sweep](https://www.nesdev.org/wiki/APU_Sweep), [envelope](https://www.nesdev.org/wiki/APU_Envelope), [length](https://www.nesdev.org/wiki/APU_Length_Counter), [triangle](https://www.nesdev.org/wiki/APU_Triangle), [noise](https://www.nesdev.org/wiki/APU_Noise) and [DMC](https://www.nesdev.org/wiki/APU_DMC) references. Small independently written expected schedules are not a second CPU/APU emulator. Same-device comparisons alone are labelled determinism/integration tests.

The event positions/mode delay were also inspected in [Mesen's independent ApuFrameCounter source](https://github.com/SourMesen/Mesen2/blob/master/Core/NES/APU/ApuFrameCounter.h), and open-bus handling in [NesMemoryManager](https://github.com/SourMesen/Mesen2/blob/master/Core/NES/NesMemoryManager.cpp). These sources were inspected on the audit date; **Mesen was not built/executed**, and master links may evolve. Some direct NESdev requests returned 403; indexed reference text was available. No external emulator or large library was installed. Physical [power/reset state](https://www.nesdev.org/wiki/CPU_power_up_state) is revision dependent and broader than the startup test.

## 5. Reproduced discrepancies and prioritized risks

| ID / severity | Evidence type | Observation and probable impact |
|---|---|---|
| **A03 — MEDIO** | **Reproduced failure** | At cycle 33,251 write `$80` to `$4017`; cycle 33,252 should still have the old four-step IRQ until the delayed reset. Actual IRQ is 0. `write_register` immediately changes `frame_control_`, including mode. Can alter IRQ/envelope scheduling around a mode change |
| **A02 — MEDIO** | **Reproduced reference discrepancy** | Timed `$4015` read at the first PAL IRQ assertion, cycle 33,252, returns `$40` but immediately clears the new flag. The status reference requires coincident assertion to survive. `read_status` unconditionally clears it, with no assertion-cycle information. Can lose a polling/IRQ event; finer CPU half-cycle behavior remains outside this fixture |
| **A01 — BAJO** | **Reproduced failure** | External open bus set to `$A5`; disabled `$4015` returns `$20`; subsequent unmapped read returns `$20` instead of `$A5`. Bus line 23 assigns status to external open bus although `$4015` is internal. Can change later open-bus-dependent reads; audible game impact is unverified |
| **A04 — ALTO potential** | Known approximation; game impact **unverified** | Cycle averaging followed by filtering at output sample rate is not an independently validated band-limited renderer. High-frequency triangle/noise/harmonics can alias. No perceptible error in this game was demonstrated |
| **A05 — MEDIO** | Known coverage limit | Basic DMC DMA works, but rare overlap/cancel timing remains approximate and untested. Existing DMA source comments identify this limitation. The [DMA reference](https://www.nesdev.org/wiki/DMA) distinguishes initial-load/reload scheduling; testing ordinary stalls does not certify those races |
| **A06 — MEDIO conditional** | Missing behavior, not a failed gameplay run | No soft-reset contract. Relevant if an actual console-reset feature is required; a fresh process is currently the tested startup path |
| **A07 — BAJO** | Known numerical approximation | Filter pole discrepancy above, fixed gain 1.4 and clipping. No measured analog output calibration; correcting a 0.015 Hz pole difference has little expected benefit |

**No CRÍTICO hang/corruption and no ALTO perceptible soundtrack failure was reproduced.** Invalid synthetic DMC completion intentionally throws; the normal mapped fetch path does not. It is not evidence of a game crash.

For The Addams Family, the implemented PAL clocks and ordinary channel behavior support basic playback. Whether its actual sound driver hits A01–A03, uses DMC in particular scenes, or exposes A04/A05 is **not measured in this task**. No commercial ROM, private module, game audio or extracted sound resources were used. A user's report of playable sound does not establish original fidelity.

## 6. Executed results and omissions

Host: **macOS ARM64**, Darwin 27.0.0, AppleClang **21.0.0**, CMake **4.4.4**, Release, mods ON. Full existing builds succeeded. Counts below refer only to the **final unfiltered runs**, excluding preliminary focused development runs.

| Configuration | CTest entries executed | Passed | Failed | Skipped |
|---|---:|---:|---:|---:|
| Desktop ON | 18 | 15 | 3 | 0 |
| Desktop OFF | 16 | 13 | 3 | 0 |
| Total | **34** | **28** | **6** | **0** |

New coverage comprises **21 distinct cases/groups**: 17 core, three failed references and one dummy SDL queue group. Across both configurations: **41 new case/group executions, 35 passed, six failed** (the same three defects twice). Headless intentionally has no SDL test or existing dynamic SDL module-loader test; neither is a runtime skip.

All **25 compatible preexisting CTest entry executions passed** (13 desktop + 12 headless). This includes **9,472 native opcode executions** (4,736 per configuration), both stack programs per configuration, both `$2002` regressions, 14 timed side-effect cases, RAM/MMC1/bus/PPU, ABI/cache/importer and gameplay-diagnostic checks. **Zero regressions in the previous battery.** The historical 524,288 ADC/SBC sweep was not repeated.

The three `apu_*_reference` entries are **ordinary failing tests**, labelled `known-divergence`, with no WILL_FAIL, disabled/skip setting or artificial expected value. Both full CTest invocations exit **8**. Unfiltered CI will therefore be red until the reference mismatches are addressed under a separate authorization. The existing inventory gate accepts all 18 desktop commands; its JUnit gate rejects the failing report. Its wording `Test did not run` reflects CTest's `status=fail`, **not an omitted execution**; the actual failure outputs are retained.

Not executed: normal game/song/effect comparison; hardware recordings or real speakers; NTSC mode; CPU half-cycle collisions beyond these bounded schedules; complete OAM/DMC overlap/cancellation; true soft reset; WAV export; Windows/Linux/macOS Intel execution; remote CI; mods-OFF configuration. No such verification is counted as a pass.

## 7. Changed files and reproduction

Only `CMakeLists.txt` test integration is modified. New files: `tests/apu_audio_checks.cpp`, `tests/apu_sdl_queue_checks.cpp`, and this report. Windows static-link options are applied to the new test targets consistently with existing tests. Test fixtures stay in build directories. No workflow, package, production or earlier-report change.

From this repository:

```sh
cmake -S . -B /private/tmp/nes-apu-audit-headless -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF -DNES_ENABLE_MODS=ON
cmake --build /private/tmp/nes-apu-audit-headless --parallel 4
ctest --test-dir /private/tmp/nes-apu-audit-headless --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-apu-audit-headless.xml

cmake -S . -B /private/tmp/nes-apu-audit-desktop -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON
cmake --build /private/tmp/nes-apu-audit-desktop --parallel 4
ctest --test-dir /private/tmp/nes-apu-audit-desktop --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-apu-audit-desktop.xml
```

Actual reused build directories were `/private/tmp/nes-b10-headless-build` and `/private/tmp/nes-technical-baseline-build`, verified to use the current checkout. No dependencies were downloaded for this audit. On Windows/Linux choose equivalent external build paths and the existing platform toolchain. This is not a claim of execution there.

Run the passing isolated APU groups with `<build>/apu_audio_checks core`. Reproduce each failure with `status_open_bus`, `status_irq_collision` or `frame_mode_delay` as its argument; each currently exits 1. `ctest --test-dir <build> -L known-divergence --verbose` executes all three and also fails. An explicitly filtered non-divergence developer run does not establish that the complete suite is green.

## 8. Three recommended corrections — not implemented

1. **Delay `$4017` mode application until its scheduled reset (A03)**, preserving immediate IRQ inhibition separately. Best expected timing benefit; moderate localized cost. Use the failed edge case and add adjacency-clock cases before implementation. No scheduler rewrite is proposed.
2. **Represent frame IRQ assertion/read-clear ordering (A02)** with an explicit cycle convention, preserving the status flag when assertion coincides with the read. Moderate benefit and moderate precision risk/cost; first resolve any half-cycle interpretation against an independent PAL trace. Do not merely suppress the failing assertion.
3. **Preserve external open bus on `$4015` reads (A01)**. Smallest likely code correction and existing bus interfaces suffice; low direct audio benefit. The current isolated failure provides a cheap regression.

Defer filter/resampler replacement, region support and complete DMA/reset changes until register timing is reviewed and a short private, reproducible original-audio comparison identifies an audible benefit. This audit is complete; **no correction is authorized or performed**.
