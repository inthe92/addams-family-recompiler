# B10 — Headless build correction

Date: **2026-10-10**. Scope: only the `NES_ENABLE_DESKTOP=OFF` failure identified as **B10** in [TECHNICAL_BASELINE.md](TECHNICAL_BASELINE.md). The older reports remain historical snapshots.

## Source and preservation

HEAD remains **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**, branch **`verify/phase-3-1-bus-timing`**, application version **0.2.1**, module ABI **2**. All existing tracked and untracked changes were preserved. A starting hash manifest covers 98 public source files; only `CMakeLists.txt` changed during this task, and this report is the only added public file. No files or tests were deleted. There were no commits, pushes, releases or workflow dispatches.

## Reproduced cause

Before the correction, a fresh Release configuration with `BUILD_TESTING=ON`, desktop OFF and mods OFF failed with exit **1** during CMake generation. Both `user_rom` and `desktop_presenter` linked the missing target `SDL2::SDL2-static`.

`cmake/dependencies.cmake` already correctly creates SDL and finds OpenGL only when `NES_ENABLE_DESKTOP` is ON. The top-level CMake file created their consumers unconditionally. `user_rom` includes SDL UI, filesystem and module-loading operations; its importer probe and the dynamic module ABI test also require that loader. Keeping those consumers in a build without SDL would require changing runtime functionality, which is outside this correction.

## Minimal correction

Only target creation in **`CMakeLists.txt`** changed:

- Gate `user_rom`, its Cocoa/Windows dialog linkage, `desktop_presenter` and `nes_native` on `NES_ENABLE_DESKTOP`.
- Gate `import_probe`, `module_abi_checks` and its three synthetic dynamic module fixtures on the same option.
- Apply their Windows static-runtime link flags only when those targets exist; keep the flags for all five SDL-independent native test executables in both configurations.

The runtime, optional Lua engine, game-view library, generated synthetic C fixtures and all SDL-independent tests remain build targets. No production source, ABI, emitter, renderer, game logic, workflow, dependency configuration or packaging script changed. The desktop configuration retains every previous target and all 12 CTest entries.

Headless CTest has **11 mandatory entries**, rather than registering an unavailable SDL loader test or disguising it as skipped/passed. `module_abi_contract` is retained and mandatory whenever its production SDL loader is built. Python importer validation and cache/ABI tests remain available without desktop. The frontend importer process check requires `import_probe` and is exercised with the desktop build.

## Executed verification

Host: **macOS 27.0.1 ARM64**, AppleClang **21.0.0**, CMake **4.4.4**, Ninja, system Python **3.9.6**. All three configurations use Release, `BUILD_TESTING=ON` and explicit `CMAKE_OSX_ARCHITECTURES=arm64`.

| Configuration | Build performed | CTest executed | Result |
|---|---|---|---|
| Desktop OFF / mods ON | Fresh full default build, including runtime, Lua, game view and five native check executables | All **11** compatible entries | **11 passed**, 0 failed, 0 skipped |
| Desktop OFF / mods OFF | Full default build in the tree that originally failed generation; no earlier successful binaries | All **11** compatible entries | **11 passed**, 0 failed, 0 skipped |
| Desktop ON / mods ON | Regenerated existing verified desktop tree and built the full default target; existing object files reused because production sources are unchanged | All **12** entries, including real module loading | **12 passed**, 0 failed, 0 skipped |

**34 CTest executions passed across the three configurations**, without disabled, expected-failure or skip exemptions. Each suite executes the **4,736 native instruction cases**, two stack programs, both PPUSTATUS timing regressions, 14 timed side-effect cases, 17 RAM/MMC1/bus/PPU device cases and 12 Python cases. The desktop suite additionally executes three dynamically loaded ABI module cases. Total native instruction executions in this task: **14,208**; no arithmetic helper sweep was repeated.

Verification also checked:

- Headless inventories equal the normal inventory minus only `module_abi_contract`. Each registered executable exists; JUnit contains exactly those executed names, passing statuses and expected count diagnostics.
- Ninja target graphs and command lines contain no SDL build targets/sources, desktop presenter, GUI executable, importer probe, ABI loader fixtures or OpenGL/Cocoa link flags in either headless configuration. `nes_runtime` and `user_game_view` are built in both.
- `otool -L` on all five native test executables in both headless trees shows no SDL, OpenGL or Cocoa dependency.
- The normal inventory/JUnit passed the unchanged **12-test CI gate** in `tests/verify_ctest_suite.py`. The normal target graph retains all SDL-dependent frontend/importer/ABI targets.
- Existing frontend importer process checks passed using synthetic invalid input: paths with spaces, early stdout/stderr and executable launch failure.
- Existing bundled Python/native compiler smoke compiled, signed and loaded a synthetic function returning 42. Its first attempt failed because the sandbox denied Zig's default cache path `/Users/sebas/.cache/zig`; a second execution passed with both Zig caches redirected to writable `/private/tmp` directories. This environmental failure required no repository change and was not counted as a passing attempt.
- Public source export audit and `git diff --check` passed. Hash comparison confirms preservation of the other 97 starting public files.

No commercial ROM, extracted resources or translated game module was used or added. Synthetic fixture C stays outside Git in build directories.

## Reproduction

Run from the public repository checkout:

```sh
cmake -S . -B /private/tmp/nes-b10-headless-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF -DNES_ENABLE_MODS=ON -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build /private/tmp/nes-b10-headless-build --parallel 4
ctest --test-dir /private/tmp/nes-b10-headless-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-b10-headless-mods-on-junit.xml

cmake -S . -B /private/tmp/nes-b10-reproducer-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF -DNES_ENABLE_MODS=OFF -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build /private/tmp/nes-b10-reproducer-build --parallel 4
ctest --test-dir /private/tmp/nes-b10-reproducer-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-b10-headless-mods-off-junit.xml

cmake -S . -B /private/tmp/nes-technical-baseline-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build /private/tmp/nes-technical-baseline-build --parallel 4
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error --output-junit /private/tmp/nes-b10-desktop-junit.xml
ctest --test-dir /private/tmp/nes-technical-baseline-build --show-only=json-v1 > /private/tmp/nes-b10-desktop-inventory.json
python3 -B tests/verify_ctest_suite.py --inventory /private/tmp/nes-b10-desktop-inventory.json --junit /private/tmp/nes-b10-desktop-junit.xml
```

On other operating systems omit `CMAKE_OSX_ARCHITECTURES` and use suitable build paths/generator. The existing CI gate deliberately requires the complete desktop suite; it was not weakened to accept an 11-entry desktop inventory. Use unfiltered CTest for headless configurations.

The additional smoke/process checks used previously available local content-free resources whose recompiler tool copies match this checkout:

```sh
ZIG_GLOBAL_CACHE_DIR=/private/tmp/nes-b10-zig-global-cache ZIG_LOCAL_CACHE_DIR=/private/tmp/nes-b10-zig-local-cache /private/tmp/nes-phase3-resources/toolchain/python/bin/python3 -B tests/public_toolchain_smoke.py /private/tmp/nes-phase3-resources
/private/tmp/nes-phase3-resources/toolchain/python/bin/python3 -B tests/public_import_process_checks.py /private/tmp/nes-phase3-resources /private/tmp/nes-technical-baseline-build/import_probe
```

## Completion and limits

**B10 is corrected and locally verified** for both headless mod settings and the normal desktop configuration. There are no unresolved build or CTest failures in the configurations executed after the correction. Existing benign desktop dependency messages about unavailable optional PkgConfig and upstream CMake deprecation remain unchanged.

This task did not run Intel, Windows or Linux builds/tests or remote Actions; the previous platform report remains a historical result, not validation of this new CMake change on those platforms. No graphical gameplay QA was performed. Lua functionality has no dedicated regression entry beyond the existing suite. Dependency archive verification/extraction remains unchanged, including extraction of bundled SDL sources in headless builds; SDL is not configured, compiled or linked there. Headless builds intentionally provide the motor/libraries and synthetic tests, not a new CLI game launcher or SDL-free dynamic ROM importer.

Evidence is retained outside this public Git checkout in the surrounding workspace's **`outputs/headless-build-2026-10-10/`**: original failure, starting source hashes, incremental CMake diff, configure/build/CTest logs, inventories, JUnit, dependency-graph checks, auxiliary test outputs and results ledger. No further phase was undertaken.
