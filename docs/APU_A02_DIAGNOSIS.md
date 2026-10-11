# A02 — PAL IRQ/status timing diagnosis (no correction)

Date: **2026-10-11**. **Stopped at reference validation, before production changes. A02 is unresolved and remains an ordinary failing test.** The existing test's immediate `irq_pending() == true` expectation is not sufficiently confirmed by the independent reference. Making it green with a same-cycle retention check could introduce a spurious CPU IRQ.

## Checkout and preservation

Checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` in the local workspace. Branch: **`verify/phase-3-1-bus-timing`**. HEAD: **`b649d6bd906788aaa83d0c5d5b336297344d7324`**. Origin: `https://github.com/inthe92/addams-family-recompiler.git`. Starting Git status was clean; remote branch matched HEAD. Main remained `1588f31e3c3375adfbb4f75266d44eff1fdb25e5`.

Read `APU_AUDIO_AUDIT.md`, `APU_A01_FIX.md`, `APU_A03_FIX.md` and `CI_WINDOWS_JUNIT_FIX.md`. Earlier reports remain unchanged historical snapshots. The prior [four-host run 38126149097](https://github.com/inthe92/addams-family-recompiler/actions/runs/38126149097) remains valid evidence for that unchanged source, not a new execution in this task.

The only repository addition in this task is **this diagnosis**. Production, test bodies, assertions, labels, CMake, workflow, ABI, caches and original reports are unchanged. No commit, push, branch change, tag or release was performed. Synthetic probes, logs and fetched reference source are outside the public repository in the surrounding workspace `outputs/apu-a02-diagnosis-2026-10-11/`. No commercial ROM or resource was used.

## Exact reproduced software ordering

The existing `status_irq_collision()` test in `tests/apu_audio_checks.cpp:321`:

1. Advances the instruction anchor to 33,249.
2. Requests a timed `$4015` read with offset 3.
3. `Bus::sync_devices` advances both devices to 33,252 before the read.
4. `AudioState::tick` calls the frame counter; the PAL sequencer asserts `frame_irq_` at 33,252.
5. `read_status` samples that boolean into bit 6, then unconditionally clears it (`runtime/src/audio_state.cpp:97`).
6. Status is `$40`; `irq_pending()` is false. The unchanged reference assertion requires true and fails.

`irq_pending()` is also the source queried by `Bus::boundary` to deliver CPU IRQs. It is not a separate status-latch accessor. The test samples **immediately after the bus callback**, without finishing the instruction or specifying an internal CPU phase. Devices are already at 33,252, while the instruction anchor is still 33,249.

An external, observation-only C++ probe linked the existing native runtime. Fresh APU instances were observed at each landmark; no hardware expectations were supplied:

| Completed CPU cycles | IRQ before read | Status | IRQ immediately after |
|---:|---:|---:|---:|
| 33,251 | 0 | `$00` | 0 |
| 33,252 | 1 | `$40` | 0 |
| 33,253 | 1 | `$40` | 0 |
| 33,254 | 1 | `$40` | 0 |
| 33,255 | 1 | `$40` | 0 |

The final row has a latched earlier IRQ, not a new assertion at 33,255. These are separate fresh schedules, not repeated reads in one run. Immediate `$4017=$40` writes at 33,251–33,254 clear the current IRQ and return status `$00` in this runtime.

In the exact timed-read probe, completing a four-cycle instruction with `advance(4)` brings both clocks to **33,253**, where the sequencer reasserts IRQ and `irq_pending()` becomes **true** again. Thus the failing immediate snapshot alone does not demonstrate a lost interrupt at the instruction boundary. It also does not certify that the current runtime handles every status/IRQ race correctly.

## Independent references and uncertainty

### NESdev hardware documentation

The indexed [APU status reference](https://www.nesdev.org/wiki/APU_Status) describes the read-to-clear rule and an exception for simultaneous assertion. It describes both RP2A03 and RP2A07. Direct wiki retrieval returned HTTP 403; the indexed text was available and was consulted. This supports a collision investigation, but does not fix the sub-cycle location of this runtime callback or prove the particular immediate CPU IRQ-line assertion in A02.

The original [PAL timing measurements](https://forums.nesdev.org/viewtopic.php?t=13604) support a 33,252-CPU-clock landmark. They do not supply the phase-tagged post-read flag/IRQ trace needed here. No NTSC measurement is treated as PAL proof.

### Pinned Mesen2 source

Inspected and saved source from **`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`**; no moving-master assumption is used for this diagnosis. Mesen2 was not built or executed.

- [ApuFrameCounter.h](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/APU/ApuFrameCounter.h) supplies PAL assertion positions 33,252, 33,253 and 33,254. Its private `_irqFlag` and `_irqFlagClearClock` preserve the legible flag independently; `GetIrqFlag()` schedules/checks acknowledgement using master-clock phase. This is not the CPU IRQ source itself.
- [NesApu.cpp](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/APU/NesApu.cpp) runs pending APU activity before a read, obtains bit 6 through `GetIrqFlag()`, then **unconditionally calls `ClearIrqSource(FrameCounter)`** for `$4015`. The status response and CPU line can therefore differ immediately after a read. A returned `$40` does not by itself require an asserted post-read CPU source.
- [NesCpu.cpp](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/NesCpu.cpp) surrounds memory reads with `StartCpuCycle`/`EndCpuCycle`; APU advancement precedes the read, and IRQ polling is tracked separately at cycle end. The current runtime's integer callback does not identify those same master-clock positions.

The [2025-06-11 change 62122d5](https://github.com/SourMesen/Mesen2/commit/62122d515abfb7933c814e78b8605ba33139cbc9), titled `NES: Improved behavior for 4015.6 bit reads`, explicitly separates the readable flag from the previously shared CPU IRQ-source observation. This reinforces the distinction; it is not evidence that simply keeping our shared boolean true is correct.

The associated [AccuracyCoin source project](https://github.com/100thCoin/AccuracyCoin) describes get/put-phase acknowledgement tests but identifies its hardware target as **NTSC RP2A03G**. It was neither downloaded as a ROM nor executed, and cannot be promoted to an independent PAL hardware trace.

### Decision

The PAL event schedule is confirmed by source; the required **immediate post-read `IRQ=1`** is not. The current A02 fixture conflates the readable status latch, CPU IRQ source and integer-cycle collision convention. Mesen's source provides a concrete competing ordering, not confirmation of the assertion.

This does **not** establish that our current immediate clearing of both observables is hardware-correct, nor does it establish that A02 is harmless. It establishes that the proposed same-cycle retention fix lacks the required reference justification. Per the requested stop condition, **no production correction or test-expectation change is made**. The failure stays visible.

## Actual tests executed

Host: macOS ARM64; existing Release desktop build using this unchanged checkout.

| Invocation | Executed | Passed | Failed | Skipped |
|---|---:|---:|---:|---:|
| A02 reproducer | 1 | 0 | 1 (A02) | 0 |
| APU/SDL selection | 6 | 5 | 1 (A02) | 0 |

The first row is a preliminary reproduction; it is not added to the six-entry APU-suite count. JUnit confirms all six selected names ran: core APU, A01, A03, A03 boundary schedules and SDL pass; A02 fails with the original cycle/status/IRQ output. Both CTest invocations exit **8**.

The separate external probe prints five read schedules, four inhibit observations and one timed-read/instruction-completion trace. These are software observations, not independent hardware pass cases and not additions to CTest. A02 is unchanged before/after diagnosis; there is no fixed-after result.

Full desktop/headless CTest, CPU/bus/PPUSTATUS/DMA batteries and remote CI were **not rerun in this task**, because work stopped before implementation and those sources/configurations remain unchanged. Prior results are not relabelled as newly executed. No new compiled-production verification is claimed.

Reproduction:

```sh
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^apu_status_irq_collision_reference$' --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^apu_' --output-on-failure --verbose --no-tests=error --output-junit /tmp/nes-a02-diagnosis.xml
git diff --check
```

## Minimal next investigation (not implemented)

Obtain a PAL reference trace with separate observables for `$4015.6`, frame IRQ source, CPU sampling, master-clock phase and instruction completion, around all three assertions and after `$4017` inhibition. Establish the mapping to the existing offset ABI before selecting expected values.

If that trace confirms a distinction, a bounded design could separate the status latch and IRQ-source state within `AudioState`, preserving the service ABI and current scheduler. Acknowledgement and inhibition would need explicit tests for both observables. A same-cycle guard on the shared boolean is unsafe without that contract. Modules affected by such a future design: `audio_state.hpp/.cpp`, APU/bus regression tests and their CTest integration. No scheduler rewrite or new interpreter is proposed.

**Current outcome: A02 blocked at reference validation; work preserved; nothing published in this task.**
