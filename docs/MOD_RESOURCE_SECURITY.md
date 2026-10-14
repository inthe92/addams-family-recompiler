# Resource-overlay safety model

2026-10-13. The Lua sandbox remains in-process and is not OS isolation. This data-only slice creates **no Lua VM**; it grants a bounded resource permission rather than arbitrary memory writes.

## Boundaries

- Commercial PRG/CHR belongs to the owner's private module; no original resource bytes are shipped in profiles or ZIPs.
- Profiles/codecs are trusted, versioned host files checked against the supported instruction map. Packages select named IDs and cannot provide a profile or native behavior.
- Author data is UTF-8 JSON. Duplicate keys/paths, traversal, alternative path forms, symlinks/special files, unknown IDs/fields/types, malformed data, missing permissions, incompatible targets, control-stream substitutions and overflows are rejected.
- Data-only archives cannot contain `main.lua`, executable content or undeclared translation files. Inspection/installation never runs scripts. Installation is disabled; enable is an explicit existing manager choice.
- Normal ZIP limits remain: 8 MiB archive, 16 MiB expanded, 128 entries, bounded expansion, 32 installed mods. Translation JSON ≤16 KiB, catalog ≤64 KiB, at most 16 active fields ×64 bytes. All buffers are owned; no host pointer exposure.
- One owner per resource. Enable rejects conflicts before state persistence. Complete native publication validates all ranges/source CRC/glyph bytes before replacing the previous list. A publication mismatch rejects/disables translations and returns to original data.
- Publication occurs after frontend transactions, outside native instruction execution. No extra bus reads, Lua callbacks or device cycles are introduced. MMC1 resolves physical banks normally and replacement bytes drive the normal bus latch.

## Restore and limitations

Disabling removes lookup buffers at the next safe simulation boundary. It does not reverse past game state, clear VRAM, erase saves or rewrite a completed framebuffer. Reload the inventory for visible restoration. Source ROM and cached module are not written by this API. Leaving the cartridge releases resources. Unclean-session recovery uses the existing manager marker and requires explicit re-enable.

The importer validates exact ROM SHA/module identity; the native per-field CRC is not collision-resistant. A known-instruction map does not certify every unknown code path. Trusted profiles require manual/private analysis; a malicious host binary, altered profile, compromised filesystem or concurrent external file modification is outside the Lua threat model. Installed user-owned JSON is revalidated before activation/startup; no claim of adversarial OS isolation is made.

Supported scope: fixed PRG glyph fields only. Hardware registers, PPU/DMA/mapper writes, code bytes, jumps/immediates, CHR, palettes, audio and arbitrary patches are unavailable to translations. A02 and the historical Linux experimental GL discrepancy are neither corrected nor concealed.

## Phase 4B bounded display resources

Translation schema2 requires an approved host consumer. Up to8 own declarations
and8 authored cells per package, each exact16-byte8x8NES2bpp; exact lowercase
`assets/<name>.chr` paths, no undeclared/duplicate/alternate/missing files.
Inspection/install never execute Lua. All descriptors/buffers validate before
publication; normal resource owner conflicts and atomic disabled installation
remain. CPU PRG/CHR/PPUDATA and mapper writes are unavailable. Prepared private
base-font bytes never leave the runtime or appear in mod ZIPs.

Composition changes display color, deliberately preserving original sprite-zero
and priority masks; it is not a physical-font/collision replacement. Disable
releases composition before the next native unit; completed paused snapshots
remain immutable. Validation is not OS isolation, hostile-host protection,
legal clearance or a full hardware-accuracy certificate.
