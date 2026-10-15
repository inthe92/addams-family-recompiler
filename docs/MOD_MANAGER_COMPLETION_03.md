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

## Historical status in the initial revision

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


## Second native run and portability follow-up — 2026-10-15

Run38639203100 on d9e2fcccde85c0cb48e514695d190ec74ad66d2c completed.
Both Macs and Linux built, executed85 CTest entries (84PASS/1FAIL A02/0omitted),
passed57 manager GL checks, and generated/verified diagnostic packages. Linux's
separate historical high-refresh GL pixel/letterbox/orientation mismatch remains
FAIL under Mesa/Xvfb; its existence is not erased by package eligibility.
Windows built and executed85 entries:79PASS/6FAIL/0omitted. Besides A02, three
Unicode-path fixtures, a locale-dependent own-glyph JSON read and the unchanged
example-ZIP hash contract failed. Windows packaging was correctly blocked.

The follow-up preserves all expectations and original four ZIPs. It normalizes
Unicode command-line arguments in the three synthetic Win32 fixtures using the
native wide command line, converts filesystem inputs explicitly from UTF-8,
reads the authored glyph JSON explicitly as UTF-8, fixes ZIP creator metadata
independently of host OS, and selects Python3.11 explicitly for Windows build/
CTest/helpers (the failed run selected hosted Python3.14). ZIP byte identity must
still be established by the unchanged hash assertions in native Windows CI;
metadata alone is not claimed as the entire demonstrated cause. No runtime,
A02, generated CPU or ABI production code changes are made by this follow-up.
The new test-only UTF-8 header expands the reviewed cumulative allowlist to161
paths. Ruby YAML parsing and local authored ZIP checks pass; all four hashes
match the original packages. A subsequent native exact-SHA run is required.

Local follow-up Release desktop build completed on macOS ARM64:85 executed,
84PASS/1FAIL A02/0omitted; unchanged expectations, CTest exit8. Public source
audit354files PASS, no private inputs. Original four example ZIP hashes match.


## Third native run — d0f91831e7c0d8aaf7c832e845c513130bdce077

https://github.com/inthe92/addams-family-recompiler/actions/runs/38705495907
All four native builds and all85 CTest entries ran:84PASS/1FAIL A02/0omitted
on each platform. The five Windows regressions are resolved with their original
expectations, including original ZIP hashes. Manager GL57 checks pass on both
Macs and Linux; Linux high-refresh GL still FAILS independently.

Windows packaging reached actual extraction/dependency/Python/Zig/mod helper/
synthetic ABI3/rejection of ABI2/cache checks successfully. Its old picker-cancel
harness then tried a normal interactive launch, which now enters the new main
menu; observed dialog was "Main menu unavailable", not the expected picker.
This is NOT recorded as a successful normal boot. The graphics cause is not yet
isolated: no claim that it is only missing driver capability is justified yet.
The corrective harness selects the existing --import-only path for importer-only
dialog checks, preserving their exact outcome/log/process-cleanup assertions.
A separate actual Windows manager GL probe is added with an ordinary failing
exit if anything fails, retained log and no continue-on-error. Its failure is
never converted to a UI PASS or concealed by synthetic importer success.
Diagnostic ZIP verification explicitly limits its scope; Windows graphical
certification remains BLOCKED until the separate probe and physical testing pass.
No production code is changed by this separation.

The ARM64 Actions ZIP was also downloaded, CRC/hash checked, extracted under
/private/tmp, strictly codesign verified, and its actual installed-mods interface
visually observed without a ROM and with a fresh isolated store. Escape closed
that --mods-only instance; no owned process remained. This is BOOT TESTED manager,
not GAMEPLAY TESTED. Other physical platforms/controllers remain NOT RUN.


## Final implementation verification — 2026-10-15

**IMPLEMENTED / VERIFIED source integration; EXPERIMENTAL diagnostic packages.**
This report certifies only the executed scope below, not a stable release or
physical gameplay. Complete cross-platform graphical acceptance remains BLOCKED.

- Branch: [feature/mod-manager-beta](https://github.com/inthe92/addams-family-recompiler/tree/feature/mod-manager-beta).
- Exact tested implementation: `10fbb1d8ad829d20f3872f47ced16a727002c126`.
- [Complete native execution](https://github.com/inthe92/addams-family-recompiler/actions/runs/38707087575): completed, overall **FAIL**.
- All four embedded identities: version0.9.0, ABI3, dirty=false, same commit.
- Common source fingerprint: `72c7a2eda609878b4c59142286097eb95bdbc05c7aae6b4cf99203c592be7758`.
- Later documentation-only commits must not be presented as the embedded commit
  of these binaries. This table and hashes remain bound to this exact execution.

| Native job | CTest run/pass/fail/omitted | Manager GL | Package payload/import check | Normal packaged UI |
| --- | --- | --- | --- | --- |
| macOS15 ARM64 | 85/84/1/0 | 57PASS | PASS | BOOT TESTED locally on ARM64 macOS27.0.1 |
| macOS15 Intel | 85/84/1/0 | 57PASS | PASS | Build GL probe PASS; packaged interactive UI NOT RUN |
| Windows x64 | 85/84/1/0 | FAIL: missing glCreateShader | PASS (explicit importer-only scope) | BLOCKED on hosted runner |
| Linux x64 | 85/84/1/0 | 57PASS (Mesa/Xvfb) | PASS | Packaged identity/import tested; physical UI/game NOT RUN |

Totals:340 CTest entries,336PASS/4FAIL/0omitted. Every failure is the original
`apu_status_irq_collision_reference` A02. All82 IRQ cases and4736 native execution
cases remain covered by the complete inventories; no filters, DISABLED, WILL_FAIL
or changed expectations. Native helper/Lua/configuration/profile/security and
safe-mode contracts retain their original assertions. Linux's **separate**
high-refresh GL pixel/letterbox/orientation mismatch also FAILS. Windows' separate
manager GL probe FAILS because the required glCreateShader entry point is absent;
this identifies the missing function in that environment, not physical-PC support.
No renderer fallback or NES modification is attempted. Therefore none of the four
job conclusions or the global workflow is described as completely PASS.

### Exact downloadable payloads

These are **inner payload** hashes/sizes, not Actions container hashes. Each
artifact also contains its checksum sidecar. GitHub login may be needed to download.

| Platform | Payload | Bytes | SHA-256 | Actions artifact |
| --- | --- | ---: | --- | --- |
| Windows x64 | `NES-Native-Lab-Windows-x86_64.zip` | 133529595 | `3d3e4ee87c969aee3565bea042e0441bcf4a53d445b058610712a580e8a63a5b` | [Download](https://github.com/inthe92/addams-family-recompiler/actions/runs/38707087575/artifacts/11959022449) |
| macOS ARM64 | `NES-Native-Lab-macOS-arm64.zip` | 137529794 | `65210198327194c20e712c7452ab81c749666a0c0b87fc40d5bb43e80761b9c0` | [Download](https://github.com/inthe92/addams-family-recompiler/actions/runs/38707087575/artifacts/11959560194) |
| macOS Intel | `NES-Native-Lab-macOS-x86_64.zip` | 142436907 | `8a752eb60cadb998d714d1541c0a2a58b7a751011f114b5f19b276a33737bce9` | [Download](https://github.com/inthe92/addams-family-recompiler/actions/runs/38707087575/artifacts/11959471918) |
| Linux x64 | `NES-Native-Lab-Linux-x86_64.AppImage` | 111749624 | `e49ce74e4c431bccae3f8ada1643c3e7c170fd0a1d3624603211631105a81829` | [Download](https://github.com/inthe92/addams-family-recompiler/actions/runs/38707087575/artifacts/11959575149) |

All four payloads were downloaded, hashed and matched to their native reports and
sidecars. ZIP CRC/member checks passed; platform-native package audits verified
icons, licences, public tools and absence of ROM/private modules/caches. These
technical content checks are not legal certification. Windows was extracted
outside the build in spaced paths with PATH restricted to System32; bundled
Python3.11.17/Zig0.17.0, synthetic compiled ABI3 execution and ABI2 rejection passed.
Its real native picker canceled with exit0, incompatible synthetic input produced
exit1/log, and neither left owned processes. This is not normal OpenGL/game boot.
Both macOS architectures passed strict ad-hoc codesign before/after extraction
and tool execution, architecture/minimum15.0 checks and synthetic module return42.
Linux passed ELF/dependency/icon audits, direct AppImage extraction-and-run identity,
synthetic incompatible-input/importer tests and ABI3 module checks. Physical FUSE,
Steam Deck and game/controller play remain NOT RUN.

The four original authored example ZIP hashes match on every platform. Super+
Speedrun+Parallax coexist; Configurable Jump is an exclusive alternative to Super.
No original mod ZIP is edited to obtain a PASS. Installation remains disabled by
default; profile/config transactions, defaults, conflicts, rollback/persistence,
recovery and --no-mods are exercised by the existing native/public fixtures.

### Local execution and performance scope

A clean public clone at cb96b21 built Release desktop/headless and yielded84/85
and76/77 respectively, A02 only. The Unicode follow-up rebuilt desktop locally
(84/85,62.68s) and built headless from a fresh clone at d0f9183 (76/77,50.70s),
zero omissions. Local actual manager SDL/OpenGL57 checks PASS. These are explicitly
historical local build SHAs, not relabelled as builds of the subsequent CI commit.
For the exact implementation SHA10fbb1d, the ARM64 CI ZIP was separately downloaded,
strictly signature-checked and opened normally from a fresh /private/tmp extraction
and isolated store. Actual main menu -> Mods -> Escape/Back -> main menu succeeded;
window close exited with no owned process. No ROM was used. No new real gameplay,
audio/controller session, Super jump measurement or Steam Deck run is claimed.

One-sample empty-store GL opening measurements in native run38707087575 are
recorded in the downloadable mod-manager-gl.log per platform. They measure only
that probe's Frontend construction, not total app launch, input latency or FPS;
no cross-platform performance ranking is justified. Completion02's original
operation benchmarks are historical, not freshly measured here.

### Source safety, provenance and publication boundary

The cumulative reviewed list now contains162 public paths, including the new
UTF-8 test header and corrected Windows package verifier. Complete public tree:
354files, no private build inputs. Initial consolidation preserved Completion02
and all older worktrees. Recovery manifests and explicit path reviews were used;
no mobile/PortMaster/PAL-NTSC/foreign gameplay files were incorporated. Original
Completion02 still has159 pending paths; it was never committed, reset or edited
by this integration. An older352-file export manifest predates three final report
revisions; that old manifest alone must not certify the current original docs.
The original current source comparison against the initial integration has only
the six intentional workflow/status/inventory/guide/checklist/CI-plan differences;
Completion02 implementation/report source remains preserved.

Licences and pinned third-party archive hashes remain checked. New changes are
reviewed fixture/UTF-8/ZIP metadata/build-Python/importer-harness/CI corrections and
reports. Core CPU, PPU, APU, bus, mapper, ABI3, A02 and gameplay policy are untouched
by these portability corrections. No ROM/private module/cache/receipt/capture/WAV/
secret or personal path is added. No main, release, tag, merge, reset or forced
push operation was performed. Public beta0.9.0 is retained.

### Reproduction and next acceptance

Use a new clone of the exact tested implementation SHA, not just historical95c194a:

```sh
git clone --branch feature/mod-manager-beta https://github.com/inthe92/addams-family-recompiler.git
cd addams-family-recompiler
git checkout --detach 10fbb1d8ad829d20f3872f47ced16a727002c126
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure --output-junit "$PWD/build/ctest-results.xml"
```

CTest is expected to report the real A02 failure; do not filter or invert it.
The workflow retains inventory, full JUnit, LastTest, strict gate, diagnostic
eligibility, manager/high-refresh logs and native package reports independently.
For headless use a separate build directory with NES_ENABLE_DESKTOP=OFF; no SDL
entries are registered in that configuration, rather than skipped tests.

**Next:** run the existing manual checklist on a physical Windows PC and Steam
Deck OLED using these artifacts and an owned ROM. Verify the required graphics
functions, manager/Back/focus/fullscreen/controller paths and real mods/gameplay.
Keep Linux high-refresh graphics as a separate open issue. Do not begin Rocket
Boots or call cross-platform manager acceptance complete before these blockers
have evidence. No public experimental release was created.
