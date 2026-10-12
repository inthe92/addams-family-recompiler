# Distribution synchronization and Linux AppImage diagnosis

Date: **2026-10-12**. Outcome: publication mismatch confirmed; existing upstream Linux verification recovered; new identity/icon checks and diagnostics prepared locally. **The user's Linux game closure is still pending, not claimed fixed.**

## 1. Initial audit and preservation

Local branch `verify/phase-3-1-bus-timing` initially pointed to `1029ddadf2671f5359afd3a1b28635ded651a48c`. The icon integration and previous reports were pending (34 files, including untracked assets/helpers). Their hashes and private backups were saved before extending anything.

The remote branch was four commits ahead, at `37c433b2ff6b9c822ef15005f5d2f42230312fcf`:

1. `949a969…`: add native Linux AppImage verification.
2. `ff241349…`: correct its ABI-header path.
3. `020540a…`: exercise direct AppImage import-error execution.
4. `37c433b…`: document verification.

Those changes affect only `.github/workflows/build.yml`, `tests/linux_appimage_checks.py` and `ABI_V3_LINUX_APPIMAGE_VERIFICATION.md`; no path overlapped with the 34 pending files. After announcing the audit diagnosis, a **fast-forward only** recovered these existing commits. All 34 pending files matched their original hashes immediately afterwards. No commit was created, no rebase/reset/branch switch or remote write occurred. Initial icon files/helpers, ROM implementation and historical reports remain unchanged; only the listed build/frontend/test/packaging files were extended.

## 2. What is actually available to users

The GitHub release API and tag references were queried live. The package's release label, internal version and build SHA must not be treated as synonyms.

| Download / source | Release label | Internal version evidence | Source provenance | ABI evidence | Official new icons |
|---|---|---|---|---|---|
| Published Windows ZIP | v0.3.0-alpha | 0.2.1, as documented in release notes | Windows `da4f990803518c4a57d31d40486d08909cf8860d` | ABI v2 in that source and published verification | Not this local icon integration |
| Published macOS ARM64/Intel ZIPs | v0.3.0-alpha | 0.2.1, as documented in release notes | Mac `64dd1064d4a24241554f2ce5d94aa92255bd688f` | ABI v2 in that source/verification | Not this local icon integration |
| Only published Linux AppImage | v0.2.0-alpha | Source tag is 0.2.0; binary version not read natively here | Tag resolves to `1588f31e3c3375adfbb4f75266d44eff1fdb25e5`; no embedded binary commit manifest | No versioned module ABI header in tag source; do not invent an exact binary ABI number | Old packaging icon |
| Latest existing CI, four platforms | No release | Source project version 0.2.1 | All jobs of run 38175008081: `37c433b2ff6b9c822ef15005f5d2f42230312fcf` | ABI v3 checks in CI | Predates the pending local icons |
| New local Mac package in this task | No release / no RC | Compiled identity 0.2.1 | Base `37c433b…` **plus dirty source fingerprint** | ABI v3, real SDL guard and synthetic C execution | Verified ICNS and official bytes |
| New local Windows/Linux/Intel preparation | Not generated/executed natively here | Expected project version 0.2.1 | Pending local changes; no new clean commit published | Verification prepared for v3 | Native checks still pending |

[Published v0.3.0-alpha](https://github.com/inthe92/addams-family-recompiler/releases/tag/v0.3.0-alpha) explicitly excludes Linux and documents the earlier Mac SHA. [Published v0.2.0-alpha](https://github.com/inthe92/addams-family-recompiler/releases/tag/v0.2.0-alpha) is still the source of the public AppImage download.

Public Linux hash: **`a52edfc55e882de6afcc6236b036330c8f4aac3db61ea9ca9f63f0427665b22e`**, 121,514,488 bytes. The cached local AppImage matches it exactly. This confirms that cached file is the old published package. The hash of the user's Linux machine's file was not supplied; exact identity there remains to be checked with `sha256sum`.

The Windows user's successful real-game report and the functional Mac report are accepted as user-provided context. They were not repeated here, and the display text “Alpha 0.2.1” alone cannot identify which SHA their packages contain.

## 3. Cause of the mismatch versus cause of the closure

**Confirmed publication mismatch:** v0.3.0-alpha promoted Windows and Mac, deliberately omitting Linux. Actions artifacts are generated independently and never automatically replace release assets. Consequently the latest public Linux file remains v0.2.0-alpha even though newer ABI v3 AppImages exist in CI. This is not evidence of a current Linux compilation failure or automatic stale-binary reuse by Actions.

**Existing workflow issue already corrected upstream:** older jobs stopped packaging after A02, and initially lacked native AppImage verification. Those changes were already present in the recovered commits; no duplicate correction was made. All four jobs now build from the event SHA, and Linux verifies its generated image before uploading it. A02 still makes the workflow red.

**Unresolved execution issue:** an old version may have bugs, but its age does not prove why a compatible ROM closes. No real-ROM Linux execution was available here. No CPU, PPU, APU, ABI, mapper or importer fix was attempted to manufacture a positive result.

## 4. Existing native Linux evidence, kept separate

[Run 38175008081](https://github.com/inthe92/addams-family-recompiler/actions/runs/38175008081), SHA `37c433b…`, was already complete. The small Linux verification JSON was downloaded; the large AppImage was not redundantly downloaded.

- Result **PASS** on native Linux x64.
- AppImage hash **`ba5ff840937172885fdd01251322a264466e642e3253ce08e855bd28e60abf13`**, 109,369,848 bytes.
- Extraction in a clean path with spaces, dependency checks, bundled Python/Zig and native C synthetic load: PASS.
- Direct import-error path: **exit 1**, diagnostic “ROM incompatible: se requiere la revision europea…”; this is an expected synthetic rejection, not a game crash.
- No real compatible ROM, room transition, graphical game session or Steam Deck test is demonstrated by that JSON.

[Existing CI AppImage artifact](https://github.com/inthe92/addams-family-recompiler/actions/runs/38175008081/artifacts/11716000688). It is newer than the release download, but has **neither the new local icons nor the new identity/diagnostic changes**. It is not silently replaced or presented as the new distribution.

## 5. Minimal changes prepared locally

- `tools/public/build_identity.py`: generate version/commit/ABI/target/source fingerprint; normalize text CRLF for cross-platform comparison; keep binary asset bytes exact. Unknown/unversioned or dirty sources are explicitly labelled.
- `CMakeLists.txt`: regenerate identity before frontend compilation, embed it in the executable; bundle version follows `PROJECT_VERSION`; add mandatory `package_identity_contract`.
- `app/desktop.cpp`: read-only **`--build-info`** returns the compiled JSON before ROM lookup or SDL initialization. No game-option or NES-runtime changes.
- `tools/public/package_platform.py`: require build metadata matching current source; place `PACKAGE_INFO.json` inside each platform's resources; reject pre-existing package destinations rather than merging stale files or deleting user files; produce ZIP SHA-256 sidecars.
- `tools/public/package_linux.sh`: AppImage SHA-256 sidecar, retaining the existing official-PNG verification.
- `tools/public/diagnose_linux.sh`: shipped alongside AppRun; creates a private local log with identity, dependency observations, stdout/stderr and exit status. No ROM bytes/modules are copied or uploaded.
- `tests/linux_appimage_checks.py`: verify official PNG/hicolor/desktop/DirIcon, content, checksum, binary identity, ABI headers, Python/Zig dependencies, direct/extracted identity, synthetic module via the real SDL guard; compare direct AppImage, extracted AppRun and unpackaged frontend rejection paths; require exact exit 1 and `import.log`, not a vague nonzero result containing “import”.
- `tests/macos_package_checks.py`, `tests/windows_package_checks.py`: compare compiled identity to package/ABI header and checksum; optional expected SHA requires a clean tree.
- `tests/public_toolchain_smoke.py`: reuses the existing real `module_abi_checks` target and its missing/v1/v2 fixtures, replacing only its current-module input with the newly compiled synthetic module. No new interpreter or runtime target was introduced.
- `tests/package_identity_checks.py`: seven methods covering clean-SHA policy, invalid/stale metadata, binary/header mismatch, real-guard inventory, LF/CRLF fingerprinting, and rejecting crashes as successful import-error tests.
- `tests/icon_asset_checks.py`: synthetic package fixture now supplies metadata; explicitly verifies that an old destination is rejected.
- `tests/verify_ctest_suite.py` and its self-tests: metadata entry mandatory; no skip/failure exemptions.
- `.github/workflows/build.yml`: all four verifiers receive **`${{ github.sha }}`**; ZIP/AppImage checksum sidecars are preserved. Full CTest and failing JUnit gates remain mandatory; independent packaging remains observable after A02 without making the workflow green.

No version was arbitrarily bumped to RC. Current project version remains **0.2.1**. Selecting the eventual RC tag/version and publishing coherent packages is a later authorized publication step.

## 6. Verification performed in this task

Host: **macOS 27.0.1 ARM64**. Existing valid builds/tools were reused, rebuilding the affected frontend and tests; no dependency installation or exhaustive ADC/SBC repetition.

| Check | Executed result |
|---|---|
| Desktop complete build | PASS |
| Headless complete build | PASS, without SDL dependency |
| Desktop full CTest | **34 executed, 33 passed, 1 A02 failure, 0 omitted**, exit 8 |
| Headless full CTest | **31 executed, 30 passed, 1 A02 failure, 0 omitted**, exit 8 |
| IRQ cases | **82/82 per configuration** |
| Existing instruction/device/bus/PPU/APU/ABI/import/cache tests | PASS apart from unchanged A02; 4,736 native instruction executions per configuration |
| New metadata unit methods | **7/7** |
| Existing/new icon unit methods | **7/7** |
| CI gate self-tests | **6/6** |
| Windows format self-tests on Mac | **11/11**, not native Windows |
| lipo/deployment self-tests | **6/6**, tiny probes; not an Intel frontend execution |
| Linux diagnostic shell harness | PASS: stub AppRun/dependency commands, private log created and exit 7 preserved; not Linux/game execution |
| Full newly generated Mac package verifier | **PASS**: official icon, identity, archive checksum/CRC, native tool execution and synthetic compile/load |
| Actual SDL loader for bundled synthetic ABI v3 module | **PASS**: missing/v1/v2 rejected and new synthetic v3 accepted |
| Ad hoc strict signature before/after ZIP extraction and tools | **PASS**, not notarized |
| Local package pretending to be a clean SHA | Correctly **rejected** because `dirty=true` |
| Public package content audit | PASS; no ROM/game module detected; no private-ROM byte comparison |
| `git diff --check`, shell syntax | PASS |

Fresh metadata/official-icon verification on Windows x64, Linux x64 and macOS Intel has **not** run. Existing native CI results belong to the earlier clean SHA and are not relabelled as results of this local patch. The new workflow is ready for those runners but was not dispatched.

### Mac local artifact

- `NES-Native-Lab-macOS-arm64.zip`, 135789967 bytes.
- SHA-256 **`291cf193c701d43c446d0030f9ab1490495944253749e0f4b789f63af2c558a3`**.
- Identity: version **0.2.1**, ABI **3**, base SHA **37c433b…**, `dirty=true`.
- Normalized source fingerprint **`99bce870d1d1d122c3655b01926afdabe9f8d208fea2d5d61125c6a6e0b9b71c`**.

This is a traceable local test package, **not** a clean-commit RC. New packages/sidecars, logs, native JSON and private backups are outside the repository. The earlier Mac icon ZIP and cached old Linux AppImage were left untouched.

## 7. Reproducing the Linux user's closure

First establish exactly which file is being executed. Do not infer revision from its filename or window title. These shell commands run on the user's Linux machine, not on this Mac:

```sh
TASK_IMAGE="/absolute/path/NES-Native-Lab-Linux-x86_64.AppImage"
TASK_ROM="/absolute/path/to/the/users-compatible-ROM.nes"
TASK_LOGS=$(mktemp -d)
chmod +x "$TASK_IMAGE"
sha256sum "$TASK_IMAGE" > "$TASK_LOGS/image-identity.txt"
# For packages containing the new metadata:
APPIMAGE_EXTRACT_AND_RUN=1 "$TASK_IMAGE" --build-info > "$TASK_LOGS/build-info.json" 2> "$TASK_LOGS/build-info-error.txt"
# An older package may not support --build-info; retain its error and hash.

APPIMAGE_EXTRACT_AND_RUN=1 "$TASK_IMAGE" --import-only --rom "$TASK_ROM" --import-cache "$TASK_LOGS/private-cache" > "$TASK_LOGS/import-console.log" 2>&1
TASK_IMPORT_EXIT=$?
printf 'Import exit status: %s\n' "$TASK_IMPORT_EXIT" > "$TASK_LOGS/status.txt"
# Test actual launch only after successful import, using the same private cache:
if [ "$TASK_IMPORT_EXIT" -eq 0 ]; then
  APPIMAGE_EXTRACT_AND_RUN=1 "$TASK_IMAGE" --rom "$TASK_ROM" --import-cache "$TASK_LOGS/private-cache" --no-god --diagnostic > "$TASK_LOGS/game-console.log" 2>&1
  TASK_GAME_EXIT=$?
  printf 'Game exit status: %s\n' "$TASK_GAME_EXIT" >> "$TASK_LOGS/status.txt"
fi
printf 'Private logs: %s\n' "$TASK_LOGS"
```

Only perform the game-launch command after successful import; a successful `--import-only` does **not** prove module loading/rendering/gameplay. Keep the ROM and `private-cache` private: the cache contains game-derived modules. Share only relevant error lines/status/hash, redacting personal paths; do not upload the whole log folder or a core dump.

For the newly prepared distribution, extracting the AppImage also exposes `squashfs-root/diagnose-linux.sh`. Run that helper with the same `--rom`, `--import-cache`, `--no-god` and `--diagnostic` arguments. It writes under `${XDG_STATE_HOME:-$HOME/.local/state}/nes-native-lab` and preserves the exit status. It does not exist in the old published AppImage.

Compare phases:

1. Loader/dependency error before Python/import: examine stderr/`ldd`.
2. Nonzero import: inspect `private-cache/import.log`, exact compatible profile/hash and Python/compiler diagnostics.
3. Successful import but failed launch: inspect ABI guard, missing native-address messages, OpenGL/SDL and game-console output.
4. Signal exit such as 139 versus controlled exit 1: record it; do not label either as an A02 failure without evidence.
5. Repeat with extracted AppRun and the same clean cache/resources to distinguish AppImage launcher problems from frontend/runtime problems. Automated CI now makes this comparison with synthetic input only.

## 8. Status and RC blockers

- **Version mismatch:** confirmed at the publication layer; not yet repaired publicly because no release/push authorization was given.
- **Current upstream Linux synthetic execution:** verified in the retrieved native CI report, including direct error path. No claim of valid-ROM gameplay.
- **New icon/metadata integration:** local Mac ARM64 verified; native Windows/Linux/Intel jobs pending publication of the local patch.
- **Linux closure with compatible ROM:** **PENDING REPRODUCTION**. No speculative runtime correction was made.
- **A02:** remains visible, unchanged and unresolved; no playable impact is demonstrated by this task. It is not offered as the cause of the Linux closure.
- **First RC:** needs an authorized clean commit containing these changes and icons, a four-platform run whose native packages all agree on identity, and Linux valid-ROM/private gameplay evidence. The global CI will remain red for A02 until its independently justified treatment is decided; the new checks do not hide it.

No new commit, push, tag, release, PR or remote workflow was created. Main is untouched. This report and all integration work remain local. Next step is reviewing/authorizing the clean publication/CI stage, then asking the Linux tester to run the identified current artifact and provide the private-log excerpts described above.
