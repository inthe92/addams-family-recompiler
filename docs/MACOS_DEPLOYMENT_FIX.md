# macOS deployment floor and package verification

Date: **2026-10-11**. **Build/package correction completed locally: macOS 15.0 is the explicit deployment floor for ARM64 and Intel with the existing tools. Actual execution on macOS 15 remains pending.** No engine, gameplay, graphics, ABI or A02 change; no commit, push, tag, release or notarization.

## 1. Checkout and previous minimum

Existing public checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` in the local project workspace. Branch **`verify/phase-3-1-bus-timing`**, HEAD **`b649d6bd906788aaa83d0c5d5b336297344d7324`**, origin `https://github.com/inthe92/addams-family-recompiler.git`. Application version remains **0.2.1**, module ABI **2**.

Starting tracked/index state was clean. Preexisting untracked `docs/APU_A02_DIAGNOSIS.md` and `docs/RELEASE_1_0_READINESS.md` are preserved byte-identically. Read the readiness, Windows/JUnit and platform reports; their historical results are not relabelled as executions of this change. No branch was changed or user build/cache restored.

The existing development cache had **empty `CMAKE_OSX_DEPLOYMENT_TARGET`**. The published ARM ZIP's executable and previous local desktop executable declare **`LC_BUILD_VERSION minos 27.0`, SDK 27.0**. Implicit compiler/SDK defaults thus restricted those binaries to macOS 27 despite older-host CI compilation. The old release remains unchanged.

## 2. Chosen floor and independent justification

**macOS 15.0 for both architectures** is the lowest justified floor while preserving the pinned importer dependencies. The deciding requirement is the **compiler executable**, not C++ or the main application's previous load command:

| Component | ARM64 declared minimum | Intel declared minimum | Evidence |
|---|---|---|---|
| Pinned Zig 0.17.0 executable | **15.0** | **15.0** | Actual `vtool` load commands; [official Zig platform requirements](https://ziglang.org/learn/platform-support/) specify Darwin 15.0+ |
| Pinned standalone Python 3.11.17 executable/native libraries | **11.0** | **10.15** | Inspected actual pinned distributions, not python.org installer requirements |
| New frontend | **15.0** | **15.0** | Built in this task, inspected after link |
| Synthetic module built through the unchanged importer compiler selection | **15.0** | **15.0** | Compiled and dynamically loaded on this host; actual load commands inspected |

The existing ARM archive hashes match the pinned manifest. Intel Python/Zig were fetched using the existing checksum-verified recipe, without changing versions, URLs or hashes. Both compiler binaries use SDK 27.0 with **minos 15.0**; an SDK version is not a deployment floor.

C++17/filesystem, NSOpenPanel, SDL2 input/window/audio and the OpenGL 3.2 interface do not establish a higher requirement in the reviewed frontend. SDL2 2.32.8 builds at 15.0; its Cocoa/controller code contains availability guards, and optional frameworks are weak-linked. Lua is statically built C. Both fresh builds finish without an API-unavailability failure; existing SDL/Cocoa/OpenGL deprecation and optional-dependency warnings remain. Compilation is not proof of every OS-15 symbol or GUI path at runtime.

Supporting macOS 14 or earlier with self-contained first import would require replacing/rebuilding the pinned Zig distribution and validating that alternate toolchain. Lowering only the frontend label would leave the user's compiler unable to run. That dependency change is outside this small correction; **no downgrade or new dependency was introduced**. Python's different minima do not require different package floors because Zig dominates both.

## 3. Exact modifications

| File | Change |
|---|---|
| `CMakeLists.txt:2` | Set nonempty default `CMAKE_OSX_DEPLOYMENT_TARGET=15.0` on an Apple host **before `project()`**, using the cache; an existing explicit nonempty value is preserved |
| `.github/workflows/build.yml:31` | Pass `-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0` to the existing ARM/Intel macOS matrix builds |
| `tools/public/package_platform.py:17` | Read the explicit build-cache minimum; reject missing/empty deployment configuration |
| `tools/public/package_platform.py:23` | Check architecture and macOS minimum for nested Mach-O executables/dylibs/bundles with `lipo`/`vtool`; reject a dependency requiring a newer OS or a wrong architecture |
| `tools/public/package_platform.py:75` | Put the verified build floor in `LSMinimumSystemVersion`, then validate the full app before signing |
| `docs/MACOS_DEPLOYMENT_FIX.md` | This report |

The check handles the current distributions' Mach-O formats and both modern and legacy macOS minimum load commands. It ignores SDK version numbers and non-runtime compiler data/static objects. It is a declaration/architecture guard, not an availability emulator or exhaustive symbol-resolution checker. Symlink aliases are skipped; their actual regular binary targets in the bundled distributions are inspected.

Windows/Linux workflow sections are **byte-identical** to the starting versions. Their packaging branches and requirements are unchanged. The macOS cache variable is ignored by non-Apple target platforms; it does not change their compiler flags. CPU/PPU/APU, renderer, game, recompiler, service headers/ABI, profile, importer target recipes and existing tests are untouched. There is no module/cache-format change: generated modules use the unchanged target/compiler and ABI-v2 checks.

## 4. Builds and dependency inspection

Host: **macOS 27.0.1 ARM64**, AppleClang 21, Command Line Tools SDK 27, CMake 4.4.4/Ninja, Release, desktop/mods/testing ON. Full default builds were performed in **new** `/private/tmp/nes-macos15-arm64-build` and `/private/tmp/nes-macos15-intel-build` trees. Old user build directories were not reconfigured. Both caches default to 15.0; generated compile/link flags include `-mmacosx-version-min=15.0`.

| Configuration | Full build | Frontend architecture / minimum | Package metadata / guard |
|---|---|---|---|
| ARM64 | PASS | ARM64 / **15.0**, SDK 27.0 | `LSMinimumSystemVersion=15.0`; **13 native files checked**, PASS |
| Intel | PASS (cross-compiled on ARM) | x86_64 / **15.0**, SDK 27.0 | `LSMinimumSystemVersion=15.0`; **13 native files checked**, PASS |

`otool -L` on both frontends shows Apple system frameworks, `/usr/lib/libSystem.B.dylib` and libc++, with no dependency on a development build directory or external SDL/Lua installation. Frameworks include Cocoa/AppKit/Foundation, OpenGL, CoreVideo, IOKit, audio and the existing weak optional controller/Metal/QuartzCore/CoreHaptics links.

Each toolchain has **12 regular native runtime files**: Python executable/libraries/extensions and Zig; the frontend makes 13. ARM Python-related files declare 11.0; Intel files declare 10.15; Zig declares 15.0. The toolchain dependency inventory was retained. Tcl/Tk libraries/extensions carry their bundled library IDs/rpath references; those files are included, and their minima do not raise the floor. Importer Python modules and synthetic compilation/load were exercised; **optional Tk GUI operation and every Python extension were not runtime-certified**.

## 5. FinderInfo investigation and signature result

The digest-matched original ARM ZIP was extracted to a fresh **`/private/tmp/nes-macos-sign-original-copy`**. **Strict signature verification passed before any metadata removal or signing.** This contrasts with the readiness audit's extraction under Documents, where the app root carried `com.apple.FinderInfo` and strict verification failed. No AppleDouble entries were found in the original ZIP. Its digest remains `206c9b79ee23be65c8e2b695e60adcb5b82952969b4be6d621f468730f60e62a`.

Controlled temporary-copy experiment:

1. The freshly extracted original ARM copy verifies successfully.
2. Attach the same FinderInfo attribute read from the original staging app **to the temporary copy only**: strict verification fails with the reported detritus error.
3. Remove that attribute from the temporary copy: strict verification passes again, **without re-signing**.
4. Repeat attribute injection/removal on the newly generated temporary Intel app: identical failure/pass results, without re-signing it after the experiment.

This isolates the observed failure to added prohibited metadata and demonstrates it is architecture-independent. The original ZIP's seal is valid in clean extraction; the exact process that added FinderInfo in the Documents copy is not identified. Do not attribute that process to the user or claim every extractor reproduces it. [Apple QA1940](https://developer.apple.com/library/archive/qa/qa1940/_index.html) explains this rejection and that browsing bundle contents can add Finder metadata.

**No signature-policy relaxation was needed.** The existing packager already copies into a fresh `/private/tmp` staging app, clears metadata on that disposable copy, signs nested binaries, signs the app, verifies strictly, creates ZIP, extracts it, and verifies strictly again. This mechanism is retained. The new deployment metadata is written before signing. Original user app/ZIP and original toolchains were not cleaned or re-signed.

Both newly generated packages completed that pre-archive/post-extraction verification; their final staging apps also pass **`codesign --verify --deep --strict`**. They are **ad hoc signatures**, not Developer ID/notarized packages or Gatekeeper certification. No credentials were requested, and no notarization or security-check bypass was attempted.

## 6. Actual tests and tool checks

CTest intentionally excludes only `native_opcode_contract` from this deployment-focused repeat; that exhaustive matrix remains registered/unchanged. All other entries run, including A02. Intel executables run through **Rosetta on this ARM host**, not on a physical Intel Mac.

| Run | Executed CTest entries | Passed | Failed | Skipped | Exit |
|---|---:|---:|---:|---:|---:|
| ARM64 | 18 | 17 | 1 (A02) | 0 | 8 |
| Intel through Rosetta | 18 | 17 | 1 (A02) | 0 | 8 |
| Total | **36** | **34** | **2 (A02)** | **0** | — |

Each build still registers all 19 entries; one entry per architecture was not selected, rather than silently skipped/disabled. Selected passes include stack programs, both `$2002` regressions, timed bus side effects, RAM/MMC1/bus/PPU, importer/cache/ABI-v2 modules, diagnostics, APU core, A01/A03/A03 boundaries and dummy SDL audio. **No new failing selected entry appears; A02 is unchanged.** This is not a green complete-suite claim. Headless and the previous four-platform CI were not rerun; no workflow was dispatched.

Additional actual checks:

- Each **packaged** Python/Zig pair compiles, signs and loads the existing synthetic compiler fixture; return value 42, PASS.
- Each packaged Python plus its new frontend `import_probe` passes existing spaced-path/invalid-input, early stdout/stderr and Python launch-failure scenarios; synthetic inputs only, no ROM content generated.
- Separate modules compiled using the unchanged current importer's compiler selection load successfully in matching Python processes and declare **minos 15.0** in both architectures.
- Five deployment-guard probes: old empty cache rejected; old main requiring 27 rejected at floor 15; wrong architecture rejected; Zig requiring 15 rejected at floor 14; SDK 27 with minos 15 accepted.
- Both build/package paths, final app metadata, dependency/load commands, ZIP hashes and strict signatures recorded; Git whitespace check passes.

An initial external Intel probe's **diagnostic** `xcrun vtool` subprocess failed after successful compilation/loading because it inherited Rosetta while this host's `libxcrun` has only ARM slices. Native-host `vtool` inspected the resulting x64 module successfully; a separate Intel load check returned 42, and both existing packaged-tool tests passed. This is a host inspection limitation, not a hidden production/importer fix. Native Intel CLT execution still requires the external platform test.

No compatible commercial ROM, original game session, actual display/audio device test, hardware PAL reference or gameplay completion was used. No new test body or expectation was introduced to hide failures.

## 7. Verification packages and reproduction

Packages are **local test outputs outside Git**, not releases:

| Output | SHA-256 |
|---|---|
| `/private/tmp/nes-macos15-arm64-package/NES-Native-Lab-macOS-arm64.zip` | `6bae6325592116f132ab039a35e765b837342ec2f9ec2c51373a5a7f761a8987` |
| `/private/tmp/nes-macos15-intel-package/NES-Native-Lab-macOS-x86_64.zip` | `c558f6299fd62d930e689e9a2fc71589cb43f1f3b6ad24e659d26dcd8ee1e2a4` |

They were built before adding this final report; a subsequent delivery package should include the final documentation. Nothing was uploaded. Evidence stays outside public Git in surrounding workspace **`outputs/macos-deployment-fix-2026-10-11/`**: starting/original hashes, configure/build logs, JUnit, binary/dependency inventory, guard probes, signature experiments and package/tool results.

Reproduce from the public checkout; use fresh external paths or the task's existing test trees:

```sh
cmake -S . -B /private/tmp/nes-macos15-arm64-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DNES_ENABLE_MODS=ON -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build /private/tmp/nes-macos15-arm64-build --parallel 3
ctest --test-dir /private/tmp/nes-macos15-arm64-build -E '^native_opcode_contract$' --output-on-failure --verbose --no-tests=error
python3 -B tools/public/package_platform.py --build /private/tmp/nes-macos15-arm64-build --output /private/tmp/nes-macos15-arm64-package --toolchain /private/tmp/nes-runtime-mac --system Darwin --arch arm64
codesign --verify --deep --strict "/private/tmp/nes-macos15-arm64-package/NES Native Lab.app"
xcrun vtool -show-build "/private/tmp/nes-macos15-arm64-package/NES Native Lab.app/Contents/MacOS/nes_native"
```

For Intel, substitute `CMAKE_OSX_ARCHITECTURES=x86_64`, the `nes-macos15-intel-*` paths, `--arch x86_64` and `/private/tmp/nes-macos-deployment-intel-tools`. Use the matching pinned toolchain; do not copy the ARM tools into an Intel package. New packages retain `/private/tmp` staging and all strict checks. [CMake's deployment-target documentation](https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_DEPLOYMENT_TARGET.html) explains why the cache must be set before language/toolchain initialization.

## 8. Declared versus demonstrated compatibility / next step

**Demonstrated now:** both architectures compile/link targeting 15.0 with SDK 27; all 13 bundled native binaries satisfy the selected architecture/floor; clean staging/ZIP round-trip strict signatures pass; ARM and Rosetta Intel selected tests and bundled import tools execute on **macOS 27.0.1**. The original ARM ZIP is valid in clean `/private/tmp` extraction, and the FinderInfo mechanism reproduces on both temporary architecture copies.

**Not demonstrated:** running these new artifacts on macOS 15, physical Intel hardware, Finder/Archive Utility/browser-quarantined first opening, successful compatible-ROM generation/gameplay on the floor, every optional framework/Python path, or notarized/Gatekeeper acceptance. Older CI on macOS 15 tested previous source, not this uncommitted package correction. Neither cross-compilation nor a minos label substitutes for those executions.

**Next bounded step:** authorize publication/verification separately, then exercise these changes and matching package tools on the existing native macOS 15 ARM/Intel CI runners, preserving A02's failing gate. On a real macOS 15 system, test clean extraction/first-open, bundled Python/Zig, owned-ROM import/cached relaunch and an ordinary game boot. A02 still blocks later packaging steps in the current workflow; do not bypass it to present the whole workflow as green. Decide its separate reference investigation independently.

Supporting earlier macOS needs an explicitly approved alternative importer toolchain investigation. Signing credentials/notarization, presentation defaults and engine/audio/gameplay work remain outside this task. **Work stops after this deployment/package correction and report.**
