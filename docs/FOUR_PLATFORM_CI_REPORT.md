# Four-platform CI publication review

Date: **2026-10-11**. **The work branch is published. The four-host workflow completed with conclusion FAILURE:** macOS ARM64, macOS Intel and Linux each pass 18/19 CTest entries; A02 is their sole failing test. Windows fails linking the SDL queue test before CTest. This is a partial verification, not a four-platform certification.

## 1. Repository, Git and preservation

Active checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` under the local project workspace. The historical directory name does not identify the current version: CMake declares **0.2.1**, module ABI **2**.

- Published work branch: **`verify/phase-3-1-bus-timing`**.
- Starting HEAD: **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**.
- Published and CI-tested source commit: **[`7927a7f81e9ebeb199efcbbb9782d23e7672d455`](https://github.com/inthe92/addams-family-recompiler/commit/7927a7f81e9ebeb199efcbbb9782d23e7672d455)**, `Add ABI v2 bus timing verification and PAL APU regressions`.
- Origin fetch/push: **`https://github.com/inthe92/addams-family-recompiler.git`**.
- Remote HEAD/main confirmed by `git ls-remote`: **`1588f31e3c3375adfbb4f75266d44eff1fdb25e5`**. Main was unchanged before publication. The work branch was absent remotely and was created by a normal, non-force push; remote main is an ancestor of the starting local HEAD.
- Starting changes: **13 modified tracked files and 30 untracked files**, 43 pending paths. The starting index was empty. No unmerged index entries, merge/cherry-pick/revert/rebase/bisect state or index lock was found.
- **44 reviewed paths were committed and published**, including this report. The existing importer-startup commit remains in the branch ancestry. A subsequent documentation-only commit updates this report with actual CI evidence; its `[skip ci]` marker avoids repeating unchanged-source builds and does not change or exempt any test. The source run remains failed.

All **110 starting source files** match the saved final A03 hashes and remain byte-identical during this task. This report is the only repository addition and the only file edited for the final results update. Existing tracked/untracked work was preserved, reviewed and explicitly staged; no preexisting content was discarded, renamed or overwritten. No main change, tag, release, merge, force push or history rewrite occurs.

## 2. Prior reports and current APU status

Located and read: `TECHNICAL_BASELINE.md`, `TESTING_BASELINE.md`, `BUS_TIMING_REPORT.md`, `PLATFORM_VERIFICATION.md`, `HEADLESS_BUILD_REPORT.md`, `GAMEPLAY_VERIFICATION.md`, `APU_AUDIO_AUDIT.md`, `APU_A01_FIX.md`, `APU_A03_FIX.md`.

**Absent:** `INTERRUPT_DMA_TESTING.md` and `APU_A02_FIX.md`. Earlier documents remain historical snapshots; this task does not edit them.

| Issue | Observed locally (desktop/headless) and on both native macOS CI hosts and Linux |
|---|---|
| A01 | PASS: disabled `$4015` returns `$20`, subsequent external open bus stays `$A5` |
| A03 | PASS: delayed mode preserves old IRQ; original reference and eight boundary schedules pass |
| A02 | **FAIL:** `$4015` at cycle 33,252 returns `$40`, IRQ after read is 0 instead of expected 1 |

Windows did not execute these tests; no Windows A01/A03 pass or A02 reproduction is claimed.

A02 remains enabled, labelled `known-divergence`, with original assertions. No `WILL_FAIL`, disabled property, CTest filter or skip exemption is introduced. A02 is not corrected here and prevents a green complete workflow.

## 3. Local verification performed

Host: **macOS ARM64**, Darwin 27, AppleClang 21, CMake 4.4.4, Release, mods ON. Full default builds were invoked in both verified external build directories; Ninja reported no work because source/configuration match the already linked A03 state. **CTest was executed again, unfiltered**, with verbose output, nonempty-suite enforcement and JUnit.

| Configuration | Build | Executed entries | Passed | Failed | Skipped | CTest exit |
|---|---|---:|---:|---:|---:|---:|
| Desktop ON | PASS | 19 | 18 | 1 | 0 | 8 |
| Desktop OFF | PASS | 17 | 16 | 1 | 0 | 8 |
| Total | — | **36** | **34** | **2** | **0** | — |

Both failures are A02. No new regression relative to A03 is observed. Inventories match all executed JUnit names and contain no disabled/expected-failure/skip exemptions. The desktop inventory gate passes: 19 registered tests, all 13 explicitly required baseline entries present, executable and mandatory. It permits additional entries; direct inventory/JUnit review also confirms all current APU entries ran.

Coverage observed per configuration: **4,736 generated-C native opcode executions**, two stack programs, both `$2002` regressions, 14 timed bus-side-effect cases, RAM/MMC1/bus/PPU contracts, 17 APU core groups, A01/A02/A03, eight A03 boundary schedules and gameplay diagnostics. Desktop additionally exercises three real synthetic ABI modules and the dummy SDL audio queue. ABI-v2 loader rejection/acceptance and four importer cache decisions pass; headless retains the cache tests but intentionally has no SDL loader test.

DMC ordinary boundary DMA/parity, APU IRQ entry and generated CLI/SEI/PLP/RTI behavior are covered by existing APU groups. This does **not** certify full OAM/DMC overlap, cancellation, interrupt hijacking or half-cycle precision; there is no separate comprehensive DMA report. The unchanged historical exhaustive ADC/SBC sweep and private-game runs were not repeated. No commercial ROM was required.

Reproduction from this checkout:

```sh
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-four-platform-local-desktop.xml
cmake --build /private/tmp/nes-b10-headless-build --parallel 4
ctest --test-dir /private/tmp/nes-b10-headless-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-four-platform-local-headless.xml
git diff --check
```

Complete CTest currently exits 8 solely for A02; this must not be described as full-suite success.

## 4. Content review and published commit contents

A clean candidate export was assembled from Git's tracked/untracked nonignored source inventory, excluding `.git`. It contains **110 source/dependency files**, with **43 pending paths**. The public export audit passes, including pinned dependency archive checksums and exclusion rules. No candidate ROM, extracted resource file, build cache, executable, log/session file, private-key marker, recognizable GitHub/OpenAI/AWS credential or credential-bearing HTTP URL was detected. No images/audio were pending. The compatible profile contains identifiers/instruction addresses, not ROM bytes. Synthetic fixtures are generated only in external build directories.

This is a bounded source/pattern review, not an exhaustive legal or secret-detection guarantee. The optional private-ROM byte-marker scan was not executed. Existing audit documents contain machine/build-path metadata and bounded diagnostic excerpts, but no raw private session/import logs are included. The evidence directory remains outside the public checkout. **No user file was deleted to clean the export.**

The following **44 paths were included in source commit `7927a7f`**, covering the reviewed existing work plus the initial report:

```text
.github/workflows/build.yml
CMakeLists.txt
app/desktop.cpp
docs/APU_A01_FIX.md
docs/APU_A03_FIX.md
docs/APU_AUDIO_AUDIT.md
docs/BUS_TIMING_REPORT.md
docs/GAMEPLAY_VERIFICATION.md
docs/HEADLESS_BUILD_REPORT.md
docs/PLATFORM_VERIFICATION.md
docs/TECHNICAL_BASELINE.md
docs/TESTING_BASELINE.md
game/include/game/gameplay_diagnostic.hpp
game/src/actor_view.cpp
game/src/gameplay_diagnostic.cpp
game/src/god_mode.cpp
include/recomp/module_abi.h
include/recomp/runtime_services.h
runtime/include/runtime/audio_state.hpp
runtime/include/runtime/bus.hpp
runtime/include/runtime/module_abi.hpp
runtime/src/audio_state.cpp
runtime/src/bus.cpp
runtime/src/user_rom.cpp
tests/apu_audio_checks.cpp
tests/apu_sdl_queue_checks.cpp
tests/bus_timing_checks.cpp
tests/device_checks.cpp
tests/emit_opcode_fixtures.py
tests/gameplay_diagnostic_checks.cpp
tests/gameplay_diagnostic_checks.py
tests/import_cache_checks.py
tests/known_divergence_checks.py
tests/module_abi_checks.cpp
tests/module_abi_fixture.c
tests/native_program_checks.cpp
tests/opcode_codegen_checks.cpp
tests/ppustatus_timing_check.cpp
tests/run_known_divergence.py
tests/verify_ctest_suite.py
tools/recompiler/emit_blocks.py
tools/recompiler/import_user_rom.py
tools/verification/review_gameplay_log.py
docs/FOUR_PLATFORM_CI_REPORT.md
```

`git diff --check` passes. Production and workflow changes have the documented earlier-phase provenance; this task adds no production/configuration modifications.

## 5. Four-host workflow review and actual remote results

The existing `.github/workflows/build.yml` defines three jobs producing **four native host configurations**, consistent with the official [runner image labels](https://github.com/actions/runner-images). Each enables testing/desktop/mods, builds all default targets, checks inventory, and invokes unfiltered verbose CTest. macOS matrix fail-fast is false. No filter or continue-on-error hides failures. No workflow or test changes were made during publication.

Run **[38125126820 — Build content-free frontends](https://github.com/inthe92/addams-family-recompiler/actions/runs/38125126820)**, created `2026-10-11T08:31:53Z`, completed with **FAILURE**. Its head SHA is exactly the published source commit `7927a7f81e9ebeb199efcbbb9782d23e7672d455`.

| Platform / native runner | Compiler | Configure / full build / executable link | CTest executed / passed / failed / skipped | Evidence |
|---|---|---|---|---|
| macOS ARM64 / `macos-15`, `arm64` | AppleClang 17.0.0.17000013 | PASS / PASS / PASS | **19 / 18 / 1 / 0**, A02 | [Job](https://github.com/inthe92/addams-family-recompiler/actions/runs/38125126820/job/114427787625) |
| macOS Intel / `macos-15-intel`, `x86_64` | AppleClang 17.0.0.17000013 | PASS / PASS / PASS | **19 / 18 / 1 / 0**, A02 | [Job](https://github.com/inthe92/addams-family-recompiler/actions/runs/38125126820/job/114427787462) |
| Linux x64 / `ubuntu-22.04` | GNU 11.4.0 | PASS / PASS / PASS | **19 / 18 / 1 / 0**, A02 | [Job](https://github.com/inthe92/addams-family-recompiler/actions/runs/38125126820/job/114427787434) |
| Windows x64 / `windows-latest`, UCRT64 | GNU 16.2.0 | PASS / **FAIL** / main executable PASS, SDL queue test FAIL | **0 / 0 / 0 / 0**, **19 not executed** | [Job](https://github.com/inthe92/addams-family-recompiler/actions/runs/38125126820/job/114427787351) |

Remote totals: **57 executed CTest entries, 54 passed, 3 failed (A02), 0 skipped within CTest**. Windows' 19 intended entries are blocked before execution, not passed or CTest-skipped. All four jobs conclude failure. These are native runner executions; no cross-compilation or historical run is substituted.

On each of the three hosts that reached CTest, inventory contains all current 19 desktop entries and matches the local inventory. Console output and downloaded `LastTest.log` confirm all names executed; no disabled, expected-failure or skip property exempts mandatory tests. Coverage includes **4,736 generated-C native opcode executions per host (14,208 remote total)**, two stack programs per host, both `$2002` regressions, RAM/MMC1/bus/PPU tests, APU core tests, A01, A03 and eight A03 boundary schedules, synthetic ABI-v2 modules and importer-cache tests. A02 reproduces cycle 33,252, status `$40`, IRQ 0 instead of expected 1.

### Windows: newly reproduced link failure

The runtime executable `nes_native.exe` links successfully, but the default build fails at `apu_sdl_queue_checks.exe`:

```text
[358/360] Linking CXX executable apu_sdl_queue_checks.exe
libmingw32.a(...crtexewin.o): undefined reference to `WinMain'
collect2.exe: error: ld returned 1 exit status
ninja: build stopped: subcommand failed.
```

Code inspection supports an SDL entrypoint mismatch: `tests/apu_sdl_queue_checks.cpp` includes `<SDL.h>` before `int main()`, while `SDL_MAIN_HANDLED=1` is defined for `nes_native` but not this test target. SDL2's Windows header can rename `main` to `SDL_main`; the test target does not supply the SDL2main wrapper. This is the likely explanation, not a verified correction. **No fix is applied here**, as requested for new CI failures. CTest, its inventory and artifacts were never produced on Windows.

### Evidence upload: missing JUnit

Each successful-build host uploaded only `ctest-inventory.json` and `Testing/Temporary/LastTest.log`; **the expected JUnit XML is missing**. Counts above are supported by the complete console logs and these artifacts, not an invented XML result.

The workflow invokes `ctest --test-dir build --output-junit build/ctest-results.xml`, while upload expects repository-root `build/ctest-results.xml`. A separate deterministic probe using local CTest 4.4.4 reproduced resolution relative to the test directory: output actually lands at **`build/build/ctest-results.xml`**. This identifies a path mismatch; the remote nested XML was not uploaded and cannot be inspected. The probe used one trivial temporary test outside the repository; it is not counted as a project test. An absolute JUnit output path would avoid ambiguity, but **no workflow correction is applied here**.

The post-CTest JUnit gate, separate importer-toolchain/process checks and packaging steps were skipped at workflow level because the earlier step failed. This is distinct from zero skipped CTest entries. Importer/cache CTest entries ran on the three completed-build hosts, but later importer/package checks are **not certified**. No release or tag was created.

## 6. Authentication and safe publication

The initial attempt stopped at missing HTTPS authentication, without committing or pushing. The user subsequently completed GitHub CLI authentication and confirmed it. A permitted network-enabled check verified the account `inthe92`, origin refs and unchanged source hashes before publication. No credential extraction or workaround was used.

Only the 44 explicitly reviewed paths were staged. The source commit was created on the existing work branch and published with:

```sh
git push -u origin verify/phase-3-1-bus-timing
```

The push-triggered run was monitored to completion; job logs and available artifacts were collected. All 111 repository source/report files matched the reviewed publication hashes at source commit `7927a7f`. The final report is published separately with an explicit documentation-only commit; no source, workflow or test changes follow the CI run. No main checkout/merge/push, tag, release, force push or history rewrite was performed.

For independent inspection:

```sh
gh run view 38125126820 --repo inthe92/addams-family-recompiler
gh run view 38125126820 --repo inthe92/addams-family-recompiler --log-failed
gh run download 38125126820 --repo inthe92/addams-family-recompiler --dir /tmp/nes-ci-evidence
```

## 7. Pending risks for 1.0 and next step

1. **Windows full-build verification is blocked:** authorize a minimal SDL test-entrypoint correction, then rerun native Windows build and all 19 entries. Do not infer Windows CPU/APU correctness from Linux/macOS.
2. **JUnit evidence path needs a minimal workflow correction:** authorize this separately from production changes; preserve complete unfiltered tests and visible failures.
3. **A02 remains reproduced on both macOS architectures and Linux, and locally in both configurations:** PAL frame-IRQ assertion/read-clear ordering needs a separately authorized timing design and correction. Full CI remains red, without exemptions.
4. **Hardware fidelity gaps remain:** omitted fetch/dummy accesses, IRQ/DMA races, PPU evaluation/register details, adjacent APU clock collisions and independently measured audio fidelity.
5. **Game/distribution acceptance remains incomplete:** unaided complete-route evidence, widescreen behavior, real-user compatible-ROM imports, cache concurrency and platform-specific packaged startup. Later packaging steps did not execute; earlier limited synthetic/private evidence is not 1.0 acceptance.

Evidence retained outside Git: surrounding workspace `outputs/four-platform-ci-2026-10-11/`, including original/published source hashes, clean export/content audit, staged-file manifest, local build/CTest/JUnit results, full remote run/job metadata and console logs, downloaded inventories/LastTest logs, remote result summary and JUnit path probe. The historical authentication-block record is preserved as history, not the current publication status.

**Publication is complete; four-platform verification is partial. No new CI bug was corrected. Work stops after publishing this report, pending instructions.**
