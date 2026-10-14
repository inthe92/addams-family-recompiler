# Modding SDK v2 — first functional component and verified results

2026-10-13. **Functional local development delivery, not published or a complete
SDK.** Branch feature/modding-sdk-v2, HEAD/base
95c194afac4574be9bf8b495341517389a73988b; recovered and new changes uncommitted.
No commit/push/merge/tag/release. Published beta and all original dirty worktrees
preserved. Recovery, architecture, API, security and roadmap are separate docs
with this MODDING_SDK_V2 prefix.

## Delivered

Reused the completed API2/HUD work instead of recreating it. Added generic
side-effect-free canonical RAM captures, read_u8/read_range and on_load range
observers accessible in Lua. Permissions memory.read and memory.observe are
explicit; the latter requires the former. Integer/range/context/quota validation,
owned values, deterministic change order, missing-capture baseline reset, shared
VM budget, lifecycle/error cleanup and --no-mods bypass. No CPU instruction or
hardware hook; reads sample unit-final RAM, not every write. No writes exposed.

Two independent packages use the **same** API: RAM Change Logger and RAM Change
Counter (with the recovered text HUD). Different source/range/output, no C++
mechanic or second-mod-specific adapter. Package inspection/install never executes
Lua. Existing v1 ZIP/update semantics retained; v2 generic target/profile context
is checked, unknown requirements/permissions rejected. No native ABI3 change.

Callbacks compare snapshots at completed original NES units, before that mod's
on_update and after original native/hooks/diagnostic/audio. Pauses and repeated
50/60/90/120 presentation snapshots do not add callbacks/CPU/PCM. Canonical
$0000–$07FF only; mirrors, registers, cartridge/CHR/PRG/device memory rejected.
Max64 bytes/read,4 non-overlapping observers,128 watched bytes/mod. Observer batch
and on_update share the existing100000-instruction/20ms sampled guard. Lifecycle
cleanup removes registry refs/lists; no persistent modification to undo.

## Actual verification — macOS ARM64

Fresh Release SDK desktop and headless builds, macOS27.0.1, AppleClang21/SDL2.32.8.
No global dependencies added. The pinned existing Lua is reused.

| Check | Result |
|---|---|
| Full desktop CTest | **63 executed,62 PASS,1 FAIL=A02,0 skipped** |
| Full headless CTest | **57 executed,56 PASS,1 FAIL=A02,0 skipped**; desktop-only targets not built by design |
| New SDK RAM contract | **37 checks PASS**: permissions, nil snapshots, detached reads/deltas, baseline/unchanged notifications, unload,17 invalid ranges/arguments/lifecycle/quota cases, budget/memory faults and two authored Lua sources |
| New SDK store contract | **43 checks PASS**: repeats37 plus6 actual helper/Manager install/enable/disable assertions, negotiated permission masks12/14, no capture on install/after disable and HUD cleanup |
| New SDK native contract | **5 comparisons PASS**, generated own C fixture: original coupled,60/90/120 and paused120 vs no-managed-VM baseline; full CPU/RAM/raster/raw-float PCM traces, NMI/OAM DMA and no extra updates while drawing |
| Recovered tests | API1/API2/HUD/ZIP suites and all hardware,4736 instructions,82 IRQ, PAL07/08, A01/A03/$2002, devices, ABI/cache/import and high-refresh portable contracts PASS |
| Inventory/JUnit gate | Requires all63/57 entries; intentionally rejects A02, no skipped/expected failures |
| Extracted own examples | **21 helper transactions PASS** for v1 Hello, Debug HUD and both SDK ZIPs; source-byte/CRC matches; host Python only, not all-platform bundled interpreters |
| Copied experimental app | Strict ad-hoc signature PASS; default resources/cached ABI3 import-only exit0; both delivered SDK ZIPs installed/enabled,180 unthrottled native frames with both actual logs, normal unloads/no unclean marker, exit0; left Disabled for explicit user activation |

The37/43/5 counts are executed checks/comparisons, not85 independent tests: store
repeats37. Full suites include ordinary A02 failure; not all green. No original
expectation weakened, tests deleted or exemptions added. Initial build issue was
an unnecessary CPU-header dependency in the snapshot header; replaced with a
side-effect-free generic capture template, without core/ABI changes. Existing
macOS picker deprecation warning remains; unrelated refactoring not performed.

Only macOS ARM64 executed. Windows/macOS Intel/Linux compile/test/package paths
are prepared via portable targets/static Windows flags and expanded required
CTest/shared package verification. CI not dispatched. No packages for these
platforms certified. Physical Steam Deck, controller and visible monitor cadence
remain pending. The prior Linux/Xvfb high-refresh pixel/letterbox/orientation
mismatch remains unresolved/documented; not reclassified by these results.

### Private native confirmation

Owner ROM identity matched the supported PAL profile; reused validated local
ABI3 module, no new interpreter or commercial download. Both SDK mods installed
and enabled through actual Manager, then2400 native units with ordinary inputs.
Per-unit CPU/register/flags/SP/PC,2KiB RAM, framebuffer, raw-float PCM, clock,
NMI/IRQ and DMA trace CSV was byte-identical to the prior independently retained
no-mod baseline of the same unchanged runtime/inputs. Final digest
8212612913687128091,19088 player-sprite reference checks,zero mismatches. Private
logs confirmed both change callbacks and unload. CSVs/module/ROM/logs remain
outside every public worktree and ZIP. This is a trajectory/native smoke, **not a
playthrough**, hardware timing certification or subjective audio comparison.

### Measured host cost

mod_sdk_benchmark:7 serial batches ×10000 synthetic host units per mode, Release,
steady_clock, no ROM/native game/render/display/disks. Reproducible benchmark
source provided; checksum retains capture reads. Times in ns/unit:

| Mode | Minimum | Median | Maximum |
|---|---:|---:|---:|
| No SDK copy/callback baseline RAM read | 1.69 | 1.69 | 3.51 |
| 2KiB snapshot only | 1886.12 | 1938.56 | 2925.40 |
| Snapshot + unchanged64-byte observer | 2123.95 | 2140.57 | 2157.87 |
| Snapshot + changed byte notification | 2389.22 | 2413.54 | 2445.08 |

This microbenchmark isolates capture/VM observation. Mode0 is **not** a measured
full-game disabled-hook cost: source/functional tests establish no VM/copy/hooks
for --no-mods. No percentage speedup, monitor FPS or all-game performance claim.
Active example log flushing/filesystem and heavy scripts can cost more; those are
not covered by these numbers. No per-instruction Lua overhead introduced.

## Translation investigation — evidence and missing mechanisms

Read the private recovered text index, then checked the actual owner PAL bytes:
seven instruction/operand checks confirm pointer reads and PPU-address/data
writers. Three language-related complete nametable payloads each contain1024
bytes with FE terminator. One dialogue stream parsed into2 PPU segments/14 glyph
codes within its8-bit cursor. No payload, original dialogue or font image copied
here. Local evidence is retained privately. This is a verified **format/code
inspection**, not an executed translation or validation of every string.

The text model is not ASCII: custom glyph/tile codes, pointer tables, PPU-targeted
segments/high-bit controls and fixed nametable layouts. Historical transcription
suggests alphabetic low codes; the complete actual CHR/encoding mapping and
Spanish accented characters/Ñ/¿/¡ have **not** been certified. Text can appear via
background tiles, sprite text and graphics, so one RAM read/write API cannot
translate all presentation. Fixed layout widths, message cursor limits, CHR bank
sharing and control/terminator bytes constrain replacements.

Minimum generic future route (not implemented):
1. Validated revision-specific text profile generated/checked locally: symbolic
   resource IDs, data-only PRG bank/ranges, lengths/layout/controls and codec ID.
2. Generic immutable data-resource overlay for those reads, excluding executable
   opcode/immediate regions; preserve control bytes and initially payload sizes
   so original native instruction/timing paths remain testable.
3. Separate bank-aware CHR glyph override of **own** tiles through the graphics
   resource boundary; CPU bus wrapping does not intercept PPU CHR fetches.
4. Language package validation/consent/conflict policy, line overflow and missing
   glyph errors, activation at safe boundaries, removal of overlays on disable.
5. Synthetic codec/layout/control/CHR tests then private menus/dialogues and
   timing/input/PCM comparisons. Do not substitute invented text to claim success.

Illustrative future `assets/languages/es-ES.json` schema:

```json
{"schema":1,"profile":"addams-pal-text-v1","locale":"es-ES",
 "codec":"profile-defined","messages":{"symbolic-resource-id":"author's text"},
 "glyphs":{"U+00D1":"assets/own-enye.png"}}
```

This is a **design example**, not an accepted current resource/manifest schema or
an original game message. Today JSON assets beyond the manifest are rejected and
passive PNG storage is not rendered; typed allowlisting/decoder/resource APIs must
be implemented before this format works. No claim that a Spanish translation is
running. Backend/profile support will need one controlled engine change; after
that, authoring different language packages should need no per-mod C++ rebuild.

## Files and untouched boundaries

Recovered files are listed in MOD_MANAGER_2_PHASE_1.md. New SDK changes:
- runtime/include/runtime/mod_memory.hpp: bounded owned RAM capture.
- mod_api.hpp/mod_lua.hpp/mod_lua.cpp: permissions/read/range/observer registry,
  deterministic dispatch/shared budget and cleanup.
- mod_manager.hpp/.cpp, mod_frontend.hpp and app/desktop.cpp: capture demand and
  one completed-unit host integration; no change to presentation scheduling.
- tools/mods/packages.py, make_sdk_examples.py, tools/public/package_platform.py,
  tests/mod_distribution_checks.py: negotiation/own ZIPs/extracted verification.
- examples/mods/ram-change-logger and ram-change-counter: manifests/Lua/readme.
- tests/mod_sdk_checks.cpp, mod_sdk_store_checks.py, mod_sdk_native_checks.cpp,
  mod_sdk_benchmark.cpp; CMake targets and required inventory/report fixtures.
- Six MODDING_SDK_V2 documents and additive PROJECT_STATUS/API2 cross-reference.

CPU/PPU/APU/bus/mapper/emitter, include/recomp ABI, PresentationClock, GL presenter,
original private module and A02 test sources remain byte-identical to base. No
Super Gomez files merged. No commits/staged files or remote modifications.

## Reproduce and continue

```sh
cmake -S . -B /private/tmp/nes-sdk-desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build /private/tmp/nes-sdk-desktop --parallel 4
ctest --test-dir /private/tmp/nes-sdk-desktop --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit /private/tmp/nes-sdk-desktop/junit.xml
cmake -S . -B /private/tmp/nes-sdk-headless -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build /private/tmp/nes-sdk-headless --parallel 4
ctest --test-dir /private/tmp/nes-sdk-headless --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536
/private/tmp/nes-sdk-desktop/mod_sdk_benchmark
python3 -B tools/mods/make_sdk_examples.py /private/tmp/nes-sdk-mods
python3 -B tools/public/audit_public.py .
git diff --check
```

CTest exits nonzero for A02; preserve that evidence. Windows uses its existing
native MSYS2 paths/toolchain; do not claim native execution from cross-builds.

Delivery outside Git: `../Modding-SDK-v2-Demo/`:
- RAM-Change-Logger-v1.0.zip: SHA256
  0cfe7a2944c0fb775a8dea25881cbf3c36be60e0249ee7fba53838f829db7d35.
- RAM-Change-Counter-v1.0.zip: SHA256
  21ac8ac05b28c1f3e6fd3a78f4357fe43f9d18103e7d79616d916da00fb5d357.
- Checksums, local separate ARM64 SDK .app and Start-SDK-v2.command.

The local launcher references an ignored private owner cache/store outside every
public repo; **do not distribute the launcher/cache/logs**. .app/frontend/tools
contain no game. It uses existing Mac Python, not a newly verified portable public
release. Examples left installed Disabled after smoke for explicit Enable in F2;
new users can install the actual ZIPs. --no-mods bypasses both managed and legacy
paths. No controller/manual SDK gameplay certification beyond the180-frame smoke
and private native trajectory; recovered HUD's earlier interactive result retains
its prior scope.

Next: authorized native four-platform/Deck validation; then minimal typed
profile/text-resource backend with synthetic control/layout tests. Do not start
writes/randomizer/catalogue or replace Super Gomez without their own acceptance.
