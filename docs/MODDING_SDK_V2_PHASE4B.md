# Modding SDK v2 Phase 4B — bounded visual glyph composition

2026-10-14. **IMPLEMENTED and locally VERIFIED on macOS ARM64**, under the
explicit visual-only contract below. This is not general CHR replacement,
physical NES hardware certification or a complete Spanish translation.

## Recovery and source identity

Independent `feature/modding-sdk-v2-phase4b` worktree, historical HEAD
`95c194afac4574be9bf8b495341517389a73988b`. Recovered the complete dirty Phase 4:
tracked binary diff plus untracked authored source/tests/docs; **92 SHA-256
values matched before edits**. HEAD alone does not contain SDK Phases 1–4.
All previous worktrees remain intact. No Super Gómez/PAL-NTSC experiment merged,
no beta/main/version change, commits, staging, pushes, tags or remote workflows.

## Decision and consumer matrix

See [decision recorded before implementation](MOD_CHR_PHASE4B_DECISION.md).
The global reservation investigation still cannot prove a free tile. New
experiments instead establish a bounded **composition** backend with unchanged
original memory/fetches and original hardware opacity masks.

| Physical bank / consumer | Screens/evidence | Background | Sprites / CPU | Classification |
|---|---|---|---|---|
| 8 / existing letter tiles | Phase 4 HUD fetch trace | Verified, shared | Full consumers unknown | UNSAFE for global letter substitution |
| 14 / existing letter tiles | Inventory/menu font traces | Verified, shared | Full consumers unknown | UNSAFE for global letter substitution |
| 8/14 / blank tile 0 | Phase 4 actual fetches | Used | Broader consumers unknown | UNSAFE to treat as reserved |
| Other zero-filled candidates in 8/14 | No fetch in prior 1,800-unit trajectory | OBSERVED UNUSED only | UNKNOWN | No PROVEN SAFE reservation |
| Approved `inventory.heading` nametable consumer, bank 14 | Private actual field fetches, whole-field identity, live mirroring | Bounded composition verified | Original CPU/sprite sources unchanged by design and tests | PROVEN SAFE within the reviewed software visual contract; not a free CHR tile |

No absence from a short trajectory is promoted to a global safety proof. Physical
CHR reservation remains BLOCKED. The new consumer scope is **VRAM field identity**,
not guessed screen coordinates or a new unverified room/RAM-state address.

## Implemented pipeline and intentional visual divergence

`VideoState::draw_pixel` performs original background/sprite pattern accesses
and the original opacity, sprite-zero and sprite-priority decisions. Between
background fetch and sprite composition, `composition_pixel` may supply a
separate **display color index** for an approved text cell. It uses actual
`tile_v`, fine scroll, normalized nametable mapping, physical MMC1 pattern bank
and the trusted mirroring requirement. It does not postprocess an arbitrary
rectangle or use SDL/OpenGL overlays.

The whole current nametable field must equal the **original field bytes** from
the verified private module. Otherwise the substitution is inactive. Matches
are cached and invalidated on nametable writes, resource publication or mirroring
changes; the physical CHR bank is checked live, including mid-frame bank changes.
Only matching cells in the approved field are eligible. No new bus/PPUDATA reads
or extra CHR reads are performed for the mod during rendering.

**Original background opacity still controls sprite priority and sprite-zero.**
An authored glyph can have different transparency: that changes the display,
not the hardware mask. A behind-background sprite may therefore stay hidden
where the new glyph is transparent. This is deliberate, documented resource
presentation behavior; it is not ordinary physical CHR substitution. No new
collision mask, CPU flag or gameplay physics is inferred from the displayed Ñ.
The observed inventory header has no need for a new global font slot.

The schema2 mode does **not replace PRG text reads** either. It leaves text RAM,
nametable data and original game processing intact, composing the requested
translation privately. Schema1 PRG text replacements retain their previous
behavior. Do not activate both for the same field: exclusive resource ownership
rejects that conflict before state persistence.

At publication, `Cartridge::set_text_resources` validates the entire list and
source CRCs, copies required base-font tiles from private CHR into owned bounded
buffers, then replaces the active list once. Own glyph cells replace those
prepared display tiles only. Original CHR/ROM/module are never mutated or exported.
`Frontend::apply_resources` remains the same application boundary, before native
instructions and after paused UI actions. Presentation repeats do not prepare
resources, execute Lua or advance the NES. No ABI3/emitter/bus/CPU/APU/PAL or
high-refresh scheduler change; module/cache regeneration is unnecessary.

## Package/API and limits

API2 data-only manifest remains unchanged: `entry:null`,
`permissions:["resources.text"]`, exact supported game/ROM/engine requirements,
and a declared `translation` file. No new arbitrary `resources.chr` permission.
The translation format is explicitly **schema 2**:

```json
{
  "schema": 2,
  "profile_id": "addams-pal-inventory-text-v1",
  "locale": "es-ES",
  "texts": {"inventory.heading": "NIÑOS"},
  "glyphs": {"Ñ": {"file": "assets/enye.chr", "fallback": "N"}}
}
```

One Unicode scalar maps to one cell; UTF-8 bytes are not NES glyph codes.
Fallback chooses an approved base-font code for preparation, **not a NES memory
write**. Unknown characters are rejected; no silent accent removal/NFD conversion.
Glyph data must be exactly 16 bytes: 8×8 NES 2bpp. At most eight glyph declarations
and eight authored cells per package; declared files must be used and unique.
Paths are exact lowercase `assets/<bounded-name>.chr`. Undeclared, missing,
alternate-case/path, duplicate, linked, malformed, oversized or executable data
is rejected. Data-only installation cannot run Lua. Normal ZIP quotas remain.

Trusted **profile schema 2** adds `visual_contexts`: backend
`nes-background-cell-v1`, physical font bank, logical nametable start and MMC1
mirroring mode. These addresses belong exclusively to reviewed host metadata,
never to packages. Only `inventory.heading` currently has such a context:
bank14, `$2C2B`, mirroring2; its mapped aliases follow the original MMC1 policy.
Field identity/ROM fingerprints and the instruction map remain mandatory.
The unchanged profile ID preserves older schema1 packages; old hosts reject
new schema2 translations rather than silently misinterpreting them.

Native limits: 16 resources, original 1–64 byte text bounds; visual consumers
must additionally fit one nametable row (at most32 cells), bank size checked
against the actual module. Prepared tiles are owned; no pointers escape or
survive a draw call. Overlapping consumer spans are rejected conservatively.
No per-pixel JSON, Lua, allocation, blocking synchronization or framebuffer copy.

Install is disabled. Enable/disable/uninstall use existing Mods UI and persistence.
Disable releases the composition at the next safe boundary; the next complete
NES raster shows the original without requiring a VRAM reload. A completed
snapshot shown during pause stays intact until a new NES frame is published.
`--no-mods` creates no manager/resources and leaves enabled preferences intact.
Unclean-session recovery remains conservative. No claims of OS security isolation
or legal authorization follow from the package/content tests.

## New tests and final full suites

Fresh Release builds on macOS27.0.1 ARM64 / Apple Clang; deployment15.0.
Desktop and headless final default builds both exit0. Source is dirty local
SDK1–4B, not a published binary SHA.

| New CTest entry | Actual result |
|---|---|
| `text_composition_package_contract` | 18 Python methods PASS |
| `text_composition_native_contract` | 117 native assertions PASS |
| `text_composition_store_contract` | Same 117 assertions plus five Store→Manager→Cartridge integration checks: 122 PASS |
| `text_composition_frontend_contract` (desktop) | 9 SDL UI/activation/safe-mode checks PASS |

The native assertions include24 mirroring/scroll/sprite-priority variants, exact
hardware/PCM equivalence, whole-field invalidation, wrong-bank fallback, disable,
identity-cache restoration and rejection without partial publication. Python
checks cover malformed glyph sizes, duplicate/alternate/traversal paths,
unsupported permissions/fallback/Unicode/IDs, missing context, scripts,
installation-disabled persistence and schema1 compatibility. Existing Phase4
CHR hazard tests still pass unchanged.

| Full configuration | Executed | PASS | FAIL | Omitted | CTest exit |
|---|---:|---:|---:|---:|---:|
| macOS ARM64 desktop | 76 | 75 | 1: A02 | 0 | 8 |
| macOS ARM64 headless | 68 | 67 | 1: A02 | 0 | 8 |

Inventory/JUnit gates independently confirm every mandatory entry ran, with no
WILL_FAIL/DISABLED/skip exemption. Both gates exit1 for A02, not an approved
overall suite. **4,736 native CPU cases and all82 IRQ contracts pass**, including
PAL07/08; A01/A03, PPU/$2002, MMC1/RAM/bus, NMI/DMA, importer/cache, Lua/SDK/HUD,
PRG resources and deterministic presentation regressions pass. Desktop also
passes actual synthetic dynamic-module ABI acceptance/rejection. No new final
regression was found. A02 remains the unchanged `$4015` immediate-source mismatch.
Linux's historical experimental GL discrepancy is neither fixed nor concealed.

Development harness issues are retained in earlier logs: missing application-icon
link for the new SDL test was fixed in its target only; a temporary probe's SDL
include path was corrected; sandboxed WindowServer access failed. The private
SDL/OpenGL probe then ran successfully with authorized native display access.
None was hidden by changing a test expectation. Initial failed desktop suite is
historical development evidence; `*-junit-final.xml` are the final results.

## Real private execution and visual proof

Existing owner PAL ROM SHA matches the supported profile. Existing ABI3 module
SHA `2fd2a47471d582afa78bcd0624f76c31d137eed55546cda7b7edc644c6917a47` and receipt
were rechecked; generator identity remains equal. No recompile or module edit.
Private probes load that unchanged native module, with the same scripted
Start/Select input schedule for 1,800 completed NES units; no interpreter.

Executed: original/installed-disabled, active glyph, disable, restart with
persisted enabled state, and safe-mode frontend with enabled preferences. For
every recorded unit, traces match the original exactly: CPU cycles, PC/A/X/Y/SP/P,
RAM hash, float-bit PCM hash, NMI/IRQ, OAM/DMC and input. The extended recorded
PPU/MMC1 state (status/control/mask/v/t/dots, mapper registers, nametable/palette
hashes) also matches. The complete full-state proof is limited to these captures;
read-buffer/open-bus behavior additionally has explicit public native tests.

The final white-palette own Ñ is visible in **NIÑOS** in the real inventory NES
framebuffer. A private driver also exercises the actual `Frontend` ZIP drop,
Enable and Disable keyboard paths, native game module and real SDL/OpenGL.
The rendered GL capture was inspected; it is not a fabricated screenshot or HUD
overlay. Its CPU/PPU/MMC1 trace equals the original; enabled raster equals the
non-GL native probe. Disable restores the original on the next complete frame.
Safe-mode captures equal original while the persisted enabled preference remains.
These are automated real-module integration/visual checks, **not a manual full
playthrough** or Windows/Linux game test. Private captures/traces/modules remain
outside Git and are not in the delivered mod/application.

The original palette3 research glyph and a palette1 white variant are both
authored project data; the former remains unchanged. Initial palette3 raster
changed334 pixels within x88–159/y14–21. The final variant changes the same
bounded header region with its own colors; detailed private evidence is retained.
All other framebuffer pixels are compared unchanged. NIÑOS is a glyph
demonstration, not an appropriate complete inventory localization; OBJETOS/VIDA
continue as prior unaccented packages. No other accent/glyph is claimed verified.

## Performance and memory

`text_composition_benchmark` uses entirely authored fixtures in Release. It
executes200 device frames per configuration, without native game instructions
or OpenGL, and2,000 publications /2,000,000 unchanged CHR reads. Single-run values:

| Visual resources | CHR ns/read | Publication ns | Disable+enable ns | Bus/PPU/APU ms per completed frame | Prepared / authored tile bytes |
|---:|---:|---:|---:|---:|---:|
| 0 | 2.43206 | 7.8335 | 7 | 1.17747 | 0 /0 |
| 1 | 1.05531 | 104.75 | 107.021 | 1.26923 | 16 /16 |
| 2 independent | 1.04385 | 163.73 | 168.354 | 1.32570 | 32 /32 |

CHR reads do not consult visual resources; variation reflects host measurement
conditions, not a demonstrated speedup. Publication includes preparation/copies;
it excludes Python validation/IPC and UI. Final single release measurements were
0/83/83ns, too short for reliable conclusions. Real example buffers comprise9
prepared tiles (144bytes) plus9 original cell bytes and16 own glyph bytes,
excluding object/vector/string/allocator overhead. No retained framebuffer copy.
These are software microbenchmarks, not FPS, input latency, monitor120Hz or
Steam Deck certification. Existing virtual50/60/90/120 determinism contracts
pass; actual physical high-refresh measurements with this mod are NOT RUN.

## Delivery and Mac instructions

Outside Git, sibling `Modding-SDK-v2-Phase4B-Demo/` contains:
`Addams-Spanish-Enye-v1.0.zip`, `SHA256SUMS.txt`, local `nes_native.app`, private
developer launch command, README and a copy of this report. It contains no ROM,
module/cache, extracted glyphs or game captures. The launcher references existing
owner material and an **isolated copy** of the private import cache, not the
original. This is a developer build requiring local Python; it is not a new
portable/public release. Temporary owner files may disappear after reboot.

Final ZIP SHA-256:
`1e8d6452f92a1b4ba0a69cb09842100b5b90670f918e69a9b3eb5ef6ac2c431b`.
Members exactly: `mod.json`, `translations/es-ES.json`, `assets/enye.chr`,
`assets/README.txt`; no Lua/profile/ROM/original graphics/code/cache.

1. Run the local launch command (or build/run this worktree with your existing
   compatible ROM/cache). No GOD is enabled.
2. F2 → Install → choose the ZIP. It starts disabled. Select Enable, then Back.
3. Start the game, select English and open inventory using **Right Shift (SELECT)**.
   Observe NIÑOS and the authored Ñ in the original4:3 image.
4. F2 → Disable → Back. Next complete NES frame restores the original heading.
5. Launch with `--no-mods`: existing enabled preference must have no effect.

Full desktop application human play, physical controller/fullscreen/focus
testing of this new mod, and complete other-platform packages remain pending.
Automated SDL UI and private native/GL checks do not substitute for those tests.

## Files and reproduction

Phase4B delta: internal resource type and cartridge publication/CPU bypass;
VideoState color consumer/cache; Manager COMPOSE transport; Python profile/schema2
and declared-glyph validation; reviewed catalog context; four new test sources,
benchmark source, CMake and inventory gate; authored example/ZIP builder; related
API/profile/security/guide/status and two Phase4B documents. CPU, APU, bus, emitter,
RuntimeServices ABI3, actor/GOD logic and presentation scheduler are unchanged.

From this dirty worktree:

```sh
build_root=$(mktemp -d /private/tmp/nes-phase4b.XXXXXX)
cmake -S . -B "$build_root/desktop" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON
cmake --build "$build_root/desktop" --parallel 4
cmake -S . -B "$build_root/headless" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build "$build_root/headless" --parallel 4
ctest --test-dir "$build_root/desktop" -R '^text_composition_' --verbose --output-on-failure
ctest --test-dir "$build_root/headless" -R '^text_composition_' --verbose --output-on-failure
ctest --test-dir "$build_root/desktop" --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$build_root/desktop-junit.xml"
ctest --test-dir "$build_root/headless" --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$build_root/headless-junit.xml"
"$build_root/headless/text_composition_benchmark"
python3 -B tools/mods/make_glyph_example.py /private/tmp/phase4b-own-mod
```

Retain CTest exit8 for A02. Use platform-appropriate temp/toolchain paths on
other systems; those commands are prepared procedures, not execution evidence.
Current temporary evidence root: `/private/tmp/nes-sdk-phase4b-_yzl8u1g`.
Final build/logs/JUnit/inventory and benchmark are separate from private
`owner-*` rasters/traces, probes and `demo-import-cache`. Never publish the whole
directory. No private ROM/code/captures are included in public sources/tests/docs.

## Matrix and next step

| Platform | Result |
|---|---|
| macOS ARM64 | IMPLEMENTED/VERIFIED bounded backend, authored ZIP, native/GL real-module display and restore; full CTest FAIL solely A02 |
| macOS Intel | NOT RUN |
| Windows x64 | NOT RUN |
| Linux x64 / Steam Deck | NOT RUN; historical GL issue remains open |

Next: independent review of the visual-only opacity/priority contract, followed
by authorized native CI and manual Mac/Deck checks. Expand trusted consumers
and authored glyphs incrementally. General CHR mutation, variable text lengths,
new HUD contexts, automatic font allocation and all-language translation remain
BLOCKED/planned; no claim of global free tiles or complete SDK platform maturity.
