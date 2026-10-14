# Modding SDK v2 — Phase 3

2026-10-14. Local functional validation completed on macOS ARM64.
Other native platforms remain NOT RUN; remote execution is BLOCKED by the
explicit no-publication/no-dispatch scope of this task.

## Recovered base and preservation

Isolated continuation `feature/modding-sdk-v2-phase3`, base HEAD
`95c194afac4574be9bf8b495341517389a73988b`. The base commit alone **does not contain**
Phase 1/2: recovered the full dirty tracked diff plus Git-listed authored untracked
files from `feature/modding-sdk-v2-resources`, verified **76 file SHA-256 values**
before writing this report. Original worktrees and Phase 2 deliverables remain
intact. No reset/clean/switch of the original, no staging/commit/remote changes.

| Worktree/branch | HEAD | Initial work |
|---|---|---|
| diagnostic/high-refresh-phase-a | 80af76aa5e4408f37dcb0e7047355c21be186ae8 | Unrelated reports, not merged |
| feature/mod-manager-v1 | 95c194afac4574be9bf8b495341517389a73988b | Historical CI report untracked |
| feature/mod-manager-v2 | same 95c194a | Dirty recovered API2/HUD |
| feature/modding-sdk-v2 | same 95c194a | Dirty Phase 1 snapshots/observers |
| feature/modding-sdk-v2-resources | same 95c194a | Dirty complete Phase 2, 76 changed/new files |
| feature/super-gomez-v1 | same 95c194a | Dirty separate gameplay experiment, not merged |
| feature/modding-sdk-v2-phase3 | same 95c194a | Verified copy of Phase 2; current continuation |

Read AGENTS, PROJECT_STATUS, Mod Manager and all SDK/resource reports. Actual
Phase 2 code includes trusted profiles, instruction/operand exclusion, typed PRG
backend, normal MMC1/bus reads, data-only API2/ZIP/ownership, 33 profile tests,
native/frontend contracts and two same-field translation packages. Existing
historical result is 66/67 desktop and 59/60 headless on macOS ARM64, A02 ordinary
FAIL and zero skips. Baseline rerun executed here: **66/67**, exit 8, A02 sole failure, zero skips.

Owner-local ROM SHA and existing private module/ABI3/ARM64 receipt were checked;
no regeneration or mutation. Private provenance, captures and traces remain in a
separate temporary directory outside Git. Catalog extension requires evidence
beyond finding alphabetic bytes. The initial record approved no new candidate. Subsequent flow analysis, timed
read tracing and real raster/UI observations approved only `hud.life`, below.

## Platform authorization

macOS ARM64 is available. Other native environments are not currently established.
Review and prepare existing native CI without dispatch/publication. This task does
not authorize commits, pushes or remote workflows. A02 and the historical Linux
high-refresh Mesa/Xvfb discrepancy must remain separately visible.

## Incremental implementation and verified resource

No new native production mechanism was necessary. Reused the Phase 2 immutable
bounded PRG backend and transactional manager. Added `hud.life` to the trusted
schema-1 catalog and a normal API2 data-only ZIP translating both it and
`inventory.heading`. Manifest, Lua API, native ABI3 and language-selector logic
are unchanged. The older two ZIPs retain their exact hashes. A host with the older
catalog rejects the new symbolic ID rather than interpreting raw mod addresses.

`hud.life`: physical bank 0, offset 13130, length 4, reader bank0/PC 0xB317,
fixed-length copy to `$2007` at PC 0xB31A. It is the English-selected label, with
language table/count/control/instruction bytes outside the approved range.
Recorded 15 five-glyph uploads including the leading blank outside the field:
75 writes, destinations $232D–$2331 (7 copies), $272D–$2731 (8 copies). Resource
itself occupies $232E–$2331 or $272E–$2731. The language pointer/offset selection
is preserved. Known instruction+operand exclusion and non-overlap validate.
A similar label in the earlier frame initializer is overwritten and was rejected
as an independent visible resource. No arbitrary ROM data search result was
accepted without this flow and actual raster evidence. See `MOD_RESOURCE_PROFILES.md`.

Replacement VIDA uses the existing reviewed capital glyph codec; actual native
HUD rendering verifies these four letters. No new mapping for accents, Ñ/ñ, Ü/ü,
¿/¡, variable-length text or general control/pointer streams. Choose original
English for this partial Spanish demonstration. Other original language branches
were inspected structurally, not independently played through in this session.

## Real native integration, separate from synthetic tests

Owner ROM SHA verified before/after: supported profile
`6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880`.
Existing ARM64 ABI3 module SHA stayed
`2fd2a47471d582afa78bcd0624f76c31d137eed55546cda7b7edc644c6917a47`;
import receipt/module unchanged, no recompile or ROM patch.

One private native harness and the same unchanged module ran four fresh modes:
installed-but-disabled original, active multicampo, disabled after active,
and manager-bypassed no-mods. **1,800 completed NES units each**. Compared each
unit's cycles, PC/A/X/Y/SP/P, controller input, RAM hash, float-bit PCM hash,
NMI/IRQ/OAM/DMC DMA counters. All 1,800 trace rows match in every comparison.
Final totals: 59,837,942 cycles, 1,759 NMI, 0 IRQ, 3,516 OAM DMA, 2,052 DMC DMA.
Zero IRQ entries in this particular play sequence do not certify delivery of IRQ;
the existing synthetic IRQ battery provides separate regression coverage.
Hashes of RAM/PCM are comparisons, not a physical hardware precision certificate.

| Fresh comparison vs original | HUD unit 1350 | Inventory unit 1590 | Stable trace |
|---|---|---|---|
| Active | 54 different pixels, x112–142/y201–207 | 255 different pixels, x88–159/y14–20 | 1,800 identical rows |
| Disabled then reloaded | Identical complete raster | Identical complete raster | 1,800 identical rows |
| Manager bypass | Identical complete raster | Identical complete raster | 1,800 identical rows |

Private harness source SHA:
`c54ea84ff6b3710930f83424c9156903f207b2b2eb5ac294849a382aa7598ae4`.
Private traces, captures, ROM and module stay outside this Git worktree, under a
separate temporary evidence directory. No commercial code/data/captures are
included in public tests or the example ZIP.

Actual SDL/OpenGL GUI on this ARM64 Mac: boot → F2 Install ZIP through the native
chooser → observed Disabled → Enable → choose original English → intro/hint →
real Garden HUD visibly VIDA → Enter inventory visibly OBJETOS. F2 Disable paused
safely; closing/reopening inventory visibly restored INVENTORY. Normal exit,
restart with the same store, F2 confirmed Disabled preference; another fresh
original game visibly restored LIFE. These are the original game pixels, not a
Lua/application overlay. Both sessions exited through the confirmation dialog.
Audio was muted for GUI checks; actual audible quality was not assessed.

A final full-application boot used `--no-mods --no-god --mute --frames 60
--unthrottled` with the translation **enabled** in the test store: exit 0,
OpenGL 4.1 Metal, 60 units/presentations, GOD=0, NMI54/OAM106 and 57,373 generated
samples. The store preference was then restored to Disabled. This is boot evidence,
not a full game/no-mods visual playthrough. SDL synthetic safe-mode test also now
uses the same populated active store and proves the original fallback while
leaving the managed selection active.

First demo launch failed because an unbundled CMake app lacked importer tools;
prepared the local copy with the existing packager's `copy_resources`. Sandbox
launch attempts did not complete GUI startup; an allowed desktop launch did.
No runtime/importer fix was made. The owned temporary app passed
`codesign --verify --deep --strict`, exit 0. Its executable SHA remained
`6e2f88db7957a5dd4045121ef7931c26b0c839ea47b9fee80118dde32e6d986e`
through active/restored/safe-mode comparisons. It is a developer app using local
Python and existing private cache, not a newly certified portable toolchain ZIP.

## Automated tests and fidelity

Three new mandatory CTest entries:

| Entry | Actual result (desktop and headless) |
|---|---|
| mod_resource_multi_profile_contract | PASS, 21 Python tests |
| mod_resource_multi_native_contract | PASS, 12 native assertions |
| mod_resource_multi_store_contract | PASS, 20 native assertions: the same 12 plus 8 manager/persistence assertions |

These are 41 distinct new checks (53 executed assertions/tests counting the
repeated native baseline). Real generated C from the existing emitter runs two
independent authored fields through timed PRG reads to PPU output. No interpreter.
Tests compare cycles, unaffected registers, complete RAM and PCM bits, input,
NMI/IRQ/OAM/DMC DMA, unit counts and expected pixel regions. Synthetic A can
transiently hold different intentional glyph data and is explicitly excluded
from unaffected-register comparison; the private real-game per-unit A was equal.
Disable reload restores the complete synthetic frame. Source PRG remains immutable.

Security tests exercise two valid fields, invalid second field at installation
and enable with unchanged state, partial conflicts, disjoint owners, overlap,
wrong bank/revision, instruction/operand rejection, fixed lengths, unsupported
Unicode, malformed/duplicate JSON, duplicate ZIP, traversal/backslashes/absolute
and alternative Windows paths, no script execution, uninstall, restart and v1
compatibility. Native manager→mapper transport and transactions run in host paths
with **spaces, ñ, Ω and Japanese characters** on macOS. They are prepared for other
OSs, not evidence that those filesystem/argv conventions pass there.

| Suite | Executed | PASS | FAIL | SKIPPED | Exit |
|---|---:|---:|---:|---:|---:|
| Recovered Phase 2 desktop baseline | 67 | 66 | 1 A02 | 0 | 8 |
| Phase 3 desktop Release | 70 | 69 | 1 A02 | 0 | 8 |
| Phase 3 headless Release | 63 | 62 | 1 A02 | 0 | 8 |

Full suites include 4,736 native CPU executions, all 82 IRQ cases, PAL07/08,
A01/A03, PPU/$2002, RAM/MMC1/bus, DMA/NMI, importer/cache/ABI3, v1/Lua API2,
RAM snapshots/HUD, 4:3 and presentation-rate determinism. Seven desktop-only
contracts are not built in headless, according to the existing configuration;
all required headless tests ran. No WILL_FAIL/DISABLED/skip properties or A02
expectation changes. Inventory/JUnit gate confirms all mandatory names and execution
counts; exits **1 because A02 genuinely failed**, as intended. After strengthening
safe mode against an already active store, its nine SDL checks passed again.

An initial new Python test incorrectly used the profile tuple element as a resource
ID; corrected the test harness indexing. Temporary distribution/benchmark scripts
also required a created working directory/canonical resolved macOS temporary path;
corrected harness setup, not production validation. No remaining new failures.

## Platform and package matrix

| Native platform | Build/CTest | Resource/ZIP | Real visual game | Complete distribution |
|---|---|---|---|---|
| macOS ARM64 (macOS27.0.1, AppleClang21, SDL2.32.8) | PASS builds; suites FAIL only A02 | PASS | PASS partial translation/restoration | Developer app/helper checks PASS; full bundled-toolchain ZIP NOT RUN |
| macOS Intel x86_64 | NOT RUN | NOT RUN | NOT RUN | NOT RUN |
| Windows x64 | NOT RUN | NOT RUN | NOT RUN | NOT RUN |
| Linux x64 | NOT RUN | NOT RUN | NOT RUN | NOT RUN |

Remote CI: **BLOCKED by lack of authorization to publish/dispatch this dirty
successor**, not a claimed authentication failure. No runs/artifacts from an older
SHA are used as proof. Local minimum macOS15.0 declaration is not execution on
macOS15 or Intel. Linux's historical Xvfb/Mesa high-refresh discrepancy remains
open, not reproduced/repaired/certified away here; inspect its separate graphics
report when native Linux CI is authorized.

Existing `build.yml` already builds the four native jobs with mods enabled,
full unfiltered CTest/JUnit (65,536-byte output), independent package checks after
A02 and evidence upload. No workflow change needed. CMake and the mandatory suite
gate now include the new tests; shared macOS/Windows/Linux extracted-package
checks require the third authored ZIP and field/owner transport, persisted selection,
and no resources after Disable. Package copying includes the updated trusted
catalog, instruction map, helper and authored translation. No remote execution.

Local extracted resource copy: **45 helper transactions** across Hello, HUD,
two RAM observers and three translations passed, with member/source-byte/hash
checks, install-disabled, enable/restart/disable/uninstall and correct multicampo
ownership. Used system Python and an empty test toolchain. This cannot certify
bundled Python/Zig or a Windows/Linux portable package.

## Release microbenchmarks

ARM64 AppleClang Release; 2,000,000 mapper reads over a 4KiB pattern (mostly misses),
volatile sink; one measured run, warm host, no comparable hardware/OS benchmark.
No Lua decoding in the lookup. Values rounded; scheduling/timer noise applies.

| Fields | Mean ns/cpu_read | Mean ns/publication | Mean ns/clear | Descriptor+glyph bytes |
|---:|---:|---:|---:|---:|
| 0 | 2.34 | 14.08 | 15.19 | 0 |
| 1 | 3.07 | 46.62 | 32.83 | 91 |
| 2 | 3.59 | 61.35 | 41.04 | 182 |
| 16 (maximum) | 8.74 | 329.65 | 127.95 | 1,456 |

Publication/clear: 10,000 pairs per size, individual monotonic timestamps; timer
cost matters especially at zero fields. Memory column counts `sizeof(PrgTextResource)`
and three owned glyph bytes per fixture; excludes allocator overhead/capacity,
string allocations in larger IDs, profile/Python storage and process baseline.
The real two-field mod owns 13 glyph bytes; descriptor+glyph lower accounting is
189 bytes with this build. No claim of exact RSS.

Separate Python Store synthetic two-field benchmark (100 enable/disable pairs):
mean enable **649.32 microseconds**, disable **483.25 microseconds**, including
profile encoding/revalidation and atomic state-file transaction, excluding process
spawn and UI time. These are microbenchmarks, not input latency, audio quality or FPS.
No optimization or new threads/backend refactor was justified.

## Files changed in Phase 3 (relative to preserved Phase 2)

- `tools/mods/resource_profiles.json`: additive reviewed `hud.life` descriptor.
- `tools/mods/make_translation_examples.py`, `examples/mods/addams-spanish-multi/`:
  normal data-only authored multicampo ZIP.
- `tests/mod_resource_multi_checks.py`, `emit_resource_multi_fixture.py`,
  `mod_resource_multi_native_checks.cpp`, `mod_resource_multi_store_checks.py`:
  public synthetic security/native/transaction contracts.
- `tests/mod_resource_frontend_checks.cpp`: same active-store safe-mode fixture;
  existing expectations preserved/strengthened.
- `tests/mod_resource_native_checks.cpp`: add 2-field lookup and publication/clear
  measurements; existing 21/28 assertions unchanged.
- `tests/mod_distribution_checks.py`: third ZIP, exact owners/fields, persistence,
  empty resource result after disable in extracted-package verification.
- `CMakeLists.txt`, `tests/verify_ctest_suite.py`, `verify_ctest_suite_checks.py`:
  new mandatory tests and independent gate report fixtures.
- `PROJECT_STATUS.md`, `docs/MOD_RESOURCE_API.md`, `MOD_RESOURCE_PROFILES.md`,
  `MOD_TRANSLATION_GUIDE.md`, this Phase 3 report.

No Phase 3 native production, hardware, mapper rules, ABI headers, recompiler,
PAL scheduler, A02, high-refresh renderer/planner, Super Gomez or public beta
changes. Native backend changes visible relative to HEAD belong to recovered
Phase 2, not a new rewrite. Parent Phase 2's 76 changed/new files remain hash-identical.

## Reproduce public tests

From this successor worktree, fresh directories:

```sh
task_build=$(python3 -c 'from pathlib import Path; import tempfile; print(Path(tempfile.mkdtemp(prefix="nes-sdk3-")).resolve())')
cmake -S . -B "$task_build/desktop" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON
cmake --build "$task_build/desktop" --parallel 4
ctest --test-dir "$task_build/desktop" --show-only=json-v1 > "$task_build/desktop/inventory.json"
python3 -B tests/verify_ctest_suite.py --inventory "$task_build/desktop/inventory.json"
ctest --test-dir "$task_build/desktop" --output-on-failure --no-tests=error --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$task_build/desktop/results.xml"
python3 -B tests/verify_ctest_suite.py --inventory "$task_build/desktop/inventory.json" --junit "$task_build/desktop/results.xml"
cmake -S . -B "$task_build/headless" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF -DNES_ENABLE_MODS=ON
cmake --build "$task_build/headless" --parallel 4
ctest --test-dir "$task_build/headless" --output-on-failure --no-tests=error --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$task_build/headless/results.xml"
"$task_build/desktop/mod_resource_native_checks"
python3 -B tools/mods/make_translation_examples.py "$task_build/examples"
python3 -B tools/public/audit_public.py .
```

CTest returns 8 and the JUnit gate 1 while A02 remains open. Do not use shell
success overrides. Windows MSYS2 uses `python` with UCRT64/Ninja as in `build.yml`.

Private experiments require the owner's exact validated local ROM/module. The
private harness, provenance and traces were retained outside Git; reproducibility
is contingent on that temporary cache surviving. Public tests do not require it.
Do not put the harness's private captures/ROM/module into CI artifacts.

## CI procedure requiring later authorization

1. Review the complete dirty successor against HEAD and the 76-file recovered
   baseline, audit an explicit source/test/docs allowlist including inherited
   Phase1/2 and new Phase3 work. HEAD95c194a alone is v1 and **cannot** be dispatched
   as SDK2 evidence. No broad staging of sibling outputs/private evidence.
2. Only after authorization, commit/push this successor (no main/force/tags) and
   record its new exact SHA. Push triggers all four jobs. If a manual run is needed:

```sh
gh workflow run build.yml --ref feature/modding-sdk-v2-phase3 -f platform=all
gh run list --workflow build.yml --branch feature/modding-sdk-v2-phase3 --limit 5 --json databaseId,headSha,status,conclusion,url
gh run view RUN_ID --json headSha,jobs,url
gh run download RUN_ID --dir /tmp/sdk3-ci-evidence
```

3. Confirm headSha on every native job. Inspect full inventory/JUnit: expected
   70 executed/69 PASS/1 ordinary A02 FAIL/zero skips, unless a newly justified
   additional contract changes inventory. Inspect new test count footers, 82 IRQ
   and 4,736 CPU counts. Any new failure requires diagnosis, not exemptions.
4. Inspect package verification JSON/ZIP checksums/build identity (ABI3, exact
   SHA, dirty=false), all three translation ZIPs and trusted catalog/map in each
   extracted package. Resource subprocess Unicode/space tests must actually run
   on Windows/Intel/Linux. Preserve Linux GL failure separately if it recurs.
5. Then manual F2/install/enable/render/disable/reload on native desktop/Deck,
   with owner ROM locally. Never include game captures or cache in CI uploads.

## Delivery, limits and next phase

`Addams-Spanish-Multi-v1.0.zip`, SHA-256
`a4acf72b4f8d04bcee4033b7a16f4687b6cddd808f3755e9ae46bcf427393742`.
Members: `mod.json`, `translations/es-ES.json`, `assets/README.txt`; no script,
ROM, original glyphs/music/code, module/cache or profiles. Local demo deliverables
are in the sibling `Modding-SDK-v2-Phase3-Demo` folder, outside this Git worktree.
Its developer launcher requires the existing private cache and local Python;
it is not a public portable release. Read its README before use.

Functional local acceptance is met for two reviewed fields, real native display,
restoration, transaction/security tests and no-mod fidelity. Multiplatform
certification is **not complete**: three native platforms and complete bundled
packages need the authorized CI/manual steps above. A02 remains ordinary FAIL;
Linux's historical graphical discrepancy remains open. No complete playthrough,
all-language translation, runtime security isolation or legal certification claimed.

Next: authorize reviewed CI publication first. Then Phase4, separately: verify
original/banked font coverage and author-owned CHR glyph replacement for Spanish
accents; design bounds/ownership/atomic restoration; investigate variable-length
streams and pointer relocation with executable-range exclusions; expand only
resources proven by flow and raster tests. Plan author validation tools and future
randomizer integration. None of these Phase4 features is implemented here.

## Final preservation and technical content checks

Final source audit with private owner-ROM byte markers: PASS, 284 source files.
Staged developer app scan: PASS, 136 files, no matched ROM blocks/full image,
private module, import receipt or remembered-ROM file. The normal three-member
multicampo ZIP was checked for integrity, data-only entry and both symbolic IDs.
Strict signature verification of the independently staged delivered copy passed
without changing executable bytes. Launcher syntax passed. These bounded scans
and explicit member/source allowlists are technical checks, not legal clearance
or proof against every possible encoded/derived resource.

Phase 3 delta relative to the preserved dirty parent: **20 files**, including
**8 new files**; the complete successor has 22 tracked modifications and 62
untracked files inherited/created across Phases1–3. Nothing staged; HEAD unchanged,
no commits/pushes/merges/tags/releases or remote workflows. All 76 preserved parent
SHA values match again at completion. The host's bundled documents are its build
snapshot; the current report and README beside the delivered app supersede them.
