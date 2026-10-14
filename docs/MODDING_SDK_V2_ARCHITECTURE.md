# Modding SDK v2 — architecture and audit

2026-10-13, feature/modding-sdk-v2, base95c194afac4574be9bf8b495341517389a73988b,
uncommitted. Recovered code/evidence is in MODDING_SDK_V2_RECOVERY.md.

## Four layers

1. **NES Native Runtime:** original CPU state, generated C, timed bus, MMC1, PPU,
   APU, IRQ/NMI/DMA. Existing ABI v3 unchanged; no interpreter fallback.
2. **Modding SDK:** package/VM lifecycles, capabilities, detached captures,
   deterministic observer dispatch, bounded host logging and UI display lists.
   No Bus/CPU/process pointer exposed to Lua. Newly implemented generic memory
   observes only completed canonical internal RAM snapshots.
3. **Game Profiles:** current supported SHA-validated European revision and
   conservative compiled game-observation adapter. A declarative symbol/event/
   resource schema is planned, not loaded/executed today. Profile metadata cannot
   authorize arbitrary native functions or substitute for ROM validation.
4. **Mod Packages:** installed disabled ZIPs with reviewed Lua/own assets and
   engine/game/API requirements. New scripts using existing primitives require
   no C++ changes or frontend recompilation. Profile compatibility remains exact;
   a generic RAM primitive is not universal NES game support.

## Integration and timing

app/desktop.cpp's original simulation body retains hooks, native execution,
diagnostics and audio. v1 still updates before native execution. v2 completed
updates happen after that original unit. Only when active scripts need memory,
the host copies 2 KiB via const MainRAM::read; it never calls Bus::read8 or touches
registers. Manager shares that capture into detached per-state observations.
Lua observers compare unit-final captures, then on_update runs, under **one**
100,000-instruction/20ms sampled guard for that entire v2 unit. Drawing consumes
an already published bitmap; 60/90/120 repeated presentations do not dispatch
observers, run Lua, add audio or alter PAL logic. Pause freezes the capture.

No-mods creates no managed VM/store and installs no hardware hooks. Inactive/no
memory-capability scripts cause no RAM copy. This observer is not a write trace:
intermediate changes or changes reversed within a unit cannot be recovered.

## Why there is no universal interception point

Verified emit_blocks.py/read_operand translates immediate operands to C literals,
uses recomp_read8/read16_zp for data and pointer reads, emits page/dummy reads,
and emits RMW read/old-write/new-write with separate offsets. CPU->PC, flags and
registers are direct C state operations. Opcode/operand fetches are not all
represented as runtime bus accesses; static compilation already consumes them.
A RuntimeServices wrapper cannot override an immediate embedded in generated C.

runtime_services.h forwards timed data reads/writes plus advance, boundary and
poll_irq. Instructions carry poll metadata; IRQ decisions must not be changed by
mod observation. Bus::sync_devices advances PPU/APU to access offsets; $2002,
$4015 and controller reads have effects. These cannot be diagnostic peeks.
Bus::deliver_interrupt and perform_dma perform their own internal read8/write8
calls; they do not necessarily pass through an outer RuntimeServices wrapper.
VideoState::draw_pixel/read_memory obtains CHR via Cartridge::chr_read, bypassing
CPU read8 entirely. MMC1 bank mapping adds identity/state to PRG addresses.

Therefore future PC/bus/resource hooks need distinct typed contracts and scope
labels. Do not claim that wrapping read8 intercepts all NES access or that polling
RAM catches every write. Do not add Lua calls per instruction by default. Prefer
bounded native capture/filter hooks that batch events at safe boundaries; altering
emitter instruction metadata/callback contract might require regenerated private
modules and explicit ABI/cache version control. None is implemented in this slice.

## Future writes, events and resources (design only)

Writes: named profile allowlists, masks/ranges, safe phase and state predicates,
explicit UI consent, per-owner leases and deterministic conflict rejection.
Hardware-register/ROM writes are not initially allowed. Deactivation revokes future
requests, not collisions/items/positions already changed during play.

Events: verified session/simulation/activation/pause/reset records with sequence
numbers; profile-derived death/items/rooms only after independent evidence.
Storage: host-owned per-ID JSON with quotas, strict typed schemas and atomic writes;
no Lua filesystem. Input: immutable sampled controls; future transformations have
explicit opt-in order and replay tests, without modifying original RNG/calendar.

Resources: separate typed text/tile/glyph/CHR/audio domains with profile identity,
size/layout and source fingerprints validated locally. Keep generated game content
private. Overlays are separate from NES pixels. Audio resource/timing overrides
cannot be invented as generic PCM replacements without APU/DMC integration tests.
The existing private translated module remains the executor; no second CPU.

## Super Gomez migration assessment

The independent feature/super-gomez-v1 reference retains a bounded initial-room
phase extension, not a generic velocity/gravity setter. It wraps generated
RuntimeServices writes at two verified instructions, retains RMW offset order and
poll forwarding, applies state/collision guards and revokes the remaining lease.
Its 36-pixel private jump improvement is historical evidence, not a new SDK test.
No Super Speed/all-room implementation was recovered or enabled here.

A future generic filtered write hook would need bank+PC+address+offset+old/new
value predicates and a declarative safe transformation/lease policy, preserving
CPU flags/cycles and original collision processing. Lua should request the
capability, not execute on every write. Profile/permission/ownership design and
all equivalent public/private collision tests must pass before migration. The
original Super Gomez worktree is untouched. This SDK exposes no write/hook API.
