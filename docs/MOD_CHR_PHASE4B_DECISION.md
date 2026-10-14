# Phase 4B decision — bounded NES background composition

2026-10-14. Decision recorded before production edits. Dirty Phase 4 preserved
in an independent `feature/modding-sdk-v2-phase4b` worktree; all 92 hashes match.
Historical HEAD95c194a alone is not the SDK source. No remote operations.

## Evidence and choice

Global CHR reservation remains UNKNOWN. The previous native consumer trace
establishes font banks 8/14 and shared glyphs, not a free tile. Reusing blank
indices is OBSERVED UNUSED at best; blank zero/shared letters are UNSAFE as
global replacements. No global slot is PROVEN SAFE. Additional bank allocation
would require mapper changes. We choose **one backend: bounded composition**.

A new temporary native experiment (own data only) compared the original raster
with own Ñ composed in the background-color stage, before normal sprite
selection. Fourteen checks pass: different raster, equal cycles/dots, equal
sprite-zero status, equal buffered PPUDATA and original restoration. This is
initial software evidence; complete tests and owner-game verification follow.

The backend is not a fetch replacement: original background/sprite CHR fetches,
nametables, palette/register reads, opaque masks and sprite selection remain
unchanged. A separate bounded display pixel replaces the background color only.
The original background opacity continues to determine sprite-zero and sprite
priority. Thus a sprite behind the original background may remain occluded even
where an authored glyph is transparent. This is an explicit **visual resource
contract**, not the behavior of a physically substituted NES CHR chip. Never
claim translated pixel opacity is a new collision/hardware mask.

No OpenGL overlay or global framebuffer postprocessor: insertion occurs in
`VideoState::draw_pixel`, at an approved nametable consumer using live tile_v,
fine scroll, mapper bank and mirroring. Extra presentations reuse the resulting
completed framebuffer and dispatch no new simulation callbacks.

## Narrow profile/package contract proposed for implementation

Only a trusted host profile selects the bounded nametable span, mirroring mode,
physical background font bank and exact original text identity. The current
inventory header is the first reviewed consumer. A context must match the whole
original field in mirrored nametable memory before any display substitution.
It follows scrolled nametable cells, not screen coordinates or an invented RAM
screen ID. The scope is the verified inventory-header VRAM identity, including
its correctly mirrored aliases; no global font replacement.

Translation schema 2 supplies authored text and optional original 16-byte glyph
files. Glyphs use a base-font fallback identifier for encoding and exclusively
owned display pixels; this fallback is not written into NES RAM/PRG. Runtime
preparation copies required existing font tiles privately from the loaded
module, replaces selected cells with own data, and retains owned immutable
buffers. No original glyph bytes are exported or included in ZIPs.

All text/graphics/profile validation precedes one cartridge resource publication.
Schema 1 PRG translations remain unchanged. Visual-only resources must be
excluded from CPU PRG overlays. Installation stays data-only/disabled, scripts
never execute, and conflicts remain one owner per symbolic resource. Mod ZIPs
cannot specify addresses, banks, profiles, executable instructions or framebuffer
rectangles. API2/ABI3 remain distinct, unchanged contracts.

Release references only on the application thread between native units. Draws
do not keep pointers across calls. Disable returns to original composition on
the next rendered NES frame without editing VRAM; completed snapshots naturally
remain unchanged until replaced. `--no-mods` must publish nothing.

## Comparison

| Property | Physical CHR reservation | Bounded composition |
|---|---|---|
| CPU/PPUDATA | New byte values unless consumers excluded | Original byte values |
| PPU hardware masks | New opacity affects sprite-zero/priority | Original masks; documented display divergence |
| Mapper | Correct physical slot essential | Live bank/mirroring gate, mapper unchanged |
| Restoration | Stale tile indices/reload contract required | Original nametables untouched; next complete raster |
| ABI3 | Can remain unchanged | Can remain unchanged |
| Cost | Lookup in every affected CHR read | Bounded validated consumer/color lookup; measure |
| Current evidence | No safe slot proven | Synthetic initial proof; full validation required |
| Reuse | Trusted per-game reservations | Trusted per-game consumer profiles, no per-language C++ |

Acceptance requires CPU/RAM/PCM/PPU/MMC1 equivalence with mods inactive and
visual-only resources active, intentional framebuffer difference restricted to
approved cells, original restoration and adversarial/profile/lifecycle tests.
If those fail, stop instead of hiding a hardware divergence.

Background priority and sprite-zero behavior are documented by
[NESdev PPU rendering](https://www.nesdev.org/wiki/PPU_rendering) and
[PPU sprite priority](https://www.nesdev.org/wiki/PPU_sprite_priority).
The new visual contract deliberately keeps those original decision inputs.
No new PAL-console measurement or free-tile certification is claimed.
