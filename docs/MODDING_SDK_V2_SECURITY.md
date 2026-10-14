# SDK v2 security and capability boundaries

2026-10-13. Extends MOD_SECURITY.md; same in-process Lua restrictions, not an OS
sandbox, malware guarantee or security certification. Use reviewed ZIPs.

New memory.read/observe capabilities receive **owned 2 KiB RAM captures only**.
Integer canonical address/range validation, ≤64 bytes/read, ≤4 observers and128
watched bytes/mod. No overlaps or registration outside on_load. Neither values
nor Lua tables alias the emulator. No bus register peeks, ROM/device read, write,
process pointer, native execution or executable resource decoder. API1 grants no
new permissions. Unknown or observe-without-read declarations reject installation.

Existing ZIP limits/path/link/signature checks, disabled installation, local-only
transactions, conservative unclean-session recovery and exact API/engine/profile
checks retained. Requirements are rechecked at enable/startup. No online catalogue
or automatic updates. Installed manifests remain subject to same-user tampering
limits; one process's VM restrictions do not isolate a compromised host process.

Each VM has2MiB allocator limit. Count hook every1000 instructions; entire v2
observer batch+on_update shares100000 instructions/20ms sampled monotonic deadline,
not a reset budget per observer. Lua C library work is not hard preempted. Logs
bounded128×300 characters/state, drained to a manager-owned private ~1MiB file.
Logging/filesystem time is outside the VM deadline and can add host latency.
Snapshot/observer lists and callback deltas have separate fixed bounds. Budget,
argument or callback errors disable the mod; partial HUD and observer references
are removed, persistence failure reported. Disable/unload/shutdown releases refs;
force termination has no unload guarantee, so startup recovery remains necessary.

No active observer means no snapshot copy; --no-mods creates no managed state,
helper/storage or hooks. Rendering never dispatches observers. Data only sampled
at completed simulation units; no instruction-level security/timing promise.
Do not distribute private memory logs with game data.

Future dangerous write/hook/resource/input capabilities must be denied by default,
shown with understandable scope/risk and granted explicitly in the UI. A manifest
alone is not user consent for those future capabilities. Require validated
profiles, owner leases, deterministic overlapping-write conflict rejection,
cleanup and acceptance tests. Revoking a lease prevents future actions; it cannot
rewind motion/collisions/items already changed. No such dangerous API exists yet.

Pending: fuzzing, independent review, other-platform filesystems, physical Steam
Deck, broad C-library time profiles, instance concurrency and OS-level isolation.
A02 and Linux GL failures are not changed to claim safe or accurate execution.
