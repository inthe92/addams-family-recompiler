# Mod Manager Completion02 — CI and consolidation plan

2026-10-14. Local feature/mod-manager-completion-02 only. No commit/push/workflow
execution/release authorized or performed in this delivery. Historical HEAD95c194a
is not the accumulated SDK source on its own.

## Source publication boundary

MOD_MANAGER_PUBLIC_FILES.txt is the explicit allowlist of cumulative changed/new
public files from SDK1–4B, Phase5 and Completion01/02. No ROM/import receipts/
module/cache/capture/trace/build/owner file is eligible. The preparation helper
requires exact agreement with current changed/untracked files, refuses links/
private binary suffixes/credential-like content and exports a fresh source tree
with SHA256/provenance manifest outside Git. It never stages or publishes.

```sh
python3 -B tools/public/prepare_mod_manager_export.py --destination <fresh-source-export> --manifest <outside-source>/source-manifest.json
python3 -B tools/public/audit_public.py <fresh-source-export>
git diff --check
```

The allowlist must be reviewed by a human/agent before publication; it is not an
automatic authorization for unknown later work. Technical checks and pinned
third-party archives are not legal certification or a full security audit.

## Only after subsequent explicit publication authorization

```sh
git branch --show-current
git status --short
git fetch origin
git log --oneline --left-right HEAD...origin/feature/mod-manager-completion-02
# If the remote branch is new, verify the agreed base instead. Stop on remote advancement.
# Work on feature/mod-manager-completion-02; never reset/rebase/force-push.
git add --pathspec-from-file=docs/MOD_MANAGER_PUBLIC_FILES.txt
git diff --cached --check
git diff --cached --name-only
# Confirm the staged list exactly matches the reviewed allowlist and no foreign files.
git commit -m "Consolidate SDK and stabilize experimental Mod Manager"
git rev-parse HEAD
git push -u origin feature/mod-manager-completion-02
# Push triggers the existing four-platform workflow. Inspect only the new SHA.
gh run list --repo inthe92/addams-family-recompiler --branch feature/mod-manager-completion-02 --workflow build.yml --json databaseId,headSha,status,conclusion,url
```

Do not automatically commit a later CI report, open a PR, merge main, tag or
publish a release. No command above has been executed for publication here.

## Matrix and evidence

Existing workflow: macos-15 ARM64, macos-15-intel x86_64, windows-latest x64 with
MSYS2 UCRT64, ubuntu Linux x64/AppImage. Native execution is pending for Intel,
Windows/Linux and physical Deck. Build desktop+mods; local headless suite remains
a required reproducible comparison, not a new unverified hosted matrix claim.
CMake registers Completion02 navigation/store and diagnostic eligibility tests,
with old mandatory tests preserved. JUnit/inventory gates require every test,
actual counts and no skips/expected-failure flags. New graphics probe is explicit:
run mod_completion02_gl_checks only with a real valid context; unavailable context
is NOT RUN, not a passing skip. Linux historical Mesa/Xvfb discrepancy remains open.

CTest and strict results gate continue to fail for A02. Separate diagnostic
package eligibility validates the same complete inventory/JUnit, reports
suite_pass=false/failures=[A02], and permits diagnostics only with no other failed,
missing, skipped or incomplete-count tests. An unexpected failure blocks toolchain/
packaging. This does not change CTest/CI status or claim the suite PASS.

All platform package checkers use the packaged Python/helper and now install the
four unchanged authored ZIPs, validate image assets/config/defaults and execute
profile export/inspection/import/apply/delete/persistence with public synthetic
data. This is helper validation; Lua/native rendering/physical controls require
the corresponding native/GL tests. Synthetic import compiles/loads ABI3 C code,
with private commercial inputs forbidden.

Keep ctest-inventory.json, ctest-results.xml, LastTest.log, mod-package-eligibility.json,
package verification JSON/logs and build identity per exact new SHA. All artifacts
must share commit, dirty=false after authorized commit, numeric version0.9.0,
ABI3, source fingerprint, icons and experimental component marker. Default store
mods-completion02 separates this component from older hosts/beta. .gitattributes
keeps reviewed script/manifest bytes LF and treats icons/own glyph/archive files
as binary, preventing CRLF-only ZIP identity drift.

## Extra native graphics probe (prepared, not a hosted result)

After generating the four authored example ZIPs through the existing example
builders, invoke `build/mod_completion02_gl_checks <fresh-store> <ZIP-directory>`
(executable suffix .exe on Windows). On a native graphical session this exercises
real GL readback, resize/fullscreen, events, profiles/defaults and delayed modal
focus without a ROM. On Linux the comparable command is
`xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 build/mod_completion02_gl_checks <fresh-store> <ZIP-directory>`;
that is virtual-display Mesa, not physical Deck/controller/scanout certification.
The existing workflow builds this probe but does not yet run it as a mandatory
CTest test; include its separate log/result in the authorized native acceptance
run. An unavailable context is NOT RUN, not a skipped PASS. Existing high-refresh
GL workflow checks remain unchanged and their historical Linux issue stays visible.

## Completion03 authorized integration

Current target is feature/mod-manager-beta, not the earlier example publication
branch. The coherent accumulated SDK is committed only after explicit staging/
source audit. A new clean local clone must configure/build desktop and headless
without private/worktree dependencies. Push this branch only, never --force.
The existing workflow runs four native jobs on the exact commit. macOS and Linux
now run Completion02's actual GL probe with the four generated unchanged ZIPs;
a manager GL failure blocks packaging. Windows context remains unverified and
NOT RUN. Existing high-refresh checks and Linux discrepancy remain visible.
Experimental artifact labels start mod-manager-beta; exact package filenames
are unchanged. See Completion03 for actual outcomes and final publication SHA.


## Completion03 executed portability follow-up

See MOD_MANAGER_COMPLETION_03.md for exact runs/commits and artifact hashes.
Native Windows UTF-8 fixtures now use wide command-line conversion, authored ZIP
creator metadata is fixed, and build Python3.11 is explicitly selected. Original
hash/count/Unicode expectations remain unchanged. All four native CTest inventories
at d0f9183 and the follow-up execute85 entries:84PASS/1FAIL A02/0omitted.

Windows now also attempts the actual manager GL probe as a distinct ordinary
failing step. `Missing OpenGL function: glCreateShader` is retained in its log;
this is a BLOCKED hosted graphical validation, not a passing/not-executed test.
The separate portable-payload/importer check uses `--import-only` for the native
picker-cancel and invalid-input cases. CLI/importer success must not imply normal
menu/game boot success. A diagnostic ZIP may be produced independently, just as
Linux diagnostic packaging remains distinct from the known high-refresh GL FAIL.
There is no continue-on-error and neither graphics failure is reclassified by the
A02-only CTest eligibility gate. No additional NES/ABI/renderer changes are made.
Physical Windows/Deck graphics and controllers still require manual evidence.
