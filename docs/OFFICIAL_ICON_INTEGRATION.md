# Official NES Native Lab icons — integration and verification

Date: 2026-10-12. Branch: `verify/phase-3-1-bus-timing`. Initial and final HEAD: `1029ddadf2671f5359afd3a1b28635ded651a48c`. **Local changes only: no commit, push, workflow dispatch, tag or release.**

## Artwork and preserved work

The user supplied the official Icons v2 set: white moon, pixel stars, blue castle and black background. The supplied 8 PNGs, ICO and ICNS were copied **byte-for-byte**, without drawing, resizing or downloading images. `assets/icons/SHA256SUMS.json` records the original hashes. All copies were compared again against the supplied originals at the end.

Initial changes were the modified `docs/ABI_V3_PACKAGING_CI_FIX.md` and four untracked reports (final platform status, four-platform CI, NESRecomp comparison and release readiness). Their original hashes were saved privately; all five remain unchanged. The branch and HEAD were retained.

The old `assets/nes-native-lab.svg` remains in the source tree; packaging no longer selects it. No existing user assets or ZIPs were deleted or overwritten. New build/package/evidence folders are temporary and outside the repository.

## Implementation

| Target | Official format | Integration |
|---|---|---|
| Windows | `nes-native-lab.ico`, 7 images: 16/24/32/48/64/128/256 | Primary EXE resource ID 1, compiled by CMake RC. Explorer and user-created shortcuts inherit the executable icon. ICO also included in package resources. |
| macOS | `nes-native-lab.icns`, images up to 1024 | CMake copies ICNS into `Contents/Resources` and declares `CFBundleIconFile`. Packager preserves that declaration before signing. Native Cocoa uses the complete ICNS for the application/Dock. |
| Linux | PNG, 8 sizes: 16/32/48/64/128/256/512/1024 | AppDir root 256px PNG, `.DirIcon`, hicolor registrations, matching root and `usr/share/applications` desktop entries; linuxdeploy selects the official PNG. |
| SDL windows | Official 64px PNG pixels | Lossless build-time RGBA embedding; game window, import progress and Linux drop window call the shared helper. No runtime working-directory lookup. |

Python's standard library validates/decodes the fixed RGBA8 PNG at build time. No SDL_image, image decoder or new library is linked into the runtime. Python was already used by the build/import/packaging infrastructure; desktop builds now explicitly locate that interpreter for the branding header. Headless still needs no SDL.

Cocoa receives the full ICNS instead of a 64px SDL bitmap, because SDL2's Cocoa backend would replace the Dock image with the smaller bitmap. A plain developer executable without a bundle falls back to the embedded SDL icon. macOS normally shows the application icon in Finder/Dock, not a Windows-style titlebar icon.

Only icon presentation calls were added to frontend/import windows. The ROM parsing, validation, compilation, cache, gameplay, renderer, emulation, ABI and A02 expectations are unchanged.

## Modified / added files

- `assets/icons/`: the ten original files, hash manifest and `windows_icon.rc.in`.
- `CMakeLists.txt`: lossless window resource generation, SDL branding library, Cocoa source, ICNS bundle property, Windows RC resource and rebuild dependency, two CTest entries.
- `runtime/include/runtime/application_icon.hpp`, `runtime/src/application_icon.cpp`, `runtime/src/application_icon_mac.mm`: branding helper only.
- `app/desktop.cpp`, `runtime/src/user_rom.cpp`: apply the icon to existing windows; no import-flow changes.
- `tools/public/icon_assets.py`: PNG CRC/filter checks, ICO/ICNS bounds, official hash checks, x64 PE icon-resource extraction, bundle/AppDir verification, header emission.
- `tools/public/package_platform.py`, `tools/public/package_linux.sh`: package the official formats and validate registrations. Linux is rechecked after linuxdeploy and before appimagetool.
- `tests/icon_asset_checks.py`, `tests/window_icon_checks.cpp`, `tests/window_icon_checks.py`: deterministic resource/negative checks and real compiled SDL surface.
- `tests/macos_package_checks.py`, `tests/windows_package_checks.py`: check the icon again in extracted packages; Windows compares each embedded PE image payload with the original ICO.
- `tests/verify_ctest_suite.py`: require the two branding entries in desktop and the resource entry in headless, without changing A02 handling.
- `tests/public_toolchain_smoke.py`: one extra `-B` on its child Python process, as explained below. No ABI expectation changes.
- This report.

## Checks actually executed

Host: **macOS 27.0.1 ARM64**. No native Windows/Linux runner was launched in this task, and no old CI result is relabelled as a verification of these new icons.

| Check | Result |
|---|---|
| Complete new Release desktop configure/build | PASS, all targets |
| Complete new Release headless configure/build | PASS, all targets |
| Desktop full CTest | **33 executed, 32 passed, 1 failed (A02), 0 omitted**, exit 8 |
| Headless full CTest | **30 executed, 29 passed, 1 failed (A02), 0 omitted**, exit 8 |
| New resource unit checks | **7/7 methods**, including hand-calculated vectors for all five PNG filters, corrupt/truncated files, synthetic x64 PE resources, bundle declarations and Linux packager fixtures |
| Compiled SDL surface / assignment | PASS: exact official RGBA bytes including alpha, hidden SDL window and null-window behavior. CTest uses the dummy video driver; this is not an OS screenshot test. |
| IRQ regression cases | **82/82 in each configuration**, same original expectations |
| CPU/device/PPU/APU/import/cache/ABI suite | All pass apart from original A02; 4,736 native instruction executions per configuration remain covered |
| Existing Windows package format self-tests | **11/11** on this Mac; not native Windows execution |
| Existing lipo/deployment self-tests | **6/6**, tiny ARM64/Intel probes; not an Intel frontend build |
| CI gate self-tests | **6/6** |
| Real RC and x64 PE resource roundtrip | PASS: existing bundled Zig compiled a synthetic Windows PE; all seven embedded payloads match the ICO. **Cross-compilation only**, no Windows execution. |
| Native Cocoa icon observation | PASS: synthetic SDL window using the production helper, actual `cocoa` driver, Dock image maximum resolution **1024**. Rendered Dock image inspected against the official design. |
| Native Finder icon observation | PASS: AppKit/NSWorkspace resolved the icon of the newly packaged real `.app`; its rendered preview shows the official design. |
| `git diff --check`, shell syntax | PASS |

The compiler normalizes the ICO's unspecified plane field from 0 to 1. The PE verifier accepts either representation while still requiring correct dimensions, bit depth, lengths and **byte-identical image payloads**. A real compiler demonstrated this normalization; negative tests reject plane 2 and changed image bytes. No icon design was converted or substituted.

The native Cocoa observation ran in a separate synthetic app using the same compiled helper. Its first sandboxed launch lacked WindowServer access; the authorized native observation outside that sandbox exited 0. No commercial ROM was used, and no real game session was started. The game-window call site is compiled, but a visual inspection during gameplay is still pending.

## New macOS ARM64 package

A new package was generated using the newly compiled frontend and already available verified importer tools. No original archive or cached published package was modified.

- Name: `NES-Native-Lab-macOS-arm64.zip`.
- Size: 135783549 bytes.
- SHA-256: `67e97cf4f54d144a81093cf051402e9df092c780cc3e15410fbb7d7f88ea57c2`.
- `CFBundleIconFile`, ICNS format and byte identity: **PASS** in the extracted package.
- CRC, extraction outside the build, all 13 Mach-O architecture/deployment inspections: **PASS**.
- Public package audit: 21,620 files; no ROM or generated game module detected (no private-ROM byte scan).
- Final rebuilt frontend code/data sections match the packaged frontend; its raw bytes differ because packaging applies a separate ad hoc signature.
- Minimum declared macOS: **15.0**; actual execution here: **27.0.1 ARM64**. This does not prove a new macOS 15/Intel execution.
- Strict ad hoc signature before extraction, after extraction and after exercising packaged tools: **PASS** in the final check; not notarized.
- Bundled Python/Zig, synthetic C ABI v3 compile/load/poll execution and three importer-process scenarios: **PASS** in the final check.

### Diagnostic attempts retained

The first package-check attempt could not write Zig's default cache outside the sandbox. Subsequent checks used private writable cache folders; the package itself was not changed to work around that permission.

The next attempt passed signing/extraction and tool execution, but failed the final signature verification. Hash comparisons traced this to the **existing smoke test's child Python `-c` process**, which omitted `-B` and created/updated `.pyc` files inside the sealed temporary bundle. Adding only `-B` to that test subprocess prevented the mutation. The final unweakened `codesign --verify --deep --strict` passed after all tools ran. All failed logs and reports were retained; no signing check was disabled and no ROM/import production code was modified.

The icons and the original new ZIP remained unchanged throughout these checks. The smoke test correction affects the test process only, not the shipped ROM import behavior.

## Remaining native checks

- **Windows x64:** build the actual frontend with the existing MinGW resource compilation, run full CTest and the extracted-package verifier; inspect Explorer, a new shortcut, taskbar and SDL game/progress windows. A synthetic cross-built PE is not the frontend or a native Windows validation.
- **Linux x64:** build/run the actual frontend and package script with linuxdeploy/appimagetool; inspect the AppImage, installed desktop launcher and SDL window. Here only the AppDir layout/PNG/desktop contract was executed with synthetic data. No AppImage was generated or executed on Linux.
- **macOS Intel / macOS 15:** build/sign/extract and run the updated package on those hosts. No new Intel application or macOS 15 session was executed here.
- **Gameplay window:** test with the user's private compatible ROM on each platform. No ROM is necessary for the icon unit checks; this task did not generate a game module.
- Native system dialogs and icon-cache refresh policies belong to the OS. Test newly built packages/paths and newly created shortcuts rather than relying on an old cached icon. No automatic desktop shortcut installation was added.

A02 stays visible and unrelated: cycle 33,252, `$4015` status `$40`, immediate IRQ-source expectation unresolved. No exemption, filter or expected-failure classification was added.

## Reproduction

```sh
cmake -S . -B <new-desktop-build> -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build <new-desktop-build> --parallel 4
ctest --test-dir <new-desktop-build> --output-on-failure
# Full CTest remains nonzero because A02 is unchanged.
cmake -S . -B <new-headless-build> -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build <new-headless-build> --parallel 4
ctest --test-dir <new-headless-build> --output-on-failure
python3 -B tests/icon_asset_checks.py
python3 -B tools/public/icon_assets.py
python3 -B tools/public/icon_assets.py --appdir <generated-AppDir>
# Existing macOS/Windows package verifiers now require the official icons.
git diff --check
```

Replace angle-bracket placeholders with quoted paths. For restricted local tool execution, set `ZIG_GLOBAL_CACHE_DIR` / `ZIG_LOCAL_CACHE_DIR` to writable private directories. Build directories, native observation code/images, source snapshots, test logs, ZIP and verification JSON stay outside the public repository.

**Outcome:** official resources integrated across all three packaging paths; native macOS ARM64 presentation/package verified; native Windows, Linux, Intel and macOS 15 checks remain explicitly pending. Existing work and supplied designs preserved. No remote changes.
