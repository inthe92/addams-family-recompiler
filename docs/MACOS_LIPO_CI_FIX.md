# macOS lipo packaging fix and native CI verification

Date: **2026-10-11**. **The lipo packaging defect is fixed and verified on both native macOS 15 hosts.** ARM64 and Intel app ZIPs are generated, strictly verified after clean extraction, exercised using their bundled tools and retained as downloadable CI artifacts. **The complete workflow is still FAILURE because A02 fails on all four hosts.** No release, tag, main merge or A02 correction.

## Checkout and published change

Existing public checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler`; branch **`verify/phase-3-1-bus-timing`**; starting HEAD **`9cde8c9307a91424184610eba63930562b2b6ca7`**. Origin matches `https://github.com/inthe92/addams-family-recompiler.git` and its work branch matched HEAD. Read `MACOS_15_CI_VERIFICATION.md` completely, inspected the packager and downloaded the two package-failure artifacts from [run 38129810628](https://github.com/inthe92/addams-family-recompiler/actions/runs/38129810628).

Published correction: **[`7c2418d97f485f34c6aa9cd8b888f61b3d627f2b`](https://github.com/inthe92/addams-family-recompiler/commit/7c2418d97f485f34c6aa9cd8b888f61b3d627f2b)**. The final report follows in a documentation-only commit with `[skip ci]`; all executed source checks below refer to 7c2418d. No branch switch, conflict, incomplete Git operation, history rewrite or force push. Preexisting local `APU_A02_DIAGNOSIS.md` and `RELEASE_1_0_READINESS.md` remain unchanged and untracked. No previous report was overwritten.

## Confirmed cause and minimal correction

The old command placed the input after the architecture list:

```text
lipo -verify_arch arm64 file
```

Apple's [pinned cctools parser](https://github.com/apple-oss-distributions/cctools/blob/e0d56624eca2a76c2ace4c21850df9e666de4ca5/misc/lipo.c#L443-L465) parses every argument following `-verify_arch` as an architecture. **Both macOS 15 runners now reproduce the old call on a valid synthetic ARM dylib using their actual Xcode 16.4 lipo.** Each returns 1, with stderr identifying the dylib path as an `unknown architecture specification flag`. Thus the earlier parser diagnosis is now confirmed by actual runner stderr, rather than inferred from an opaque exception.

The corrected command is:

```text
lipo file -verify_arch arm64
# Intel: lipo file -verify_arch x86_64
```

The same valid synthetic files pass the corrected checks; both unchanged frontend build configurations produce executables that pass architecture/minimum validation. No executable-generation change was needed. This isolates the defect to the architecture-check command, not malformed or incorrectly generated frontend Mach-O files.

| File | Exact scope |
|---|---|
| `tools/public/package_platform.py:32-37` | Input-before-option command order; retain `check=True`; capture text stderr and include return code, relative file path and expected architecture in a fatal ValueError, preserving the original exception |
| `tests/macos_lipo_checks.py` | Six deterministic checks using actual Apple tools and two tiny synthetic C dylibs; log the old-order probe without declaring it a passing architecture check |
| `.github/workflows/build.yml` | One macOS-only command to execute these six tests in the existing independently gated tool verification step |
| `docs/MACOS_LIPO_CI_FIX.md` | This report |

The source commit contains the first three files only. No packager refactor or new dependency. Nonzero lipo results remain fatal; empty stderr is explicitly reported as `(empty)`. The existing vtool minimum checks, architecture requirement, CMake deployment floor **15.0**, metadata, signing, ZIP round-trip, CTest and JUnit gates remain intact. Windows/Linux workflow sections and packaging branches are byte-identical to starting HEAD. CPU/PPU/APU, ABI, recompiler, graphics, panoramico, gameplay and GOD are unchanged.

## Local checks

Host: **macOS 27.0.1 ARM64**, Command Line Tools. Newer local lipo accepts the old ordering (probe returns 0); this difference explains why the old invocation passed local inspection but failed on Xcode 16.4 runners.

| New test | Actual local result |
|---|---|
| Synthetic ARM64 dylib / minimum 15.0 | PASS |
| Synthetic Intel dylib / minimum 15.0 | PASS; architecture inspected, not Intel code execution |
| Wrong architecture | PASS: rejected; error includes actual lipo stderr or explicit empty marker |
| Invalid text input to real lipo | PASS: rejected with nonzero return |
| Application with no native Mach-O file | PASS: rejected |
| Binary minimum 15.0 against package floor 14.0 | PASS: rejected; floor validation retained |

**Six executed / six passed / zero failed / zero skipped.** Real existing ARM and Intel test bundles also pass the corrected guard: **13 native files each**, floor 15.0. This is local declaration/architecture inspection, not macOS-15 runtime certification. Python syntax, workflow YAML and `git diff --check` pass. Reviewed candidate source export: **116 files / 8,723,576 bytes**, public audit PASS; bounded credential-marker scan found no matches. No ROM, extracted game resources, temporary executable, build cache or raw log was staged.

An initial draft fixture setup used `-nostdlib`; Apple ld rejected a dylib without libSystem before any test case ran. Only the synthetic test fixture's link command was adjusted to normal Apple system linking. The final six checks above run successfully; this was not a production/linker change or a package regression. Existing engine CPU/APU tests were left to the full native CI run instead of redundantly repeating them locally.

## Native CI results

[Run **38131057549**](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549), push of **7c2418d**, completed **FAILURE**. Both Mac runners execute natively on **macOS 15.7.9**, using the matching architecture. The lipo harness compiles/inspects fixtures for both architectures; it does not execute cross-architecture fixture code. Engine tests and synthetic module loading match the native host; no Rosetta execution is claimed.

| Check | [ARM64 job](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549/job/114445301629) | [Intel job](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549/job/114445301782) |
|---|---|---|
| Full default build and main executable link | PASS | PASS |
| Six new lipo/deployment checks | 6/6 PASS, 0 skipped | 6/6 PASS, 0 skipped |
| Old-order probe on valid fixture | Exit 1, path misparsed as architecture | Exit 1, same parser error |
| Full bundle architecture guard | PASS: 13 native files | PASS: 13 native files |
| vtool: frontend minimum / Info.plist floor | **15.0 / 15.0**, PASS | **15.0 / 15.0**, PASS |
| vtool: Python 3.11.17 minimum | **11.0**, PASS | **10.15**, PASS |
| vtool: Zig 0.17.0 minimum | **15.0**, PASS | **15.0**, PASS |
| All nested native minima / otool dependency inventory | PASS / recorded | PASS / recorded |
| Complete .app creation and ad hoc signing | PASS | PASS |
| `codesign --verify --deep --strict` before ZIP | PASS | PASS |
| ZIP generation / CRC integrity | PASS / PASS | PASS / PASS |
| Fresh external extraction and strict signature verification | PASS | PASS |
| Extracted Python and Zig version/execution checks | PASS, 3.11.17 / 0.17.0 | PASS, 3.11.17 / 0.17.0 |
| Extracted tools compile, sign and load synthetic C; return 42 | PASS | PASS |
| Extracted bundled Python + import_probe, three synthetic failure scenarios | PASS | PASS |
| Strict signature after extracted-tool execution | PASS | PASS |
| Final ZIP artifact upload | PASS | PASS |

The verifier checks the native host and runs tools/importer scenarios with an external temporary working directory and extracted bundle resources. This proves those packaged paths work outside the build directory. It does **not** certify GUI startup, owned-ROM import/gameplay, every optional framework/Python extension or every macOS 15 patch version. Actual runtime test hosts are 15.7.9; minimum declarations are 15.0.

Across local and the two native CI hosts, the new six-case harness has **18 executions / 18 passes / 0 failures / 0 skips**. Old-order probes are diagnostics, not additional successful test cases. No new failing package step appeared after the correction, so no unrelated correction was attempted.

## ZIP artifacts and hashes

| Native CI ZIP | SHA-256 of the inner application ZIP, recorded by the runner |
|---|---|
| `NES-Native-Lab-macOS-arm64.zip` | `6f9c6ea752dbf1d5e53e07b7b36281453162521e41bbf50b49d598dc5f3f31ec` |
| `NES-Native-Lab-macOS-x86_64.zip` | `ab2f47694409d9fddeec11297169810336da89ceb21ae2a3500b6fd3585c6cd6` |

Downloadable artifacts confirmed through successful upload steps and GitHub's artifact API (nonexpired, nonzero size): [frontend-macos-15](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549/artifacts/11698924968), 126,888,472 bytes; [frontend-macos-15-intel](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549/artifacts/11699540706), 131,778,904 bytes. At inspection, their retention expiry is **2027-01-09**. GitHub artifact download requires access/sign-in and is temporary, not a release. Artifact wrapper sizes/digests are distinct from the inner application ZIP hashes above. Large archives were not downloaded and redundantly retested locally; CRC/extraction/signature/tool checks actually ran on each runner before upload.

`macos-package-verification-arm64` and `macos-package-verification-x86_64` retain package logs and JSON with all command return codes, vtool/otool output, tool versions, signature checks and ZIP hashes. All recorded verifier commands return 0. Four CTest artifacts retain inventories, JUnit and LastTest logs. Downloaded small evidence was inspected; raw data stays outside public Git in the surrounding workspace `macos-lipo-ci-fix-2026-10-11/`.

## CTest, A02 and other platforms

| Actual native platform | Complete build/link | Executed CTest entries | Passed | Failed | Skipped |
|---|---|---:|---:|---:|---:|
| macOS 15 ARM64 | PASS | 19 | 18 | 1, A02 | 0 |
| macOS 15 Intel | PASS | 19 | 18 | 1, A02 | 0 |
| [Windows x64](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549/job/114445301842) | PASS | 19 | 18 | 1, A02 | 0 |
| [Linux x64](https://github.com/inthe92/addams-family-recompiler/actions/runs/38131057549/job/114445301786) | PASS | 19 | 18 | 1, A02 | 0 |
| **Total** | **4 successful builds** | **76** | **72** | **4** | **0** |

All four retained JUnit reports list the exact 19 registered tests with no omitted execution. Each host passes **4,736 generated-C instruction cases**, both stack programs, both $2002 regressions, bus/device, cache/import and ABI-v2 contracts. A01 and A03/boundaries pass. No new failing CTest case versus the previous run.

**A02 remains failing and unchanged:** `apu_status_irq_collision_reference`, cycle 33,252, status $40, IRQ after read 0 versus expected 1. The PAL reference issue is not settled by this packaging fix. No expected-value change, exclusion, WILL_FAIL, disable/skip or continue-on-error was introduced. CTest and its unchanged JUnit gate remain failed; the successful Mac package steps do not turn the workflow green. Failed JUnit `status="fail"` is an executed failure, not a skip.

Windows/Linux combined build/test steps show failure because CTest exits 8 after successful full compilation and linking. Their tool/package steps are skipped by their unchanged gates; this task demonstrates build/test continuity, **not a newly verified Windows ZIP or Linux AppImage**. Existing independent Mac prerequisites/evidence handling continue unchanged except the added six-case command.

## Reproduction and RC1 limits

```sh
python3 -B tests/macos_lipo_checks.py
git diff --check
gh run view 38131057549 --repo inthe92/addams-family-recompiler
gh run download 38131057549 --repo inthe92/addams-family-recompiler --name frontend-macos-15 --dir /tmp/nes-arm64-ci-package
gh run download 38131057549 --repo inthe92/addams-family-recompiler --name frontend-macos-15-intel --dir /tmp/nes-intel-ci-package
# On a matching native macOS 15 host after reproducing the existing build/package steps:
python3 -B tests/macos_package_checks.py --build build --output dist --arch arm64 --report /tmp/nes-macos-package.json --require-native-macos15
```

For Intel use `--arch x86_64` and its matching native host/toolchain. No ROM is required for any of these checks. Ad hoc signature integrity is proved; **Developer ID/notarization, Gatekeeper/browser-quarantine first opening and real user GUI/game acceptance are not proved**.

Remaining RC1 gates: independently confirm A02's PAL reference without hiding the failure; verify current Windows/Linux final packages; test clean Mac first-open and owned-ROM import/cache relaunch; complete a normal unaided gameplay route and the presentation/input/audio acceptance checks. Those require separate authorization and are not implemented here. `main` remains at `1588f31e3c3375adfbb4f75266d44eff1fdb25e5`; local preexisting work is preserved. **Stop after this fix and report; no release or further correction.**
