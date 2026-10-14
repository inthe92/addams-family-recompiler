# Trusted resource profiles

2026-10-13. Declarative host catalog: `tools/mods/resource_profiles.json`. Schema 1 is validated by `tools/mods/resources.py`. Profiles are reviewed engine data; ZIPs select them by ID and cannot install profiles or codecs.

## Required schema

Catalog: exact fields `catalog_schema: 1`, `profiles` (1–8). A profile has exact fields:

| Field | Contract |
|---|---|
| `profile_schema`, `profile_id`, `game_id` | Schema 1 and bounded symbolic identities |
| `rom_sha256`, `rom_size`, `prg_size` | Exact compatible revision and bounded MMC1 PRG layout |
| `codec` | Verified mapping, codec ID, padding, explicit uppercase display |
| `resources` | 1–16 named bounded text descriptors |

Each descriptor requires `type`, `bank`, `offset`, `expected_length`, `replacement_length`, `max_chars`, `codec`, `original_sha256`, `original_crc32`, `reader_bank`, `reader_pc`, `controls`. Only `type: text`, equal fixed lengths, 1–64 glyphs and `controls: []` are accepted in this slice. Controls and terminators must be **outside** the approved range. Profile ranges and profiles for the same revision must not overlap. The last six bytes of each bank are excluded conservatively because MMC1 can expose interrupt vectors there; vector bytes are never text resources.

The catalog must have a matching `tools/recompiler/compatible_profile.json` instruction map. Validation rejects overlap with every known instruction start plus a conservative three-byte span, and verifies the reader PC is a known instruction. A missing/mismatched map fails closed. This is conservative protection over recovered coverage, **not proof that every possible execution address is known**. Authoring a profile still requires resource-flow analysis and private native tests. The current fixed last-bank address convention follows the supported eight-bank game; new game layouts require reviewed profile validation changes, not a mod workaround.

## Verified initial profile

`addams-pal-inventory-text-v1` targets `addams-family-pal-eu-enfrde`, SHA-256 `6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880`. Initial Phase 2 resource: **`inventory.heading`**, the nine-cell inventory heading. It is a glyph field inside a larger PPU-directed nametable payload. The banked data reader loads through an indirect timed PRG read and subsequently writes `$2007`. The field excludes border, control bytes and the terminator. Native capture observed 1,024 nametable uploads on entering inventory; original and translated read counts match.

The host descriptor records physical provenance and fingerprints, without original bytes, text dumps, font images or generated game C. Private analysis verified this field against the owner's exact ROM, the recovered code/data intervals and the actual inventory/font output. Locations are maintained in the reviewed host catalog; translation authors use the symbolic ID.

The glyph map supports space and A–Z. Lowercase input deliberately displays uppercase through documented aliases. Native CHR/raster inspection verified those glyphs in the active inventory font. All 256 tiles of its active 4 KiB pattern table were inspected privately; remaining tiles include screen/object imagery and have no approved discrete mapping for the requested Spanish symbols. This does not certify other banked fonts or composite glyph techniques. Spanish-specific Ñ/ñ, accents, Ü/ü, ¿/¡ and combining marks have **no approved mapping** and are rejected. Their absence from this codec does not prove they are absent from every font in the ROM.

## Access and integrity limits

Immediate operands are embedded in generated C, direct zero-page/stack paths may bypass services, and CHR reads go directly through `Cartridge::chr_read`. A PRG data overlay is not a universal hook. Only an independently verified data field accessed through the mapped PRG backend is supported. Instruction targets, opcodes, immediates and ambiguous executable/data overlap are unsupported.

Full ROM identity is checked by the existing importer, not inferred from a CRC. The profile carries the privately verified original SHA for provenance; the native publication guard compares **CRC32 of the original field**. CRC32 is an accidental mismatch guard, not a cryptographic security boundary. Mods cannot choose the fingerprint. The host catalog and native module/import identity remain trusted components; replacing host files is outside Lua sandbox isolation.

## Phase 3 additive catalog — 2026-10-14

The same schema/profile ID now also includes **`hud.life`**, a four-cell fixed
label selected by the original English language branch. Existing inventory-only
ZIPs are unchanged and still accepted; an older host catalog rejects the new ID.
This is an additive trusted catalog update, not a per-language native patch.

Private native execution traced physical bank **0**, offset **13130 (0x334A)**,
four bytes, through reader **bank 0 / PC 0xB317** into `$2007` at PC `0xB31A`.
A fixed five-iteration copy includes a preceding blank outside the resource.
Observed PPU writes place the four glyphs at **$232E–$2331** or **$272E–$2731**;
15 observed uploads alternated the nametable destination. Language-offset tables,
loop-count/control bytes, code and operands are outside the replacement span.
Known instruction/operand-map exclusion and catalog non-overlap pass. The earlier
HUD-frame initializer also contained candidate letters, but its label is overwritten
by the language-selected copy and was not approved as another resource.

`VIDA` was visibly rendered in the actual native SDL/OpenGL game HUD, and the
private raster comparison changed only 54 pixels in x112–142/y201–207. This verifies
these replacement glyphs in the active HUD font. French/German language branches
remain original; choose **English** for this two-field Spanish demonstration.
`hud.life` accepts 1–4 approved letters/spaces; `VIDAS` is rejected as overflow.
No variable-length streams or new accented glyph mappings were added.

HUD text can remain in VRAM after Disable. Restart the game into an equivalent
state (or an original HUD reload) before comparing restoration; Disable does not
rewrite the already rendered image. See `MODDING_SDK_V2_PHASE3.md` for actual results.

## Phase 4B trusted profile schema2

Schema1 remains accepted. Schema2 adds exact `visual_contexts` keyed by existing
text resource IDs: `backend`, `chr_bank`, `nametable`, `mirroring`. Backend is
exactly `nes-background-cell-v1`; a bounded row/physical bank/mirroring and whole
original nametable-field identity gate the display. Currently only
`inventory.heading` is approved (physicalbank14, logical$2C2B, mirroring2).
Modifiers cannot install profiles or choose those addresses. A zero-filled CHR
tile is not reserved; the mechanism allocates no hardware tile. Full supported
ROM/import identity and original PRG fingerprint checks still apply. See
[decision](MOD_CHR_PHASE4B_DECISION.md) for the audited consumer and limits.
