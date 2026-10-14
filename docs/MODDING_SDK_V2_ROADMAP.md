# SDK v2 roadmap — phased acceptance

First delivered: recover completed-state HUD/API2 and add generic canonical RAM
read-only captures/observers with two authored ZIPs. Not a full modding platform.

| Phase | Plan, not already implemented | Acceptance before next phase |
|---|---|---|
| v2 continuation validation | Native four-platform CI, RAM observer/UI/package checks, physical Steam Deck | Same SHA/new tests/no skips, A02 visible; Linux GL mismatch separately diagnosed |
| v2.1 | Controlled writes, filtered hooks, ownership/conflicts, versioned declarative profiles | No-mod baseline identical including PCM/IRQ/RMW order; explicit risky consent; unknown profiles rejected; equivalent Super Gomez guard/collision tests before replacing it |
| v2.2 | Text/resource codecs, language packages, own fonts/glyphs | Verified private language/CHR mapping; layout/cursor bounds; no code/opcode byte override; authored language demo; no original resources in ZIPs |
| v2.3 | Existing text HUD broadened to shapes/images, speedrun events/splits and typed configuration/storage | Coordinate/asset quotas; detached render snapshots; real/simulation time separated; pause/reset/ending evidence; atomic per-ID persistence |
| v2.4 | Seeded randomizer, progression model and complex mods | Own reproducible RNG separate from game's; verified graph and solvability; replay/seed tests; no arbitrary memory hacks |
| Community catalogue | Reviewed JSON catalogue, ZIP hash/compatibility/signatures/PR process; eventual in-game download | Threat model, explicit install/activation, trusted provenance, no scripts on inspection and no insecure automatic updates |

Dependencies/conflicts/version migrations/profiles/transactional replacement are
still required before general multi-mod gameplay. Existing install refuses ID
replacement; no online updates or configuration export/import provided today.
The recovered initial-room Super Gomez and beta remain separate and unchanged.
No Super Speed/all-room eligibility or full main-menu rewrite in this delivery.
