# A03 — delayed PAL frame-counter mode application

Date: **2026-10-11**. **A03 corrected and locally verified in desktop and headless builds. A01 still passes. A02 remains an enabled, ordinary failing reference test.** No commits, pushes or branch changes.

## 1. Checkout and preservation

Repository: `/Users/sebas/Documents/Codex/2026-10-03/referenced-chatgpt-conversation-this-is-an/outputs/public-alpha-0.2.0/addams-family-recompiler`.

Branch **`verify/phase-3-1-bus-timing`**, HEAD **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**, application **0.2.1**, module ABI **2**. Both build caches point to this source directory. The existing uncommitted timing, headless, gameplay diagnostics, APU tests and A01 changes were preserved. `APU_AUDIO_AUDIT.md` and `APU_A01_FIX.md` were read completely and remain unchanged historical records.

Starting hashes cover **109 public source files**. **105 remain byte-identical**; four existing files changed and this report was added. No original files were deleted. Evidence, original worktree copies, task-only diffs, commands, logs, CTest inventories and JUnit results are outside the public checkout, in the surrounding workspace's `outputs/apu-a03-fix-2026-10-11/`.

## 2. Cause and independent PAL reference

Previously, `AudioState::write_register($4017, value)` immediately assigned the entire value to `frame_control_`. The existing parity-based `frame_reset_delay_` delayed the counter reset, but **not the mode bit**. Consequently the old four-step sequence stopped asserting IRQ as soon as bit 7 was written, before its delayed reset. The reverse transition could likewise lose the old five-step final half-frame clock.

The independent [Mesen ApuFrameCounter implementation](https://github.com/SourMesen/Mesen2/blob/master/Core/NES/APU/ApuFrameCounter.h) was inspected on this date, including its explicit PAL tables and `WriteRam`/`Run` separation. It retains the active mode until the pending write expires, while IRQ inhibition applies at the write. Its CPU-cycle convention gives three cycles for even writes and four for odd writes. PAL four-step IRQ landmarks are 33,252–33,254; the five-step final half-frame is 41,565 from reset. Mesen was **not built or executed**; this is a source-based independent reference, not a hardware trace. The master link may change.

The [NESdev frame-counter reference](https://www.nesdev.org/wiki/APU_Frame_Counter) is the associated hardware documentation; direct retrieval returned HTTP 403 here, so it is not claimed as separately fetched evidence. No NTSC schedule was substituted for PAL.

## 3. Minimal correction and contract

The existing sequencer and event positions are retained. One private byte, `frame_pending_mode_`, stores the requested bit 7:

- **At the write:** retain the active mode; update bit 6 immediately; clear the existing frame IRQ immediately when inhibition is enabled; save the pending mode; restart the existing three/four-cycle delay.
- **Before expiry:** the active sequence continues, including its existing events and wrap. Disabling inhibition is also immediate.
- **At expiry:** apply the pending mode, reset `frame_cycle_`, and use the existing quarter/half clock behavior for a five-step reset. A four-step reset does not force these clocks. Changing mode alone does not clear an already latched IRQ.
- Repeated writes continue to replace the pending request and restart the existing delay; an additional queue or scheduler was not introduced.

No extra CPU/APU advancement is added. Timed bus writes still reach the same cycle. The production `read_status` function, A02 test and A01 bus fix remain byte-identical. No recompiler, renderer, gameplay, PCM mixer/filter, DMA, module ABI or cache logic changes. The new field is internal C++ runtime state, not part of the generated module's service ABI; the full builds recompile its dependent runtime targets.

## 4. Exact files changed

| File and current lines | Change |
|---|---|
| `runtime/include/runtime/audio_state.hpp:64` | Add the private pending-mode byte |
| `runtime/src/audio_state.cpp:88–93` | Separate immediate inhibition from delayed mode |
| `runtime/src/audio_state.cpp:121–124` | Apply pending mode at existing reset expiry |
| `tests/apu_audio_checks.cpp:333–386,396` | Add/register `frame_mode_edges`; existing reference assertions unchanged |
| `CMakeLists.txt:136–138` | Remove resolved A03's known-divergence label and register the boundary group |
| `docs/APU_A03_FIX.md` | This report |

## 5. Before and after

Before any production change, full builds completed and `apu_frame_mode_delay_reference` **failed in both configurations**, CTest exit **8**:

```text
$4017 write=33251; observe=33252 IRQ=0; old four-step IRQ expected=1 before delayed mode reset
```

The new boundary group was also compiled and executed against the unchanged production code in headless mode: **FAIL**, executable exit **1**, at the old four-step IRQ assertion. It therefore detects the original error without changing expected values.

After the production fix, the unchanged original reference **passed in both configurations**, CTest exit **0**, before changing its classification:

```text
$4017 write=33251; observe=33252 IRQ=1; old four-step IRQ expected=1 before delayed mode reset
```

The boundary group passes **eight synthetic schedules per configuration**: each write parity exercises four-to-five transition around the old IRQ/half/wrap, five-to-four transition around the old final half, immediate inhibition of an already asserted IRQ, and immediate inhibition release before the next old-sequence IRQ. It checks pre-reset state, exact reset clocks, length counters, retained IRQ and the restarted mode's subsequent schedule. Even/odd forward resets are **33,253/33,255**; reverse resets are **41,569/41,571**. Expectations are independently declared landmarks, not runtime-derived answers. Acknowledgement in the forward schedule occurs after reset, away from A02's collision.

## 6. Final executed results

Host **macOS ARM64**, Darwin 27, AppleClang **21.0.0**, CMake **4.4.4**, Release, testing ON, mods ON. Both complete default builds succeeded. Counts below are the final **unfiltered** CTest runs, excluding preliminary/focused repeats:

| Configuration | Executed | Passed | Failed | Skipped | CTest exit |
|---|---:|---:|---:|---:|---:|
| Desktop ON | 19 | 18 | 1 | 0 | 8 |
| Desktop OFF | 17 | 16 | 1 | 0 | 8 |
| Total | **36** | **34** | **2** | **0** | — |

The sole failure in each is **`apu_status_irq_collision_reference` (A02)**: cycle **33,252**, returned status **$40**, IRQ after read **0**, expected **1**. Its test body and production status-read body are unchanged. It retains `known-divergence`, has no `WILL_FAIL`, disable or skip exemption, and continues to make the complete suite red. A01 returns status **$20**, preserves external **$A5**, and passes twice. A03 and the new boundary group are ordinary mandatory passing tests.

The separately executed `ctest -L apu` runs contain **five entries each: four pass, A02 fails**, exit 8. The 17 existing APU core groups pass in each configuration, including PAL channel timing, DMC/IRQ integration, generated-C register access and PCM determinism. Desktop dummy SDL audio also passes.

Full-suite evidence additionally confirms **4,736 native opcode executions per configuration**, two stack programs each, both PPUSTATUS regressions each, 14 timed bus-side-effect cases each, RAM/MMC1/bus/PPU contracts, ABI/cache/importer and gameplay diagnostics. Comparison with the saved final A01 JUnit reports confirms: **no original test removed, only A03 changed from failing to passing, one passing boundary entry added, zero new regressions**. Inventories match JUnit execution sets and contain no disabled/skip/expected-failure exemptions.

## 7. Risks and limits

The intended change can alter channel clocks and thus sound only where a driver changes mode during a pending reset; no original-game soundtrack or whole-game fidelity claim is made. No commercial ROM/resources were needed.

Existing precision limits remain: A02's assertion/read-clear collision, half-cycle hardware phase, and suppression/ordering of coincident or adjacent sequencer/reset clocks beyond these eight schedules. The existing reset-before-event precedence was not redesigned; the added cases do not certify every possible quarter/half/reset collision. Rapid replacement of pending writes is retained but not exhaustively tested. These limits do not invalidate the reproduced delayed-mode correction and are not reclassified as passing hardware-fidelity tests.

No Windows, Linux, macOS Intel, physical PAL console or remote CI execution was performed. The historical exhaustive ADC/SBC sweep was not repeated. The existing CI JUnit gate still rejects A02; no workflow or distribution changes were made to hide it.

## 8. Reproduction

From this checkout, using the verified existing external build directories:

```sh
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^apu_frame_mode_(delay_reference|edges_contract)$' --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-technical-baseline-build -L apu --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-a03-desktop.xml

cmake --build /private/tmp/nes-b10-headless-build --parallel 4
ctest --test-dir /private/tmp/nes-b10-headless-build -R '^apu_frame_mode_(delay_reference|edges_contract)$' --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-b10-headless-build -L apu --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-b10-headless-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-a03-headless.xml
```

Focused A03 commands should exit 0; APU/full runs should exit 8 solely for A02. For a fresh build, configure with Ninja, `-DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON` and the corresponding `-DNES_ENABLE_DESKTOP=ON` or `OFF`, then run the same commands with that build path. **Work stops at A03; A02 is not corrected.**
