# Mod Manager — implementation and verification

2026-10-13. Implemented locally on **feature/mod-manager-v1** in a separate worktree, based on `80af76aa5e4408f37dcb0e7047355c21be186ae8`. Changes are uncommitted. The original high-refresh checkout retains its branch, pending reports and published beta; no remote action was performed.

## Available

Install/list/enable/disable/uninstall ZIP packages; strict manifest validation; persistent state; failure deactivation; unclean-session recovery; isolated bounded Lua; real lifecycle callbacks; read-only snapshots; English Mods UI with mouse/keyboard/controller-event support; native ZIP picker macOS/Windows and Linux drop-to-install; safe mode and a working authored example. No heavy dependency or global installation was added.

F2 or right-stick click opens/closes Mods during gameplay. Up/down select a mod, left/right or Tab select an action, Enter/Space/controller A activates, Escape/controller B returns. Uninstall requires a second activation of its confirmation. Opening Mods pauses original simulation/audio, protects game inputs and retains the original framebuffer/4:3 presentation. F11 remains available. Normal window close/Cmd+Q/Guide reach the original exit protection. Analog-stick navigation is not implemented; use D-pad, mouse or keyboard.

`--mods-only` opens the same manager without importing any ROM. `--no-mods` prevents loading/installing scripts or touching managed storage. `--mods-dir <path>` selects separate developer/test storage. Default storage is SDL_GetPrefPath("nes-native-lab","mods"):

- Windows: under `%APPDATA%/nes-native-lab/mods`.
- macOS: `~/Library/Application Support/nes-native-lab/mods`.
- Linux/Steam Deck: under `$XDG_DATA_HOME/nes-native-lab/mods`, normally `~/.local/share/nes-native-lab/mods`.

Packages live in `packages/<id>`, state in `state.json`, and diagnostics in `operation.log`/`mods.log`, outside the ROM cache and distribution. Installed packages start Disabled. No updates overwrite previous installations.

## Files

New: `runtime/include/runtime/mod_{lua,manager,panel,frontend}.hpp`, matching `.cpp` implementations, `mod_process.cpp`, `mod_picker_mac.mm`, `game/include/game/mod_observation.hpp`, `tools/mods/{packages,make_example}.py`, `examples/mods/hello/{mod.json,main.lua}`, four native test sources plus two Python harnesses, and four MOD documents.

Modified: CMake target/test/resource integration; desktop option/event/update/overlay integration; frontend external pause; bitmap menu/glyph; GL generic panel drawing using its existing separate UI texture; package resource copying and required-file checks; CTest required-name/output gates. CPU, PPU, APU, bus, mapper, emitter, module ABI and the high-refresh scheduler/snapshot sources remain byte-identical to the base. The original GL image path/cache is unchanged; only the UI overlay drawing is factored into a shared panel function. No private module regeneration or ABI/cache invalidation is necessary.

The normal packager includes `tools/mods` and creates `examples/mods/Hello-NES-Native-Lab.zip`; native package checkers require these two artifacts. An always-run build resource target copies the current helper to developer binaries/macOS Resources, avoiding stale helper copies after Python-only edits. Lua was already statically bundled and licensed. The existing four CI jobs build all new targets and CTest gates; **CI was not dispatched and packages were not rebuilt/published for other systems**.

## Executed on macOS ARM64

Fresh Release builds: desktop and headless. Full CTest:

| Configuration | Executed | PASS | FAIL | Skipped |
|---|---:|---:|---:|---:|
| Desktop | 56 | 55 | 1: A02 | 0 |
| Headless | 50 | 49 | 1: A02 | 0 |

New cases: Lua sandbox/lifecycle **24**, panel model **11**, ZIP/store Python **25**, native C++ store/order/recovery **11**, generated-C determinism at 60/90/120 **3**, SDL frontend/safe mode **11**. Total **85 unique cases**, all pass on desktop; headless executes the 74 non-SDL cases. The final desktop suite includes the session-marker guard, stale-activation cleanup and focus protection; targeted mod tests were also repeated during development.

The 63 original timing contracts, 4,736 instruction executions, 82 IRQ cases, PAL 07/08, A01/A03, $2002, RAM/MMC1/bus, DMA/NMI, import/cache/ABI guards all pass in the full suites. A02 remains an ordinary failing test and CTest returns 8. No WILL_FAIL, DISABLED, filtering of mandatory CI tests or changed A02 expectation.

Real OpenGL integration probe: **23/23 PASS** on local Metal/OpenGL 4.1, including original pixels, aspect ratio, resize/fullscreen, UI and retained textures. This is not a Windows/Linux or physical 120 Hz certification.

Real UI session: installed the authored ZIP through the macOS picker, observed Disabled/Enabled/Disabled states, metadata and English actions; private logs recorded on_load/on_unload. The manager-only session exited 0 and removed its session marker. No on_update is expected without a game. Installation/removal, persistence and updates were additionally executed by the native store/generated-C harnesses. No private ROM or real playthrough was used to certify this feature.

The UI automation's follow-up observation reopened the developer executable without its manager-only arguments and showed the pre-existing missing-importer-tools path for an unpackaged build. That is not counted as a game launch. Use the explicit commands below; a packaged frontend contains its importer tools. Physical keyboard automation was intermittent, so keyboard/controller behavior is principally backed by SDL synthetic event tests; no Steam Deck/Bluetooth hardware certification is claimed.

Public source export audit: **220 files**, no ROM, generated game module, private cache or temporary build payload detected. `git diff --check` passes. The developer helper was compared byte-for-byte with the source after copying. These are technical checks, not legal certification.

## Reproduce

```sh
cmake -S . -B <build> -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON
cmake --build <build> --parallel 4
ctest --test-dir <build> --verbose --output-on-failure --output-junit <build>/results.xml
# For headless use a separate build with NES_ENABLE_DESKTOP=OFF.
python3 -B tools/mods/make_example.py <private-test-dir>/Hello-NES-Native-Lab.zip

# macOS developer build, no ROM:
<build>/nes_native.app/Contents/MacOS/nes_native --mods-only --mods-dir <private-test-dir>/store
# Windows/Linux: <build>/nes_native[.exe] with the same options.
# Safe startup:
<frontend> --no-mods
```

For a normal developer game build, retain the existing `--import-tools <checkout>/tools/recompiler` and private import-cache procedure. Packages discover their importer and mod helper beside the executable/Resources. Test with an owned compatible ROM only; do not upload it, generated modules, caches or gameplay captures.

## Limits and next checks

Windows x64, macOS Intel, Linux x64 and Steam Deck: prepared in CMake/native package checks, **not executed for these changes**. The previous high-refresh Linux pixel-probe failure is not fixed by this feature. Steam Deck desktop should be tested with D-pad/right-stick click, a mouse/file drop and the existing Escape/F11/audio/focus flow; select a separate store for experimental testing. Installation may require mouse/file selection initially; no touch UI or analog menu navigation is claimed.

Player coordinates are deliberately unavailable until a reliable adapter exists. Assets remain passive; no graphics/audio mods, physics modifications, mod networking, Lua require, priority configuration, automatic updates, install replacement or universal NES support. ASCII package metadata, same-user filesystem races, stale-lock recovery and manually corrupted installed-folder handling remain limitations. The API is read-only; the legacy trusted developer mod route is documented separately.

Next: review this local implementation, run native CI only after authorization, then a private game session exercising Mods/escape/focus/fullscreen/audio and physical controller input. Do not publish a Mod Manager release or describe other platforms as verified from this Mac build.

## Authorized experimental publication preparation (2026-10-13)

The owner subsequently authorized publication of this feature branch and native CI, without a merge, tag or release. A second pair of fresh macOS ARM64 Release builds reproduced desktop **55/56** and headless **49/50**, with A02 as the only failure and zero skips. All **85** manager cases and **23/23** real SDL/OpenGL checks passed again. New/modified files were separately inspected for private content. Existing historical reports remain unchanged.

`tests/mod_distribution_checks.py` now runs six ZIP/store transactions through the extracted package's own Python/helper (list, install Disabled, enable, persisted list, disable, uninstall), in private storage with spaces. It checks that the packaged example is byte-identical to the authored source. All three native package verifiers invoke it; its local host-Python check passed. This is a tooling check, not execution of Lua inside a packaged game or Steam Deck certification. Exact new-SHA CI/package results will be recorded in `MOD_MANAGER_FOUR_PLATFORM_VERIFICATION.md`.
