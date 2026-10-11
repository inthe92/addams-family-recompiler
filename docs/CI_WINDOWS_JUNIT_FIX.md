# Windows SDL entrypoint and CI JUnit fixes

Date: **2026-10-11**. Scope: two infrastructure corrections; no production logic, ABI, assertions or dependencies change. A02 remains an enabled, ordinary failing test.

## Checkout and starting state

Checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` in the project workspace. Branch: **`verify/phase-3-1-bus-timing`**. Starting HEAD: **`9a12e2e0f45e49d58bb0104d5a0d5ae83430c432`**. Git was clean, with no unrelated pending work. Origin: `https://github.com/inthe92/addams-family-recompiler.git`. Remote branch matched HEAD and main remained `1588f31e3c3375adfbb4f75266d44eff1fdb25e5` before publication.

Read `FOUR_PLATFORM_CI_REPORT.md` and inspected CMake, the workflow, SDL headers/source and existing APU tests. The reference [run 38125126820](https://github.com/inthe92/addams-family-recompiler/actions/runs/38125126820) showed a Windows link failure and missing JUnit artifacts on the other three hosts. A01/A03 were passing; A02 was not fixed.

## Windows cause and minimal correction

`apu_sdl_queue_checks.cpp` includes SDL headers and declares `int main()`. SDL 2.32.8's `SDL_main.h` sets `SDL_MAIN_AVAILABLE` for desktop Windows unless `SDL_MAIN_HANDLED` is defined, then substitutes `main` with `SDL_main`. This removes the ordinary console entrypoint. The test links `desktop_presenter`/static SDL2, without the SDL2main wrapper; MinGW consequently reports an unresolved `WinMain`. The principal executable already defines `SDL_MAIN_HANDLED=1`.

**Correction:** apply that same private compile definition to **only `apu_sdl_queue_checks`** in CMake. No source edit, additional wrapper library, audio change or target-wide definition is required. SDL's desktop Windows branch uses `SDL_MAIN_AVAILABLE`, rather than `SDL_MAIN_NEEDED`; the bundled SDL source starts `SDL_MainIsReady` true for this branch.

A local preprocessing probe of the actual bundled `SDL_main.h`, with Windows selection isolated, emits `int SDL_main()` before and `int main()` after the definition. This verifies the header mechanism, not Windows compilation or execution. The complete test cannot be Windows-preprocessed using the host SDK (missing `process.h`); no cross-toolchain is installed. Native CI is required for the actual link/run result.

## JUnit cause and correction

The previous relative `--output-junit build/ctest-results.xml` resolves against CTest's `--test-dir build`, yielding `build/build/ctest-results.xml`. Upload and validation instead expect repository-root `build/ctest-results.xml`.

**Correction:** all three job definitions (four hosts through the macOS matrix) now pass **`--output-junit "$PWD/build/ctest-results.xml"`**. This is absolute in each job's shell; MSYS2 performs native argument path conversion on Windows. Upload and verifier locations remain unchanged.

The existing JUnit verifier is moved into its own **`if: always()`** step so it runs even after A02 makes CTest exit nonzero. Validation is not relaxed: it still rejects failing/skipped/missing tests and retains the inventory/diagnostic checks. It is expected to fail for A02. Artifact upload remains unconditional. No `continue-on-error`, expected-failure property, disabled test or CI filter is added.

## Files changed

- `CMakeLists.txt`: one private test-target definition.
- `.github/workflows/build.yml`: three absolute output paths and separate mandatory JUnit-validation steps.
- `docs/CI_WINDOWS_JUNIT_FIX.md`: this report.

All other starting files are preserved. Logs, fixtures and XML evidence stay outside Git in the surrounding workspace `outputs/ci-windows-junit-fix-2026-10-11/`.

## Local verification

Host: **macOS ARM64**, Release, desktop/mods/testing ON, using `/private/tmp/nes-technical-baseline-build`.

| Check | Actual result |
|---|---|
| CMake configure/generate | PASS; existing SDL deprecation/optional PkgConfig warnings |
| Full default build | PASS; changed SDL test recompiles and links |
| CTest inventory gate | PASS: all 19 entries registered, executable, mandatory |
| APU/SDL selection (`^apu_`) | **6 executed, 5 passed, 1 failed (A02), 0 skipped** |
| SDL dummy queue | PASS |
| A01 / A03 / A03 boundary schedules / APU core | PASS |
| A02 | **FAIL**, unchanged cycle 33,252: status `$40`, IRQ 0 instead of 1 |
| Absolute JUnit parsing | PASS: six actual selected names, five successes and A02 failure |
| Exact workflow `$PWD` path probe | PASS: XML at `build/ctest-results.xml`, no nested `build/build` file |
| `git diff --check` | PASS |

The path probe uses one trivial external harness test and is excluded from project counts. The selected local APU tests do not substitute for full CI: unchanged exhaustive CPU tests were intentionally not repeated locally. The inventory is full (19); the local JUnit selection is intentionally six. Headless was not rerun because these changes concern a desktop-only test and CI XML collection.

Commands from the checkout:

```sh
cmake -S . -B /private/tmp/nes-technical-baseline-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build --show-only=json-v1 > /tmp/nes-ci-inventory.json
python3 -B tests/verify_ctest_suite.py --inventory /tmp/nes-ci-inventory.json
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^apu_' --output-on-failure --verbose --no-tests=error --output-junit /tmp/nes-ci-apu.xml
git diff --check
```

CTest exits **8** for A02; this is not a full-suite success.

## Published commit and native CI evidence

Pending publication and actual four-host execution at the time of the initial report commit. This section will be updated from the run's actual logs and downloaded XML; no remote success is claimed in advance.

## Remaining risks and stopping condition

A02's PAL frame-IRQ/read-clear collision remains unresolved and must remain visible. Broader interrupt/DMA/PPU fidelity, game-route acceptance and packaged imports remain outside this task. No ROM, extracted resource or generated game code is used or published.

If a new platform failure appears, document it without further corrections. Stop after these two infrastructure changes and their evidence report; do not fix A02 or start another phase.
