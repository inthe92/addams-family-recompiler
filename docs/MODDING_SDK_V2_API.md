# Modding SDK v2 — developer guide

Implemented in the local feature/modding-sdk-v2 continuation. Mod API version is
2; native runtime module ABI remains3. The recovered game/HUD APIs in MOD_API_V2
remain supported. **Only canonical RAM snapshots/read-only observers are new.**

## Make your own mod without C++

ZIP root: mod.json and main.lua; optional passive assets under assets/. Package
source data must be yours. This manifest is an actual example, not future syntax:

```json
{
 "id":"sdk.ram-change-logger","name":"RAM Change Logger","version":"1.0.0",
 "api_version":2,"description":"Generic read-only RAM snapshot change log.",
 "author":"NES Native Lab","entry":"main.lua",
 "permissions":["memory.read","memory.observe"],
 "target":{"game_id":"addams-family-pal-eu-enfrde","engine_min":"0.9.0",
           "engine_max_exclusive":"0.10.0"}
}
```

All fields required, unknown fields rejected. API1 ZIPs remain accepted unchanged.
Known v2 permissions: observe.player, overlay.text, memory.read, memory.observe.
Memory.observe requires memory.read. No write/advanced-hook/assets/storage/input
permissions or APIs are accepted yet. Install/enable/startup verifies requirements;
exact game ID and numeric version range, not wildcard ranges. Unknown profiles
fail closed. Installing inspects/extracts only; no scripts execute until Enable.
Enabling explicitly authorizes these low-risk read/overlay capabilities.

Lua lifecycle on_load/on_update(dt)/on_unload and private log.info remain. Mods run
in ascending ID order; observers in registration order; each change list sorts by
increasing canonical address. Errors disable only that mod and remove observers.

## nes.memory (available)

| Function | Permission | Contract |
|---|---|---|
| read_u8(address) | memory.read | One integer address0..2047, returns detached integer0..255, or nil if no completed capture |
| read_range(first,length) | memory.read | Canonical start, integer length1..64, entire range ≤2048; new 1-based Lua byte array or nil |
| observe(first,length,callback) | memory.read + memory.observe | Only inside on_load; same bounds; returns registration token; at most4 non-overlapping ranges, total128 bytes per mod |

Only $0000–$07FF. Mirrors $0800–$1FFF are deliberately rejected rather than
normalizing ambiguous identities. PPU/APU/controller registers, cartridge RAM,
PRG/CHR-ROM and device memory are unavailable and rejected. No host pointer,
peek/read with device effects, write function or PC hook.

```lua
function on_load()
    nes.memory.observe(0,16,function(changes)
        for _,c in ipairs(changes) do
            log.info("RAM "..c.address.." "..c.old_value.." TO "..c.new_value)
        end
    end)
end
```

Callback receives a detached array of {address,old_value,new_value}. The first
available completed capture establishes a baseline silently. Unchanged ranges
produce no call. Missing capture invalidates the baseline; the next available
capture re-establishes it without synthetic changes. Observers notify before that
mod's on_update using the **same** completed snapshot and shared unit budget.
Mutating returned tables affects neither RAM nor host comparisons.

Registration cannot occur during the source chunk, on_update, an observer or
on_unload. There is no per-observer unobserve API in this first slice: disable,
uninstall, failure, unload or shutdown releases every registry reference/list for
that mod. Re-enable constructs a new state/baseline. No callbacks during pause or
extra presentation draws. Lifetime logging does not imply memory/game changes
are undone; this slice performs no writes at all.

Important: this is sampling, not every write. Transient changes that return to
baseline before the capture are not observable. It is not suitable for subcycle
IRQ diagnosis or instruction-precise gameplay patching. Game semantics are not
inferred from numeric bytes. Check game/profile-specific evidence before calling
a byte lives, item, player, ending or a randomizer variable.

## Two real examples

- examples/mods/ram-change-logger: canonical range0..15; private delta log.
- examples/mods/ram-change-counter: range256..271; counts changed bytes and
  notification batches in the recovered bounded HUD. These bytes are not claimed
  as any particular game variable. Same API, no C++ for the second example.

Build their ZIPs (only authored text):

```sh
python3 -B tools/mods/make_sdk_examples.py /private/tmp/nes-sdk-mods
```

Use this **SDK development frontend**, not the published beta: F2 → Install Mod
(select ZIP, or Linux drop ZIP into panel) → Enable → Back. On_load executes at
Enable, updates after game units; --mods-only has no simulation/changes. Logs are
mods.log in the private Mods directory. Disable removes all callbacks/HUD. Use
--no-mods for managed/legacy bypass. A new script/range/package ID using these
APIs requires rebuilding its ZIP, **not recompiling the application**.

## Planned API namespaces — unavailable

nes.hooks: filtered bank/PC/access metadata, scope-labelled and batched; resource
and safety policy first, no interpreter/per-instruction Lua default.
nes.events: session/update/pause/reset/lifecycle; profile events later.
nes.storage: per-ID typed JSON/read/write with quota and atomic host operations.
nes.graphics: extend current hud.text with bounded shapes/assets/coordinate spaces.
nes.assets: profile-mapped texts/codecs/own glyphs/tiles/audio domains.
nes.input: snapshot observation then explicit deterministic transform leases.
nes.memory writes: separate reviewed allowlists/consent/conflict/phase contracts.
These names are designs only; calling them today is unsupported.
