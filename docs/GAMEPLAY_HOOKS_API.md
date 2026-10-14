# Gameplay requests — implemented API2 capability

Local experimental Phase5, 2026-10-14. Requires the current Phase5 host; no ABI3
module regeneration. Standard v1 ZIPs remain compatible. The old standalone Super Gómez prototype
used a separate nonstandard capability extension; install the new API2 Gameplay
ZIP in this host rather than its old prototype ZIP. Older hosts reject the new
permissions instead of pretending to support them.

## Manifest and callable API

```json
{
  "id": "example.jump", "name": "Example jump", "version": "1.0.0",
  "api_version": 2, "description": "Bounded jump extension",
  "author": "Example author", "entry": "main.lua",
  "permissions": ["gameplay.jump"],
  "target": {"game_id": "addams-family-pal-eu-enfrde", "engine_min": "0.9.0",
             "engine_max_exclusive": "1.0.0"}
}
```

```lua
function on_load()
    game.player.set_modifier("jump_extension", 12)
end
```

Only this modifier is implemented. Value must be an integer 0..12; zero withdraws
it. Calls require on_load/on_update context and the declared permission. No
`write_ram`, position setter, speed multiplier or generic velocity/gravity API
exists. Disable/unload/error forcibly revokes it even if on_unload fails.

## Semantics and limits

The verified original jump uses indexed displacement phases, not a floating
velocity. The adapter grants a finite request only at a newly initiated manual
jump in the validated initial outdoor numeric room0. It retains original movement
and collision instructions, original INC read/old-write/new-write offsets, cycles
and N/Z classification; the new phase can remain at the approved ascent phase
for at most 12 extra original updates. The private measured full jump adds36px.
Two integration sites and fail-closed state guards are confined to
`game/src/super_jump.cpp`; scripts do not refer to them. No original commercial
instruction/table dump is included.

Menus, loading, death, ceiling contact, ladders, ropes, alternate motion,
attachments, descent and other rooms cancel or refuse extension. Enabling mid-air
does not acquire a request. Increasing the configured limit does not replenish an
existing jump; decreasing clamps remaining steps. Disabling removes future steps,
not prior movement. A fresh original jump after Disable uses original behavior.
No permanent ROM, cache, save or gameplay-resource patch is made.

Only one owner is allowed; another Enable reports the current conflicting mod.
This is a bounded experimental capability with an audited adapter, not unrestricted
programmability or proof of every collision/room. Super Speed remains BLOCKED
pending safe movement/collision evidence.

## Author workflow

Put mod.json/main.lua at ZIP root, own optional README under assets/, install with
F2 > Install Mod, explicitly Enable, Back. Use Z (NES A) for the full jump in the
first ordinary room. Disable via F2 before comparing a new original jump. See
`examples/mods/super-gomez-gameplay` and `tools/mods/make_phase5_examples.py`.
