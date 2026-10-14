# Mod Manager 2.0 — roadmap and first vertical slice

2026-10-13. Development worktree `outputs/mod-manager-v2`, branch `feature/mod-manager-v2`, base `95c194afac4574be9bf8b495341517389a73988b`. No commit/push/release or beta replacement is authorized here. The v1 worktree's pending CI report, separate Super Gomez experiment and original high-refresh/PAL research remain untouched.

## Audited baseline

The published v1 code has bounded ZIP transactions, strict seven-field manifests, disabled-by-default installation, persistence, per-mod Lua states, lifecycle/update callbacks, budgets, error deactivation, conservative unclean-session recovery and an SDL/bitmap/OpenGL Mods panel. API v1 is read-only. Player coordinates are nil for Addams; its legacy room field actually samples the screen selector, not the level ID. That legacy observable must not silently change in a compatibility release.

The frontend's active hardware path and ABI v3 are separate from the manager. v1 updates precede native simulation once per original NES unit. PresentationClock repeats immutable images at 60/90/120; it does not authorize extra Lua updates/audio/hooks. Existing default 256x240/4:3 and PAL timing must stay unchanged. A02 remains an ordinary failing test. Historical Linux Xvfb/Mesa pixel mismatch is not resolved by this work.

Previous docs describe historical snapshots, not today's certification. The v1 CI report is local/untracked in its original worktree. No stale platform results will be attributed to this branch.

## Layers and ownership

1. **NES hardware/core:** unchanged CPU/bus/PPU/APU/mapper and generated C/ABI. No mod-facing RAM API.
2. **Game adapter:** revision-specific, validated observations and, later, bounded actions. Owns address interpretation and unknown-state rules; generic mod services contain no game addresses.
3. **Generic mod services:** value snapshots, version/permission negotiation, deterministic lifecycle and per-owner resources.
4. **Lua:** isolated states, allowlisted methods and bounded work. Same-process defenses are not an OS security boundary.
5. **Overlay:** authored UI commands committed at a simulation boundary; renderer consumes a frozen host bitmap. No Lua render callback, live RAM lookup or framebuffer mutation.
6. **Packages/UI:** offline ZIP validation, compatibility and activation, private storage, existing UI pause/input protection. Future menu navigation uses an explicit caller context, not assumptions about Back.

No extra thread, new CPU, UI framework or dependency is needed for the first slice. Game-specific writes from the separate Super Gomez branch are not imported into this read-only delivery.

## First delivery: VS1 (subset of the larger phases)

Implement API v2 negotiation, read-only verified player observations, a bounded text overlay and Gomez Debug HUD. This is **not** completion of the entire advanced package phase.

- API v1 packages remain strict and retain their existing functions/update point.
- API v2 manifests require explicit permissions and a target: game profile ID plus numeric engine minimum and exclusive maximum. Unsupported API/permission/target is rejected without running Lua.
- v2 on_update runs once **after** the unchanged native/diagnostic/audio unit, using a detached completed-state observation. v1 remains before the unit. No v2 callback runs on repeated draws or pause.
- Initial player coverage is conservative: expose only contexts verified by private native execution. Return nil elsewhere; no actor-slot guess, fabricated coordinate or simulated data presented as gameplay.
- Overlay commands are bounded, validated ASCII text in a fixed NES-relative HUD region. Stage a new list per callback, publish only on success, remove on failure/disable. Existing owned font and UI texture are reused. The NES raster and viewport remain unchanged.
- Keep --mods-only as standalone diagnostics; Back closes it. In-game Back returns to the game. A main application menu is deferred.

Acceptance: native v1 compatibility; malicious v2 rejection; actual player movement/jump data observed with the owned ROM; no-mod and read-only observer determinism; pause/resize/fullscreen/disable safe; 50/60/90/120 update counts independent of draws; full CTest with A02 visible; ZIP and public-content audit; actual macOS ARM64 UI proof, other platforms clearly pending.

## Subsequent phases (planned, not implemented)

| Phase | Deliverable | Acceptance / risk |
|---|---|---|
| P1 Advanced packages | Strict bounded version requirements, mod dependency graph, conflicts, deterministic topological ordering, offline transactional update/rollback and profiles/config export/import | Cycles/missing deps/version mismatch fail explicitly; no scripts during inspection/install; crash-state fault injection and preserving original packages. High risk: multi-file atomicity and same-user filesystem races. No Internet updater. |
| P2 Observation/events | Expand independently verified adapter coverage; completed-unit events for sessions/rooms/player/death/reset/ending | No invented names or completion events; unknown/loading cases explicit; exactly once independent of render/pause. Distinguish screen, level and observed game state. |
| P3 Gameplay actions | Per-game bounded requests with owner leases, arbitration, safe simulation points and restoration | Private collision tests for ground/ceiling/walls/transitions/death/special states before enabling; no direct position patch, global clock change or arbitrary RAM. Keep same-process security limits explicit. |
| P4 Overlay primitives/assets | Rectangles, bars, own images and scaling/visibility | Separate coordinate spaces, clipping, aggregate quotas and asset decoding limits; immutable render list; no additional simulation callbacks. Resource/textures erased on disable. |
| P5 Options/profiles UI | Schema-versioned bool/ranged number/enum/bindings, keyboard/mouse/D-pad configuration, validated persistence and migrations | Unknown values/ranges fail; deterministic apply timing; profile import cannot install/enable unchecked code; user data separate from caches. |
| P6 Reference mods | Debug HUD (VS1), then seeded deterministic speedrun timer/splits and bounded Super Gomez | Separate real/simulation time and pauses; no false run-completion signals; each mod has its own ZIP/tests. Super Gomez requires wider collision acceptance beyond its existing initial-room experiment. |
| P7 Randomizer | Progression model, item/room dependencies, seeded generator independent of the game RNG | Reachability/solvability proof and reproducible replay; no claims from shuffling alone. This is a separate research phase. |
| Continuous security | Adversarial packages/arguments/quotas, recovery, isolation and native platform filesystem testing | Review exploit surface, bounded host allocations, fuzzing and force-close cases; never describe Lua's in-process restrictions as an OS sandbox. |

## Decisions to preserve

- No ABI v3 or private module/cache change for read-only snapshots/UI.
- Only side-effect-free MainRAM/clock/frame getters in the adapter; no PPU/APU register reads used for logging.
- Observations label the host completed-unit boundary, not an unverified subcycle or interpolated sprite/bounding box.
- Permissions are explicit capabilities, not arbitrary scripts granted access to host pointers.
- Package API version is distinct from engine version, game revision and runtime module ABI.
- Lua requests in future gameplay phases require policy for multiple owners; arbitrary last-writer-wins memory patching is rejected.
- Seeded randomness, dependencies, profiles, updates, assets, generic gameplay and all-room identity remain absent until implemented and tested.
- Steam Deck OLED physical controls/visibility, Windows/Intel/Linux native results and actual display cadence remain external verification requirements. CI readiness does not certify hardware.
