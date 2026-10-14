# Gameplay & Visual SDK — Phase5 architecture

2026-10-14, local `feature/gameplay-visual-sdk-phase5`, historical HEAD
`95c194afac4574be9bf8b495341517389a73988b` plus recovered, uncommitted SDK1–4B.
This is not the content of that published commit alone. No remote publication.

## Delivered separation

| Layer | Responsibility | Actual implementation |
|---|---|---|
| NES core | Generated native C, CPU/PPU/APU/MMC1, PAL, audio/input | Preserved from recovered SDK; no new interpreter or ABI changes |
| Approved game adapter | Bounded jump-phase decision at two verified writes | `game::SuperJump`, selectively reused from the owned Super Gómez worktree |
| Generic mod services | Lifecycle, permissions, deterministic captures, exclusive ownership | Existing Manager/LuaMod extended with gameplay requests and visual commands |
| Presentation | Frozen UI/owned PNG command composition, alpha planes, cached GL textures | `mod_visual.cpp`, `GLPresenter::present_mod_plane` |
| Package/UI manager | ZIP inspection, disabled installation, explicit Enable, persistence, errors | Existing helper, Panel and Frontend; no parallel manager |

A mod requests `jump_extension`; the adapter validates original player state and
intervenes at the original write. Lua is never invoked per instruction, pixel or
host presentation. This is a decision hook, distinct from read-only observations.
The approved adapter contains game-specific knowledge; Lua packages contain no
private addresses. New supported values/scripts do not require another host build.
A genuinely new mechanic still needs an audited adapter capability.

API2 observations and visual captures run after a completed NES unit, original
hooks/diagnostics and PCM delivery. The next simulation unit consumes the request.
Activating/deactivating via the paused Mods panel cannot interrupt an instruction.
The native service adapter forwards read/write/bank/advance/boundary/poll_irq.

## Ownership and order

Only one enabled `gameplay.jump` owner is allowed. Enable rejects conflicts before
persistence. Startup fails closed for conflicting restored owners. Requests are
revoked on unload, callback error, Disable and Uninstall. A trajectory already
changed is not rewound. Mods execute in sorted mod-ID order. Within each visual
plane, commands are sorted by z, then mod ID, then submission order.

Viewport plane draws first, bounded window plane second, existing legacy HUD next;
built-in controls/exit confirmation/Mods panel draw above them. z does not move a
command across planes. Both planes use owned 320×240 RGBA arrays, independent of
NES's 256×240 indexed framebuffer. Images are validated during inspection/activation; VM-owned pixels are loaded once and
frozen with the simulation capture. Extra 60/90/120 presentations reuse those
planes and cached GL textures, without extra Lua, input or audio execution.

## Quotas and failure isolation

Per VM: existing 2 MiB Lua budget, 100,000-instruction / 20 ms callback limit,
16 visual commands, 100,000 candidate pixels per capture, two declared images
at most 128×128 RGBA8 each (128 KiB total). At most eight active new UI/visual
mods; existing package cap remains 32. Shared CPU planes: 300 KiB each, allocated
only while used; command sorting uses a bounded stack array instead of a new
per-capture item vector. Same-sized GPU plane textures are released when empty. Failure
of one callback disables that owner, clears its requests/captures and recomposes
other mods. No native pointers, native execution, network or arbitrary RAM writes.
Lua in-process limits are not an OS sandbox or a hard GPU/real-time guarantee.

## Next Scene API — design only

Verified game state → immutable scene snapshot → optional renderer. Before
entity-linked layers or 2.5D, identify room/camera/world coordinates, actual scroll,
sprite identity/occlusion and transitions. Preserve original collision logic and
make apparent surfaces correspond to it. The delivered parallax camera is
synthetic decoration; it is not an extracted scene, background replacement or
2.5D support. Background behind an opaque NES image would be occluded; no pixels
are removed to expose it. Entity layers, 3D lighting, maps, enemy/item modifiers,
resource substitution and generalized execution hooks remain future work.
