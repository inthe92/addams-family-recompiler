# Mod Manager Completion02 — local stabilization

2026-10-14. IMPLEMENTED / VERIFIED locally on macOS ARM64; EXPERIMENTAL.
Not a new published beta and not four-platform certification.

## Recovery and source identity

Successor branch `feature/mod-manager-completion-02` was created from
`feature/mod-manager-completion`, historical HEAD
`95c194afac4574be9bf8b495341517389a73988b`. All 146 modified/new files were recovered
with SHA256 identity checks; the original worktree remains intact. That historical
commit alone does not contain this SDK. The successor remains dirty/uncommitted;
no commit, push, merge, main edit, tag, release or hosted workflow dispatch occurred.

Inspected existing worktrees: Mod Manager v1/v2, SDKv2/resources/phases3/4/4B,
Gameplay Visual Phase5, Completion01, Super Gómez and high-refresh diagnostic.
Completion01 was the coherent latest manager base. No unrelated PortMaster,
Android, mobile, PAL/NTSC or new gameplay experiment was imported. The recovered
Phase5 adapter/observers/HUD/own PNG/PRG backend were preserved, not reimplemented.

Recovery hashes, build logs, complete JUnit/inventories, GL, performance, package
and source-audit evidence remain outside the public source in a local evidence
folder. See the delivered `Evidence` directory for the final selected records.

## Implemented changes

- Profile Delete has Cancel-first confirmation, cannot delete Original, preserves
  installed mods/other profiles. Deleting an active label does not silently unload
  its currently running mods; use Original to disable them deliberately.
- Export/import host-only portable schema1 JSON: name, game/engine identity,
  exact mod/API versions, enable selections and typed settings. No scripts, ROM,
  cache, modules, secrets or absolute paths are included. Inspection never runs
  Lua; import is reviewed, SHA-checked and stores an inactive profile. Apply is a
  separate confirmed action and revalidates requirements.
- Strict duplicate/unknown-field/type/name/quota validation; maximum 64 KiB input,
  32 mods, 16 custom profiles plus Original. Exact version locks are conservative
  experimental compatibility, not general SemVer migration.
- Export atomically publishes without overwriting existing files. Local hard-link
  support is required; choose another filename or export locally then copy JSON.
- Restore Defaults uses declared boolean/integer/float/enum values, confirmation
  and existing validation. Changed active settings restart the bounded lifecycle;
  equivalent settings and effective profiles preserve Lua/callback state.
- Fixed profile settings loss: selecting a profile preserves settings of other
  disabled mods. Save materializes enabled mods' defaults, so another profile
  cannot accidentally change their later restored behavior.
- Keyboard/mouse/D-pad/A/B/shoulders and neutral-rearmed left-stick navigation;
  stick+D-pad within one event batch does not double-navigate. Blank actions are
  skipped. No new game input assignment or physics.
- Native profile pickers on macOS/Windows; Linux imports dropped JSON and exports
  under its store. Case-insensitive `.JSON` detection. Main/game/mods-only Back
  contexts and existing F2/F11/Escape pause/audio/neutral-input paths are retained.
- Real macOS selector exposed delayed SDL focus events canceling a newly created
  review. Reviews are deferred until the event batch completes; timestamped
  focus events from before that review cannot cancel it. A later real focus loss
  still cancels. Tick-wrap and unknown-timestamp behavior are tested. Native
  picker restores its own existing key window only while this app is active.
- Separate experimental default store `mods-completion02`; explicit `--mods-dir`
  still selects a reviewed store. `--no-mods` bypasses manager creation/execution.
- Diagnostic package eligibility checks complete strict JUnit/inventory. A02
  remains ordinary FAIL; only that known failure permits diagnostic artifacts.
  A new failure, omission, skip or wrong contract count blocks eligibility. The
  full CTest/CI result remains failed. No `continue-on-error` or expectation change.
- Packaged helper checks exercise all four unchanged ZIPs and profile/config
  transactions. Component identity marker and LF/binary Git attributes prevent
  ambiguous beta storage and CRLF-only authored ZIP hash drift.

Persistence remains one existing JSON store with atomic writes. Host transactions
can validate/persist atomically; later Lua initialization/unload is not globally
atomic. Errors remain per-mod and dependent mods conservatively deactivate.
No new storage DB, Internet updater, arbitrary RAM writes, Scene API, CHR API,
physics, ABI or PAL scheduler changes were introduced.

## Compatibility and capability inventory

| Capability | Status / evidence |
|---|---|
| ZIP/manifests/v1/API2/settings/dependencies/conflicts | VERIFIED synthetic native/helper tests |
| Lua sandbox/quotas/recovery/safe mode | VERIFIED existing and new adversarial contracts; in-process, not OS isolation |
| Profiles create/save/rename/apply/delete/export/import | VERIFIED host, native, GL and native picker import |
| HUD/text/PNG visual resources | VERIFIED public fixtures and GL; existing API retained |
| Read-only RAM observers/PRG resources | VERIFIED inherited synthetic contracts; unchanged backend |
| Initial-room jump modifier | EXPERIMENTAL existing Phase5 capability; not expanded |
| Physical controller, long owned-ROM session | NOT RUN in Completion02 |
| General movement/2.5D/new CHR/randomizer | NOT IMPLEMENTED, outside scope |
| Four-platform publication | BLOCKED pending authorization and native CI evidence |

API2 and native ABI3 are unchanged. The host helper's setting row now carries the
validated default alongside its current value; this private host/helper protocol
is distributed together. It is not a new ABI service. Legacy package bytes remain
identical; old manager-state schema1 profiles remain readable.

## Verification and known failure

Clean independent Release desktop/headless directories were configured and built
from recovered source without old binaries. Final changes were rebuilt and the
entire suites rerun without filters. Final results:

| macOS ARM64 suite | Executed | Passed | Failed | Omitted |
|---|---:|---:|---:|---:|
| Desktop CTest |85|84|1|0|
| Headless CTest |77|76|1|0|

Both failures are `apu_status_irq_collision_reference` (A02), unchanged and real.
Neither suite is PASS. Strict inventory/JUnit gate fails on A02; separate diagnostic
eligibility reports `suite_pass=false`, `package_eligible=true`, no new failures.

Coverage includes CPU/native instruction 4736 executions, 82 IRQ cases, PPU/$2002,
APU/A01/A03, bus/RAM/MMC1/NMI/DMA/PAL, ABI/importer/cache, sandbox/observers/
resources/Phase5/UI,4:3, determinism and high-refresh contracts. Desktop-only
loader/SDL tests are not built in the defined headless configuration; every test
registered for each configuration ran. Physical refresh is not certified.

Completion02 contracts: 17 direct native navigation checks, 36 native store checks
(including those 17), 34 Python store cases and 6 eligibility adversarial cases.
Completion01 remains covered: 31 Python cases and 43 native store checks (includes
29 direct navigation checks). The earlier bad-unload fixture now changes a declared
setting so it still exercises a real unload failure, rather than relying on a
no-op to restart Lua. Existing failure expectations were not weakened.

Additional actual SDL/OpenGL4.1 Metal probe: 57 checks PASS, readback confirms UI
composition without altering source framebuffer; real resize/fullscreen plus
synthetic keyboard/controller events, modal focus timing, import/defaults/cancel,
profiles and neutral stick. This is not a physical gamepad or gameplay session.
A native Cocoa picker was also operated visually with an owned empty synthetic
profile: Cancel remained selected, explicit confirmation imported it inactive.
The probe used a temporary bundle identifier to disambiguate other installed
copies; that identifier is not in the distributed package.

Intermediate problems were retained as evidence: an icon/package fixture revealed
an unnecessary CMakeCache requirement, removed via the explicit unavailable-cache
metadata path; package harness initially lacked its generated CTest inventory;
native-modal event ordering required the frontend fix described above. Package smoke initially hit sandbox denial writing Zig's default user cache; rerun uses dedicated temporary Zig caches, without
changing package bytes or user cache. Temporary
trace logging and altered probe bundle are excluded from public source/package.

## Four unchanged mods

All four install Disabled, validate permissions/settings/resources, and persist.
Super+Speedrun+Parallax run together in native/GL fixtures. Configurable Jump is
an alternative exclusive owner and conflicts with Super; the conflict is rejected
without partial enable. They must not all modify jump simultaneously.

| ZIP | SHA256 |
|---|---|
| Super-Gomez-Gameplay-v1.0.zip |`d1114dbd01484f782455d8f5b218cd01a13546164a2fbd04bd190329bb35f3e4`|
| Speedrun-HUD-v1.0.zip |`ef3ff4587ae92dc15c11433b3f1bebc5d3acc0f78cddb9740302dc81c455397d`|
| Mansion-Parallax-Demo-v1.0.zip |`88ebf414b1cfdc8d94d7dfc7a4985e409f65a924b48291b882622712e7428571`|
| Configurable-Jump-Example-v1.0.zip |`7e4bdc048493bd1bd6b63abe322fa6340465cf344ff23833fc7df7c4e55e783b`|

Super's historical private gameplay evidence is in Phase5/Completion01. This task
has not repeated or extended a real commercial-game session or claimed Steam
Deck gameplay; use the release checklist for the new manual acceptance session.

## Performance

Release benchmark uses seven fresh-store batches and monotonic time. Measurements
include helper process startup and validation; not FPS, GPU timing/input latency.
Final measured medians: empty manager264.686ms, enumeration133.284ms, helper
startup37.730ms, ZIP install269.118–272.013ms, Enable401.894–529.118ms,
Disable265.684–270.255ms, profile restore810.978ms, Original270.323ms,
no-op configuration131.506ms, export133.353ms, inspect133.887ms and inactive
import268.552ms. Detailed samples are in `Evidence/benchmark-final.log`. PNG decode
64x32RGBA,100samples: median1528.896microseconds (own bytes, excludes I/O/process
startup). One final GL frontend empty-store open sample is recorded in gl-final.log;
not whole application launch latency. Lua/image/composition tracked allocation
peaks48805/8192/307200bytes, tracked cleanup0. These are selected logical owned
allocations, not total RSS; RSS collection failed under sandbox system permissions.
No worker thread/scheduler change or risky optimization was justified. Helper
startup/repeated validation costs remain visible for a future bounded refinement.

## Platform and package status

| Platform | Build | Package | Boot | Gameplay / physical controller |
|---|---|---|---|---|
| macOS ARM64 |BUILT Release desktop/headless|PACKAGED, extracted synthetic verification|BOOT TESTED manager|NOT RUN this delivery|
| macOS Intel |NOT RUN|NOT RUN|NOT RUN|NOT RUN|
| Windows x64 |NOT RUN|NOT RUN|NOT RUN|NOT RUN|
| Linux x64 |NOT RUN|NOT RUN|NOT RUN|NOT RUN|
| Steam Deck |NOT RUN|NOT RUN|NOT RUN|NOT RUN|

macOS host 27.0.1 ARM64. Package declares minimum 15.0, ARM64 binaries checked;
this is not a native15.0 machine test. Pinned Python3.11/Zig toolchain reused
read-only from the previously verified local package, never redownloaded. All
required licences/importer/public tools/headers/own icons are included. Strict
ad-hoc signing before/after extraction and tools execution, bundled Python/Zig,
synthetic C compile/load(return 42), import process 3 scenarios and packaged helper
profile/config/mod transactions pass. Not notarized. No ROM/private module/cache.

Final local app/ZIP and SHA256 are delivered under
`outputs/Mod-Manager-Completion02-Experimental`. `EXPERIMENTAL_MOD_MANAGER.json`
identifies Completion02 and its separate store; the binary truthfully reports
historical HEAD, dirty=true, version0.9.0, ABI3 and source SHA. No clean published
commit is claimed. Final ZIP:136129814bytes, SHA256
`cc476173ef06d67a6927363c14ea5e44329b75b7d87d91be1deda21cbd1a112f`.
Binary source SHA256:
`1bf29b639f29f4f1a9a593d6334d5e6321aa27a5441ace863c3a8a72abb85520`.
Earlier test packages are not this final artifact. The final delivered app opened
and navigated to Profiles and exited normally (code0), without a ROM. Copying this
app into the managed Documents folder added com.apple.FinderInfo and strict
codesign rejected that copied app. Removing only that metadata from the newly created local copy initially restored
strict verification, but the destination re-added it after launch. This managed-
folder extracted copy is BLOCKED for stable strict-signature use and preserved
outside delivery as evidence. Delivery commands instead extract the unchanged ZIP
in /private/tmp, verify strict codesign, start it and remove only that temporary
extraction on exit. User mod data, ROM/caches/preferences remain outside it.
Neither ZIP nor executable bytes changed; no re-signing or quarantine removal.
The original ZIP passed strict extraction in a clean temporary location.

Historical Linux/Mesa/Xvfb graphics discrepancy remains documented, NOT RUN here.
No certification of Alt+F4/Cmd+Q/native Windows picker/AppImage/FUSE/Deck hardware.

## Files, security audit and consolidation

Main changes: tools/mods/packages.py; runtime mod_manager/mod_frontend/mod_panel
headers/sources, host mod_api.hpp/default value, Cocoa picker and desktop event
batch completion; CMake registration/store namespace; package_platform.py,
mod_distribution_checks.py, strict JUnit/eligibility tests and workflow gate;
.gitattributes; new Completion02 tests/benchmark, public export helper and docs.
Completion02 changes35paths; the cumulative reviewed allowlist contains159paths
and the full audited export352files. The cumulative SDK list is explicit in `MOD_MANAGER_PUBLIC_FILES.txt`; compare
with parent recovery hashes to separate this phase from inherited changes.

Public export validates exact allowlist correspondence, unmerged/link/private
suffix/credential-like candidates, and records full SHA/provenance without staging.
Existing public audit checks export including reference byte scan where available.
No new binary/game resource was added in this phase; own icons/PNG/Lua/SDL archives
and licences were preserved. No new third-party code or dependencies. Technical
content checks are not legal certification, malware analysis or exhaustive audit.
No commercial bytes, modules, receipts, private traces/WAV/captures or user store
belong in the public source allowlist or distributed package.

## Reproduction and next step

From the successor source, use fresh directories; no ROM is required:

```sh
cmake -S . -B <fresh>/desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build <fresh>/desktop --parallel 6
cmake -S . -B <fresh>/headless -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF -DNES_ENABLE_MODS=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build <fresh>/headless --parallel 6
ctest --test-dir <fresh>/desktop --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit <fresh>/desktop.xml
ctest --test-dir <fresh>/headless --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit <fresh>/headless.xml
ctest --test-dir <fresh>/desktop --show-only=json-v1 > <fresh>/desktop/ctest-inventory.json
python3 -B tests/verify_ctest_suite.py --inventory <fresh>/desktop/ctest-inventory.json --junit <fresh>/desktop.xml
python3 -B tests/mod_package_eligibility.py --inventory <fresh>/desktop/ctest-inventory.json --junit <fresh>/desktop.xml --output <fresh>/eligibility.json
<fresh>/desktop/mod_completion02_gl_checks <new-disposable-store> <four-authored-ZIP-directory>
<fresh>/desktop/mod_completion02_benchmark <new-benchmark-dir> python3 tools/mods/packages.py <four-authored-ZIP-directory>
python3 -B tools/public/package_platform.py --build <fresh>/desktop --output <fresh>/package --system Darwin --arch arm64 --toolchain <verified-pinned-tools>
python3 -B tests/macos_package_checks.py --build <fresh>/desktop --output <fresh>/package --arch arm64 --report <fresh>/package.json
git diff --check
```

CTest/strict gate return failure on unchanged A02; run subsequent independent
commands explicitly rather than using that failure to call the suite successful.
See MOD_MANAGER_CI_PLAN.md for review/export and **future-authorized** publication
commands and exact-SHA native jobs. See MOD_MANAGER_RELEASE_CHECKLIST.md for Mac/
Deck manual acceptance. Local functional stabilization is ready for that review;
multiplatform distribution approval remains pending native CI, physical controls,
longer game session and inspection of all four exact-SHA experimental packages.
