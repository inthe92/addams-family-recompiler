# A01 — preserve external open bus on APU status reads

Date: **2026-10-10**. **A01 corrected and locally verified. A02/A03 remain unfixed, enabled and failing.** No commits, pushes, branch changes or updates to main.

## 1. Checkout and preserved work

Repository: `/Users/sebas/Documents/Codex/2026-10-03/referenced-chatgpt-conversation-this-is-an/outputs/public-alpha-0.2.0/addams-family-recompiler`.

Branch **`verify/phase-3-1-bus-timing`**, HEAD **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**, application **0.2.1**, ABI **2**. Initial status includes the existing uncommitted timing/headless/gameplay/APU work. Starting hashes match all **108 files** recorded at the end of the APU audit; no newer local changes were overwritten. Existing build caches point to this checkout.

`APU_AUDIO_AUDIT.md` was read completely and remains an unchanged historical snapshot. This report supersedes only its A01 status. No test source, expected value, recompiler, ABI, cache, PCM, graphics, gameplay or widescreen implementation changed.

## 2. Exact cause and technical correction

The old `$4015` branch assigned `audio_.read_status(open_bus_)` to `open_bus_`. With external bus `$A5` and disabled channels, `AudioState::read_status` correctly returns `$20`: only retained bit 5 is part of the CPU-visible status response. The assignment then incorrectly replaces the complete external latch with `$20`.

`$4015` is internal to the CPU and does not drive the external data bus, so **all eight bits of the external latch must survive**, not just bit 5. The CPU must still receive the status byte. This follows the independent [NESdev status-register contract](https://www.nesdev.org/wiki/APU_Status).

The correction is a direct return:

```cpp
else if (address == 0x4015) return audio_.read_status(open_bus_);
```

The status function is still called exactly once with the existing latch. Its channel/IRQ bits and existing read side effects are preserved. The timed overload still synchronizes devices before calling this function; access offsets, CPU/APU/PPU cycles and final instruction advancement are unchanged. Writes to `$4015` and reads/writes of every other register keep their existing paths. There is no ABI change or reason to invalidate native module caches.

## 3. Files and lines changed in this task

- **`runtime/src/bus.cpp:23`**: explanatory comment; **line 24**: replace the assignment with the direct return. This is the only production change.
- **`CMakeLists.txt:135`**: remove `known-divergence` from A01's labels after its unchanged reproducer passed. Its name, command, assertions and timeout remain the same. A02/A03 keep their existing labels and ordinary failing behavior.
- **`docs/APU_A01_FIX.md`**: this new report.

Of 108 starting files, **106 remain byte-identical**; only the two files above differ. Existing pre-task changes within them are preserved. No files were removed.

## 4. A01 before and after

Both available full builds were checked before modification, then the existing A01 CTest entry was executed in each configuration.

| Configuration | Before: status / subsequent open bus | Before result | After: status / subsequent open bus | After result |
|---|---|---|---|---|
| Desktop ON / mods ON | `$20` / `$20`, expected `$A5` externally | FAIL; CTest exit 8 | `$20` / `$A5` | PASS; focused CTest exit 0 |
| Desktop OFF / mods ON | `$20` / `$20`, expected `$A5` externally | FAIL; CTest exit 8 | `$20` / `$A5` | PASS; focused CTest exit 0 |

The passing focused runs occurred **before** changing the CTest label. No test expectation or failure handling was changed to obtain the result.

## 5. Complete verification

Host: **macOS ARM64**, AppleClang 21, CMake 4.4.4, Release. Both complete builds succeeded. Final CTest runs were **unfiltered**, with verbose output and JUnit retained.

| Configuration | Registered / executed | Passed | Failed | Skipped | CTest exit |
|---|---:|---:|---:|---:|---:|
| Desktop ON / mods ON | 18 / 18 | **16** | **2** | 0 | 8 |
| Desktop OFF / mods ON | 16 / 16 | **14** | **2** | 0 | 8 |
| Total | **34 / 34** | **30** | **4** | **0** | — |

The audit's full results were 15/18 and 13/16 respectively. Compared with those retained results, **only `apu_status_open_bus_reference` changes from FAIL to PASS in each configuration**. Test inventories are identical, no previously passing test regresses, and no test was removed, disabled, skipped or marked WILL_FAIL.

The **17 APU core groups pass in each configuration**; the desktop dummy SDL audio queue test also passes. The final runs include all existing CPU, stack, bus/device, PPUSTATUS, ABI/importer and gameplay-diagnostic entries, including **4,736 opcode executions per configuration**. No ROM/private module was used. Focused before/after runs are separate evidence and are not added to the 34-entry full-suite count.

## 6. A02 and A03 remain known divergences

- **`apu_status_irq_collision_reference` (A02): FAIL in both builds.** The exact cycle 33,252 read still returns `$40` and clears the newly asserted frame IRQ immediately. Expected preservation is not implemented.
- **`apu_frame_mode_delay_reference` (A03): FAIL in both builds.** The cycle 33,251 `$4017` mode write still suppresses the old four-step IRQ at cycle 33,252 before the delayed reset.

Both remain registered, executable, labelled **`known-divergence`**, and ordinary failures with no skip/expected-failure exemption. Consequently the complete suite and unfiltered CI **remain red**. The four failures are these two unchanged discrepancies repeated across two configurations; they are not new regressions. No attempt was made to correct them.

## 7. Risks and limits

The intended behavioral difference is the value retained for subsequent open-bus-dependent reads after `$4015`. CPU-visible APU status and ordinary IRQ acknowledgement are preserved, including the already documented A02 race. Existing integration/device tests pass, but no full gameplay or original-audio comparison was performed.

The runtime's simplified external-bus/fetch model and other precision limits remain. This change does not add missing instruction fetches, model bus decay or establish hardware-exact DMA/IRQ races. Verification here is local macOS ARM64; Windows, Linux and macOS Intel execution, hardware audio and remote CI were not performed or claimed.

## 8. Reproduction and retained evidence

From the repository, using the actual verified build directories:

```sh
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^apu_status_open_bus_reference$' --output-on-failure --verbose
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-a01-desktop-junit.xml

cmake --build /private/tmp/nes-b10-headless-build --parallel 4
ctest --test-dir /private/tmp/nes-b10-headless-build -R '^apu_status_open_bus_reference$' --output-on-failure --verbose
ctest --test-dir /private/tmp/nes-b10-headless-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-a01-headless-junit.xml
```

For fresh external build directories, configure from this checkout with `-DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON` and the appropriate `-DNES_ENABLE_DESKTOP=ON` or `OFF`, then run the same build/test commands. Full CTest exit 8 is currently expected from A02/A03; the focused A01 entry must exit 0. `ctest --test-dir <build> -L known-divergence --verbose` now selects **only A02/A03**, and both fail.

Run isolated core groups with `<build>/apu_audio_checks core` and A01 with `<build>/apu_audio_checks status_open_bus`. The before result is preserved in logs and JUnit; the current source correctly cannot reproduce the old A01 failure without reintroducing the bug. No branch restoration was used.

Evidence outside public Git is retained at surrounding workspace **`outputs/apu-a01-fix-2026-10-10/`**: initial Git status/108 hashes, pre-change source copies, task-only diffs, build logs, focused before/after and full CTest output/JUnit, label/inventory checks, preservation results and public export audit. Source export audit and `git diff --check` pass. Work stops here; A02/A03 require separate authorization.
