# macOS 15 native CI verification

Date: **2026-10-11**. **Publication completed; package compatibility verification is BLOCKED.** Both native macOS 15 hosts compile/link and execute the complete suite, and their staged Python/Zig compile and load synthetic C. Both package attempts fail in the new architecture guard before signing or ZIP creation. No further correction was applied after that failure. The complete workflow remains **FAILURE**.

## Published source and preservation

Repository: `inthe92/addams-family-recompiler`, existing checkout `outputs/public-alpha-0.2.0/addams-family-recompiler`. Branch **`verify/phase-3-1-bus-timing`**; starting HEAD **`b649d6bd906788aaa83d0c5d5b336297344d7324`**. Published source/configuration commit: **[`b226778dbd69277dcf4191ec6d4bdf8a3b5fadd7`](https://github.com/inthe92/addams-family-recompiler/commit/b226778dbd69277dcf4191ec6d4bdf8a3b5fadd7)**.

Read the full `MACOS_DEPLOYMENT_FIX.md` and reviewed the existing pending changes before publishing. No branch switch, restore, conflict, incomplete Git operation, authentication workaround or force push. Origin matched the intended repository and work branch. Existing deployment changes were retained; preexisting untracked `docs/APU_A02_DIAGNOSIS.md` and `docs/RELEASE_1_0_READINESS.md` remain unchanged and local. All starting files except the explicitly extended workflow retain their task-start hashes.

`main` was `1588f31e3c3375adfbb4f75266d44eff1fdb25e5` before and after source publication, unchanged. No merge, release, tag, dependency update, ABI change or engine/game edit. Application version stays 0.2.1, ABI 2. This report is a separate documentation-only follow-up with `[skip ci]` to avoid rebuilding unchanged code; the completed red run below tests the source commit above. No source/test change accompanies that follow-up.

## Files included

| File | Reviewed change |
|---|---|
| `CMakeLists.txt` | Previous task's Apple deployment default 15.0 before language initialization |
| `tools/public/package_platform.py` | Previous task's explicit floor, bundle metadata and nested native architecture/minimum guard |
| `.github/workflows/build.yml` | Explicit macOS 15.0; split mandatory CTest from build/inventory; independently gated tool/package steps; retain package logs even after failure |
| `tests/macos_package_checks.py` | Content-free ZIP CRC/path checks, strict signatures, native host checks, vtool/otool inventory, bundled-tool compilation/load and importer process checks outside build directory; JSON evidence on success/failure |
| `docs/MACOS_DEPLOYMENT_FIX.md` | Previous deployment investigation, published unchanged |
| `docs/MACOS_15_CI_VERIFICATION.md` | This final report |

The source commit contains the first five files only. A clean candidate export passed the existing public audit: **114 files / 8,704,230 bytes**. Reviewed the exact staged allowlist and passed `git diff --check`; a bounded private-key/GitHub/AWS credential-marker scan found no matches. No ROM, extracted resource, generated game module, build cache, executable or raw diagnostic log was staged. The export audit is a bounded content check, not a legal/provenance certification or private-ROM byte comparison.

Windows and Linux workflow sections are byte-identical to starting HEAD; their packaging branches and requirements are unchanged. CPU/PPU/APU, service ABI, importer/recompiler, renderer, panoramico and GOD behavior were not modified.

## Actual native execution

[GitHub Actions run **38129810628**](https://github.com/inthe92/addams-family-recompiler/actions/runs/38129810628), event **push**, source SHA **b226778**, final state **completed / failure**.

| Native runner / actual OS | Full default build and main link | CTest executed | Passed | Failed | Skipped |
|---|---|---:|---:|---:|---:|
| [macos-15 / ARM64](https://github.com/inthe92/addams-family-recompiler/actions/runs/38129810628/job/114441609983), macOS **15.7.9** | PASS | 19 | 18 | 1, A02 | 0 |
| [macos-15-intel / x86_64](https://github.com/inthe92/addams-family-recompiler/actions/runs/38129810628/job/114441609998), macOS **15.7.9** | PASS | 19 | 18 | 1, A02 | 0 |
| [Windows x64](https://github.com/inthe92/addams-family-recompiler/actions/runs/38129810628/job/114441609852), Server 2025 / 10.0.26100, UCRT64 | PASS | 19 | 18 | 1, A02 | 0 |
| [Linux x64](https://github.com/inthe92/addams-family-recompiler/actions/runs/38129810628/job/114441609990), Ubuntu **22.04.5** | PASS | 19 | 18 | 1, A02 | 0 |
| **Total** | **4 builds pass** | **76** | **72** | **4** | **0** |

Windows/Linux have a combined build-and-CTest step marked failed: logs show successful full builds/main links, followed by CTest exit 8 for A02. Do not interpret that step's overall failure as a compiler/linker failure. No new failing CTest entry appears relative to the previous four-platform run.

Each inventory contains the same 19 executable mandatory entries; JUnit lists all 19 unique names. No disabled, WILL_FAIL or skip exemption is introduced. Failed JUnit cases have `status="fail"`, which indicates an executed failure, not an omitted test. The unchanged JUnit gate fails on A02; none of its checks was relaxed.

Every host passes **4,736 generated-C instruction executions** (18,944 across four hosts), two stack programs, both $2002 regressions, 14 timed bus side-effect cases, RAM/MMC1/bus/PPU, importer/cache and real ABI-v2 module contracts, diagnostic and APU/SDL checks. These are actual output diagnostics in retained JUnit, not inferred from successful compilation. A01 (`apu_status_open_bus_reference`), A03 (`apu_frame_mode_delay_reference`) and its boundary cases pass on all hosts.

**A02 remains an ordinary failing test** (`apu_status_irq_collision_reference`): cycle 33,252, $4015 returns $40, post-read IRQ 0 versus expected 1. Its PAL phase/reference uncertainty remains; neither production nor the assertion was changed. CTest and JUnit validation both remain failing gates. No exclusion, WILL_FAIL, artificial expectation, disabled test or continue-on-error was added.

## Native macOS package results and stopping cause

| Check on macOS 15.7.9 | ARM64 | Intel x86_64 |
|---|---|---|
| Configure with `CMAKE_OSX_DEPLOYMENT_TARGET=15.0`, correct target architecture | Executed / PASS | Executed / PASS |
| Full build/link, complete mandatory CTest | Executed; build PASS, CTest FAIL A02 | Executed; build PASS, CTest FAIL A02 |
| Checksum-verified existing toolchain fetch | Executed / PASS | Executed / PASS |
| Staged Python + Zig compile/sign/load synthetic C, return 42 | Executed / PASS | Executed / PASS |
| Staged bundled Python + current `import_probe`, 3 synthetic failure scenarios | Executed / PASS | Executed / PASS |
| Package copied into temporary staging outside build directory | Executed, reached architecture guard | Executed, reached architecture guard |
| Native architecture guard | **Executed / FAIL, lipo exit 1** | **Executed / FAIL, lipo exit 1** |
| Frontend/all dependency minimum inspection with vtool/otool in CI | **Not executed**, stopped before first vtool | **Not executed**, stopped before first vtool |
| Strict final app signature | **Not executed** | **Not executed** |
| ZIP generation, CRC/extraction, strict extracted-app signature | **Not executed; no ZIP generated** | **Not executed; no ZIP generated** |
| Run tools and synthetic module from extracted ZIP | **Not executed** | **Not executed** |
| Final frontend ZIP artifact upload | **Not executed** | **Not executed** |

Both runners fail at **`tools/public/package_platform.py:32`**, before the first `vtool` invocation:

```text
/usr/bin/lipo -verify_arch arm64 <temporary app>/Contents/MacOS/nes_native
/usr/bin/lipo -verify_arch x86_64 <temporary app>/Contents/MacOS/nes_native
# Both exit 1; CalledProcessError terminates packaging.
```

**Source-supported diagnosis: nonportable lipo argument order.** Apple's [cctools parser, pinned at e0d56624, lines 443-465](https://github.com/apple-oss-distributions/cctools/blob/e0d56624eca2a76c2ace4c21850df9e666de4ca5/misc/lipo.c#L443-L465) consumes every remaining argument after `-verify_arch` as an architecture. The trailing executable path is consequently interpreted as an architecture name. Input-before-option ordering is required by that parser. The newer local tool accepts the published ordering, so local success did not demonstrate compatibility with the runner's tools.

Both traces confirm the failing command and exit code. **The runner's stderr was captured but not printed by the existing guard**, and the exact native lipo implementation was not inspected separately. Thus the parser explanation is a grounded inference, not a reproduced runner stderr message or evidence that the built binary has the wrong architecture. Record that diagnostic limitation rather than claiming a binary incompatibility. Package logs are retained in CI artifacts on both hosts. No new correction, rerun or weakening of the architecture guard was attempted after discovering the native failure, as requested.

The independent macOS steps use explicit `!cancelled()`/successful audit/build/tool prerequisites so A02 does not suppress package evidence. A failed package prevents the extracted-package verifier and frontend upload; its log is still uploaded with `always()`. Mandatory failing tests retain the red final workflow. Windows/Linux package/tool steps remain skipped after A02 in their unchanged workflows; no Windows ZIP or Linux AppImage delivery result is asserted.

## Minimum, tools, dependencies and signatures: what is proved

**Configured floor: 15.0, both architectures. Actual native engine/test and staged-tool execution: macOS 15.7.9, both architectures.** This does not demonstrate execution on exactly 15.0, a finished signed package, first ROM import or gameplay.

The previous deployment task's unchanged binary/tool inspection remains valid local evidence: frontend minimum **15.0** both architectures; pinned Python **3.11.17**, minimum **11.0 ARM / 10.15 Intel**; pinned Zig **0.17.0**, minimum **15.0** both. These tools were fetched with the unchanged hash-verified recipes in CI, and their synthetic compiler/import processes actually run on native macOS 15.7.9. CLI version reporting and full nested load-command/dependency inventories from a **CI-generated ZIP** remain unexecuted because that ZIP does not exist.

Local frontend `otool -L` shows Apple system frameworks, libSystem and libc++, with no external SDL/Lua/build-tree linkage; nested Python/Tcl/Tk libraries are present and locally inspected in the prior report. **Those local results do not substitute for CI dependency inspection of the final delivery package.** Optional Python GUI/extensions and all frontend framework paths remain outside demonstrated runtime coverage.

New local verifier execution on **macOS 27.0.1 ARM64**, reusing the previous ARM test ZIP: PASS, 13 native files meet architecture/minimum checks; main/Info floor 15.0; ZIP CRC and clean external extraction pass; strict ad hoc signatures pass before extraction, after extraction and after bundled-tool execution; Python/Zig versions above, synthetic return 42 and three importer-process scenarios pass. This validates the test helper on that host, not macOS-15 delivery. The full existing ARM build was checked incrementally: no work to do. Previous full ARM/Intel builds and selected tests are documented in `MACOS_DEPLOYMENT_FIX.md`; they were not relabelled as new executions.

An initial local helper run correctly stopped at its overly broad draft cache-directory check, which also matched legitimate vendor Python `pip/.../build` source. The check was scoped to reject application/build caches while allowing the existing pinned toolchain's source/bytecode. Another local attempt encountered the session sandbox's unwritable default Zig cache; it passed using external `ZIG_GLOBAL_CACHE_DIR=/private/tmp/nes-macos15-ci-zig-cache`. Neither adjustment changed a production/test hardware expectation or A02. The final helper passed before publication, but was **not reached in either CI package run**.

Local signatures are ad hoc, not notarization, Developer ID, Gatekeeper or browser-quarantined/Finder first-open approval. The previous FinderInfo experiments are historical; **this CI failure is earlier and different**. No original user app/ZIP metadata was removed, and no original package was re-signed.

## Evidence and reproduction

Preserved CI artifacts: `ctest-macos-15`, `ctest-macos-15-intel`, `ctest-windows-x64`, `ctest-linux-x64`, `macos-package-verification-arm64`, `macos-package-verification-x86_64`. All four JUnit/inventory/LastTest logs and both packaging traces were downloaded and examined. No frontend artifacts were uploaded. Local raw evidence/export/snapshots/runner logs stay **outside public Git** in the surrounding workspace `macos15-ci-verification-2026-10-11/`; no large failed package download was needed.

```sh
gh run view 38129810628 --repo inthe92/addams-family-recompiler
gh run download 38129810628 --repo inthe92/addams-family-recompiler --pattern 'ctest-*' --pattern 'macos-package-verification-*' --dir /tmp/nes-macos15-ci-evidence
# On a matching native Mac, use a fresh external build tree:
cmake -S . -B /tmp/nes-macos15-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build /tmp/nes-macos15-build --parallel 3
ctest --test-dir /tmp/nes-macos15-build --output-on-failure --verbose --no-tests=error --output-junit /tmp/nes-macos15-ctest.xml
# CTest currently exits 8 for A02. Inspect packaging separately, without hiding this result.
python3 -B tools/public/package_platform.py --build /tmp/nes-macos15-build --output /tmp/nes-macos15-dist --toolchain /path/to/verified/matching/tools
# Only if packaging succeeds:
python3 -B tests/macos_package_checks.py --build /tmp/nes-macos15-build --output /tmp/nes-macos15-dist --arch arm64 --report /tmp/nes-macos15-package.json --require-native-macos15
```

For Intel use a native x86_64 macOS 15 host and matching toolchain/architecture arguments. The observed runner failure remains in the published source. Do not present the last verification command as executed on CI or assume it can pass until the packaging blocker is separately resolved.

## RC1 blockers and next bounded task

1. **Native Mac package generation is blocked:** authorize a separate minimal lipo invocation/diagnostic correction, then repeat native package/signature/ZIP/extracted-tool checks. Do not infer release compatibility from the successful builds.
2. **A02 is still unresolved and CI red:** obtain a reliable PAL phase/reference contract in its own task; preserve the current failing test meanwhile.
3. **Delivery acceptance remains pending:** actual finished Mac ZIP first-open/trust and owned-ROM import/cache relaunch; current Windows ZIP/Linux AppImage packaging and real desktop/Steam Deck operation. No commercial ROM was used in CI.
4. **Normal-game RC1 acceptance remains pending:** unaided route, presentation/defaults and actual audio/input checks from the readiness audit. No such changes were authorized or made here.

Stop after documenting the native packaging failure. No new package correction, A02 work, engine change, release, tag or main modification follows from this verification.
