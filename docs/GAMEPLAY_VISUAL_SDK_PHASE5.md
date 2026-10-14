# Gameplay & Visual SDK — Phase5 implementation and verification

Date:2026-10-14. Native platform executed: macOS ARM64, Apple Clang Release,
OpenGL4.1 Metal. Local branch `feature/gameplay-visual-sdk-phase5`. Historical HEAD
`95c194afac4574be9bf8b495341517389a73988b`; actual delivered code also includes
recovered dirty SDK1–4B and this phase. No commit, push, merge, tag or release.
Published beta/main and other worktrees are not modified.

## Recovery and base

Read repository instructions/status, manager/API/security/SDK reports and actual
source. Chose the functional Phase4B worktree rather than the historical v1 commit
alone. Created an independent successor and copied its tracked diff and untracked
source/docs/fixtures. All106 recovered file SHA256 values matched before changes;
rechecking the original worktree found zero changed recovered files. Its status,
patch, worktree inventory and hashes were preserved outside the public source.

Audited `feature/super-gomez-v1` independently, with its original reports/tests.
Selectively reused only the owned SuperJump service adapter and authored STA/INC
fixture; replaced its old capability API integration rather than merging Lua,
manager, input, UI or entire branch. No external recompiler code copied.

## Delivered status

| Capability | Status | What is demonstrated |
|---|---|---|
| Bounded gameplay decision request | IMPLEMENTED, VERIFIED, EXPERIMENTAL | API2 `gameplay.jump`, integer0..12, exclusive owner, validated adapter |
| Super Gómez | VERIFIED on native private module | Real original jump58px →94px; no visual imitation or global acceleration |
| UI text/rectangles | IMPLEMENTED, VERIFIED | Frozen independent320×240 planes; existing HUD preserved |
| Speedrun HUD | VERIFIED session timer | SIM/REAL/PAUSED; reset/hide; not official splits or game completion events |
| Own PNG layers | IMPLEMENTED, VERIFIED | Validated loading, alpha/order/parallax, cached GL upload, cleanup |
| Mansion Parallax | VERIFIED, EXPERIMENTAL | Own authored foreground silhouette, explicitly synthetic camera |
| Three mods together | VERIFIED | Actual Frontend drop-install/Enable/Disable plus private native C gameplay |
| Super Speed / other rooms | BLOCKED/deferred | No approved safe mechanism beyond initial room jump |
| Actual game scroll /2.5D/background replacement | NOT IMPLEMENTED | No transparent NES pixels or invented camera integration |
| Windows/Intel/Linux/Steam Deck native execution | NOT RUN | Portable targets/tests prepared; no CI publication authorized |

Architecture and exact contracts are in [architecture](GAMEPLAY_VISUAL_SDK_ARCHITECTURE.md),
[gameplay API](GAMEPLAY_HOOKS_API.md),[UI API](UI_MODDING_API.md),
[visual layers API](VISUAL_LAYERS_API.md). Lua API versioning is separate from
module ABI3; the original module still loads without regeneration.

## Files changed specifically in Phase5

Recovered historical changes remain present; do not attribute all `git diff HEAD`
to this phase. Relative to recovered Phase4B:

- `game/include/game/super_jump.hpp`, `game/src/super_jump.cpp`: approved adapter,
  bounded request rather than boolean; all six services forwarded.
- `runtime/include/runtime/mod_api.hpp`, `mod_lua.hpp`, `mod_manager.hpp`,
  `mod_frontend.hpp`, `mod_clock.hpp`, `mod_visual.hpp`: permissions, clocks,
  owned images/commands, request/publication contracts.
- `runtime/src/mod_lua.cpp`, `mod_manager.cpp`, `mod_frontend.cpp`, `mod_panel.cpp`:
  bindings, exclusive ownership, validation, lifecycle, UI permission notice.
- `runtime/include/runtime/frontend_ui.hpp`, `runtime/src/ui_bitmap.cpp`:
  expose existing own5×7 text painter, no replacement UI framework.
- `runtime/include/runtime/gl_presenter.hpp`, `runtime/src/gl_presenter.cpp`:
  independent cached planes/upload counter, cleanup and composition.
- `app/desktop.cpp`: adapter chain, completed-unit clock/input capture, three
  non-NES keyboard gestures; original scheduler/order preserved.
- `tools/mods/images.py`, `packages.py`, `make_phase5_examples.py`: bounded PNG,
  declared own assets/permissions, conflicts/quotas, deterministic ZIPs.
- `CMakeLists.txt`, `tools/public/package_platform.py`: link/package current
  services/helper/examples, public portable targets.
- `tests/emit_super_jump_fixture.py`, `gameplay_jump_checks.cpp`,
  `gameplay_visual_checks.cpp`, `gameplay_visual_package_checks.py`,
  `gameplay_visual_native_checks.cpp`, `gameplay_visual_gl_checks.cpp`,
  `gameplay_visual_benchmark.cpp`, `verify_ctest_suite.py`,
  `verify_ctest_suite_checks.py`: new mandatory contracts,
  explicit real-context probe and host microbenchmarks.
- Three `examples/mods/` source folders and the five Phase5 documents; appended the current local status to
  `PROJECT_STATUS.md`.

No new changes to CPU/PPU/APU/bus/MMC1 implementations, emitter, C ABI headers,
PAL deadlines or high-refresh planner relative to the recovered base. Historical
Phase4B video/cartridge changes are retained, not rewritten. A02 source/expectation
is unchanged. ROM, receipt and existing ABI3 module SHA256 still match their
private validated identities.

## Synthetic execution and complete suites

Full suites use unfiltered CTest with complete JUnit output. Desktop80 entries:
79PASS,1FAIL,0omitted. Headless72:71PASS,1FAIL,0omitted. The only failure is
`apu_status_irq_collision_reference` (A02), not WILL_FAIL or disabled. Inventory
gate confirms all80/72 required executables; results gate deliberately fails for
A02. Existing82 IRQ cases,4,736 native instruction cases,A01/A03,NMI/DMA,PPU/$2002,
MMC1/RAM/bus,ABI/cache/importer,SDK1–4B and original4:3 regressions passed.

Four new mandatory entries per configuration:

- gameplay_visual_contract:39 direct Lua/clock/visual validations.
- gameplay_visual_store_contract:9 Python cases plus51 native Manager/direct assertions,
  including all three packages, one-owner conflict, failing-callback isolation,
  quotas, malformed ZIP/PNG, filters, permission/manifest arguments and cleanup.
- gameplay_jump_contract:78 assertions using current emitted/native C. Original
  disabled-chain equivalence, exact RMW offsets/order, poll/advance/boundary once,
 12-step limit, manual initiation, ceiling/death/room/loading/alternate guards.
- gameplay_visual_native_contract:5 differential rate/pause scenarios at50/60/90/120.
  CPU/registers/stack/RAM/raster/PCM/cycles/NMI/DMA match no-mod execution;
  presentation repetitions do not execute Lua or NES again.

Real SDL/OpenGL public probe:16PASS, actual F2/drop-install/Enable/Back/Disable,
three mods together, actual pixel readback, synthetic camera movement, unchanged
NES input pixels, resized window, fullscreen, paused repetition, new presenter,
texture upload invalidation and safe mode. It is an explicit executable requiring
an actual context, not a passing skip in headless CTest. Physical Bluetooth input
is NOT RUN; existing synthetic controller/manager navigation contracts passed.

Initial development compile mistakes and a test-fixture `/var` symlink path were
fixed within this phase. An initial source-order check failed after input sampling
was factored; original tested call placement was retained instead of weakening its
expectation. Initial JUnit default1024-byte truncation hid final count markers;
reran with65536 output limits already used in CI. After adding the new execution-count guards, the verifier’s independent synthetic report fixture also needed the four new markers; extended it and added incomplete new-count rejection cases, without relaxing old guards. Final gates report only A02.

## Private authorized real game execution

Used the already present owner PAL ROM (validated exact supported SHA256) and
already validated private native ABI3 module. No ROM download, interpreter,
regeneration, commercial source dump or on-disk resource patch. Private probes,
state CSV and screenshots stayed outside Git/public packages.

Each scenario executed2,400 original NES units, same scheduled controller input,
GOD off. Input traversed original menus/intro into the initial outdoor room;
NES A was held for the same full jump. The current Manager/helper/Lua/API/adaptor
chain was used, not a second physics engine.

| Scenario | Initial y | Minimum y | Ascent | Extensions | Later common landing y |
|---|---:|---:|---:|---:|---:|
| Original, installed but disabled |160|102|58px|0|120|
| UI+visual only |160|102|58px|0|120|
| Super+UI+visual |160|66|94px|12|120|
| Disable Super during ascent |160|87|73px|5|120|

Original and UI/visual traces matched exactly for all2,400 units, including rolling
hashes of every RAM byte, CPU state, NES indexed framebuffer, raw float PCM bits,
cycles/NMI/IRQ/OAM/DMC counts. Gameplay prefix matched through unit1904; first
intentional difference was1905 at the approved phase hold. After gameplay changes,
state/audio/trajectories may naturally diverge; identical hashes are not demanded.
Hash/finite-run agreement is not a mathematical or hardware certification.

The current three-mod/private-game chain was also run with virtual60/90/120
presentation calendars:2,878/4,317/5,756 repeated snapshot presentations across
2,400NES units. Each complete native trace matched the original-presentation
all-mod run exactly. These are virtual presentations, not physical swaps.

A separate actual SDL/OpenGL probe installed/enabled all three through Frontend,
executed the same game input, visibly rendered the timer, own silhouette and
raised jump; private capture was inspected. Its native state hash matched the
non-GL all-mod scenario. Timer REAL used measured monotonic host time and SIM the
completed-unit contract; this unthrottled run is not a PAL-speed FPS measurement.
Actual Frontend Disable cleaned all owners after execution. This is a brief real
native gameplay scenario, not a completed game/all-room collision certification.
No physical controller or listening test of audible output was conducted; PCM
was verified numerically and existing SDL audio tests passed.

The copied local application also launched outside the build, reused its verified
private import and exited cleanly after2NES units/GOD off. That copied-app test is
startup/shutdown only, not another gameplay claim. The final copied executable
also ran `--no-mods --frames 2 --unthrottled --mute` and exited0, without
loading managed packages. Full keyboard/focus/pause
modal behavior in the actual desktop executable still deserves a longer manual
session; automated clocks/UI/focus/audio/manager tests cover their contracts.

## Release microcosts and limitations

Seven batches, native macOS ARM64. Medians (microseconds/unit): authored two native
instructions original0.410, wrapper disabled0.297, jump enabled0.318; ordering,
cache/warmup/noise make these unsuitable for claiming a speedup or reliable delta.
Lua capture of rectangle/text/image0.491; CPU alpha composition37.620; VM
activation/unload with predecoded owned image11.045. Lua allocation14,786bytes,
example source image8,192bytes, one shared plane307,200bytes. Max images128KiB/mod,
Lua limit2MiB, two shared CPU and GPU planes when used. PNG/package process startup
is outside VM activation microcost; no per-frame PNG decode is performed.
Actual Manager Enable/Disable (three batches, medians, including Python/helper,
validation, Lua and captures): Super344.656/230.484ms; timer361.436/239.422ms;
image470.409/228.335ms. These transactions occur while the Mods UI pauses the
game, not in the render loop; reducing helper-process startup is future work.

Real GL host time including glFinish,100iterations: cached plane draw893.644µs;
changed-plane upload+draw1405.66µs at probe resolution. These include driver/GPU
synchronization, not isolated GPU timers, vblank, input latency or sustained game
FPS. Repeated captures did not increase actual upload count. No new thread added.
Native private2,400-unit wall times were3.508s original,3.700s visual,3.741s all,
3.966s disable (includes the mid-run helper transaction); these uncontrolled
single runs are workload evidence, not equivalent platform performance benchmarks.
Actual60/90/120Hz monitor delivery has not been measured for Phase5; virtual
presentations are not additional unique images or interpolated movement.

## Installable deliverables and Mac demonstration

Stable local delivery folder: sibling `Gameplay-Visual-SDK-Phase5-Demo/` outside
this public worktree. It contains the three ZIPs, checksums, local development
.app, `Start-Phase5.command` and detailed README. No ROM/module in the ZIPs.
The local launcher points to a separate private import; do not redistribute it as
a public package. It is not an update to beta0.9.0 or a notarized release.

| ZIP | SHA256 |
|---|---|
|Super-Gomez-Gameplay-v1.0.zip|d1114dbd01484f782455d8f5b218cd01a13546164a2fbd04bd190329bb35f3e4|
|Speedrun-HUD-v1.0.zip|ef3ff4587ae92dc15c11433b3f1bebc5d3acc0f78cddb9740302dc81c455397d|
|Mansion-Parallax-Demo-v1.0.zip|88ebf414b1cfdc8d94d7dfc7a4985e409f65a924b48291b882622712e7428571|

Start the local launcher, F2 > Install Mod for each ZIP, select/Enable each, Back.
Play first room with Z full jump; Disable Super, Back, compare a new jump.
Ctrl+Alt+R resets timer; Ctrl+Alt+U hides UI; Ctrl+Alt+V hides synthetic layer;
F11 fullscreen, Escape ordinary confirmation. `--no-mods` is the original path.
No-mod fresh session and explicit per-owner unload do not edit ROM/cache/save.

## Reproduction

```sh
phase5_build=$(mktemp -d /private/tmp/nes-phase5-recheck.XXXXXX)
cmake -S . -B "$phase5_build/desktop" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$phase5_build/desktop" --parallel 4
cmake -S . -B "$phase5_build/headless" -DCMAKE_BUILD_TYPE=Release -DNES_ENABLE_DESKTOP=OFF
cmake --build "$phase5_build/headless" --parallel 4
ctest --test-dir "$phase5_build/desktop" --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$phase5_build/desktop.xml"
ctest --test-dir "$phase5_build/headless" --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$phase5_build/headless.xml"
python3 -B tools/mods/make_phase5_examples.py "$phase5_build/zips"
"$phase5_build/desktop/gameplay_visual_gl_checks" "$phase5_build/new-gl-store" "$phase5_build/zips"
"$phase5_build/desktop/gameplay_visual_benchmark"
```

Use a real graphical session for the explicit GL probe. Commands/tests are portable;
replace mktemp path convention on Windows. Export only reviewed public source for
`tools/public/audit_public.py`; never audit/package the private sibling caches as
public input. Public source audit with private-ROM markers passed; final ZIP
contents comprise only authored manifest/Lua/README/own PNG. `git diff --check`
passed. No staging or remote mutation. Diagnostics now retain a managed gameplay assistance
flag after a jump request has been activated, including after Disable; visual-only
mods do not set it. This prevents falsely classifying a modified trajectory as an
original run. The pure latch and actual desktop call placement are tested.

## Remaining work

1. Longer manual Mac session: pause/focus, timer reset/hide, fullscreen, F2,
   keyboard and real controller; repeat collision/ceiling/landing scenarios.
2. Native Windows/Intel/Linux CI when separately authorized, plus physical Deck.
   Historical Linux high-refresh GL discrepancy remains OPEN, not fixed here.
3. Expand approved game-state coverage before enabling jump in other rooms or
   adding speed. No speculative movement/state writes.
4. Identify real semantic camera/room transitions before Scene API/2.5D.
5. Settings/dependencies/splits/persistence beyond existing manager remain future
   capabilities. A02 remains an independent visible hardware-contract problem.
