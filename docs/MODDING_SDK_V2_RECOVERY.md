# Modding SDK v2 — recovery before development

2026-10-13. Initial inspected checkout: `outputs/mod-manager-v2`, branch
feature/mod-manager-v2, HEAD95c194afac4574be9bf8b495341517389a73988b. 19 modified
tracked files and 16 untracked authored files (33 porcelain rows because the
example directory is collapsed). No staged commit, conflict or incomplete Git
operation observed. Work was not empty or lost.

| Local worktree/branch | HEAD | State to preserve |
|---|---|---|
| public-alpha-0.2.0/addams-family-recompiler / diagnostic/high-refresh-phase-a | 80af76aa5e4408f37dcb0e7047355c21be186ae8 | Existing unrelated research/reports; not changed |
| mod-manager-v1 / feature/mod-manager-v1 | 95c194afac4574be9bf8b495341517389a73988b | Published v1 code, pending CI report |
| super-gomez-v1 / feature/super-gomez-v1 | 95c194afac4574be9bf8b495341517389a73988b | Uncommitted bounded initial-room power, separate tests/docs; not merged |
| mod-manager-v2 / feature/mod-manager-v2 | 95c194afac4574be9bf8b495341517389a73988b | Uncommitted API2, read-only game adapter, bounded text HUD, tests/docs |
| modding-sdk-v2 / feature/modding-sdk-v2 | 95c194afac4574be9bf8b495341517389a73988b | New independent continuation with a verified copy of the preceding v2 work |

## Recovered rather than reimplemented

API v1 compatibility; API v2 profile/engine requirements and explicit observe/
text permissions; completed-state capture; conservative initial-room player
observer; detached Lua values; bounded transactional HUD; installer/front-end
integration; deterministic synthetic native tests; authored Gomez Debug HUD ZIP
and package verification. See MOD_MANAGER_2_PHASE_1.md for actual prior evidence:
macOS ARM64 desktop59/60, headless53/54, A02 sole failure; 13 real GL assertions;
private sprite-reference/native determinism experiment and real initial-room UI.
These are **prior delivery** results, not certification of subsequent SDK changes.

Completed files include MOD_API_V2, MOD_MANAGER_2_ROADMAP and MOD_MANAGER_2_PHASE_1.
Temporary builds/test evidence and ignored private module/trajectory data exist
outside all public worktrees; no game content was copied into this continuation.

Not implemented: generic memory API/watchers, generic execution hooks, controlled
writes, dependency resolution, profiles, storage API, images/audio overrides,
translation, randomizer or wider gameplay API. First vertical slice was functional
but did not claim those capabilities. The separately bounded Super Gomez is a
reference only; its specialized capability/schema is not silently imported.

## Controlled integration decision

New branch/worktree from exact published95c194a. Applied the tracked binary Git
diff and copied only Git-listed untracked authored files; compared their bytes
and retained a local SHA256 snapshot. Original checkout remains dirty and intact.
No reset/clean/switch/commit/merge/push. Reuse its proven captures, package/VM/UI
contracts; add **RAM-snapshot-only** observation as a narrow generic extension.

Risks: two independent experimental permission schemas (Super Gomez vs SDK),
pre-/post-native v1/v2 callback ordering, cache/metadata provenance of dirty builds,
false all-room player claims, same-user filesystem tampering, callback/log cost,
known A02 and Linux GL mismatch. No core ABI/timing/emitter correction is required
for side-effect-free completed RAM snapshots. Production hooks/writes remain a
future design until timing, conflict and permission acceptance tests exist.
