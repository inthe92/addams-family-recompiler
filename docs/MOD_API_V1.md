# Lua Mod API v1

Managed ZIPs contain `mod.json`, `main.lua`, and optional passive `assets/` files. API v1 does not decode, play or mount textures/sounds; these are reserved package storage. No native code is accepted.

```json
{"id":"example.hello-native-lab","name":"Hello NES Native Lab","version":"1.0.0","api_version":1,"description":"Read-only Lua lifecycle and simulation observation demo.","author":"NES Native Lab","entry":"main.lua"}
```

All seven fields are required; unknown/duplicate JSON keys are rejected. IDs are lower-case letters/digits/dots/hyphens, maximum 64 characters, without `..`; versions are three decimal components. `api_version` is integer 1, not boolean. Entry is exactly `main.lua`. Manifest names/authors are at most 60 printable ASCII characters, descriptions at most 300. ASCII metadata/paths are a deliberate first-version limitation of the existing UI font; Lua source may be UTF-8 text.

| Function/event | Available behavior |
|---|---|
| `MOD_API_VERSION` | Integer 1 in each isolated script environment |
| `on_load()` | Optional function, called once after the source chunk succeeds when enabled |
| `on_update(dt)` | Optional function, called once before each original NES simulation unit; `dt = 1 / 50.007` seconds |
| `on_unload()` | Optional function, called on normal disable/removal/shutdown, subject to the same limits |
| `game.get_status()` | New table containing `frame`, `cpu_cycles`, `simulation_hz`; `room_id` when supplied by the adapter |
| `game.get_player()` | New `{x,y}` table only when the host supplies a validated player observation; **currently nil for Addams** |
| `log.info(text)` | Private bounded text log; strings up to 300 characters, controls sanitized |

An NES unit is the existing desktop loop's native execution to the next observed completed PPU frame, followed by its original audio delivery. This is not a per-instruction, scanline or monitor callback. `dt` describes that original PAL scheduling contract, not elapsed wall time; suspension and repeated presentations do not create extra updates. The first partial frame and native slice boundaries are not new hardware timing claims.

The source chunk runs before `on_load`; status is initially a neutral snapshot until a game unit supplies an observation. Missing callbacks are allowed; callbacks with non-function values are errors. Enabled mods update in ascending ID order, independent of installation order. Normal startup/load and shutdown/unload also use sorted states; enabling a mod from the UI invokes that mod's lifecycle immediately while the game is paused.

Tables are detached values. Editing a returned status/player table changes neither the host snapshot nor NES memory. No RAM read/write, button injection, physics setters, filesystem, networking or process API exists for managed mods. Player identity could not be certified from the available actor-pose observer across all scenes, so coordinates were not invented.

The working example is `examples/mods/hello/`. Generate its installable ZIP:

```sh
python3 -B tools/mods/make_example.py <destination>/Hello-NES-Native-Lab.zip
```

The normal packager also creates this ZIP in `examples/mods/` in each distribution. Install through Mods, select Enable, run a game, then inspect private `mods.log`. It logs load, first/periodic PAL updates and unload. In `--mods-only` there is **no game simulation and no on_update**.

```lua
function on_update(dt)
    local status = game.get_status()
    local player = game.get_player()
    if player then log.info(tostring(player.x)) end
end
```

Unsupported: arbitrary `require`, bytecode, coroutines, pcall/xpcall, debug, metatable manipulation, string patterns/packing, dynamic libraries, active assets and player modifications. This is a deliberately small API/library subset, not unrestricted Lua.
