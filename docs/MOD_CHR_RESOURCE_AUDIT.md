# CHR glyph resources — verified audit, not a completed capability

2026-10-14. Local `feature/modding-sdk-v2-phase4`, historical HEAD
`95c194afac4574be9bf8b495341517389a73988b` **plus the preserved, uncommitted
SDK Phases 1–3**. This HEAD alone is not the SDK implementation. Native platform:
macOS 27.0.1 ARM64. See [Phase 4 results](MODDING_SDK_V2_PHASE4.md).

## Decision

**BLOCKED for a real CHR translation mod.** The font source and two consumers
are verified, but no new destination has been established as safe across game
states. Blank bytes and absence from a short trace do not establish reservation.
The request explicitly requires stopping rather than overwriting shared tiles.
No production CHR replacement, new permission, codec mapping or installable
glyph ZIP has been enabled. This does not prove that a safe destination cannot
exist; it states exactly what this investigation has not established.

## Real route and source references

The owner-authorized European image matches the existing SHA-256 profile
`6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880`.
It is iNES mapper 1, 131,072 PRG bytes and 131,072 CHR bytes: **CHR-ROM**, comprising
32 physical 4 KiB banks. Original data, the existing private ABI3 module and
raw raster/fetch evidence remain outside Git; no commercial glyph is exported.

The actual route is CHR-ROM → MMC1 → `Cartridge::chr_offset/chr_read`
([cartridge.cpp](../runtime/src/cartridge.cpp):85–92) → PPU pattern fetches →
nametables/attributes/palette → native NES framebuffer → existing SDL/OpenGL.
`VideoState::draw_pixel` ([video_state.cpp](../runtime/src/video_state.cpp):107)
uses CHR for both backgrounds and sprites. `read_memory` at line 9 routes CPU
PPUDATA access to the same CHR source; buffered `read_register` at line 50
preserves that observable data. `write_memory` at line 15 ignores writes below
`$2000` for this CHR-ROM cartridge. There is no allocation protocol for new
CHR-RAM glyphs in this runtime/profile.

MMC1 resolves physical banks on each fetch. In 4 KiB mode the two windows are
independent; in 8 KiB mode bank 0 chooses an even/odd pair and bank 1 is ignored.
An index without bank identity is insufficient. Pattern tiles contain two
8-byte bitplanes, yielding an 8×8 array of indices 0–3, not final RGB colors.
The nametable attribute selects the background palette; index 0 is transparent.
These facts agree with [NESdev pattern tables](https://www.nesdev.org/wiki/PPU_pattern_tables)
and [MMC1](https://www.nesdev.org/wiki/INES_Mapper_001).

The PRG text mechanism is separate: `Cartridge::set_text_resources` (line 30),
`Frontend::apply_resources` ([mod_frontend.cpp](../runtime/src/mod_frontend.cpp):86),
and `compile_translation` ([resources.py](../tools/mods/resources.py):82) replace
reviewed data reads. They neither allocate CHR nor intercept PPU glyph fetches.
Immediate operands/instructions embedded in generated C are not arbitrary bus
data. [BUS_TIMING_REPORT.md](BUS_TIMING_REPORT.md) is a historical ABI2 report;
its fetch-coverage caveat remains relevant, but its former interrupt state must
not be mistaken for the current ABI3 IRQ implementation.

## Private native experiment executed

Used the already present owner image and ABI3 module; no download, reimport,
module regeneration or commercial source edit. Added observation callbacks to
a **temporary copy of our own** VideoState implementation, linked with the
existing native runtime and module, outside the worktree. It records already
resolved CHR offsets, fetch origin and nametable identity, with no additional
NES bus reads. It is not another CPU or interpreter.

Executed the same 1,800-unit Start/Select input schedule as the Phase 3 original
probe. The instrumented trace is byte-identical to the original for every row:
inputs, CPU cycles, PC/A/X/Y/SP/P, NMI/IRQ, OAM/DMC, RAM hash and PCM sample-bit
hash. This verifies the instrumentation on this trajectory, not the accuracy of
the whole PPU or all possible gameplay. Raw private captures were not published.

| Observation point | Current MMC1 control / CHR0 / CHR1 | Background source actually recorded | Result |
|---|---|---|---|
| Language screen, unit 180 | 30 / 0 / 14 | Bank 14 | Font/menu bank observed |
| HUD/garden, unit 1350 | 30 / 0 / 8 | Line 0 bank 1; line 200 bank 8 | Per-line bank distinction matters |
| Inventory, unit 1590 | 30 / 15 / 14 | Bank 14 | Inventory font bank observed |

For the two approved text resources, narrowed **actual background fetches**
establish one 8×8 tile per letter cell: four cells for `hud.life` in bank 8 and
nine for `inventory.heading` in bank 14. Their existing original PRG fingerprints
were rechecked against the owner image. The field code mapping is the existing
uppercase A–Z mapping, not ASCII. Inventory nametable addressing must be
normalized through MMC1 mirroring, rather than relying on a raw address.
This proves only these two fields, not every game text or sprite font.

Ten current mapper configurations occurred over the trajectory. Physical banks
actually fetched by background/sprites were 0, 1, 8, 14, 15, 17, 20, 21, 22 and
25. There were **2,436 CPU CHR reads**: 1,154 in bank 0, two in bank 22 and 1,280
in bank 24. **No CPU reads in font banks 8/14 were observed here.** It would be
incorrect to claim that these font banks were already read by the CPU in this
test, or that they can never be read in another state.

Font codes are shared with other screen content. Cumulative uppercase-font
background pixel fetches were 183,808 in bank 8 and 1,176,192 in bank 14, not
exclusive uses of the approved two fields. Usage CSV snapshots are cumulative
since boot; they are not per-screen-exclusive coverage. Several zero-filled
candidates within the current codec's code range were unobserved, while blank
tile 0 **was fetched** in both font banks. A candidate blank in one bank is not
necessarily blank in the other. No candidate is approved by this report.

Not covered: all rooms, deaths, endings, language selections, transitions,
hidden screens, sprite consumers and every static/dynamic reader. No global
free-tile claim, full game completion or new accented in-game rendering is made.

## Proven hazard with entirely authored data

`tests/chr_resource_research_checks.cpp` renders a synthetic nametable/sprite
fixture using the current native mapper/PPU. Replacing one blank fixture tile
with our own Ñ changes the NES framebuffer **and**:

* makes sprite-zero hit become set at an opaque overlap;
* changes the buffered byte returned to the CPU through `$2007` from 0 to the
  first own glyph row;
* leaves the prescribed CPU cycles and PPU dots equal.

A fresh original fixture restores its raster/status/readback. This is a
counterexample to "graphics-only changes cannot affect execution", not proof
of a bug in the commercial game or an implemented unload transaction. Sprite
zero and PPUDATA behavior are documented by
[NESdev PPU registers](https://www.nesdev.org/wiki/PPU_registers). Our experiment
uses the existing software PPU, not a new hardware PAL measurement.

## Authored glyph and Spanish coverage

`examples/resources/chr-research/own-enye-8x8.chr` is an original 16-byte Ñ,
with an independent pixel matrix in the adjacent JSON and the repository MIT
license. A nearest-scaled offline preview was inspected. Native synthetic PPU
rendering was exercised; no owner-game destination was used. The matrix is a
research fixture, **not an accepted resource-profile/mod schema**.

| Characters | Current result |
|---|---|
| Ñ | Original 8×8 fixture authored and checked; safe game destination BLOCKED |
| Á É Í Ó Ú / ¿ ¡ | No approved mappings or authored in-game resources |
| ñ á é í ó ú / Ü ü | No approved mappings or authored in-game resources |

The current text codec rejects every above character explicitly. We have not
proved exhaustive absence of those shapes from all original CHR; we have proved
that no approved mapping exists in the present resource contract. No accents
are silently stripped. OBJETOS/EQUIPO/VIDA compatibility remains unchanged.

## Conditional architecture; not an API announcement

Preferred future mechanism **A**, only after destination approval: immutable,
owned, typed 16-byte glyph data resolved by physical MMC1 bank/tile, with trusted
ROM-revision/profile fingerprints and an explicit reservation/consumer proof.
Profiles choose destinations; untrusted ZIPs reference symbolic IDs. Proposed
limits: 8×8 NES 2bpp only, at most 32 glyphs (512 raw bytes) per bundle initially,
unique validated ranges and exclusive ownership; these are design values, not
enforced new production permissions. Validate/encode before publication, use
no JSON, Lua or allocation per pattern fetch, and commit text/codec/glyph leases
atomically at a completed simulation boundary. Reject partial/conflicting
bundles. A matching screen/VRAM reload policy is mandatory when enabling or
disabling; releasing only CHR while stale glyph indices remain is unsafe.
`--no-mods` must bypass the entire bundle. This backend can retain ABI3 but still
needs separate CPU-read, sprite-zero and bank-transition contracts.

If reservation cannot be established, investigate **B**, a background-pattern
resource reader scoped to approved, mirrored nametable cells, physical bank and
resource/screen epoch. Separate CPU and sprite readers explicitly. This would
still render inside the NES framebuffer, not an OpenGL overlay. It requires a
new reviewed PPU contract for opacity/sprite-zero semantics, stale nametables,
bank changes and coherent release; it is **not yet proven equivalent or safe**.
Do not silently hide those effects by retaining original collision masks.

Pattern-table upload is not a distinct safe insertion point here: CHR is ROM.
Glyph-index translation cannot create absent graphics. Frontend GL substitution
would fake game text. Shared N replacement, blank-0 reuse, adding an unmapped
bank, ROM/cache patching and generic raw-address CHR patches are rejected.

## Evidence and next decision

Private evidence is retained in the temporary Phase 4 directory recorded by the
phase report: bank/fetch CSVs, raw rasters, instrumentation source and traces.
Only counts, verified identities and conclusions are public. Direct NESdev
fetches returned HTTP 403; indexed primary-page text was consulted. No emulator
agreement is claimed as an independent physical-console validation.

Approve one bounded destination/consumer experiment before implementing a CHR
mod: cover the chosen font bank/tile across all supported screens and languages,
CPU/sprite/background use, mirroring and reload, with deterministic before/after
traces. If that proof fails, review mechanism B's explicit semantics separately.
