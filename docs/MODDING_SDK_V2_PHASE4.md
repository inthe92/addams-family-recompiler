# Modding SDK v2 Phase 4 — CHR research and safety stop

2026-10-14. **Research delivery completed; CHR mod capability BLOCKED.**
Font source/consumers are verified, but no safe new tile destination has been
demonstrated. The task's explicit stopping condition applies. No unsafe graphics
patch or fake accented translation was enabled.

## Recovery and preservation

Branch `feature/modding-sdk-v2-phase4`, historical HEAD
`95c194afac4574be9bf8b495341517389a73988b`, plus the complete dirty SDK1–3 base.
A separate native Git worktree was created beside `modding-sdk-v2-phase3`.
The full tracked binary diff and Git-listed untracked source/tests/docs were
recovered; **84 file SHA-256 values matched before Phase 4 edits**. HEAD alone
contains v1, not the SDK implementation. Original worktrees were not overwritten.

| Inspected branch / worktree | HEAD | Disposition |
|---|---|---|
| `feature/modding-sdk-v2-phase3` / `modding-sdk-v2-phase3` | 95c194a… | Full dirty SDK1–3 recovered |
| `feature/modding-sdk-v2-resources` / `modding-sdk-v2-resources` | 95c194a… | Dirty Phase 2 preserved |
| `feature/modding-sdk-v2` / `modding-sdk-v2` | 95c194a… | Dirty Phase 1 preserved |
| `feature/mod-manager-v2` / `mod-manager-v2` | 95c194a… | Dirty API2/HUD preserved |
| `feature/mod-manager-v1` / `mod-manager-v1` | 95c194a… | Baseline and unrelated local report preserved |
| `feature/super-gomez-v1` / `super-gomez-v1` | 95c194a… | Separate gameplay work preserved, not imported |
| `diagnostic/high-refresh-phase-a` / original checkout | 80af76aa5e4408f37dcb0e7047355c21be186ae8 | Unrelated work preserved |

No staging, commits, pushes, merges, tags, releases or remote workflows. Published
beta unchanged. No independent Super Gómez/PAL/NTSC work merged. Inherited
high-refresh code left unchanged.

## Implemented research and blocked features

* Authored original Ñ, 16-byte NES 2bpp tile; independent 8×8 pixel matrix and
  README under the repository MIT license. Offline nearest-scaled preview viewed.
* Native public **counterexample** harness: 14 checks covering encoding,
  dimensions, MMC1 4/8 KiB mapping/bounds, own-pixel raster change, equal clocks,
  CPU PPUDATA/sprite-zero divergence and fresh original fallback.
* Six Python test methods covering own encoding, atomic undeclared-CHR-file
  rejection, unimplemented permissions, untrusted targets, all requested Spanish
  glyphs still rejected, and unknown glyph/raw-address declarations rejected.
* Two mandatory CTest entries and inventory/JUnit count checks; no exemptions.
* Temporary private observation of real native font fetches/CPU CHR reads. No
  production or generated commercial C edit; no private data in public fixtures.

**Not implemented:** CHR API/permission, approved new glyph destination/codec,
atomic text+glyph publication/unload, variable-length streams or real accented
translation. No installable Phase 4 ZIP. Existing PRG translation and v1/v2
contracts remain supported. No ABI3/cache change.

## Findings and decision

See [CHR audit](MOD_CHR_RESOURCE_AUDIT.md) for source functions and evidence.
`hud.life` consumes font bank 8, `inventory.heading` bank 14. Each verified letter
cell is 8×8; this is not a claim about every text. Per-line bank differences
prevent relying only on a final mapper snapshot. Font indices are shared; blank
bytes or an unobserved index are not a demonstrated reservation.

The 1,800-unit owner-module run observed **2,436 CPU CHR reads**, in banks 0/22/24,
none in the observed font banks. An independent **authored synthetic** setup
demonstrates that a CHR substitution can change buffered `$2007` data and sprite
zero without changing clocks. It does not prove failure for every possible
game font tile. Missing: full destination/consumer coverage, including screens,
languages, sprites, CPU reads, transitions and reload.

Conditional preferred design: immutable physical-bank glyph leases from a
trusted profile, bounded own bytes, fingerprints and exclusive ownership;
text/codec/CHR activation and restoration must be atomic with a coherent VRAM
reload. If no reservation is provable, investigate a scoped PPU-background
reader with an explicit CPU/sprite-zero/opacity contract separately. Neither
design is implemented or announced as an existing API. No OpenGL text overlay.
See [variable text research](MOD_VARIABLE_TEXT_RESEARCH.md) for viable fixed
fields versus experimental streams and rejected pointer/instruction patching.

## Builds and real test results

Fresh Release desktop/headless builds: native Apple Clang, macOS 27.0.1 ARM64,
deployment target 15.0, Unix Makefiles, existing vendored dependencies. Both full
default builds exit 0. New research subset executed before complete suites.

| Configuration | Executed CTest | PASS | FAIL | Omitted | Exit |
|---|---:|---:|---:|---:|---:|
| Desktop, mods ON | 72 | 71 | 1: A02 | 0 | 8 |
| Headless, mods ON | 65 | 64 | 1: A02 | 0 | 8 |
| Research subset, each configuration | 2 | 2 | 0 | 0 | 0 |

Each new subset contains **14 native checks + 6 Python methods**; loops/subtests
are not additional CTest entries. Previous 70/63 inventories increased by two;
none removed. Mandatory inventories/JUnit sets match; gate exits **1 solely for
A02**, retaining a failed suite. Full elapsed time: desktop 27.80 s, headless
24.73 s. No new executed-suite regression.

Both configurations retain PASS for **4,736 native instruction cases and all
82 IRQ contracts**, PAL07/PAL08, A01/A03, PPUSTATUS, RAM/MMC1/bus, DMA/NMI,
ABI3/old-module rejection, importer/cache, Lua/SDK1/HUD, PRG resource
ownership/persistence and existing timing/presentation determinism. Desktop
also executes the existing SDL/OpenGL contracts. Automated results are not a
complete playthrough or physical display certification.

A02 remains `apu_status_irq_collision_reference`:
`$4015 cycle=33252 status=64 IRQ after read=0; expected=1`. Source/expectation
unchanged. Historical Linux/Mesa/Xvfb high-refresh graphics discrepancy remains
open and was not run here. Mac results do not resolve it.

Initial standalone fixture development caught a harness error: CPU write
timestamps were equal. Test setup now advances one cycle between writes; no
production change or weakened expectation.

## Private execution and visual limits

Owner PAL SHA-256 matches the existing profile; module SHA-256
`2fd2a47471d582afa78bcd0624f76c31d137eed55546cda7b7edc644c6917a47` matches its
ABI3 receipt. No regeneration. Temporary observation code adds no NES bus reads.
All **1,800 instrumented trace rows are byte-identical** to the previous Phase 3
original trace: input, registers, clocks, RAM/PCM hashes, NMI/IRQ and DMA.
This validates instrumentation on that trajectory, not full game/hardware fidelity.

No new accented glyph was enabled in the owner game. No new CHR GUI install or
disable/reload demonstration. Prior VIDA/OBJETOS real visual results remain
historical. Own Ñ offline preview/native synthetic raster are not an installed
game translation. No private screenshots/raster/ROM/module distributed.

## Measurements

Current ordinary `chr_read`, 2,000,000 reads of owned synthetic banks, Release,
checksum consumed. Initial isolated headless: **2.62608 ns/read**; isolated
desktop: **1.75973 ns/read**. During full suites: headless 1.91162, desktop
1.15658 ns/read. Individual runs vary with cache/load; not a statistical baseline,
FPS, input latency or full PPU/CPU/frontend cost measurement.

| Metric | Evidence |
|---|---|
| Backend, zero CHR resources | Measured above; no new lookup added |
| One / multiple active CHR resources | NOT RUN — not implemented |
| Raw own glyph memory | 16 bytes; allocator/JSON/vector overhead excluded |
| Activation/deactivation/publication/unload | BLOCKED — no CHR transaction API |
| Fresh original source fallback | Native equality assertion, not timed mod unload |
| Real-glyph PPU/CPU/frontend overhead | NOT RUN |

No per-pattern-fetch production parsing/allocation/Lua or extra presentation
callbacks. Do not infer future overlay cost from the zero-resource benchmark.

## Platform/package matrix

| Platform | Build/tests | New real glyph mod | Status |
|---|---|---|---|
| macOS ARM64 | Fresh desktop/headless executed | BLOCKED | Research PASS; full CTest FAIL solely A02 |
| macOS Intel | NOT RUN | NOT RUN | Requires native verification |
| Windows x64 | NOT RUN | NOT RUN | Public tests prepared; existing MinGW link convention retained |
| Linux x64 / Steam Deck | NOT RUN | NOT RUN | Historical GL issue open |

Research tests use public fixtures, standard Python and existing CMake/runtime;
no dependency or workflow modification. They register in mods-enabled builds.
Future authorized CI must publish a commit containing the **full dirty SDK1–4**,
not dispatch HEAD95 alone. Never upload the private temporary evidence folder.

There is no installable Phase 4 ZIP: the host still rejects CHR declarations,
and producing a pretend mod would misrepresent acceptance. The own fixture is
reproducible research data only. Phase 3 ZIPs/apps were not changed or republished.

## Changed files relative to dirty Phase 3

* `CMakeLists.txt`: native target and two mandatory research entries.
* `tests/chr_resource_research_checks.cpp`,
  `tests/chr_resource_package_research_checks.py`: new contracts.
* `tests/verify_ctest_suite.py`, `tests/verify_ctest_suite_checks.py`: inventory
  and output verification; independent gate fixture updated.
* `examples/resources/chr-research/own-enye-8x8.chr`, `own-enye-8x8.json`,
  `README.txt`: authored research fixture.
* `docs/MOD_CHR_RESOURCE_AUDIT.md`, `docs/MOD_VARIABLE_TEXT_RESEARCH.md`, this
  report and append-only Phase 4 entry in `PROJECT_STATUS.md`.

**No production source differs from dirty Phase 3.** Git status also includes
inherited production changes; they are not Phase 4 changes. Preserve the full
successor when continuing, not only these twelve files.

Final preservation check: all **84 original Phase 3 hashes and its full Git
status match** the saved start state. Original owner ROM, module and receipt
hashes also match; the 1,800-row trace equality was checked again. Explicit
Git-listed public source export audit with owner-ROM byte markers: **PASS,
291 files**; no forbidden recovery/cache/module/ROM files or matched sampled
commercial blocks. These bounded content/allowlist checks are technical checks,
not legal clearance or proof against every encoded derivative/secret. The export
contains only public source/fixtures/docs and dependencies, not private probes.
`git diff --check` passes and nothing is staged.

Own Ñ fixture SHA-256:
`c16c2f1b5dc7ca092ee963d206190d925dc164e524d578e67e6fdd51800bef99`.

## Reproduction

From this worktree; public tests require no commercial input:

```sh
research_build=$(mktemp -d /private/tmp/nes-chr-research.XXXXXX)
cmake -S . -B "$research_build/desktop" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON -DNES_ENABLE_DESKTOP=ON
cmake --build "$research_build/desktop" --parallel 4
cmake -S . -B "$research_build/headless" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build "$research_build/headless" --parallel 4
ctest --test-dir "$research_build/desktop" -R '^chr_resource_.*research_contract$' --verbose --output-on-failure
ctest --test-dir "$research_build/headless" -R '^chr_resource_.*research_contract$' --verbose --output-on-failure
ctest --test-dir "$research_build/desktop" --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$research_build/desktop-junit.xml"
ctest --test-dir "$research_build/headless" --output-on-failure --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit "$research_build/headless-junit.xml"
ctest --test-dir "$research_build/desktop" --show-only=json-v1 > "$research_build/desktop-inventory.json"
ctest --test-dir "$research_build/headless" --show-only=json-v1 > "$research_build/headless-inventory.json"
python3 -B tests/verify_ctest_suite.py --inventory "$research_build/desktop-inventory.json" --junit "$research_build/desktop-junit.xml"
python3 -B tests/verify_ctest_suite.py --headless --inventory "$research_build/headless-inventory.json" --junit "$research_build/headless-junit.xml"
```

Full CTest exits 8 and each gate exits 1 for A02. Use the native platform's temp
path/toolchain conventions outside macOS. These procedures are not other-platform
execution evidence.

Current local evidence: `/private/tmp/nes-sdk-phase4.rlpy7_0b` (temporary, may
disappear after reboot). Build logs, full `desktop/headless-ctest.log`, JUnit and
inventory describe public tests. `phase3-preservation.json`, status and patch
record recovery. Private usage/bank/field CSVs, raw `*.frame`, instrumented trace,
probe source/executables remain outside Git and must not be published. Baseline
trace is in the earlier Phase 3 evidence; its absence elsewhere is not permission
to obtain another ROM.

## Acceptance and next step

PASS: preservation, actual font route, own glyph, synthetic safety research,
existing regression contracts, no hidden A02 and no production/ABI/ROM/cache
changes. BLOCKED: approved new destination, installed real glyph, atomic rollback,
active-glyph/no-mod equivalence, implemented CHR conflicts/quotas and new native
package checks. Existing PRG/no-mod tests do not certify an absent CHR API.

Next bounded task: prove one candidate destination's background/sprite/CPU
consumers across relevant screens/languages, banks/mirroring and reload. If no
global reservation can be established, review scoped PPU-reader semantics first.
Only after that implement the atomic backend, normal ZIP and real raster/restore
demonstration. Variable-length relocation remains a separate later investigation.
