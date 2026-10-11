# Windows x64 portable package validation

Date: **2026-10-11**. **The complete Windows x64 portable ZIP is generated,
cleanly extracted and verified on a native Windows runner.** Bundled tools,
synthetic import/cache checks and actual frontend picker/error shutdown pass.
**Full CI remains FAILURE because A02 fails.** No real game session is claimed.

## Source and scope

Public checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` in the
existing workspace. Branch **`verify/phase-3-1-bus-timing`**; starting HEAD
**`159e9f636eb06e82565b3085e849d1fb74501e04`**. Origin matched the requested
GitHub repository and remote branch matched HEAD. No conflicts, unmerged
index entries or incomplete Git operation. Local untracked
`docs/APU_A02_DIAGNOSIS.md` and `docs/RELEASE_1_0_READINESS.md` are preserved
byte-for-byte and excluded from publication. Read the release-readiness,
four-platform, Windows/JUnit and macOS-lipo reports. Historical reports do
not replace the current run.

Published source commits: initial Windows packaging/test change
**`05df30357565e163b98bf48b677cdacc05934a0a`**, followed by the checker-only PID
correction **`f8ea631c6b3cebf1a4f82ec45817c81df1c84a78`**, and explicit upstream
launcher-template aliases **`64dd1064d4a24241554f2ce5d94aa92255bd688f`** (four-host baseline).
[Native run 38132924396](https://github.com/inthe92/addams-family-recompiler/actions/runs/38132924396).
During review the process checker was found to include its own bundled Python
process. It now excludes only `os.getpid()`, retaining all application/worker
checks. Structural inspection of an existing private package whose Python/Zig
archive hashes match the current pins identified `setuptools/cli.exe` and
`gui.exe` as additional upstream launcher templates. Those exact paths are
recorded as templates, without weakening x64 checks on active programs.
Runs 38132527268 and 38132777490 were explicitly cancelled to avoid spending
resources on superseded harnesses; neither counts as full verification.
That four-host run built all platforms and executed 19 CTest entries on each,
but Windows package verification failed **before tool/frontend execution** with
`KeyError('SystemRoot')`. ZIP generation, CRC/required-file/content checks had
passed. Native Python uppercases environment keys; copying `os.environ` into
a regular dictionary removes its case-normalized lookup. The one-line fix
reads `os.environ['SystemRoot']` directly, as specified by
[Python's environment contract](https://docs.python.org/3.11/library/os.html#os.environ).
No package/game fault was demonstrated by this harness error.

Final source **`da4f990803518c4a57d31d40486d08909cf8860d`** adds that lookup fix
and optional Windows-only manual dispatch. [Focused native run 38142932201](https://github.com/inthe92/addams-family-recompiler/actions/runs/38142932201)
verifies this correction without rebuilding already verified Mac/Linux source.
The final report follows in a documentation-only commit; source verification
refers to the source commit, not an unexecuted new engine revision.

Changes are limited to:

| File | Change |
|---|---|
| `tools/public/package_platform.py` | One Windows-only copy of the quick-start text into the ZIP root |
| `tools/public/WINDOWS_README.txt` | English/Spanish extraction, ROM selection, controls, normal-mode and error-log instructions |
| `tests/windows_package_checks.py` | Standard-library ZIP/PE checks and native extracted-package checks; reuse existing toolchain, import-process and cache tests |
| `.github/workflows/build.yml` | Windows build/CTest separation, independent tool/package prerequisites, extracted verification, evidence/artifact upload; optional Windows-only manual dispatch |
| `docs/WINDOWS_PORTABLE_VALIDATION.md` | This report |

Production engine, recompiler, ABI **2**, game logic, renderer, widescreen and
GOD are unchanged. macOS/Linux build/test/package commands are byte-identical
to starting HEAD. Only manual-dispatch routing was added to their jobs;
normal push/PR and default manual dispatch still select all four hosts.
No dependency, tag, release, main change or force push.

## Package contract

The archive contains a single `NES-Native-Lab-Windows-x86_64/` directory:

- `nes_native.exe`: MSYS2 UCRT64 build, static SDL2/Lua/C++ runtime.
- `toolchain/python/`: complete pinned Windows x64 CPython distribution,
  including its DLLs, extensions, standard library and upstream notices.
- `toolchain/zig/`: pinned Zig, compiler resources, libraries and notices.
- `toolchain/target.json` and `runtime_tools.json`: native target and pinned
  upstream archive hashes/provenance.
- `tools/recompiler/`: importer, code emitters, opcode definitions and
  compatible-profile identifiers; no commercial instruction bytes.
- `include/`, `game/include/`: authored ABI/resource declarations.
- `WINDOWS_README.txt`, README in English/Spanish, `docs/`, project license,
  `THIRD_PARTY.md` and SDL2/Lua notices in `licenses/`.

The whole directory is required. Windows system DLLs and graphics drivers
remain system dependencies. No CMake, MSYS2, Visual Studio, separately
installed Python or Zig is required by the tested importer path.
The packager copies existing pinned distributions intact; executable DLL
closure is checked rather than adding arbitrary compiler DLLs.

No ROM, extracted game graphics/music, user-native game module, user cache,
private diagnostic log or temporary compiled test module belongs in the ZIP.
Synthetic compiler output and invalid-image fixtures exist only in private
temporary test directories. The existing final content audit is reused.
It excludes verified upstream toolchain content and performs no private-ROM
byte comparison. These bounded checks are not an exhaustive rights or
credential certification.

## Verification method

The native Windows job retains all **19 unfiltered mandatory CTest entries**,
JUnit and inventory validation. A02 is neither suppressed nor treated as an
expected pass. Successful source audit/build are prerequisites for independent
package steps even after the known CTest failure. Any tool, package or
verification failure is fatal to its own step; the ZIP uploads only after
successful extracted-package verification. `pipefail` preserves packaging
errors while recording its log.

The verifier checks ZIP CRC, safe names/case collisions, required files and
public content. It calculates the inner ZIP's SHA-256 and writes a sidecar.
It extracts into a new temporary directory with spaces outside checkout and
build, and executes from a different empty working directory. Child PATH
contains **only Windows System32**; installed compilers/interpreters/MSYS2
and Python path/home overrides are removed.

The actual extracted CPython runs the checks. PE architecture and normal/delay
DLL imports follow [Microsoft's PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
Dependencies must reside with their owning package executable/Python runtime
or resolve using [LoadLibraryEx SYSTEM32-only search](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw).
Upstream pip/setuptools launcher templates for other architectures are
explicitly identified/recorded; they are not importer runtime programs.
The active application/Python/Zig/extension binaries must be x64 PE32+.

Existing synthetic checks compile and load a C DLL returning **42**, verify
three importer error scenarios and execute four cache/ABI decision tests
against the extracted scripts/headers. Cache tests mock ROM validation and
compiler entry, as the existing unit harness does; they do **not** prove
first successful owned-ROM import or cached gameplay relaunch.

Two checks launch the **actual extracted `nes_native.exe`**, with no explicit
`--import-tools`: cancel its native ROM picker and reject a synthetic image
of the required size but incorrect hash. The verifier inspects windows owned
only by that test PID, closes the real picker/error dialog and checks normal
process exit. It inspects running process executable paths confined to its
unique extraction to detect remaining packaged frontend/Python/Zig processes.
An emergency test-process kill is a failure, never proof of clean shutdown.
These checks prove startup/import error handling, not OpenGL gameplay startup.

## Local results

Host: **macOS 27.0.1 ARM64**; no Windows runtime available locally.

- **11 new format/archive checks executed / 11 passed / 0 failed / 0 skipped**.
  PE x64/other-target identification, normal and delay DLL imports, malformed
  inputs/RVA, safe ZIP/traversal/case collision/wrong root.
- Existing relevant CTest selection: **3 executed / 3 passed / 0 failed / 0
  skipped** (`public_import_contract`, `import_cache_abi_contract`,
  `module_abi_contract`). Uses the existing current-source Release build;
  engine is unchanged. Not a complete local CPU/APU rerun.
- Python syntax and workflow YAML pass. macOS/Linux job identity and hashes
  of all unrelated/preexisting files pass. `git diff --check` passes.
- Reviewed candidate source export: **119 files / 8,764,182 bytes**, public
  audit PASS; bounded private-key/GitHub/AWS credential markers: no matches.
- Native Windows frontend/toolchain checks: delegated to actual CI; local
  architecture parsing is not Windows execution.

## Native CI results and downloadable package

**Exact verified source: `da4f990803518c4a57d31d40486d08909cf8860d`.**
[Run 38142932201](https://github.com/inthe92/addams-family-recompiler/actions/runs/38142932201),
[Windows job 114480321232](https://github.com/inthe92/addams-family-recompiler/actions/runs/38142932201/job/114480321232),
completed **FAILURE**, solely the full CTest and unchanged JUnit gates for A02.
All build/tool/package/verifier/upload steps pass. Native host is **Windows
Server 2025, 10.0.26100, AMD64**, MSYS2 **UCRT64 / GNU 16.2.0**. This is native
execution, not cross-compilation or consumer-Windows certification.

| Executed check | Actual result |
|---|---|
| Full default build / `nes_native.exe` link | PASS |
| Complete CTest / retained inventory and JUnit | **19 executed / 18 passed / 1 failed (A02) / 0 skipped** |
| Existing generated-C CPU, stack, PPUSTATUS and side effects | **4,736 instruction executions**, both stack programs, both $2002 regressions and 14 side-effect cases PASS |
| RAM, MMC1, bus, PPU, APU core, ABI-v2, diagnostics/import/cache | PASS; A01/A03 and A03 edges remain passing |
| New PE/ZIP format harness | **11 executed / 11 passed / 0 failed / 0 skipped** on Windows |
| ZIP generation / CRC / required-file checks | PASS / PASS / PASS |
| Fresh extraction with spaces and `&`, outside checkout/build | PASS; separate empty working directory |
| Installed-tool exclusion | Child PATH **`C:/Windows/System32`**; Python path/home and compiler overrides removed |
| Extracted Python / Zig execution | PASS: **Python 3.11.17 / Zig 0.17.0** |
| PE architecture / DLL inventory | PASS: **62 PE files**, **52 active x64 PE32+** and 10 explicitly identified generic upstream launcher templates |
| Active DLL imports | Resolved to package/Python-owned files or Windows-only system providers; no MSYS2/compiler-directory DLL dependency |
| Synthetic C DLL | PASS: bundled Zig compiles, bundled Python loads it and observes **42**; temporary output outside ZIP |
| Existing synthetic importer process scenarios | **3/3 PASS**: invalid-image hash rejection with spaced paths/trailing separator, startup stdout/stderr, Python launch failure |
| Existing cache/ABI decision tests on extracted resources | **4/4 PASS**: matching ABI reuse; old/missing ABI and changed generator invalidate; mocked validation/compiler as documented |
| Actual frontend ROM-picker startup/cancel | PASS: title **`Importa tu ROM compatible`**, normal exit **0** after cancel |
| Actual frontend import/error presentation | PASS: no tools override, bundled Python rejects synthetic image, real dialog shows **`SHA-256 distinto`** and log location, exit **1** after OK |
| Packaged process shutdown after each frontend case | PASS: **zero remaining package processes**; no emergency kill used |
| Final public content audit | PASS: **23,491 files**, **6,835,946 owned bytes scanned**; no named ROM/private game module/cache; upstream subtree excluded as documented |
| Frontend ZIP and SHA sidecar artifact upload | PASS; nonexpired and downloaded/inspected |

The five child command groups all return 0; frontend exits are separately
checked against their intentional 0/cancel and 1/import-error contracts. Do
not sum nested cases with CTest entries or count 52 architecture records as
52 executed applications. The reused cache tests are unit decision checks,
not real game cache generation/relaunch.

Archive **`NES-Native-Lab-Windows-x86_64.zip`**: **130,874,329 bytes**, 23,491
files. Inner ZIP SHA-256:

```text
1881b4cd623d2a2067a9d9ca1514701e45eb86ef21663cce3b2fc1479564ca50
```

[Download Windows artifact `frontend-windows-x86_64`](https://github.com/inthe92/addams-family-recompiler/actions/runs/38142932201/artifacts/11703667052).
It includes the application ZIP plus `.zip.sha256`; the artifact wrapper is
124,988,878 bytes and has a different digest from the inner ZIP. GitHub access
or sign-in may be required. At inspection its retention expiry is
**2027-01-09**. This is an Actions artifact, **not a release**.
[Verifier JSON/package log](https://github.com/inthe92/addams-family-recompiler/actions/runs/38142932201/artifacts/11703876463)
and [CTest inventory/JUnit/log](https://github.com/inthe92/addams-family-recompiler/actions/runs/38142932201/artifacts/11703143838)
retain the actual evidence. One copy of the Windows artifact was downloaded:
its ZIP hash matches both the native report and sidecar; its inventory,
Python PSF/Zig MIT notices and quick-start were inspected. Windows checkout
uses CRLF for the text instructions; their content matches after normalizing
newlines. No archive was modified. CRC/extraction/native execution were not
redundantly repeated on macOS.

## Other platforms and regression baseline

[Full four-host run 38132924396](https://github.com/inthe92/addams-family-recompiler/actions/runs/38132924396)
at **64dd106** actually built all hosts and ran all 19 CTest entries per host:

| Native platform | Full build/link | Executed / passed / failed / skipped CTest | Package outcome |
|---|---|---|---|
| macOS 15.7.9 ARM64 | PASS | **19 / 18 / 1 A02 / 0** | Architecture/floor, signing, ZIP extraction, Python/Zig and synthetic checks PASS; uploaded |
| macOS 15.7.9 Intel | PASS | **19 / 18 / 1 A02 / 0** | Same package checks PASS; uploaded |
| Linux x64 / Ubuntu 22.04 | PASS | **19 / 18 / 1 A02 / 0** | AppImage steps skipped after A02; no new Linux package verification claimed |
| Windows x64 | PASS | **19 / 18 / 1 A02 / 0** | ZIP built/audited; initial verifier failed on environment lookup; superseded by final Windows run above |

Baseline total: **76 executed / 72 passed / 4 A02 failures / 0 CTest skips**.
Final focused run: **19 / 18 / 1 / 0**, Windows only. Its macOS/Linux jobs are
deliberately **not executed**, not silently claimed as current-SHA results;
the production/compiler commands are unchanged from the four-host baseline.
No new failing engine test occurred. JUnit's existing `Test did not run`
wording on A02 is misleading: the XML contains an executed `status="fail"`
case plus failure output, not a skip. No gate or expected value was relaxed.

**A02 remains unchanged:** `$4015` cycle **33,252**, status **$40**, post-read
IRQ **0** versus test expectation **1**. The PAL reference still requires
independent validation. Neither test nor APU was modified. A01 observes $20
status while preserving external $A5; A03 and its parity/edge schedules pass.

## Limitations and next alpha gates

A01/A03 must remain passing; **A02 remains open and CI remains failed when it
reproduces**. The PAL phase/latch reference is not settled by packaging.
No commercial ROM was used in these checks. A normal unaided game completion,
real successful ROM import/cached game relaunch, graphical gameplay, sound,
controller support and consumer Windows 10/11 first opening remain separate
acceptance gates. Native Windows Server runner execution alone does not prove
every consumer OS version or graphics stack.

The experimental alpha currently defaults GOD on and includes experimental
widescreen; neither is changed here. Use `--no-god`, no G/H/mods, for normal
game testing. Closing import progress is not implemented as cancellation;
do not claim that the short error/picker shutdown checks verify cancellation
of a long compiler operation. No Authenticode/SmartScreen trust certification
or public release is created. Keep the full directory and upstream notices.

**Recommendation:** this immutable ZIP is suitable for a clearly labeled
**experimental testing alpha**, with its complete folder, checksum and known
limits. Package portability within the tested synthetic/startup scope is
demonstrated. Before opening general public distribution, have a tester run
an actual owned compatible-ROM import, OpenGL gameplay, sound and cached
relaunch on consumer Windows; record any failure rather than treating these
synthetic checks as a game run. A02/full-CI failure must remain disclosed.
Do **not** call it RC or 1.0. A normal unaided full route remains an additional
gameplay acceptance task; no release, tag or new architecture is created here.

## Simple Windows use and reproduction

Download the Actions artifact, extract its wrapper and then the entire
application ZIP. In File Explorer open `NES-Native-Lab-Windows-x86_64` and
double-click `nes_native.exe`. Select your own supported European En/Fr/De
ROM; initial local compilation can take several minutes. For unaided play,
open a terminal in that folder and run `.\nes_native.exe --no-god`. Controls: arrows, Z/X,
Enter, right Shift; Escape closes gameplay. Error dialogs give the cause and
`%APPDATA%\nes-native-lab\user-import\import.log`. Do not publish the ROM.

Developer reproduction on native Windows x64 after the existing full build:

```sh
ctest --test-dir build --output-on-failure --verbose --no-tests=error --output-junit "$PWD/build/ctest-results.xml"
python -B tools/public/fetch_runtime_tools.py --destination runtime-tools --cache runtime-cache --system windows-x86_64
python -B tools/public/package_platform.py --build build --output dist --toolchain runtime-tools
python -B tests/windows_package_checks.py --self-test
python -B tests/windows_package_checks.py --build build --output dist --report build/windows-package-verification.json
# Existing workflow, optional Windows-only native execution:
gh workflow run build.yml --repo inthe92/addams-family-recompiler --ref verify/phase-3-1-bus-timing -f platform=windows
```

CTest's A02 failure must be retained while running the independent commands.
Raw evidence stays outside public Git in surrounding workspace
`windows-portable-validation-2026-10-11/`, and in CI artifacts. Stop after this
packaging verification; no engine correction is authorized here.
