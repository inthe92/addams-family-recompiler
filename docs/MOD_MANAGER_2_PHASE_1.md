# Mod Manager 2.0 — first functional vertical delivery

2026-10-13. **Implemented and locally verified, not published.** This is VS1 of
[MOD_MANAGER_2_ROADMAP.md](MOD_MANAGER_2_ROADMAP.md), not completion of all phases.
[MOD_API_V2.md](MOD_API_V2.md) is the implemented contract.

## Initial state and preservation

Audited published v1 at `95c194afac4574be9bf8b495341517389a73988b` on
`feature/mod-manager-v1`. Its pending four-platform report was preserved.
The original high-refresh checkout and the separate Super Gomez experiment had
uncommitted work; neither was switched, cleaned, overwritten or merged.
A new independent worktree `outputs/mod-manager-v2`, branch
`feature/mod-manager-v2`, starts at that exact v1 commit. All changes below remain
uncommitted. No fetch, remote CI, push, tag, release or published-beta change.

Read AGENTS/PROJECT_STATUS, v1 architecture/implementation/API/security and
high-refresh audit/phase B/C timing contracts. Baseline v1 has ZIP transactions,
strict manifests, persistence/recovery, bounded isolated Lua states and an SDL
Mods panel. No managed write API. Legacy player is nil, and the legacy room field
is a screen selector. Existing experimental actor slots were not used as a
universal player identity. Historical CI results are not reused as new evidence.

## Chosen architecture and compatibility

The NES hardware/native module remain untouched. `game/mod_observation_v2.hpp`
is the profile adapter: const RAM/frame/clock reads become an owned GameSnapshot.
Generic `runtime/mod_api.hpp` contains no Addams RAM locations. Manager/Lua know
only detached captures and capabilities, not Bus, game addresses or host pointers.
The existing framebuffer snapshot and render scheduler remain unchanged.

API v1 retains its manifest and pre-native on_update. API v2 negotiates two
permissions and engine/profile compatibility. Its on_update executes only after
native simulation, existing hooks/diagnostics and audio delivery. Rendering
reuses a frozen bounded UI bitmap without invoking Lua, reading live RAM, running
NES instructions or repeating audio/hooks. Disable/error removes the overlay.
No native ABI/header version, emitter, mapper, PAL calendar, RNG or cache change.

The first text capability uses the existing authored bitmap font/OpenGL UI path:
256×40 virtual strip within the 4:3 viewport, maximum 8 commands/40 characters per
mod. Bounds/type/font/NUL/nonfinite/context checks, transactional per-callback
publication, per-mod errors and sorted layering. Maximum 32 mods; the same Lua
allocator/VM budgets/blocked libraries apply. In-process restrictions are not an
OS sandbox or an independent security certification. No new dependency introduced.

No generic memory writes, physics setters, events, assets rendering, profiles,
config schema, dependencies/conflict resolver, transactional hot replacement,
randomizer, speedrun completion detection or full application main menu is
claimed implemented. These have phased acceptance criteria in the roadmap.
The existing Mods Back behavior remains: returns to game, or exits --mods-only.

## Real player observation and private evidence

Only the owner's existing local European ROM was used; SHA-256 matched the
configured supported profile. No search/download. Its existing locally generated
ARM64 ABI v3 module/receipt were checked and copied into separate private storage;
no regeneration required because its generator/native ABI contract was unchanged.

Private native tracing verified the actual operands used when constructing the
player sprite drawing references; the authored observer values are not actor-slot
guesses or invented coordinates. 19,088 sprite-coordinate formula checks per
2,400-unit run, zero discrepancies. Coordinates mean **NES screen player drawing
reference**, not world position, bounds/feet or an exact PPU fetch-phase sample.
Only ordinary initial-level movement is certified. Other levels, menus, death,
transitions, stairs/rope/special/attachment states return nil for player. State/
room fields are conservative raw-ID observations, not a full progression model.

Two deterministic private runs (baseline and actual installed/enabled Manager
HUD) each executed 2,400 native units. Per-unit cumulative comparisons covered
CPU registers/flags/SP/PC, all 2 KiB RAM, framebuffer, PCM float bits, clocks,
NMI/IRQ and DMA counts. CSVs were byte-identical; final digest was
8212612913687128091 in both. Each had 933 valid player captures and 1,467 explicit
unavailable captures; observed reference X=76..96, Y=102..160 with authored walking
and jumping inputs. No game resources, original instructions, ROM, CSVs, sprite
tables, modules, private paths or captures are included in public source/docs.
This is a limited native smoke/trajectory experiment, **not a completed game**.

## Executed verification (macOS ARM64 only)

Fresh Release desktop and headless builds, Apple Clang 21, macOS 27.0.1, SDL
2.32.8; real OpenGL 4.1 Metal. Deployment declared 15.0, not tested on macOS 15.

| Check | Executed result |
|---|---|
| Desktop CTest | **60 executed, 59 PASS, 1 FAIL (A02), 0 skipped** |
| Headless CTest | **54 executed, 53 PASS, 1 FAIL (A02), 0 skipped**; six SDL/loader-specific targets are not built by headless design |
| New mod_v2_contract | 41 checks PASS |
| New mod_v2_store_contract | 47 checks PASS (repeats the 41, plus six actual Manager install/enable/complete/disable/uninstall assertions) |
| New mod_v2_package_contract | 6 unittest methods PASS, multiple version/schema/capability/context cases |
| New mod_v2_native_contract | 5 native deterministic comparisons PASS: coupled 50, 60, 90, 120 and paused 120 against no-mod baseline |
| Explicit mod_v2_gl_checks | 13 real SDL/OpenGL assertions PASS: actual Frontend ZIP install/enable/Back, visible pixel readback, resize, fullscreen/return, frozen repeat draws, disable and --no-mods |
| Extracted authored ZIP/helper check | 11 subprocess transactions PASS, v1/v2 source-byte/CRC comparison; host Python, not all-platform bundled tools |
| Existing suites | 4,736 instruction executions, stack programs, 82 IRQ cases, PAL07/08, A01/A03, bus/$2002, devices, ABI/cache/importer, v1 mods, UI and high-refresh portable contracts PASS |
| Source/ZIP audit | All source, including untracked new files, passed public-content audit plus private ROM byte-marker check; demonstration ZIP has only three authored text members |
| Local copied application | strict ad-hoc signature PASS, ARM64/minos15.0/system-framework inspection; default resource lookup import-only exit 0 with verified cached ABI3; --no-mods 20-frame native smoke exit 0, no mod storage created |

The four new CTest entries account for 99 executed check/method comparisons per
configuration; the store entry deliberately reruns 41 core checks, so these are
not 99 distinct tests. The GL probe is explicit/display-dependent and not inserted
as a mandatory display requirement into headless/native CTest. Missing graphics
capability must be reported as not executed, never silently passed/skipped.

During development the CI-gate's synthetic JUnit fixture initially lacked the
four new entries' diagnostic strings. That **harness** failure was fixed by
extending its independent report fixtures, not weakening the gate or changing NES
expectations. Final gate inventories require all 60/54 entries. With full-output
JUnit they correctly reject A02 as the sole failure. CTest's default short output
can truncate an existing native diagnostic; reproduction commands below preserve
full output. No exemptions, exclusions or expected failures were introduced.

### Interactive private game demonstration

Installed the ZIP through the macOS file picker, explicitly enabled through the
panel, returned to the original language selector and entered the initial room.
HUD showed X76/Y160; walking/jumping changed it to X91/Y120. Actual game data, not
synthetic fixture data. F11 entered/left fullscreen with the HUD; a paused exit
confirmation kept frame16585 and X91/Y120 unchanged across separate observations.
Cancel/Back resumed correctly; Disable removed the HUD and normal play continued.
Clean exit reported GOD=0 and no private mod errors. This was 17,152 native units
including long idle/pause/intro periods, **not a full playthrough**. Physical
controller and audible quality were not independently certified; PCM equality
and device submission were observed, not a listening comparison.

## A02 and platform limits

A02 remains ordinary FAIL, unchanged: cycle33252, $4015=64, IRQ-after-read=0,
expectation1. Both full suites and the gate are red for that reason. No new final
CTest failures. The preexisting Linux/Xvfb high-refresh
`rendered pixel/letterbox/orientation mismatch` remains documented; not retested,
fixed or hidden here.

| Platform | This delivery |
|---|---|
| macOS ARM64 | Native desktop/headless/full CTest, actual graphical probe, private game HUD interaction and separate local app executed |
| macOS Intel | Portable targets/packaging checker prepared; no native run, no new package certified |
| Windows x64 | Same required CTest entries/static linkage and extracted-package helper verification prepared; no native run/package certified |
| Linux x64 / Steam Deck OLED | Portable contracts and shared package verification prepared; no native run, no AppImage/physical Deck certification; known graphics evidence failure retained |

The existing four-platform workflow builds all targets and invokes the extended
inventory/JUnit gate. Shared package verification now checks the bundled authored
HUD ZIP as well as v1. No workflows were dispatched and no other-platform old
result is presented as this code's result. Future authorized CI must check the
actual new SHA, all 60 entries, artifacts and bundled Python/architecture/signing.
The graphical probe requires a valid context and is run explicitly below.

## Files changed

- Contract/game adapter: new runtime/include/runtime/mod_api.hpp, mod_hud.hpp,
  runtime/src/mod_hud.cpp, game/include/game/mod_observation_v2.hpp.
- Host/VM/UI: runtime/include/runtime/mod_lua.hpp, mod_manager.hpp,
  mod_frontend.hpp, frontend_ui.hpp; runtime/src/mod_lua.cpp, mod_manager.cpp,
  mod_frontend.cpp, mod_panel.cpp, ui_bitmap.cpp; app/desktop.cpp.
- Packages: tools/mods/packages.py, make_debug_hud.py;
  examples/mods/gomez-debug-hud/{mod.json,main.lua,README.txt};
  tools/public/package_platform.py; tests/mod_distribution_checks.py.
- Tests/integration: CMakeLists.txt; new tests/mod_v2_checks.cpp,
  mod_v2_native_checks.cpp, mod_v2_gl_checks.cpp, mod_v2_package_checks.py,
  mod_v2_store_checks.py; tests/verify_ctest_suite.py and its own checks.
- Documentation: roadmap, API v2, this report; compatibility/security addenda in
  MOD_API_V1.md and MOD_SECURITY.md. Historical reports retained.

## Reproduce

From this worktree, use fresh writable build/storage paths. No ROM for public
checks. CMake uses its existing pinned Lua/SDL dependencies; no global installs.

```sh
cmake -S . -B /private/tmp/nes-v2-desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build /private/tmp/nes-v2-desktop --parallel 4
ctest --test-dir /private/tmp/nes-v2-desktop --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit /private/tmp/nes-v2-desktop/junit.xml
cmake -S . -B /private/tmp/nes-v2-headless -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build /private/tmp/nes-v2-headless --parallel 4
ctest --test-dir /private/tmp/nes-v2-headless --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit /private/tmp/nes-v2-headless/junit.xml
python3 -B tools/mods/make_debug_hud.py /private/tmp/Gomez-Debug-HUD-v1.0.zip
# Graphical desktop only; replace storage with a fresh unique private directory.
/private/tmp/nes-v2-desktop/mod_v2_gl_checks /private/tmp/nes-v2-gl-store \
  /private/tmp/Gomez-Debug-HUD-v1.0.zip
python3 -B tools/public/audit_public.py .
git diff --check
```

CTest is expected to exit nonzero for A02. On Windows use writable native paths
and the existing MSYS2/toolchain instructions; do not interpret cross-compilation
or dummy-video operation as an OpenGL verification.

## Delivery and simple Mac demonstration

Outside Git, relative to this worktree:
`../Gomez-Debug-HUD-v1.0/Gomez-Debug-HUD-v1.0.zip` (1,398 bytes).
SHA256: `1ad4d8cdc66a607b617b149998192ccdc926b2e0c3499e5fd02542a09698cfa3`.
ZIP: mod.json, main.lua, assets/README.txt. No game assets/code.

A separate local `NES Native Lab Mod Manager 2 Experimental.app` and
`Start-Debug-HUD.command` are supplied alongside the ZIP. The local launcher
references a new ignored private cache/store outside every public repository;
**do not distribute that launcher**. The .app contains only the frontend/tools,
headers and notices, not the game. It uses this Mac's existing Python and is not
a newly verified portable public distribution. It does not replace the beta.

1. Open Start-Debug-HUD.command on the owner's Mac.
2. F2 → Install Mod → choose Gomez-Debug-HUD-v1.0.zip.
3. Enable → Back. Select original game language and start the initial room.
4. Walk and jump; inspect X/Y, state, room numeric ID and completed frame.
5. F11 toggles fullscreen; Escape/Cancel pauses/resumes; HUD does not advance paused.
6. F2 → Disable → Back removes it. Start with --no-mods for complete managed bypass.

No real movement powers are added. The ZIP needs this new API v2 frontend; the
published v1 beta cannot execute it. The full manager remains future work.

## Next phase and acceptance limits

First verify this isolated branch on the four native runners/Steam Deck with
permission to publish later. Then implement dependencies/conflicts and atomic
updates/profiles, retaining v1 coverage, or extend independently verified player
contexts. Broader event/speedrun/gameplay APIs must wait for reliable observation
and precise pause/reset/ownership contracts. No arbitrary memory API or physics
patch is justified by this read-only HUD. Priorities/risks are in the roadmap.
