# Modding SDK v2: resource API (Phase 2)

2026-10-13. Local experimental implementation, engine 0.9.0, native ABI **3 unchanged**. See `MODDING_SDK_V2_PHASE_2.md` for actual results.

## Implemented

A data-only API2 package with `entry: null` and exactly `permissions: ["resources.text"]` supplies a UTF-8 JSON translation. It uses a symbolic resource ID in a **host-installed trusted profile**, never a raw bank/address or executable hook. No Lua VM is created for this package. Script-based API1/API2 packages continue to work and cannot request this permission.

```json
{
  "id": "community.inventory-es",
  "name": "Spanish inventory heading",
  "version": "1.0.0",
  "api_version": 2,
  "author": "Your name",
  "description": "Experimental inventory heading. Reload the inventory after toggling.",
  "entry": null,
  "permissions": ["resources.text"],
  "target": {
    "game_id": "addams-family-pal-eu-enfrde",
    "engine_min": "0.9.0",
    "engine_max_exclusive": "0.10.0"
  },
  "translation": {
    "profile_id": "addams-pal-inventory-text-v1",
    "file": "translations/es-ES.json"
  }
}
```

```json
{
  "schema": 1,
  "profile_id": "addams-pal-inventory-text-v1",
  "locale": "es-ES",
  "texts": {"inventory.heading": "OBJETOS"}
}
```

The manifest's descriptive fields and ZIP paths use printable ASCII; translations are Unicode, limited to the verified glyph map. Locale must match the JSON filename. `main.lua` is forbidden in a data-only package. Optional passive `assets/README.txt` contains the author's instructions. The ZIP root contains `mod.json`, not a containing directory.

Installation validates, stages and installs **disabled** without executing code. Enable revalidates engine/game/ROM revision, the trusted profile and encoding. At most one owner may replace a resource; an error identifies the existing owner and asks to disable it. At most **16 active fields**, each **1–64 encoded glyph bytes**, are supported. A package contains at most 16 named replacements; translation JSON is at most 16 KiB. The existing ZIP quotas remain in force.

## Native contract

`Manager::resources()` publishes owned encoded buffers with a revision. `Frontend::apply_resources(Cartridge&)` commits the complete validated list after SDL event transactions and before any next native simulation unit. No instructions run during publication. Repeated presentation/HUD draws do not encode text, invoke Lua or republish unchanged buffers. `Cartridge::cpu_read` resolves the current MMC1 physical PRG bank before bounded lookup; `Bus` continues to drive the open bus and retain the original timed read. Original PRG storage is never written.

The internal `PrgTextResource` C++ type is a trusted host backend, **not a public address-patching API**. Lua and ZIPs cannot supply native pointers, bank offsets, original fingerprints or resource profiles. RuntimeServices and generated C are unchanged; no module/cache regeneration is required for translation.

Disable/uninstall publishes the original fallback at the next safe boundary. It does not undo text already copied to VRAM/RAM or mutate the completed framebuffer. Close and reopen the inventory or enter a fresh equivalent state. Native source-fingerprint failure disables the translation and returns to the original resource. Owned buffers live until the next successful publication or cartridge destruction.

## Planned, not implemented

General strings/control-stream rewriting, variable-length text, runtime Lua resource writes, CHR/glyph replacement, tile maps, palettes, audio, executable immediates/operands, PC hooks and arbitrary ROM patching are unsupported. A translation cannot replace an unknown ID or create a new profile.

## Phase 3 compatible extension — 2026-10-14

No new package schema/API method or native ABI change. The trusted catalog now
exposes `hud.life` (1–4 glyphs, original English selection) alongside
`inventory.heading` (1–9). A `texts` object may select both; all fields validate
before activation, and partial ownership conflicts reject the complete transaction.
The data-only example `addams-spanish-multi` uses both and runs without Lua.
See the updated translation guide for reload requirements and native validation.

## Phase 4B — visual-only schema2 (2026-10-14)

A bounded `nes-background-cell-v1` compositor is now implemented for the trusted
inventory header context. This is display composition inside the NES PPU, not
physical CHR/PRG mutation. API2 manifest/permission stay unchanged; translation
schema2 adds authored glyph files. Original CPU text reads, nametables, CHR,
sprite-zero and priority inputs remain unchanged. See
[Phase4B](MODDING_SDK_V2_PHASE4B.md) and its exact visual-opacity limitation.
Schema1 PRG packages remain supported. No general CHR, pointers or variable
streams API is exposed.
