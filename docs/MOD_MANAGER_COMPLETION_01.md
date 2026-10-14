# Mod Manager Completion01 — consolidated local delivery

Date2026-10-14. Native executed platform:macOS ARM64, Apple Clang Release,
OpenGL4.1 Metal. Branch feature/mod-manager-completion, historical HEAD
95c194afac4574be9bf8b495341517389a73988b plus recovered uncommitted SDK source.
No commit, push, merge, tag, release, main or public beta modification.

## Recovery and single source of truth

Inspected all ten worktrees. Chose feature/gameplay-visual-sdk-phase5, not the
historical v1 commit alone. Created a native independent successor, applied its
tracked binary diff and copied all untracked source/docs/fixtures.136 recovered
file SHA256 values matched. The original Phase5 worktree still matches those136
hashes; earlier SDK and Super Gómez worktrees are unchanged. No mobile/PortMaster/
PAL-NTSC or unrelated scene work was incorporated. Initial patch/status/hash
inventory and final evidence remain outside public source in a private temporary
completion folder. Existing reports were preserved, with append-only current API
and security notes.

One Manager, one helper and one state.json remain authoritative. Main-menu and
game windows own sequential instances of the same frontend/backend, never two
concurrent Managers/markers or a parallel database. Main-menu Manager unloads
before the existing importer/game creates its instance; its Lua callbacks receive
no live game state. Main-menu activation is not a simulation unit. No extra Lua
callbacks on host presentation frames.

| Capability | Status | Evidence/limit |
|---|---|---|
| ZIP installation/manifests | IMPLEMENTED, VERIFIED | Existing malicious ZIP tests plus new settings/dependency validation |
| Lua API1/API2 + sandbox | IMPLEMENTED, VERIFIED | Full prior contracts retained; typed configuration is additive API2 |
| Memory/player observation | IMPLEMENTED, VERIFIED | Existing completed-snapshot/determinism tests; no side-effectful hardware reads added |
| Gameplay jump hook | EXPERIMENTAL, VERIFIED locally | Same approved first-room adapter, unchanged code; no Super Speed/new physics |
| HUD/PNG layers | IMPLEMENTED, VERIFIED | Existing16-context checks retained and current three-mod private run |
| PRG/text resources | IMPLEMENTED, VERIFIED in prior SDK/current regressions | Preserved without expanding translations or CHR |
| Main menu/details/config/profiles | IMPLEMENTED, VERIFIED | New31 Python/29 navigation contracts,43 native integration checks,30 actual SDL/GL checks |
| Persistence/unclean-session recovery | IMPLEMENTED, VERIFIED | Existing plus new restart/dependency/unload-error tests |
| Forced safe mode | IMPLEMENTED, VERIFIED | No Manager/helper/store/scripts; prior native rate/no-mod contracts |
| Windows/Intel/Linux/Steam Deck execution | NOT RUN | Existing four-platform CMake/CI matrix prepared, not dispatched |
| True scene depth/2.5D/semantic parallax | BLOCKED/deferred | Not delivered by decorative PNG camera |
| A02 | Known ordinary FAIL | Same test and expectation, no exemption |
| Historical high-refresh Linux GL discrepancy | OPEN, NOT RUN here | Not fixed or reclassified |

## Functional interface

Normal interactive launches open Play/Mods/Settings/Exit before ROM import. Play
uses the unchanged SHA-validated importer. Explicit automated frame/unthrottled/
capture/audio-output/import-only commands retain their direct path. Settings
selects fullscreen/mute for this launch, not a new persistent system-settings DB.

Installed retains existing Install ZIP/Enable-or-Disable/Uninstall/Back actions.
D/X opens scrollable Details, C/Y Configure, P/left shoulder Profiles; equivalent
mouse navigation strip. Details includes category (declared or permission-derived),
full metadata, state, API, permissions, dependency version ranges, conflicts and
errors. Keyboard/D-pad/A/B work; focus and repeat protections remain. Gamepad name
editing uses X delete/Y append and Up/Down character cycling, without requiring
mouse. Analog-stick navigation and polished system onscreen keyboard are pending.

Back root context: from launch Mods to main; from in-game Mods to game; from
--mods-only closes diagnostic window. Nested Back returns to Installed. F2 during
play, original external pause/audio barrier, Escape confirmation and F11 are
preserved. Main menu Quit has no running game to interrupt. Physical Alt+F4/Cmd+Q/
Deck controller verification is pending; unchanged underlying exit contracts pass.

## Configuration and profiles

Optional API2 metadata: category/dependencies/conflicts/settings. Exact legacy
v1 and API2 manifests remain valid; no package migration required for the three
Phase5 ZIPs. Native module ABI3, C emitter and PAL planner unchanged. A package
using new metadata requires this Completion host; older helpers reject unknown
fields. Configuration/API versioning is separate from native module ABI.

Eight options maximum: boolean, bounded integer/float with step, enum with bounded
unique choices. Unknown keys/types/out-of-range/nonfinite values fail before
persistence. config.get returns typed detached values; no paths, file API or RAM
writes. Applying options restarts that mod at paused UI boundaries. Unload failure
is reported and conservatively disables it, rather than hidden or auto-retried.

Sixteen user profiles plus immutable Original. Create/save, rename, apply; names
are bounded labels. Profile stores enabled IDs and typed per-mod settings in the
same state.json management/schema1 section. Dependency ranges/cycles/conflicts,
existing jump/resource ownership, visual quotas, target compatibility and config
values are checked before atomic JSON replace. Apply requires two UI activations
and review of package permissions in Details. No auto-download or script execution
in inspection/installation/profile validation.

Original disables packages without deleting them/settings; --no-mods bypasses
storage/helpers entirely and also the legacy trusted script. Original does not
cancel explicit --mod/--god developer flags. Validated state commit is atomic;
subsequent Lua startup is not an all-VM atomic guarantee. Failing scripts and
required dependents deactivate with visible errors. Disabling a modifier does
not undo an already changed jump trajectory.

Legacy enabled/errors state is read compatibly. Older pre-Completion hosts cannot
read the added management section; the local demo uses its own --mods-dir. No
silent downgrade/reset of user state. Individually corrupted manifests/entries
become disabled error rows without blocking other packages; unsafe root/identity,
globally invalid state and stale locks still fail closed for manual review.

## Tests and actual results

Final complete unfiltered suites, with JUnit and required inventory/result gates:

| Configuration | Executed | PASS | FAIL | Omitted/skipped |
|---|---:|---:|---:|---:|
| macOS ARM64 desktop Release |82|81|1 A02|0|
| macOS ARM64 headless Release |74|73|1 A02|0|

CTest exits8. Both result gates deliberately exit1 solely for
apu_status_irq_collision_reference. Existing82 IRQ cases/4,736 native instruction
executions, CPU/bus/PPU/$2002/APU/A01/A03/NMI/DMA/MMC1, ABI/cache/importer,4:3 and
high-refresh determinism contracts ran and passed. No filters/exemptions/new
expected failure. Two new mandatory entries: mod_completion_navigation (29
native assertions) and mod_completion_store (31 Python tests plus43 native
assertions; includes the29 shared navigation assertions, not43 additional unique
navigation cases). Verifier independent fixtures include these actual counts.

New actual SDL/OpenGL probe30PASS: main-to-Mods/controller Back, real install of
three ZIPs, Enable, Details, Configure, Save/Apply Original/restore combined
profile, actual fourth package Configure→Apply changes request6→7, launch
settings, Play, safe-mode Back, resize/fullscreen and immutable NES input pixels.
The existing three-mod OpenGL probe16PASS also ran. Actual context is required;
these explicit probes are not CTest passing skips. Synthetic gamepad events do
not certify physical Bluetooth/Deck input.

Development failures were investigated: a test traversed a vector invalidated
by its own Manager calls; typed config initially pushed integer settings as Lua
floating numbers (fixed with actual lua_pushinteger, no expectation change).
A restricted SDL run lacked macOS service access; unrestricted native suite
passed. Added unload-failure diagnostics and tests before final suites.

## Private actual native game and demo bundle

Reused the already validated owner ROM/module, without regeneration/interpreter/
on-disk patches. Final current libraries installed/enabled all three unchanged
ZIPs through Frontend, then executed2,400 original NES units with scheduled input
and GOD off. Minimum player y66,12 bounded extensions, later landing y120; original
baseline from Phase5 is58px ascent, enhanced94px. All2,400 trace rows matched the
Phase5 all-mod trace exactly, including CPU/RAM/raster/PCM/cycles/interrupt/DMA
rolling state. Actual rendered timer, Super jump and own decorative silhouette
were inspected privately. Frontend Disable removed all owners/requests after
execution. No commercial screenshot/trace/module is copied into public source.
This is a brief automated real-native scenario, not a full manual playthrough,
all-room collision check or audible-output/physical-controller certification.

The copied local .app first failed because developer CMake alone had not copied
importer resources. Completed only the local demo assembly with public importer,
headers/docs/licenses. Retest outside build reused the exact existing receipt/
module identity, ran2NES units under --no-mods/mute/GOD off and exited0. No ROM or
module is in the app. This startup test does not certify a full game session.

Local deliverables: sibling Mod-Manager-Completion01-Demo. Local .app, two launch
commands (main menu/manager-only with isolated storage), guide, four authored ZIPs
and SHA256SUMS. Native ARM64 only; depends on Python3 already on this Mac, not a
newly verified public portable distribution, signed/notarized release or beta
update. The existing public packager now includes the configuration example;
no Windows/Linux/Intel packages were built in this delivery.

| ZIP | SHA256 |
|---|---|
|Super-Gomez-Gameplay-v1.0.zip|d1114dbd01484f782455d8f5b218cd01a13546164a2fbd04bd190329bb35f3e4|
|Speedrun-HUD-v1.0.zip|ef3ff4587ae92dc15c11433b3f1bebc5d3acc0f78cddb9740302dc81c455397d|
|Mansion-Parallax-Demo-v1.0.zip|88ebf414b1cfdc8d94d7dfc7a4985e409f65a924b48291b882622712e7428571|
|Configurable-Jump-Example-v1.0.zip|7e4bdc048493bd1bd6b63abe322fa6340465cf344ff23833fc7df7c4e55e783b|

## Changed paths relative to recovered Phase5

- app/desktop.cpp: pre-import interactive menu; automation/game schedule untouched.
- runtime/include/runtime/mod_{api,manager,lua,panel,frontend}.hpp and corresponding
  runtime/src/mod_{manager,lua,panel,frontend}.cpp: additive metadata/config,
  profiles, navigation, lifecycle/error isolation and same helper transport.
- tools/mods/packages.py: strict schema/store validation, typed values, profiles,
  dependencies/conflicts, recoverable bad-package listing; no Lua execution.
- tools/mods/make_completion_example.py and examples/mods/configurable-jump/*.
- CMakeLists.txt, tools/public/package_platform.py: portable targets/resources.
- tests/mod_completion_{checks,gl_checks}.cpp, mod_completion_store_checks.py,
  verify_ctest_suite.py and its independent fixtures.
- This report/roadmap/user guide, appended MOD_API_V2/MOD_SECURITY/PROJECT_STATUS.

Historical recovered changes are not new Completion changes. No new changes to
core NES implementations, ABI/C headers/emitter, SuperJump adapter, PAL clocks,
renderer or original ZIP contents. Git diff --check and source export audit
passed. Hash/evidence inventories outside Git permit comparison to dirty Phase5.
Technical content checks are not legal certification.

## Reproduce and continue

```sh
cmake -S . -B <private-build>/desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build <private-build>/desktop --parallel 4
cmake -S . -B <private-build>/headless -DCMAKE_BUILD_TYPE=Release -DNES_ENABLE_DESKTOP=OFF
cmake --build <private-build>/headless --parallel 4
ctest --test-dir <private-build>/desktop --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit <private-build>/desktop.xml
ctest --test-dir <private-build>/headless --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit <private-build>/headless.xml
python3 -B tools/mods/make_phase5_examples.py <private-build>/zips
python3 -B tools/mods/make_completion_example.py <private-build>/zips/Configurable-Jump-Example-v1.0.zip
<private-build>/desktop/mod_completion_gl_checks <fresh-private-store> <private-build>/zips
```

Use a real graphical session for the GL executable; do not claim a skip as PASS.
Next delivery: longer manual Mac/Deck interaction and authorized four-native CI;
profile export/delete/update confirmations and UI polish before broader release.
No further physics, scene renderer, resource research, Internet updater or OS
sandbox claim. A02 and historical Linux GL discrepancy remain visible/open.
