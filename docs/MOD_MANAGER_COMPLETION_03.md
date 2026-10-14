# Mod Manager Completion03 — integration and native CI

2026-10-14. Authorized experimental branch: feature/mod-manager-beta.
No main edit, merge, release or modification of public v0.9.0-beta.

## Initial recovery

Base: Completion02, historical95c194afac4574be9bf8b495341517389a73988b
plus159 reviewed changed/new public paths. Full352-file public source export
matched the prior SHA inventory. All146 recovered Completion01 files and the
original Completion02 worktree remain intact. Native independent successor
recovered all159 file hashes. No unrelated mobile/PortMaster/PAL-NTSC work.

Baseline evidence is historical Completion02: desktop84/85, headless76/77,
OpenGL57 checks; A02 sole ordinary failure. These are not results of the new SHA.

## Integration scope

One coherent consolidation commit is preferred to broken intermediate SDK states.
Only accumulated public source, owned fixtures, tests, packaging and reviewed
technical documentation are included. Native Mod Manager GL checks are prepared
in macOS runners and Linux Mesa/Xvfb with the four unchanged authored ZIPs.
Windows graphics remains NOT RUN unless an actual supported context is available.
New manager GL failures block diagnostic packaging independently of A02.
Artifact labels identify Mod Manager Beta separately from the public beta.
Actual executable/ZIP filenames and engine0.9.0/ABI3 remain unchanged.

## Safety and procedure

Explicit reviewed allowlist, SHA/provenance export, dependency archive hashes/
licences, credential/path/binary scans, git diff --check and staged-list equality
are required before publication. The public build needs no ROM. Private game
modules, owner stores, captures/traces/receipts and commercial resources are
excluded. Technical checks are not legal certification.

Clean-clone Release desktop/headless build and complete unfiltered CTest/JUnit,
strict gate and separate diagnostic eligibility will be recorded here. A02 stays
FAIL; no skips/expectation changes. Native package checks must use the exact new
SHA, bundled importer tools and safe synthetic inputs.

## Current status

Consolidation/build/push/native CI: pending execution in this report's initial
revision. Final commands, SHAs, URLs, artifacts and results will be appended only
after actual execution. Steam Deck/physical gamepad and owned-ROM manual sessions
remain NOT RUN; no Rocket Boots, sprites, new physics or Scene API in this task.

## First published verification — executed

Consolidation cca5937c3ca267f439969bf062b3cc1aeff2432b; one whitespace-only
cleanup cb96b2162798066b7f2d746edb662896b0fca36c was applied before first push.
The160-path staged set and353-file export passed public checks;350reachable
historical blobs had no private filename/credential candidates. Original159
Completion02 SHA hashes remain intact. Core/ABI/A02/jump adapter bytes unchanged.

Clean independent clone at cb96b2162798066b7f2d746edb662896b0fca36c built both
Release configurations: desktop84/85, headless76/77, no skips, A02 only. Actual
SDL/OpenGL57 checks PASS. Clean-clone ARM64 package extracted and passed strict
ad-hoc signing, bundled Python/Zig/helper/mod/profile checks, synthetic ABI3
compile/load(return42) and import process3scenarios. Not a physical gameplay test.

Authorized push created feature/mod-manager-beta and triggered
https://github.com/inthe92/addams-family-recompiler/actions/runs/38638249382
on that exact SHA. ARM64 native CI produced its package with A02 as the sole
CTest failure. Linux build exposed missing explicit <memory> in a synthetic SDK
test. Windows build exposed an implicit path-to-string conversion in a synthetic
resource-catalog fixture. Both are test compilation portability defects, not NES
logic/A02/graphics defects. Corrections add <memory> and explicit UTF-8 conversion;
no expectations or count guards are relaxed. Local focused tests pass; subsequent
exact-SHA native CI is required before declaring them fixed on those platforms.
