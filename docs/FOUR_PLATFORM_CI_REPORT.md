# Four-platform CI publication review

Date: **2026-10-11**. **Publication is blocked by GitHub authentication. No new commit, push or workflow execution was performed. Local builds succeed; A02 remains the sole failing CTest entry in both configurations.** This is not a four-platform certification.

## 1. Repository, Git and preservation

Active checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` under the local project workspace. The historical directory name does not identify the current version: CMake declares **0.2.1**, module ABI **2**.

- Current/planned publication branch: **`verify/phase-3-1-bus-timing`**.
- Local HEAD: **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**.
- Origin fetch/push: **`https://github.com/inthe92/addams-family-recompiler.git`**.
- Remote HEAD/main confirmed by `git ls-remote`: **`1588f31e3c3375adfbb4f75266d44eff1fdb25e5`**. The work branch does not yet exist remotely. This matches the earlier audit, rather than an unexpected remote divergence; remote main is an ancestor of local HEAD.
- Starting changes: **13 modified tracked files and 30 untracked files**, 43 pending paths. The index is empty. No unmerged index entries, merge/cherry-pick/revert/rebase/bisect state or index lock was found.
- **Published commit: none. Files included in a new public commit: none.** The existing importer-startup commit is one local commit ahead of remote main and would accompany future branch publication.

All **110 starting source files** match the saved final A03 hashes and remain byte-identical during this task. This report is the only repository addition. Existing tracked/untracked work is not discarded, staged, renamed or overwritten. No main change, tag, release, merge, force push or history rewrite occurs.

## 2. Prior reports and current APU status

Located and read: `TECHNICAL_BASELINE.md`, `TESTING_BASELINE.md`, `BUS_TIMING_REPORT.md`, `PLATFORM_VERIFICATION.md`, `HEADLESS_BUILD_REPORT.md`, `GAMEPLAY_VERIFICATION.md`, `APU_AUDIO_AUDIT.md`, `APU_A01_FIX.md`, `APU_A03_FIX.md`.

**Absent:** `INTERRUPT_DMA_TESTING.md` and `APU_A02_FIX.md`. Earlier documents remain historical snapshots; this task does not edit them.

| Issue | Current observed result on local ARM64 desktop/headless |
|---|---|
| A01 | PASS: disabled `$4015` returns `$20`, subsequent external open bus stays `$A5` |
| A03 | PASS: delayed mode preserves old IRQ; original reference and eight boundary schedules pass |
| A02 | **FAIL:** `$4015` at cycle 33,252 returns `$40`, IRQ after read is 0 instead of expected 1 |

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

## 4. Content review and proposed commit contents

A clean candidate export was assembled from Git's tracked/untracked nonignored source inventory, excluding `.git`. It contains **110 source/dependency files**, with **43 pending paths**. The public export audit passes, including pinned dependency archive checksums and exclusion rules. No candidate ROM, extracted resource file, build cache, executable, log/session file, private-key marker, recognizable GitHub/OpenAI/AWS credential or credential-bearing HTTP URL was detected. No images/audio were pending. The compatible profile contains identifiers/instruction addresses, not ROM bytes. Synthetic fixtures are generated only in external build directories.

This is a bounded source/pattern review, not an exhaustive legal or secret-detection guarantee. The optional private-ROM byte-marker scan was not executed. Existing audit documents contain machine/build-path metadata and bounded diagnostic excerpts, but no raw private session/import logs are included. The evidence directory remains outside the public checkout. **No user file was deleted to clean the export.**

Proposed files, **not staged or committed**, are the following reviewed existing work plus this report:

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

The existing `.github/workflows/build.yml` defines three jobs producing **four native host configurations**, consistent with the current official [runner image labels](https://github.com/actions/runner-images). Each enables testing/desktop/mods, builds all default targets, checks inventory, executes unfiltered verbose CTest with JUnit, and uploads test evidence under `if: always()`. macOS matrix fail-fast is false. No filter or continue-on-error hides failures.

| Platform | Reviewed runner/configuration | Remote configure / compile / link | Remote CTest executed / passed / failed / skipped |
|---|---|---|---|
| macOS ARM64 | `macos-15`, explicit `arm64` | **Not run** | **0 / 0 / 0 / 0 — not launched** |
| macOS Intel | `macos-15-intel`, explicit `x86_64` | **Not run** | **0 / 0 / 0 / 0 — not launched** |
| Windows x64 | `windows-latest`, MSYS2 UCRT64 GCC/Ninja | **Not run** | **0 / 0 / 0 / 0 — not launched** |
| Linux x64 | `ubuntu-22.04`, GCC/Ninja | **Not run** | **0 / 0 / 0 / 0 — not launched** |

These zeros are **absence of execution**, not passing or skipped tests. No run URL exists for this unpublished work. [Repository Actions](https://github.com/inthe92/addams-family-recompiler/actions) is the destination to inspect after publication, not evidence of a current run. Historical successful CI for old main, older Rosetta tests and cross-builds are not substituted for this worktree's four-platform execution.

All four hosts should register the current **19 desktop entries**. A02 is expected to fail but must be observed independently on each. Because the build step stops on failing CTest, its subsequent JUnit gate and importer-toolchain/packaging steps can remain unexecuted; evidence upload is still unconditional. This prevents falsely certifying later importer/package checks. No release/tag publication exists in the workflow; no workflow changes were made here.

## 6. Authentication block and manual intervention

The restricted shell's initial remote read failed DNS; the permitted network-enabled retry successfully confirmed origin/main and the absent branch. The nonmutating authentication check was then:

```sh
GIT_TERMINAL_PROMPT=0 git push --dry-run origin HEAD:refs/heads/verify/phase-3-1-bus-timing
```

It exited **128**:

```text
fatal: could not read Username for 'https://github.com': terminal prompts disabled
```

GitHub CLI is not installed; Homebrew is available. Per the user's explicit authentication stop condition, **no commit or actual push follows**. No credential extraction, alternate identity or workaround was attempted.

In the user's own terminal, install the CLI and complete interactive browser authentication for the account that can push to this repository:

```sh
brew install gh
gh auth login --hostname github.com --git-protocol https --web
gh auth setup-git --hostname github.com
gh auth status --hostname github.com
```

The [official login](https://cli.github.com/manual/gh_auth_login) and [Git credential setup](https://cli.github.com/manual/gh_auth_setup-git) document these operations. This intervention was **not executed by the agent**. Successful authentication does not grant missing repository/workflow permissions; any subsequent permission rejection must be reported, not bypassed.

After the user confirms authentication, recheck source hashes/status and remote refs, stage only the explicit reviewed paths, create a descriptive commit, and push only `verify/phase-3-1-bus-timing` without force. Record its exact SHA, then inspect the push-triggered run and all four job logs/JUnit artifacts. Confirm real configure/compile/link and all test names/counts, retain A02 failures, and document new failures before considering any fix. No main merge, release or tag is needed.

## 7. Pending risks for 1.0 and next step

1. **Publication/CI remains incomplete:** authenticate, publish the reviewed branch and obtain actual four-host evidence. Local ARM64 success of individual contracts cannot certify Windows/Linux/Intel execution.
2. **A02 remains reproducible:** PAL frame-IRQ assertion/read-clear ordering needs a separately authorized timing design and correction. Full CI should remain red until it is resolved without exemptions.
3. **Hardware fidelity gaps remain:** omitted fetch/dummy accesses, IRQ/DMA races, PPU evaluation/register details, adjacent APU clock collisions and independently measured audio fidelity.
4. **Game/distribution acceptance remains incomplete:** unaided complete-route evidence, widescreen behavior, real-user compatible-ROM imports, cache concurrency and platform-specific packaged startup. Existing limited synthetic/private smoke evidence is not a 1.0 acceptance result.

Evidence retained outside Git: surrounding workspace `outputs/four-platform-ci-2026-10-11/`, including starting hashes/status, clean export/content audit, reviewed-file manifest, local build/CTest/JUnit inventories/results and authentication-block record. **Work stops here pending manual authentication; no new bug is corrected.**
