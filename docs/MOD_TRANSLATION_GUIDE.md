# Create an experimental translation package

2026-10-13. This experimental successor supports an inventory heading and the English-selected life label, **not a complete Spanish translation**. It does not need per-language C++ changes or a generated-module rebuild.

1. Start from `examples/mods/addams-spanish-inventory`.
2. Choose a unique ASCII mod ID and author/name/version in `mod.json`. Keep API2, `entry: null`, `resources.text` and the exact target/profile.
3. Edit `translations/es-ES.json`: `inventory.heading` accepts **1–9 characters**. Use verified A–Z and spaces. Lowercase displays uppercase; shorter labels are padded with spaces. `OBJETOS` and `EQUIPO` are the two authored demonstrations.
4. Do not remove accents silently. Unsupported characters produce a clear `Unsupported glyph U+…` error. Ñ, accented vowels, Ü, ¿ and ¡ require future verified glyph replacement before they can be supported.
5. Validate with the existing package tool, using disposable storage and the explicit compatible context:

```sh
python3 -B tools/mods/packages.py --root /tmp/my-translation-test \
  --engine-version 0.9.0 --game-id addams-family-pal-eu-enfrde \
  --rom-sha256 6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880 \
  install My-Translation.zip
```

This checks declared compatibility and encoding; it does **not** validate a user's actual ROM or prove visible rendering. The normal importer still checks the ROM.

6. ZIP `mod.json`, `translations/es-ES.json` and optional `assets/README.txt` at archive root. No Lua, profiles, raw patches, ROM, original glyphs, graphics or music. Use `python3 -B tools/mods/make_translation_examples.py /tmp/translation-examples` to reproduce the three reference ZIPs.
7. Open the **Phase 3 experimental host**, press F2, Install Mod, choose the ZIP, select it and Enable. Back returns to the game. The published beta has no resource backend and cannot run these packages.
8. Start with your own compatible ROM; play normally and press **Enter (NES START)** during the game to open inventory (verified in the GUI); Right Shift maps NES SELECT. Confirm the top heading, not an application overlay. Close inventory before changing the active translation. Disable through F2; reload inventory or restart into a fresh equivalent state to restore the original text.
9. To compare a second package, disable the first before enabling it. A conflicting owner is rejected with an actionable error; ZIP install order does not choose a winner. No recompile is needed.

Keep test captures and owner-generated modules private. Distribute only your original translation JSON, manifest and instructions. Technical exclusion of commercial files is not legal clearance; authors are responsible for their own permissions.

## Report unsupported text

Provide the supported profile ID, symbolic screen/resource, engine build identity, mod version, the proposed authored translation and observed error. Do not submit ROM dumps or original CHR/text byte streams. Text without a reviewed symbolic ID, variable-length control streams and characters not in the approved map require a separate profile/backend investigation. Changing raw offsets in a ZIP is never a supported solution.

Windows/macOS Intel/Linux native validation is pending for this local successor. No new release or CI run has been published.

## Two-field package (Phase 3)

Start from `examples/mods/addams-spanish-multi`. The manifest contract is unchanged;
there is no `main.lua` and no raw address in the mod. Use this translation payload:

```json
{
  "schema": 1,
  "profile_id": "addams-pal-inventory-text-v1",
  "locale": "es-ES",
  "texts": {"inventory.heading": "OBJETOS", "hud.life": "VIDA"}
}
```

Install `Addams-Spanish-Multi-v1.0.zip` with F2; it starts Disabled. Enable before
starting the game, choose original **English**, pass the introduction/hint, observe
VIDA in the original HUD and press Enter to observe OBJETOS in inventory.
F2 pauses safely. Disable, close/reopen inventory to restore its heading; restart
the game to restore the life label uploaded during HUD initialization. Installation
and toggling never rewrite the ROM or module. The same host accepts different
approved labels and a subset of fields; no C++ rebuild per translation.

A mod sharing either field conflicts with this package. Disable the current owner
first; rejection must leave the entire previous selection unchanged. Symbols with
no approved glyph mapping still fail explicitly. This demonstration is partial,
not a translation of hints, story, items or room names.

## Own Ñ example — Phase4B

Use the Phase4B host; old beta hosts reject translation schema2. A data-only API2
ZIP retains `permissions:["resources.text"]`, target and declared translation.
See the actual `examples/mods/addams-enye-inventory/` manifest and JSON: its
`schema:2` text NIÑOS names the approved inventory field and declares
`"Ñ":{"file":"assets/enye.chr","fallback":"N"}` in `glyphs`. The file is an
authored16-byteNES2bpp tile; fallback is a base-font encoding choice, never a
PRG/nametable write. Only explicitly supplied, approved-size glyphs are accepted;
accents are not normalized or silently stripped. Each Unicode scalar occupies
one fixed cell. This example renders uppercase Ñ; other Spanish glyphs require
authored files and testing, not invented mappings.

Build ZIP: `python3 -B tools/mods/make_glyph_example.py <output-directory>`.
Install disabled using F2/Mods, Enable, Back. Choose English and open inventory
with Right Shift/SELECT. Disable returns original on the next complete NES
frame, without VRAM reload. Full paused snapshots remain until simulation resumes.
Do not ship original font tiles, ROM, cache, modules or game captures. Distribute
only your own glyphs/text and license. NIÑOS is a typography demo, not a complete
inventory translation. See [Phase4B report](MODDING_SDK_V2_PHASE4B.md).
