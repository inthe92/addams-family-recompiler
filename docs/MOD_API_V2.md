# Mod API v2 — completed-state read-only HUD contract

2026-10-13. First vertical implementation, not the full Mod Manager 2.0 roadmap.
API version, engine version, supported game profile and native module ABI are
separate contracts. This extension does not change ABI v3.

## Package negotiation

API v1's seven fields and callbacks remain supported unchanged. API v2 adds
exactly `permissions` and `target`; unknown/duplicate fields or permissions fail.

```json
{
  "id":"nes-native-lab.gomez-debug-hud",
  "name":"Gomez Debug HUD",
  "version":"1.0.0",
  "api_version":2,
  "author":"NES Native Lab",
  "description":"Read-only completed-state player HUD.",
  "entry":"main.lua",
  "permissions":["observe.player","overlay.text"],
  "target":{
    "game_id":"addams-family-pal-eu-enfrde",
    "engine_min":"0.9.0",
    "engine_max_exclusive":"0.10.0"
  }
}
```

Numeric major.minor.patch comparisons, inclusive minimum/exclusive maximum,
exact game-profile identity. No wildcard/range expressions or prerelease syntax
in manifests. The frontend passes its numeric engine version and selected game
profile. Installation and enable/startup require compatible context. Listing,
disable and removal stay available for recovery after incompatibility.
Installation is disabled by default and never executes the script.

Existing ZIP/link/expansion/text/transaction restrictions still apply. Dependency
resolution, conflicts, profiles, hot updates and configuration schemas are not
implemented by this slice. Existing IDs cannot be overwritten; uninstall before
replacing. No Internet update channel exists.

## Callbacks and captures

`MOD_API_VERSION == 2`. `on_load()` and `on_unload()` retain lifecycle semantics.
`on_update(dt)` runs once **after** an existing native simulation unit, hooks,
diagnostic and audio delivery, before its framebuffer snapshot is published.
The application supplies the original PAL interval `1 / 50.007`; this describes
host scheduling, not every partial native slice's exact CPU duration. v1 still
updates before native execution with its original interval.

This boundary is a host completed-unit capture, not a new subcycle, scanline or
PPU sprite-phase guarantee. Startup is unavailable; enabling while paused uses
the last completed observation (or unavailable if none). No update during pause,
focus suspension, the Mods panel or repeated 60/90/120 presentations. `--mods-only`
has no game updates. Render code never invokes Lua or reads game memory.

Always available: `log.info(text)` and `game.get_status()`, returning a detached
table with frame, cpu_cycles, simulation_hz, state, and, when known, game_id and
room_id. State vocabulary in the Addams adapter: unavailable, menu, transition,
dead, playing. `room_id` is a numeric level ID without an invented room name.
Legacy v1 room_id keeps its existing screen-selector meaning; do not equate it
with v2's level ID.

Permission `observe.player` exposes `game.get_player()`. Returns nil if unknown,
otherwise a detached table:

```lua
{x=76, y=160, space="nes_screen", coordinate_kind="player_reference"}
```

Coordinates are the game's player drawing reference, **not** a world position,
bounding box, feet location or an interpolated position. The current adapter
only certifies ordinary movement in level 0; menus, death, transitions, stairs,
rope/attachment/special contexts and other levels return nil. HUDs must display
unavailability explicitly. The native observer uses only const RAM/frame/cycle
accessors; no bus reads with register side effects.

## Text overlay

Permission `overlay.text` exposes `hud.text(x,y,text)` only in on_load/on_update.
Coordinates are integers in a **256 × 40** virtual strip anchored inside the top
of the original 4:3 game viewport. This is a UI layer, never NES pixels.
6-pixel character advance, 7-pixel height, up to 40 characters and 8 text
primitives per mod per callback. Text must fit completely inside the strip.
Supported glyphs are letters (displayed uppercase), digits and the existing
small UI font's punctuation; unsupported/control/NUL/non-ASCII characters fail.
No images, shapes, colors, font downloads or arbitrary resources in this slice.

Each successful callback replaces that mod's entire display list. Missing update
callback therefore clears its previous list at the next unit. Failure publishes
no partial or stale list; the Manager disables that mod. Normal disable, uninstall
and shutdown remove its list. There is no graphics callback.

The Manager composites bounded detached lists in ascending mod-ID order; later
IDs paint over earlier IDs. This is deterministic layering, **not** a general
conflict resolver. Maximum 32 mods × 8 primitives; a fixed 40 KiB RGBA bitmap is
rebuilt on completed v2 updates/activation changes and reused during extra draws.
The current existing OpenGL UI path uploads this small bitmap for every draw;
there is no new GPU texture cache. It hides while the Mods panel is open and sits
below built-in controls/exit dialogs. Resize/fullscreen uses original_viewport;
NES texture, aspect ratio and snapshot identity are unchanged.

```lua
function on_update(dt)
    local s=game.get_status()
    local p=game.get_player()
    hud.text(4,2,"MY HUD")
    hud.text(4,12,p and ("X "..p.x.." Y "..p.y) or "PLAYER UNAVAILABLE")
end
```

No memory write/read API, host pointer, input injection, gameplay modifier,
filesystem, networking, process creation, randomizer or event API is added.
Existing per-state 2 MiB Lua allocator, VM instruction/deadline budgets, blocked
libraries and conservative recovery remain. Host text allocations are separately
bounded. This remains in-process isolation, not an OS sandbox; use reviewed mods.

## Recovered continuation: generic SDK memory

The separate feature/modding-sdk-v2 worktree adds memory.read/memory.observe as
additive explicit API2 permissions. See MODDING_SDK_V2_API.md for actual functions,
canonical RAM bounds, lifecycle, quotas and completed-snapshot limitations. The
prior two-permission HUD delivery retains its original manifest behavior.

## Completion01 additive metadata and configuration (2026-10-14)

In the isolated Completion host, API2 optionally accepts `category`,
`dependencies`, `conflicts` and `settings`. No API3/native ABI change. Legacy
v1 and original API2 manifests/ZIPs remain valid unchanged. Each declared setting
has a validated type, label/default; numeric types require min/max/step; enum
requires bounded unique choices. Maximum eight settings. See
[User Guide](MOD_MANAGER_USER_GUIDE.md) and [Completion report](MOD_MANAGER_COMPLETION_01.md).

`config.get("extension")` returns the declared typed current value: boolean,
Lua integer, Lua number, or enum string. Unknown IDs raise an error. API1 has no
`config` global. It exposes no storage paths/native pointers. Applying a setting
persists atomically and restarts that mod's bounded lifecycle at the paused UI
boundary; it does not mutate a running Lua state. Extra presentations never
invoke settings callbacks. Existing package scripts remain unchanged; the new
Configurable Jump Example demonstrates all four types through existing jump/UI
capabilities. Its integer controls the existing initial-room 0..12 request, not
new physics. It conflicts with the original Super Gómez owner.

Dependencies map mod IDs to numeric `min`/`max_exclusive` version ranges;
conflicts list mod IDs. Enable/Profile Apply require installed enabled compatible
dependencies and reject cycles/conflicts before writing. Execution remains sorted
mod-ID order; dependencies do not introduce cross-state Lua calls or a general
hook priority resolver. Failed dependencies conservatively deactivate dependents.

## Completion02 compatibility (2026-10-14)

The Lua API remains version2; native RuntimeServices remains ABI3. No new Lua
gameplay, write-memory or graphics API is introduced. Applying an unchanged
effective configuration or profile keeps active VMs and callback state intact.
Restore Defaults submits declared defaults through the same validation path.
A changed active setting still restarts its bounded lifecycle; successful JSON
persistence is not a promise that later Lua initialization/unload will succeed.

Profile exchange is a separate host-only schema1 document with exact game/engine,
mod version/API requirements and typed settings. It grants no permissions and
contains no scripts. Inspection/import never executes Lua; import stores an
inactive profile; Apply requires explicit review and revalidation. See
MOD_MANAGER_USER_GUIDE.md and MOD_MANAGER_COMPLETION_02.md. Old v1/API2 ZIPs and
existing schema1 manager state remain compatible. Default experimental store
`mods-completion02` isolates this host from the published beta.
