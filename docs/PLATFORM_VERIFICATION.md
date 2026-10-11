# Phase 3.1 — Platform verification

Date: **2026-10-10**. This report distinguishes executed tests from cross-compilation and remote CI. Read with the preserved [technical audit](TECHNICAL_BASELINE.md), [Phase 2 testing snapshot](TESTING_BASELINE.md) and [bus timing contract](BUS_TIMING_REPORT.md).

## Source and scope

Starting and ending HEAD: **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**. Application version: **0.2.1**. Native module ABI: **2**.

The checkout initially contained the uncommitted Phase 2/3 implementation, tests and reports. Their starting hashes/status were saved outside Git. A new local branch, **`verify/phase-3-1-bus-timing`**, was created at that HEAD, carrying all existing changes. Creating the branch did not commit them or publish a new revision.

This phase changes only `.github/workflows/build.yml`, adds `tests/verify_ctest_suite.py`, and adds this report. Production sources, CMake registrations, ABI, emitter, game behavior, devices, renderer, widescreen and distribution scripts remain byte-for-byte as they were at the start of Phase 3.1. There were no commits, pushes, workflow dispatches or releases. Tests use synthetic fixtures; no ROM was used during this phase.

## Platform results

Local host: **macOS 27.0.1 ARM64**, AppleClang **21.0.0**, CMake **4.4.4**, system Python **3.9.6**. All configurations use Release, `BUILD_TESTING=ON`, `NES_ENABLE_DESKTOP=ON`, `NES_ENABLE_MODS=ON`, C99 generated fixtures and C++17 drivers.

| Platform | Compilation performed during Phase 3.1 | Tests executed | Passed / failed | Pending verification |
|---|---|---|---|---|
| macOS ARM64 | Full default target, explicit `CMAKE_OSX_ARCHITECTURES=arm64`; Mach-O ARM64 confirmed | Complete CTest + inventory/JUnit gate; bundled compiler smoke; frontend importer process checks | **12/12 CTest**, 0 failed/disabled/skipped; additional checks passed | Native GitHub macOS 15 runner; this local host has a newer macOS |
| macOS Intel | Full default target, explicit `CMAKE_OSX_ARCHITECTURES=x86_64`; Mach-O x86_64 confirmed | Complete CTest executed through Rosetta; inventory/JUnit gate; Intel frontend importer process checks | **12/12 CTest**, 0 failed/disabled/skipped; process checks passed | Physical Intel CI host and its bundled x86_64 Python/Zig package integration |
| Windows x64 | **Full CMake cross-build and link**, LLVM-MinGW Clang **23.1.3** / UCRT; frontend, all native tests, import probe and ABI DLLs; PE32+ x86-64 confirmed | CTest JSON inventory/executable-presence gate only; DLL export inspected | Build passed; **12 registered entries**, all executable files present; **0 runtime CTest entries executed** | Full suite, module loading and importer/package-tool checks on real Windows x64 |
| Linux x64 | **Full CMake cross-build and link**, Zig **0.17.0** / Clang **22.1.8**, target `x86_64-linux-gnu.2.31`, previously available Linux SDK; ELF x86-64 confirmed | CTest JSON inventory/executable-presence gate only; shared-library symbol inspected | Build passed; **12 registered entries**, all executable files present; **0 runtime CTest entries executed** | Full suite, module loading and importer/package-tool checks on real Linux x64 |

Cross-compilation does **not** demonstrate execution, loader behavior or device correctness on Windows/Linux. There is no configured Windows/Linux x64 test execution route in this task: no Docker/Podman, Wine, Lima or QEMU command was available. VirtualBox is installed, but no provisioned guest test channel was configured or used. No VM was started or modified.

The local Intel frontend process check used the existing ARM64 bundled Python as a separate child process and synthetic invalid input. It verifies the x86_64 frontend's process/error handling under Rosetta, not the shipped Intel interpreter/module toolchain. Only the ARM64 bundled compiler smoke compiled and dynamically loaded its synthetic function in this phase.

All default build targets were built, including `nes_native`, `import_probe`, six native check executables, `native_test_fixtures`, the three ABI fixture modules, runtime/game/desktop libraries and their existing dependencies. Dependencies marked excluded by upstream SDL do not contain this project's tests; none of the required project targets was excluded. Inspection confirmed `nes_recomp_abi_version` is exported by both the Windows current-ABI DLL and the Linux current-ABI SO. Calling those exports on their target OS remains pending.

## Executed CTest coverage

The following results were independently observed in **both** macOS architecture runs. Counts are per complete suite, not multiplied by repeated development runs:

| Entry | Executed passing coverage per macOS architecture |
|---|---|
| `public_import_contract` | 3 Python cases |
| `diagnostic_harness_contract` | 5 Python cases; diagnostic errors propagate |
| `native_opcode_contract` | **4,736 native instruction executions**; all 151 official encodings, registers/flags/PC/SP, access order/offsets and total cycles |
| `native_stack_programs` | 2 complete JSR/PHA/PLA/RTS and BRK/RTI programs |
| `ppustatus_read_timing` | **Both** `$2002` regressions: read cycles 25,683 and 58,930; A=$80, P=$ED, vblank clearing and four-cycle totals; warm address-latch reset |
| `native_bus_side_effects` | 14 native timed register-access cases, including dummy reads, page crossings and RMW ordering |
| `import_cache_abi_contract` | 4 synthetic importer cache decisions |
| `module_abi_contract` | 3 dynamically loaded synthetic modules; missing/old ABI rejected, ABI v2 accepted |
| `synthetic_ram_contract` | 3 cases / 10,248 assertions |
| `synthetic_mmc1_contract` | 4 cases / 45 assertions |
| `synthetic_bus_contract` | 5 cases / 35 assertions |
| `synthetic_ppu_contract` | 5 cases / 20 assertions |

**12 passed entries per architecture; zero failures or skipped entries.** These contain 4,786 instruction/program/device/module/Python cases per architecture, using the same counting convention as Phase 3. The two macOS runs therefore execute the 4,736-case native matrix twice. Historical ADC/SBC helper sweeps and the previous private 1,800-frame game smoke were not repeated or counted in this phase.

Additional checks passed:

- Existing bundled ARM64 Python/Zig smoke: synthetic module compiled, signed and loaded; function returned 42.
- Existing frontend importer process checks against both ARM64 and x86_64 probes: spaced paths, early stdout/stderr capture and executable launch failure, using synthetic files only.
- YAML parsing and workflow structure check: expected three job definitions/four host configurations, explicit test enablement, inventory/JUnit verification and unconditional evidence upload present. This is local syntax/structure verification, not a GitHub workflow execution or an `actionlint` result.
- **Ten negative gate probes**: missing test, disabled test, `WILL_FAIL`, skip exemptions (including return code zero), missing executable, omitted JUnit entry, skipped JUnit entry, incorrect native count and failed JUnit entry were each rejected with exit 1. Only temporary report copies were modified; these are additional harness checks, not ten new CTest entries or unresolved production failures.

## Workflow review and minimal verification changes

The existing workflow already has four native hosts: `macos-15` (ARM64), `macos-15-intel` (x86_64), `windows-latest` (UCRT64 GCC) and `ubuntu-22.04` (x86_64 GCC). The macOS label/architecture mapping was checked against the official [runner image list](https://github.com/actions/runner-images) and [hosted-runner reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

All jobs build the complete default target and run unfiltered CTest. The two macOS matrix entries retain `fail-fast: false`; Windows/Linux are independent jobs. There is no `continue-on-error`, CTest filter, expected-failure classification or skip exemption for the required suite.

Potential verification gaps were configuration/evidence gaps, not reproduced portability defects:

1. Test enablement previously depended on CMake defaults. CI now explicitly sets `BUILD_TESTING`, desktop and mods ON. Both macOS architectures are explicitly selected in the matrix/configuration.
2. `--no-tests=error` rejects an empty suite but does not reject a partially registered or skipped suite. The new **stdlib-only** `verify_ctest_suite.py` validates JSON inventory before execution and JUnit afterward. It requires all 12 named entries, existing executable files, no disabled/expected-failure/skip properties, identical registered/executed name sets, passing `status=run` cases and the expected CPU/device/ABI/importer count diagnostics.
3. Each job now writes JUnit XML and uploads `ctest-inventory.json`, `ctest-results.xml` and `LastTest.log` under `if: always()`, including useful evidence when the build/test step fails. Missing artifact files only warn; they do not convert a failing build/test step into success.

A gate failure exits 1 and fails its job. It does not alter CPU results or manufacture expected device values. The guard has a deliberately explicit Phase 3 coverage list/counts; a future intentional suite change must update that contract. It allows extra tests but requires every registered entry to execute and pass.

Existing source audit, toolchain/process checks and content-free packaging steps were retained unchanged. No app functionality or runtime portability correction was required by any build/test executed here. Windows GNU GCC and Linux GNU GCC behavior remains unverified locally because the cross-builds use Clang; native CI must close that distinction.

## Reproduction

Run from this repository; all build/evidence paths below are outside Git. The report/gate commands use [CTest's JUnit support](https://cmake.org/cmake/help/v3.30/manual/ctest.1.html#cmdoption-ctest-output-junit), introduced in CTest 3.21; local CTest is 4.4.4.

```sh
cmake -S . -B /private/tmp/nes-technical-baseline-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build --show-only=json-v1 > /private/tmp/nes-phase31-arm64-inventory.json
python3 -B tests/verify_ctest_suite.py --inventory /private/tmp/nes-phase31-arm64-inventory.json
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-phase31-arm64-junit.xml
python3 -B tests/verify_ctest_suite.py --inventory /private/tmp/nes-phase31-arm64-inventory.json --junit /private/tmp/nes-phase31-arm64-junit.xml
```

For Intel, use `/private/tmp/nes-phase3-intel-build`, `-DCMAKE_OSX_ARCHITECTURES=x86_64` and distinct inventory/JUnit filenames. Its binaries execute through Rosetta on this host.

The full cross-builds used the already available local toolchains, unchanged:

```sh
cmake -S . -B /private/tmp/nes-phase31-windows-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_TOOLCHAIN_FILE=/private/tmp/nes-windows-toolchain.cmake
cmake --build /private/tmp/nes-phase31-windows-build --parallel 4
cmake -S . -B /private/tmp/nes-phase31-linux-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_TOOLCHAIN_FILE=/private/tmp/nes-linux-toolchain.cmake
cmake --build /private/tmp/nes-phase31-linux-build --parallel 4
```

CTest `--show-only=json-v1` plus the inventory gate was run on each cross-build. CTest execution was deliberately not attempted on those foreign binaries. Toolchain files/compiler/SDK paths are local prerequisites, not portable repository additions.

## GitHub Actions status and required external action

**No Phase 3.1 GitHub run was launched or verified.** The branch exists only locally and its changes are uncommitted, so remote Actions cannot test this worktree. No `gh` CLI was available. Public web/API fetches did not provide a current run listing; the local read-only API attempt failed DNS resolution in the restricted environment. The historical successful run recorded in the technical audit concerns older public code and is not Phase 3.1 evidence.

The remaining external verification is exact and bounded: run this complete branch revision on all four hosted runners, particularly Windows/Linux runtime execution and native Intel package-tool integration.

For the user, after reviewing/authorizing publication of the current content-free changes:

1. In the **public checkout** on `verify/phase-3-1-bus-timing`, review `git status --short` and `git diff`. Include the previously untracked Phase 2/3 files as well as this phase's files in the reviewed commit; publishing only the workflow would omit the test drivers/ABI implementation. The changes currently fit these public-source paths:

   ```sh
   git add -- .github/workflows/build.yml CMakeLists.txt docs include/recomp runtime game/src/actor_view.cpp game/src/god_mode.cpp tools/recompiler tests
   git diff --cached --stat
   git commit -m "Verify timed bus reads and ABI v2 on four platforms"
   git push -u origin verify/phase-3-1-bus-timing
   ```

   **These commands have not been executed by the agent.** They are the user's publication step. No release is necessary.

2. The push triggers the existing workflow. Open [repository Actions](https://github.com/inthe92/addams-family-recompiler/actions), choose **Build content-free frontends**, and select the run for the branch and **new committed SHA**. If a manual run is needed, choose **Run workflow → Branch: verify/phase-3-1-bus-timing → Run workflow**, as described in [GitHub's manual-run instructions](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow). Do not substitute a run of old `main` or the unchanged local base SHA.
3. Inspect `macos (macos-15)`, `macos (macos-15-intel)`, `windows`, and `linux`. Each **Build engine without ROM** step must finish successfully, print **12/12 CTest passed**, and print the inventory/result gate messages confirming mandatory entries and counts. The `$2002` output must show **both** correct read cycles and PASS; `native_opcode_contract` must report **4736** executed cases; ABI output must reject missing/old versions and accept the current module.
4. Download the four test artifacts: `ctest-macos-15`, `ctest-macos-15-intel`, `ctest-windows-x64`, `ctest-linux-x64`. Verify each JUnit has 12 executed cases, zero failures/disabled/skipped and the same names as its inventory. Keep the run URL, SHA, runner image and logs with the results. Confirm the separate bundled toolchain and frontend importer steps pass on each host as well.
5. If packaging later fails while the test/gate step passed, record the test evidence separately and report the packaging failure; do not call the entire workflow successful. Production fixes require a demonstrated failure and the same complete suite afterward.

## Completion and remaining limits

**Phase 3.1 is complete under the external-verification criterion:** all locally available configurations were checked and the missing external runs are explicitly identified. Full four-platform runtime verification is **not complete**.

There are no new observed CPU/device/ABI failures, no demonstrated compile/link portability defect, and no test removal. macOS execution passed in both architectures; Windows/Linux full compile/link passed but execution remains pending. The omitted fetch/dummy and interrupt/DMA/device fidelity limits in the bus report remain unchanged and were not worked on.

Evidence is retained outside Git in the surrounding workspace's `outputs/platform-verification-2026-10-10/`: starting hashes/status, configure/build logs, all four CTest inventories, two actual JUnit/CTest runs, importer/toolchain logs, gate sensitivity output, binary/export inspections, source audit and a machine-readable result ledger. Generated code is synthetic and stays in build trees. No commercial input, resources or native game module was added to Git or to this evidence directory.
