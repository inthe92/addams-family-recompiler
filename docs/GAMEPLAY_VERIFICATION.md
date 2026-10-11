# Gameplay verification — optional route diagnostics

Date: **2026-10-10**. Starting HEAD: **`e027f5a23f5a2cd49c7a7366c4a211479b7866dd`**, branch **`verify/phase-3-1-bus-timing`**, application **0.2.1**, ABI **2**. Existing local changes, including the B10 headless correction, are preserved. No commits, pushes, releases or distribution changes were made.

## Purpose and verified scope

The diagnostic observes the existing statically generated native execution. It does not inject controls, grant items, prevent damage, request rooms, write RAM, add bus reads, advance devices or change instruction budgets. It is **OFF by default**. With no `--diagnostic`, the observer returns the original `RuntimeServices` object verbatim and opens no diagnostic file.

The reported manual completion with GOD/all items is user-provided context, not a normal-game validation performed in this task. **No complete normal playthrough was executed or certified here.** The implemented system provides evidence for that next manual test.

## Start a normal test

Use a **new process**, the supported ROM you own, and these options:

```sh
nes_native --rom "/absolute/path/to/your-compatible.nes" --no-god --diagnostic
```

For a packaged macOS application, launch its executable from Terminal:

```sh
"/path/to/NES Native Lab.app/Contents/MacOS/nes_native" --rom "/absolute/path/to/your-compatible.nes" --no-god --diagnostic
```

For Windows PowerShell:

```powershell
.\nes_native.exe --rom "C:\ROMs\your-compatible.nes" --no-god --diagnostic
```

For an existing Linux AppImage, pass the same `--rom`, `--no-god` and `--diagnostic` options to the AppImage executable. No new package was produced by this task.

The alpha still defaults to GOD enabled, so **`--no-god` is required**. All-items starts disabled in a fresh process. **Do not press G or H, and do not pass `--mod`.** These existing controls remain available; the diagnostic marks the entire session as assisted permanently if any of them is used, even if GOD is subsequently disabled. Restart the process to begin another normal test. Imported module caches contain compiled content, not a saved gameplay RAM state.

Begin through the normal menus with Enter/Start; play using the existing controller or arrow keys and Z/X. Finish by quitting normally after observing the ending so the log gets a terminal record. `--frames` is optional for bounded smoke tests, but a frame-limit exit does not mean the game was completed.

For an unpackaged development build, explicitly provide the existing importer paths as needed:

```sh
cmake -S . -B /private/tmp/nes-gameplay-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON
cmake --build /private/tmp/nes-gameplay-build --parallel 4
/private/tmp/nes-gameplay-build/nes_native.app/Contents/MacOS/nes_native --rom "/absolute/path/to/your-compatible.nes" --import-tools "/absolute/path/to/repository/tools/recompiler" --import-cache "/absolute/path/outside/repository/private-import" --no-god --diagnostic
```

The last command is the macOS build layout. On Windows/Linux use the generated `nes_native.exe`/`nes_native` path. Headless builds provide diagnostics tests, not an interactive game launcher.

## Local storage and contents

The frontend prints **`Gameplay diagnostic: <absolute filename>`**. SDL's user preference directory for organization `nes-native-lab`, application `diagnostics`, determines storage:

| System | Default directory |
|---|---|
| macOS | `~/Library/Application Support/nes-native-lab/diagnostics/` |
| Windows | `%APPDATA%\nes-native-lab\diagnostics\` |
| Linux | `$XDG_DATA_HOME/nes-native-lab/diagnostics/`, or `~/.local/share/nes-native-lab/diagnostics/` |

Each invocation gets a timestamp-based `session-*.jsonl` filename. Files are opened for append rather than truncating previous evidence. Canonical path validation rejects Git checkout ancestors, `.app`/`.AppDir` trees and source/portable-package trees containing `tools/recompiler`. Logs are outside the checkout and public packages; packaging scripts were not changed and these local preference files are never copied by them. Nothing is uploaded automatically.

Each JSON line contains schema, sequence, event type, CPU cycle, PPU frame and the last observed PC/physical bank. `cpu_available=false` marks events before the runtime attaches. The file records **no ROM bytes, generated source, textures, maps, audio, dialogue text, full RAM dumps, input history or ROM filesystem path**. IDs and addresses are numeric; no room or item names are invented.

Normal event writes are buffered and flushed once per completed frame. Session boundaries, errors, missing native addresses and stall warnings flush immediately. Disk I/O failure disables recording and produces a frontend warning; game execution continues through the original callbacks. Such a partial log cannot verify a complete session. Force-kill, power loss or a process crash can leave the final buffered events or terminal record missing.

## Observed contracts

These identifiers come from the existing observers and local recovery evidence inspected during this task. Only numeric contracts were added to public code; no commercial ASM, ROM data or generated game code was copied.

| Observation | Contract / interpretation |
|---|---|
| Screen and room | Screen state `$045B`, room ID `$0071`, layout ID `$004D` |
| Transition | Pending request `$0471`, fade state `$00B1`, loader state `$0080`; request `$FF` is the consumed sentinel, not a destination |
| `room_enter` | Two different completed-frame samples with screen 0, request `$FF`, fade 0, loader 0, death 0 and the same room ID; this filters intermediate loader states |
| `room_exit` | Loss of stable-room conditions or a changed ID, including death/reload; final session closure explicitly uses reason `session_end` |
| Bank mapping | Effective PRG mappings at `$8000` and `$C000`, sampled after instruction boundaries; unchanged serial writes do not create events |
| Death | `$0070` changes from zero to nonzero during gameplay; repeated counter increments do not become repeated deaths |
| Respawn | A prior observed death followed by a newly stable room; clearing `$70` during loading alone is not a respawn |
| Confirmed pickup | Only flag `$0633`: zero→one through bank 1, PC `$A80A`, room ID 2, valid X slot `<12` and entity type `$09` at `$0320+X`; emitted as `item_flag=1587`, without a name |
| Other progress | Changes of 26 selected fields: `$0608`, `$0621/$0623–$0626`, `$0630–$0639`, `$063B–$0643`, `$04ED`; resets/consumption/stages remain numeric progress changes, not invented pickups |
| Ending activity | `$063A` zero→nonzero records activation only, avoiding per-tick timer logging |
| Ending screen | Screen 4 written specifically through bank 7, PC `$C405`; raw screen 4 from another origin is insufficient |

Writes are forwarded exactly once with their original address, value and cycle offset. Actual RAM is inspected afterward, so mirrored writes, unchanged RMW dummy writes and writes filtered by an existing GOD observer are handled without fabricating changes. Progress is compared at instruction boundaries after the original interrupt callback and once per completed frame; the latter also sees direct RAM writes from existing mods/cheats. Pickup classification requires the native write site, so a sampled inventory grant cannot become a pickup event.

Errors include missing compiled bank/PC and a generic `runtime_exception` code. Last observed stamps are cached so error/session-end reporting remains safe after runtime stack unwinding. The observer also reports 16 returned slices with no CPU cycle progression and approximately one PAL CPU second of cycle progression without any new video frame. These are **warnings**, not automatic aborts or repairs. A host hang inside a single native slice cannot be diagnosed by this cooperative mechanism.

## Review a completed attempt

From a source checkout, read the local file using the stdlib-only reviewer:

```sh
python3 -B tools/verification/review_gameplay_log.py "/absolute/path/to/session-....jsonl"
```

It streams the file, validates JSON/schema/sequence, checks session lifecycle and monotonic cycle stamps, and prints event counts, visited numeric room IDs, consecutive observed room pairs, last progress values, assistance status and problem events. Exit 0 means the file was parsed and summarized; it does **not** certify a successful game. Missing terminal records are reported as `session_ended=false`. Malformed/truncated JSON or invalid order fails with exit 1.

1. Retain the exact executable SHA-256, source/build revision, platform and launch options alongside the **private** log, plus the tester's brief notes. The application version alone does not distinguish this uncommitted diagnostic build from older 0.2.1 packages.
2. Require one `session_start`, one terminal `session_end`, `assisted=false` throughout, no missing native entries or error/stall events, and a recorded `ending_screen_observed`. A missing file, output warning or incomplete session remains unresolved evidence.
3. Check each requested transition against subsequent stable entry. Death/respawn pairs and same-ID reloads remain separate from ordinary room changes. Room pairs are consecutive observed entries, not certified direct door edges; menus/deaths can intervene.
4. Check the confirmed pickup and numeric progress changes against the actions the tester remembers. Other object identities and causes remain unverified. RAM reset, dialogue/load transitions and legitimate item consumption can decrease progress fields.
5. Confirm the ending visually and record that confirmation in the tester's notes. `normal_completion_candidate=true` means an unaided, terminated runtime session reached the known ending selector without recorded problems and visited a stable room; **`manual_gameplay_review_required=true` always remains**. It is not proof of correct physics, combat, puzzles, hardware fidelity or every possible route.
6. For coverage, retain separate logs for alternative routes, deaths, continue/restart paths and different puzzle orders. Completing one normal route does not validate all 37 room IDs, all objects or all transitions. Never treat unvisited rooms as passed or infer missing names.

The reviewer prints to stdout; no summary is written into the repository. Copying a log for review or sharing it with a tester is a manual user action.

## Tests executed in this task

Host: **macOS ARM64**, AppleClang 21, CMake 4.4.4, Release. All existing registered tests were run unfiltered after the final production/test changes.

| Configuration | Full build | CTest result |
|---|---|---|
| Desktop ON / mods ON | PASS; frontend, importer and all test targets | **13/13 passed**, 0 failed/skipped |
| Desktop OFF / mods ON | PASS; includes observer and its new native test | **12/12 passed**, 0 failed/skipped |
| Desktop OFF / mods OFF | PASS | **12/12 passed**, 0 failed/skipped |

Each configuration retains the existing **4,736 native instruction cases**, two complete stack programs, both `$2002` timing regressions, 14 bus side-effect cases, 17 RAM/MMC1/bus/PPU device cases and importer tests. Desktop also retains real dynamic ABI-v2 loading. **37 CTest entry executions passed** across the three configurations; the preexisting native matrix ran 14,208 cases in total. Historical arithmetic sweeps and private game boot probes were not repeated or counted.

`gameplay_diagnostic_contract` adds **eight native synthetic test groups**: disabled-service identity; stable rooms/transitions/reloads; pickup filtering/death/respawn/progress; effective bank changes/missing PC/error/stall warning; sticky assistance/ending selector; state and access transparency; failed/throwing sinks and reporting after runtime destruction; protected local paths/previous-file preservation.

Transparency tests execute the existing **generated C** JSR/PHA/PLA/RTS and BRK/RTI programs, PPUSTATUS LDA and PPUSTATUS RMW instruction paths with and without the observer. They compare A/X/Y/SP/P/PC, all 2,048 RAM bytes, observable bus access order/values/exact cycles, controller reads, CPU/APU/PPU clocks, PPU status/address/frame/OAM and interrupt/DMA counters. A paired hardware NMI boundary also verifies the original callback result and CPU mutation.

An independent Python parser validates **11 complete synthetic sessions per configuration**, JSON escaping, event sequence, numeric identities and assistance summaries. Reviewer tests reject corrupt sequences, distinguish truncated/assisted/error evidence, and recognize only the deliberately synthetic unaided-ending candidate while requiring manual review. A no-progress slice warning is positively tested; the no-video-frame warning is checked for absence when real video frames advance, without freezing or modifying the production PPU.

The unchanged normal-suite coverage requirements plus the new diagnostic entry pass the CI inventory/JUnit gate (now **13 required desktop entries**). The new test is mandatory; no skip or expected-failure exemption was added. Both headless suites retain every compatible entry, with only the existing SDL module-loader test conditional on desktop. The new headless executable links no SDL/OpenGL. Public export audit and source-preservation checks passed.

## Changed files and limits

Changes from the starting worktree are limited to `app/desktop.cpp`, `CMakeLists.txt`, `tests/verify_ctest_suite.py`, new `game/include/game/gameplay_diagnostic.hpp`, `game/src/gameplay_diagnostic.cpp`, `tests/gameplay_diagnostic_checks.cpp`, `tests/gameplay_diagnostic_checks.py`, `tools/verification/review_gameplay_log.py` and this document. Existing engine, ABI, emitter, devices, physics, renderer, widescreen, GOD implementation and distribution scripts are unchanged.

The frontend parses options before importing so the diagnostic can capture importer/runtime exceptions and cancellations; importer implementation and exit codes remain unchanged. Native service instrumentation only attaches when recording is enabled. Startup file errors and later recording failure warn and leave gameplay callbacks operational.

No ROM or private generated module was used in the tests. No real normal playthrough, frontend GUI session or four-platform execution of this change was performed. Default SDL preference paths were inspected in the bundled source; file/event storage was tested with synthetic directories outside Git. Testing of the actual desktop preference-directory launch remains part of the manual procedure above. Diagnostic host I/O can affect wall-clock performance; tests establish unchanged emulated cycles/accesses, not identical host pacing/audio queue timing. Bank churn can produce large logs, but there is no per-instruction trace or full-state snapshot.

Coverage remains conservative: only one pickup site is classified; other inventory changes have numeric IDs. Two-frame stable entry can miss a room that never remains stable long enough. Direct RAM changes are sampled once per frame; transient changes wholly between samples can be missed. The ending-selector contract is evidence of the native ending path, not independent proof that every objective was fulfilled legitimately. Hard hangs/crashes need external process investigation; device, IRQ/DMA, graphics/audio and gameplay fidelity limits in earlier reports remain open.

Evidence is retained outside the public checkout in **`outputs/gameplay-verification-2026-10-10/`**: starting hashes/status, task-only diffs, build and CTest logs, inventories/JUnit, preservation audit, test gate output and results ledger. All recorded test events are artificial and contain no commercial resource data. Work stops here; the next action is the documented normal manual playthrough.
