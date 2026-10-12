# Original presentation — preparation for 1.0

Date: 2026-10-12. **Presentation change implemented locally; the port is not certified for a stable 1.0 release.** Version remains 0.2.1, module ABI remains 3. No commit, push, tag, release or remote workflow was performed.

## Source baseline and preservation

Checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` in the existing workspace. Branch `verify/phase-3-1-bus-timing`, HEAD `404d138de628a00e9e1e9cb3fcc1e889a1c5a32c`. Results below concern the working tree changes, not the unchanged HEAD binary or existing published packages.

Initial pending work was `docs/ABI_V3_PACKAGING_CI_FIX.md` plus five untracked reports: `ABI_V3_FINAL_PLATFORM_STATUS.md`, `ABI_V3_FOUR_PLATFORM_CI.md`, `DISTRIBUTION_FOUR_PLATFORM_CI.md`, `NESRECOMP_COMPARISON.md`, `RELEASE_1_0_READINESS.md`. All six were preserved byte-for-byte against the initial SHA-256 snapshot. The older readiness report remains historical and has not been overwritten.

## Rendering audit and dependency decisions

| Component | Previous role | Decision for original presentation |
|---|---|---|
| `runtime/src/video_state.cpp`, `VideoState::frame()` | Original 256 × 240 palette-index PPU framebuffer, original scroll and sprites | Unchanged; now the direct frontend image source |
| `runtime/src/gl_presenter.cpp`, `GLPresenter::present` | Palette conversion; nearest filtering; fixed 16:9 viewport; variable-size textures | Fixed original framebuffer, full quad in a centered 4:3 viewport; same palette/filtering |
| `game/src/world_view.cpp`, `WorldView::compose` | 427 × 240 composition; center original pixels at x=85; reconstruct outside-map columns and lateral actors | Removed from application call chain and application linkage; retained unchanged in test-only library |
| `game/src/actor_view.cpp`, `ActorView::services` | Observe RAM/OAM/boundaries for extended actor poses; forward ABI services | Removed from application service chain; original tests still compile and exercise this observer |
| `game_world_maps`, level-13 continuation tables and `emit_world_maps.py` | ROM-specific extension/map resources and import exports | Retained unchanged for ABI/import compatibility and existing research/tests; unused by the original presenter |
| Original camera/scroll | Game RAM and PPU register writes drive visible NES image | Unchanged; no host camera, world-coordinate reconstruction or additional sprites |
| `app/desktop.cpp` window | 960 × 540; resizable/high-DPI; no explicit fullscreen control | Initial 960 × 720; still freely resizable; F11/`--fullscreen` for desktop fullscreen |
| PAL pacing/audio | Advance original execution to the next PPU frame; drain APU; host deadline at 50.007 Hz | Same execution loop, rate and audio path; existing deadline calculation extracted for deterministic testing |
| GOD/diagnostic | Alpha started with GOD active and unconditional G/H shortcuts | Normal startup is unassisted; G/H work only after explicit `--god` or `--diagnostic`; implementations unchanged |

`user_game_view` now contains GOD and diagnostics only. `experimental_world_view` contains the unchanged WorldView and ActorView sources and is defined only with `BUILD_TESTING=ON`. Existing `cpu_irq_edge_checks` and `import_probe` link it explicitly. Thus the previous experimental probe and adapter tests remain available without putting the compositor in the shipped frontend. `nm -C` of the newly linked macOS frontend found no WorldView or ActorView symbols (search exit 1, no matches). No experimental widescreen command, fallback or hidden presentation toggle remains.

The importer still authenticates and emits all existing ABI resources, including maps. Removing these exports would be an ABI/importer change outside this task. Keeping unused compatible exports does not activate extension. No mapper, CPU, PPU, APU, interrupt, DMA, game-rule or generator file changed.

## Presentation contract

* Source is exactly 256 × 240, including all rows and columns. Extended, cropped, zero-sized or null input is rejected before reading or altering conversion output.
* Display is exactly 4:3. Mapping the full image to 320 × 240 display units gives a 5:4 horizontal pixel aspect. This implements the requested display convention, not a measurement/certification of analog PAL CRT geometry.
* Windowed mode prefers whole multiples of 320 × 240, preserving integer vertical scaling. With non-square pixels, horizontal source pixels need not occupy a uniform integer number of output pixels; exact 4:3 takes precedence over square-pixel scaling. Nearest filtering avoids blending.
* Fullscreen and windows too small for 320 × 240 use the largest exact 4:3 integer rectangle fitting the drawable (rounding by at most three horizontal/two vertical pixels). The image is centered and remaining pixels are black. Minimized/tiny drawables produce no draw.
* Drawable pixels, rather than logical window points, determine the viewport. This handles Retina scaling as required by [SDL_GL_GetDrawableSize](https://wiki.libsdl.org/SDL2/SDL_GL_GetDrawableSize).
* Fullscreen uses `SDL_WINDOW_FULLSCREEN_DESKTOP`, retaining the desktop resolution, per [SDL_SetWindowFullscreen](https://wiki.libsdl.org/SDL2/SDL_SetWindowFullscreen). The game framebuffer is never resized.
* On this Mac, the initial Cocoa Spaces transition returned -1 with an empty SDL error after successful windowed checks. Selecting desktop borderless fullscreen via `SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES=0` made the same strict checks pass. This setting is applied before window creation in the frontend and graphical probe, only on macOS. [SDL documents this policy](https://wiki.libsdl.org/SDL2/SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES): it also removes the native fullscreen titlebar button; F11 remains available. No assertion was relaxed.
* Presentation cannot advance CPU/APU/PPU. The frontend still executes to a PPU frame and uses the pre-existing 50.007 Hz deadline and 100 ms late-host rebase. It does not use monitor Hz or vsync as a game clock. This task does not claim wall-clock/speaker drift measurements.

Examples in physical drawable pixels:

| Drawable | Windowed picture | Fullscreen picture |
|---|---|---|
| 960 × 720 | 960 × 720 | 960 × 720 |
| 1920 × 1080 | 1280 × 960, centered | 1440 × 1080, centered |
| 800 × 600 | 640 × 480, centered | 800 × 600 |
| 600 × 900 | 320 × 240, centered | 600 × 450, centered |

## Modified and added files

Modified: `CMakeLists.txt`, `app/desktop.cpp`, `runtime/src/gl_presenter.cpp`, `runtime/include/runtime/gl_presenter.hpp`, `tests/verify_ctest_suite.py`, `tests/verify_ctest_suite_checks.py`, `README.md`, `README.es.md`.

Added: `runtime/include/runtime/original_presentation.hpp`, `runtime/src/original_presentation.cpp`, `tests/presentation_checks.cpp`, `tests/presentation_gl_checks.cpp`, this report.

No production NES logic or experimental source was rewritten. No dependency was added. Official icons and distribution scripts remain unchanged. No private game data was added to the repository.

## Executed verification

Host: macOS 27.0.1 ARM64, AppleClang 21, vendored SDL 2.32.8, OpenGL `4.1 Metal - 91.7`. Deployment floor remains macOS 15.0; execution on macOS 15 or Intel was not performed here.

Both builds used new directories under `/private/tmp/nes-original-presentation-tj5sp9gi`, not previous binaries. All existing targets, including generated synthetic-C fixtures and adapter/import probes, compiled. Desktop and headless retained the default Lua configuration.

| Verification | Executed / passed / failed / skipped | Result |
|---|---|---|
| Desktop complete CTest | 37 / 36 / 1 / 0 | Sole failure: original A02 |
| Headless complete CTest | 34 / 33 / 1 / 0 | Sole failure: original A02 |
| New portable presentation groups per configuration | 32 / 32 / 0 / 0 | 18 geometry, 7 pixel, 7 pacing groups |
| Real SDL/OpenGL probe on host | 10 / 10 / 0 / 0 | Initial window, four resizes, fullscreen entry/exit, rejection of 427-wide image, clock immutability, final wide-window capture |
| Native generated-C instruction cases per configuration | 4,736 passed | Existing register/flags/bus/cycle contracts |
| IRQ cases per configuration | 82 passed | PAL 07/08, masks, branches, edges, NMI/DMA controls, all adapter chains |
| Source export audit | Exit 0 | `audit_public.py`; no new game assets/modules/ROM in source tree |
| Whitespace and pending-file preservation | Exit 0 | `git diff --check`; all six initial hashes unchanged |

The geometry group contains 17 authored rectangle cases plus a 3,990-combination resize/bounds/aspect sweep counted as one group. Pixel checks verify every converted source pixel against the unchanged palette and frame immutability, and reject five malformed/extended inputs. Pacing checks use different presentation counts (50/60/120/144), the existing PAL period and long-pause behavior. They establish code-level clock independence, not a timed real-display benchmark.

The graphical probe uses only authored colored quadrants and synthetic cartridge data. It reads back actual OpenGL pixels/viewport, checks black bands/orientation, rejects an extended frame, and verifies that 180 repeated presentations leave bus cycles, APU cycles, PPU frame count and framebuffer unchanged. A synthetic capture was visually inspected: centered 4:3 quadrants with black sides. This is automated real-graphics execution and visual inspection of its output, not a human gameplay test. The display-dependent probe is an explicitly runnable build target, outside mandatory CTest, because headless CI can lack an OpenGL display. The three portable presentation contracts are mandatory in both CTest inventories and the existing four-platform inventory gate.

The first graphical attempt in the restricted environment exited 1 with `The video driver did not add any displays`; access to the host graphical session enabled windowed rendering. The initial native Spaces attempt also exited 1 (`Enter fullscreen:` with empty SDL message). The final probe with the documented borderless policy exited 0. These initial failures are retained as evidence, not counted as passing attempts.

The first CTest reports used the default 1,024-byte passed-output limit, preventing execution-count validation for some IRQ cases. Both suites were rerun with the existing CI 65,536-byte limits. Their final inventories and JUnit execution sets match completely; the verifier exits 1 **only for A02**, not for missing counts or skips. Existing CPU, stack, RAM, MMC1, bus, PPU/$2002, A01, A03, audio queue, diagnostics, importer/cache/ABI and IRQ/DMA contracts pass. This does not expand hardware coverage beyond those tests' established limits.

A02 remains `apu_status_irq_collision_reference`: at PAL cycle 33,252, status=64, IRQ after read=0, expected=1. The assertion, labels, expectations and runtime are unchanged. No WILL_FAIL, DISABLED, skip rule or CI filter was added.

### Private local startup

The locally supplied compatible ROM was authenticated by the unchanged import profile, and a fresh native ABI v3 ARM64 module was generated into a private temporary cache. The existing user cache contained ABI v2; it was read only and preserved rather than replaced. The fresh desktop frontend loaded the private ABI v3 module and rendered 250 frames, reaching the language selector. Exit 0, `God mode=0`, `blocked_damage=0`, `rescues=0`. Its private capture was visually inspected at 1920 × 1440 drawable pixels (960 × 720 logical window).

This verifies actual ROM import/module loading, startup, language-selector rendering and default unassisted state on this Mac. It **does not** verify movement, room transitions, death recovery, a completed normal playthrough, speaker audio or Windows/Linux gameplay. The run used `--unthrottled --mute`; PAL core timing remained the original code, but this run is not an audiovisual pacing measurement. ROM, generated module, logs and game capture remain outside Git and public artifacts.

## Reproduction

From the checkout:

```sh
cmake -S . -B /private/tmp/nes-original-presentation-tj5sp9gi/desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build /private/tmp/nes-original-presentation-tj5sp9gi/desktop --parallel 3
ctest --test-dir /private/tmp/nes-original-presentation-tj5sp9gi/desktop --verbose --output-on-failure --no-tests=error --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit /private/tmp/nes-original-presentation-tj5sp9gi/desktop-results.xml

cmake -S . -B /private/tmp/nes-original-presentation-tj5sp9gi/headless -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build /private/tmp/nes-original-presentation-tj5sp9gi/headless --parallel 3
ctest --test-dir /private/tmp/nes-original-presentation-tj5sp9gi/headless --verbose --output-on-failure --no-tests=error --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit /private/tmp/nes-original-presentation-tj5sp9gi/headless-results.xml

# Requires a graphical session; contains no game data.
/private/tmp/nes-original-presentation-tj5sp9gi/desktop/presentation_gl_checks /private/tmp/nes-original-presentation-tj5sp9gi/synthetic.ppm
python3 -B tools/public/audit_public.py .
git diff --check
```

CTest exit 8 and verifier exit 1 are currently expected observations of the unresolved ordinary A02 failure, not an expected-failure test setting. Evidence files include `desktop-ctest.log`, `headless-ctest.log`, corresponding inventories/JUnit/gate logs, fresh-build logs and `gl-final.log` in the temporary evidence directory. Private import/startup evidence is separate and must not be copied into public deliverables.

Normal local play uses `nes_native --rom <own-compatible-ROM> --import-tools <tools/recompiler> --import-cache <private-cache>`. No debug flags are necessary. Use `--fullscreen` or F11 for fullscreen; resizing is unrestricted. Diagnostic play without aids uses `--diagnostic --no-god` and must avoid G/H. `--god` explicitly enables the retained test aid.

## Platform and release readiness

| Platform | Current change executed here | Still required |
|---|---|---|
| macOS ARM64 | Clean desktop/headless builds; full suites; real GL resize/fullscreen/readback; private language-selector startup | Manual normal gameplay/room transitions and audible pacing; macOS 15 run; final signed/extracted-package check |
| macOS x86_64 | Not executed | Native build, full suites, graphical probe and extracted-package check |
| Windows x64 | Not executed | Native build, full suites, resize/F11/HiDPI graphical check, extracted portable-package and normal gameplay check |
| Linux x64 / Steam Deck | Not executed | Native build/full suites, GL fullscreen/resize, extracted and direct AppImage checks; reproduce or resolve the previously reported ROM-load closure |

The existing workflow builds all desktop targets and runs unfiltered CTest on four native hosts. Updating the mandatory inventory includes the three new contracts for the next authorized run. No remote run was triggered and no older CI result is evidence for this working tree.

Minimum remaining 1.0 gates: (1) resolve or independently establish the contract of A02 without hiding its failure; (2) perform a normal unassisted full-game verification including transitions, deaths/progression and sound; (3) verify these changes and final packages natively on all four architectures, including the unresolved Linux loading report; (4) verify first-open, fullscreen/input/focus and packaging integrity with consistent source/ABI/version and retained licenses. The presentation change removes the experimental exposure path; it does not prove the entire port matches original hardware. Keep the release status experimental until these gates have evidence.
