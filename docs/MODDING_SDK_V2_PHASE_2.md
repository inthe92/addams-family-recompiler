# Modding SDK v2 — Phase 2: bounded text resources

2026-10-13. Local experimental successor `feature/modding-sdk-v2-resources`, HEAD **95c194afac4574be9bf8b495341517389a73988b** plus recovered Phase 1 and uncommitted Phase 2 changes. No commit/push/merge/tag/release or remote CI dispatch. Public beta unchanged.

## Recovery and base

Read AGENTS, PROJECT_STATUS and the six Phase 1 SDK reports before extending. Inspected the original checkout and Mod Manager v1, Super Gómez, recovered Mod Manager v2 and SDK v2 worktrees. Reused the working Phase 1 successor. Created an isolated successor at the common v1 HEAD, applied its tracked diff and copied its Git-listed untracked files. Verified **53 source/document files** by SHA-256 against the parent snapshot; the original Phase 1 worktree remains byte-for-byte unchanged. Super Gómez and separate PAL/NTSC work were not merged or edited.

Repeated the complete existing Phase 1 desktop suite before changes: **63 executed, 62 passed, 1 failed (A02), zero skipped**. Actual code captures canonical 2 KiB RAM by side-effect-free `MainRAM::read`, not `Bus::read8`; API2 observers run on completed NES units, after original simulation/hooks/audio, not extra presentations. Baseline callback budgets, cleanup, permissions, v1 and safe-mode contracts passed. No platform beyond macOS ARM64 was available for native execution.

## Architecture implemented

Four distinct layers: original owner-provided PRG/CHR in the private module; reviewed host resource profile/codec metadata; authored mod translation JSON; generic cartridge data backend. The public runtime contains no Spanish strings or per-language native patches.

One vertical slice: **fixed-length PRG glyph fields**. Typed owned overlays are published outside instructions by the frontend after manager transactions. `Cartridge::cpu_read` resolves the MMC1 bank and uses approved replacement bytes, or the original data. The returned byte enters the normal Bus open-bus/timing path. No additional reads/writes/cycles, no Lua decoding per instruction, no instruction/opcode/immediate replacement. Original PRG and cached module remain immutable. RuntimeServices/generated C/ABI **v3 unchanged**; no cache invalidation or regeneration.

Catalog/profile validation is strict and requires the matching known-instruction map, verified reader and conservative exclusion of instruction/operand intervals. The initial native source guard uses CRC32, not a claimed cryptographic guarantee; full identity remains the existing ROM/module importer SHA contract. See `MOD_RESOURCE_PROFILES.md` and `MOD_RESOURCE_SECURITY.md` for trust/coverage limits.

API2 supports data-only packages (`entry: null`, `resources.text`) without a Lua VM. Installation is disabled and never executes scripts. Enable rechecks target, profile, glyphs/length and exclusive ownership. Same-resource conflicts fail before state mutation; at most 16 active fields. Disable/uninstall removes owned lookup data at the next boundary. Already-uploaded VRAM is not retrospectively rewritten: reload the relevant screen.

## Verified game resource and private experiment

Owner-authorized compatible PAL ROM SHA was reverified. Only **`inventory.heading`**, a nine-glyph field, is supported. Private inspection confirmed an indirect banked data reader feeding `$2007` inside an inventory nametable stream, not executable bytes. Native capture observed 1,024 uploads with equal read counts. Active CHR/font output visually verified A–Z and space; all 256 active pattern-table tiles were inspected privately without finding an approved discrete Spanish-symbol mapping. No approved Spanish-specific accent/punctuation glyph mapping is claimed.

Two data-only ZIPs use exactly the same API/profile and unchanged host:

| Package | Authored replacement | SHA-256 |
|---|---|---|
| Addams-Spanish-Inventory-v1.0.zip | OBJETOS | 7ffa7698cd4586a8b45ceb602489e9570df785677825bbb522540ffa41c3502d |
| Addams-Spanish-Equipment-v1.0.zip | EQUIPO | ba342bced31404247f91447ba790c3b38fa84000d02078d8a0be50ba60be5be2 |

The private integration harness uses the existing ABI3 module, the current native runtime, actual Manager ZIP installation/enable and the real native PPU raster. Four fresh deterministic runs, **1,800 completed units each**: original, OBJETOS, disabled/restored, EQUIPO. All per-unit cycles, NMI/IRQ/OAM+DMC DMA counts and PCM-bit hashes match exactly. The OBJETOS raster changes **255 pixels**, only within x=88–159, y=14–20 (the heading field). Disabled fresh raster equals the original byte-for-byte. Both rendered Spanish rasters were visually inspected. These are native menu smoke tests, not a complete playthrough or translation of the entire game.

Private ROM/module bytes, captures, font sheet, generated code and detailed traces remain outside Git. The original ROM SHA and private module SHA were unchanged. The private native comparison harness has SHA-256 `4fbf2db04cbd9d3f8038503a760d8122a8607acf3e741332601dc8bc01b11729`; the single GUI test app has SHA-256 `0921534d16922f1482476f67a8025c089035147e4fe8857bb51ea7bc07710e61`. Each was unchanged across its two-package comparison. A subsequent picker-title-only rebuild does not invalidate those comparisons. The module was not rebuilt to load either package.

## Tests and platforms

| Execution | Executed | Passed | Failed | Skipped |
|---|---:|---:|---:|---:|
| Phase 1 baseline desktop, macOS ARM64 | 63 | 62 | 1 A02 | 0 |
| Phase 2 desktop, macOS ARM64 | 67 | 66 | 1 A02 | 0 |
| Phase 2 headless, macOS ARM64 | 60 | 59 | 1 A02 | 0 |

Four new mandatory CTest entries:
- `mod_resource_profile_contract`: **33 Python tests** (profiles, instructions/operands, Unicode/overflow, control exclusion, ZIP paths/duplicates, revision/permissions, disabled installation, conflict atomicity, lifecycle, malformed installed data, v1 compatibility).
- `mod_resource_native_contract`: **21 checks**, own emitted-C fixture and native Bus/MMC1/PPU/audio/input/IRQ/NMI/DMA. Atomic rejection, bank/range isolation, open bus, original PRG ownership, raster substitution/restoration, unaffected deterministic traces.
- `mod_resource_store_contract`: repeats 21 checks + **7 native Manager store/transport/lifecycle checks** (28). Two independently authored synthetic packages, same binary, no commercial data.
- `mod_resource_frontend_contract` (desktop): **9 real frontend event/backend checks** with SDL dummy video. Installation, Enable, safe publication, Back, repeated-unit lifetime, Disable, second package, --no-mods. This is not a visual GL certification.

Existing 4,736 native instruction executions, all 82 IRQ cases, PAL 07/08, PPU/$2002, A01/A03, memory/MMC1/bus, DMA, importer/cache/ABI, Phase 1 observers and native 50/60/90/120 deterministic tests remain passing. A02 is ordinary FAIL, untouched. Full inventory/JUnit gate confirms every required entry and zero skips, then intentionally returns nonzero for A02. The first integration run exposed a missing new-output fixture in the verifier's synthetic test; corrected the harness fixture. Synthetic PPU fixture initially set mask only during power-on gating; it now writes reviewed registers during its own vblank loop. No production expectations were weakened.

Packaged helper/resources copy tested from a separate directory, including spaces: **33 transactions** across Hello, HUD, two RAM SDK examples and two translation ZIPs passed. This uses local Python and a deliberately empty test toolchain; it does not certify a complete portable application or other operating systems. Packaging now includes the catalog, codec helper, instruction map and both authored ZIPs. ZIP integrity/member/source-byte checks passed.

Actual SDL/OpenGL application test on macOS ARM64: F2 Install ZIP via native file chooser → installed Disabled → Enable → original language selector/intro/gameplay → Enter opens inventory → visibly **OBJETOS**. Closed inventory, F2 Disable, reloaded inventory → visibly original **INVENTORY**. Installed/activated the second ZIP in that same running app → visibly **EQUIPO**. Normal exit confirmed and session marker removed. No framebuffer application overlay was used to display the translated heading. GUI test was muted; audible quality was not assessed (PCM equality is automated above).

Initial GUI preparation stalled opening a file under Documentos. A brief private process sample showed Python blocked in file open and the app waiting for its child. The exact OS/File Provider cause is not established. Using verified copies of the owner's ROM/cache and public tools entirely under `/private/tmp` completed import verification and normal gameplay, with original module/ROM hashes unchanged. No importer/runtime fix or OS permission change was made. The local demonstration launcher stages public tools outside Documentos for this reason. It references the existing private test cache without distributing it.

macOS Intel, Windows x64 and Linux x64: portable test targets prepared, **not executed**; remote CI not authorized/launched. Historical Linux experimental high-refresh OpenGL discrepancy remains pending, independently of this headless data backend; no Linux graphical result is claimed.

## Performance (one local Release microbenchmark)

Native mapper/data lookup, 2,000,000 reads over a 4 KiB pattern, mostly misses:

| Active fields | ns/read | Owned glyph payload |
|---:|---:|---:|
| 0 | 2.14 | 0 bytes |
| 1 | 2.62 | 3 bytes (synthetic) |
| 16 | 8.41 | 48 bytes (synthetic) |

Publication+clear of 16 fields: **435 ns/pair**, 10,000 pairs, native validation only. Trusted Python store: install **5.96 ms**; ten enable operations mean **3.19 ms** (2.91–3.71); disable mean **0.64 ms** (0.50–1.04). These exclude process launch/GUI time and are not game-FPS or cross-platform benchmarks. Real initial translation owns 9 bytes. Global maximum glyph payload is 1,024 bytes; vectors/strings/descriptors/parsed JSON have additional bounded overhead. No per-read allocation or Lua execution.

## Files added/changed by Phase 2 (beyond preserved Phase 1)

- Runtime: `text_resources.hpp`; `cartridge.hpp/.cpp`; `mod_api.hpp` Target identity; `mod_manager.hpp/.cpp`; `mod_frontend.hpp/.cpp`; picker title in `mod_picker_mac.mm`.
- Integration: `app/desktop.cpp`, `game/include/game/mod_observation_v2.hpp` target identity, `CMakeLists.txt`.
- Tools: `tools/mods/resources.py`, `resource_profiles.json`, `packages.py`, `make_translation_examples.py`; `tools/public/package_platform.py`.
- Tests: `mod_resource_checks.py`, `emit_resource_fixture.py`, `mod_resource_native_checks.cpp`, `mod_resource_store_checks.py`, `mod_resource_frontend_checks.cpp`, distribution checks and inventory-gate fixtures.
- Examples: the two authored data-only translation directories.
- Documentation: this report and the four resource/API/profile/translation/security guides.

No CPU, PPU, APU, emitter C, mapper selection/write rules, ABI headers or PAL planner changes. The only native NES backend addition is a bounded PRG data-return layer. No commits or staging.

## Reproduce

```sh
cmake -S . -B /tmp/nes-sdk2-desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build /tmp/nes-sdk2-desktop --parallel 6
ctest --test-dir /tmp/nes-sdk2-desktop --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit /tmp/nes-sdk2-desktop/results.xml
cmake -S . -B /tmp/nes-sdk2-headless -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build /tmp/nes-sdk2-headless --parallel 6
ctest --test-dir /tmp/nes-sdk2-headless --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536
python3 -B tools/mods/make_translation_examples.py /tmp/translation-examples
python3 -B tools/public/audit_public.py .
```

CTest remains nonzero for A02. Enlarged output bounds retain complete native measurement results in JUnit (default CTest truncation otherwise hides the success footer; this is not a simulation failure). No workflow changes required: existing CI already sets those bounds. `MOD_TRANSLATION_GUIDE.md` describes F2 and screen reload. Never add private reproduction files/captures to Git or public CI.

## Limitations / next step

Small safe vertical slice, not arbitrary patching, full-game Spanish, glyph replacement or App Store/deployment work. Control streams, pointers, variable-length layouts and embedded immediates need separate verified profiles/backends. Accents need a future CHR/font capability. Next: validate these unchanged public synthetic targets on the other three native platforms when branch/CI publication is authorized, then identify a second independent original text field. Keep no-translation traces and visible restoration as acceptance gates before widening the profile.

## Local deliverables

Two content-free ZIPs and a locally built ARM64 host are in the sibling output directory `Modding-SDK-v2-Phase2-Demo` (outside this Git worktree). `Launch-Private-Translation-Demo.command` starts an isolated staged copy and references the owner's existing private test cache, not a ROM embedded in the application. Read its local README. This is a developer demonstration, not a bundled-toolchain public distribution. Temporary private evidence may be lost on reboot; regenerate from the owner's supplied ROM if needed. Neither ROM nor private module is present in that deliverable folder.

Delivered ARM64 developer-app copy was also launched from a separate temporary directory with included tools, `--no-mods --no-god --mute --frames 60 --unthrottled`: exit 0, OpenGL 4.1 Metal, 60 presented units, GOD=0. This is a boot smoke, not a playthrough. Public source audit with owner-ROM markers and deliverable-app byte scan passed; no ROM/private module/cache was included.

The newly created local demo bundle acquired FinderInfo at its root while stored under Documentos. Removed that attribute **only from this session-created demo copy**, then ad-hoc signed and verified with `codesign --verify --deep --strict` (exit 0). No original user ZIP, existing app or published package was changed. This is not notarization or cross-platform package certification.

Subsequent inspection found FinderInfo reappearing at the Documentos bundle root; a later direct strict verification of that source copy failed with the same metadata diagnostic. Its exact OS source is not proven. The final independent copy made with `ditto --norsrc --noextattr` under `/private/tmp` passes **the same deep/strict verification, exit 0**, without resigning it or changing any file bytes. The local launcher uses this tested copy procedure and refuses to launch if strict verification fails. The Documentos copy alone is therefore not described as a persistently verified distribution package.
