# Estado técnico comprobado

Fecha de auditoría: 2026-10-12. Este documento resume el checkout actual; los informes detallados e históricos enlazados conservan su contexto original.

## Estado actual

- Versión declarada: **0.9.0**, preparación de beta pública. No es una versión 1.0.
- El frontend público compila sin ROM y genera el módulo privado desde una ROM local compatible.
- ABI declarada: **v3**, en `include/recomp/module_abi.h`.
- Plataformas objetivo: macOS arm64/x86_64, Windows x86_64 y Linux x86_64/AppImage. La compilación o inspección de un paquete no prueba ejecución nativa.
- Mods Lua están habilitados por defecto mediante una API restringida; no son un sistema de mods genérico para otros juegos.
- El lanzador 3D es un prototipo separado en `launcher/`, compilable mediante el objetivo `nes_launcher_prototype`. No está integrado con el juego.

## Pruebas y discrepancias

La CI construye sin ROM, ejecuta CTest, audita el contenido público y verifica paquetes por plataforma. Los contratos cubren CPU/generación, IRQ, bus, dispositivos, ABI, diagnóstico, audio, SDL y estado del lanzador. La cobertura no demuestra corrección completa del juego, audio, rutas ni hardware.

**A02 sigue visible y pendiente:** `apu_status_irq_collision_reference` falla en la colisión de IRQ de frame y lectura de `$4015`. No debe ocultarse ni marcarse como pasado.

El smoke ABI v3 y los diagnósticos de gameplay son automatizados; no equivalen a una partida completa. La presentación original 4:3 está activa y el panorámico queda aislado fuera del frontend. Siguen pendientes partidas completas sin ayudas, rutas, saltos, sonido y validación de ROM compatible en Linux/Steam Deck. La nueva interfaz inglesa, el banner temporal y la confirmación de salida requieren los resultados CI del commit de esta beta antes de publicación.

## Documentación de referencia

[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/TECHNICAL_BASELINE.md](docs/TECHNICAL_BASELINE.md), [docs/TESTING_BASELINE.md](docs/TESTING_BASELINE.md), [docs/PLATFORM_VERIFICATION.md](docs/PLATFORM_VERIFICATION.md), [docs/VERIFICATION.md](docs/VERIFICATION.md) y [docs/PUBLISHING.md](docs/PUBLISHING.md). Los informes ABI/AppImage y gameplay son históricos y deben leerse con su SHA y fecha.

## Local Mod Manager 2.0 first vertical delivery — 2026-10-13

Independent feature/mod-manager-v2 worktree from 95c194a, uncommitted/unpublished.
Read-only API v2 player capture and bounded text HUD; see
[MOD_MANAGER_2_PHASE_1.md](docs/MOD_MANAGER_2_PHASE_1.md) and
[MOD_MANAGER_2_ROADMAP.md](docs/MOD_MANAGER_2_ROADMAP.md). macOS ARM64 full suites:
59/60 desktop and 53/54 headless, A02 sole visible failure. Other platforms pending.
Advanced dependencies/profiles/events/gameplay/randomizer are planned, not delivered.

## Local Modding SDK v2 continuation — 2026-10-13

feature/modding-sdk-v2 independent recovery, uncommitted/unpublished. Generic
read-only canonical RAM snapshots/observers and two authored ZIPs, documented in
[MODDING_SDK_V2_PHASE_1.md](docs/MODDING_SDK_V2_PHASE_1.md). Actual macOS ARM64:
62/63 desktop,56/57 headless, A02 sole failure; other platforms/Deck pending.
Writes/hooks/profiles/assets/translation/randomizer remain designs, not APIs.
Original Mod Manager2/Super Gomez worktrees and published beta preserved.

## Modding SDK v2 Phase 2 — local successor (2026-10-13)

`feature/modding-sdk-v2-resources` preserves Phase 1 in a separate worktree. Bounded named PRG text overlays, trusted profiles/codecs and data-only API2 ZIPs implemented; ABI3/emitter/PAL unchanged. Actual macOS ARM64 SDL/OpenGL inventory showed OBJETOS, restored original after Disable/reload, then EQUIPO in the same host. Full desktop 66/67 and headless 59/60; A02 remains the sole ordinary failure, zero skips. Other native platforms and historical Linux GL discrepancy remain pending. No remote changes/publication. See [Phase 2 report](docs/MODDING_SDK_V2_PHASE_2.md). This is one inventory field, not a full Spanish translation.

## Modding SDK v2 Phase 3 — local successor (2026-10-14)

`feature/modding-sdk-v2-phase3` preserves the full dirty Phase 2 worktree. Adds the
verified English-selected `hud.life` resource and a normal two-field data-only ZIP,
with security/native/persistence/Unicode-path contracts. Actual macOS ARM64:
69/70 desktop,62/63 headless, zero skips, A02 sole ordinary FAIL. VIDA and OBJETOS
visibly rendered in the real native game; Disable/reload restores original data.
No ABI/PAL/emitter/hardware changes in Phase 3. Other native platforms remain
NOT RUN; existing CI integration is prepared but publication/dispatch is not
authorized. See [Phase 3 report](docs/MODDING_SDK_V2_PHASE3.md). Not a full translation.

## Modding SDK v2 Phase 4 — local research stop (2026-10-14)

`feature/modding-sdk-v2-phase4` preserves all 84 dirty Phase 3 source files in
an independent worktree. Real CHR-ROM font consumers verified privately; no
safe new tile destination demonstrated. CHR capability/installed accented
translation remains BLOCKED. Own original Ñ fixture plus 14 native and 6 Python
research checks pass per configuration. Fresh macOS ARM64 full suites:
71/72 desktop,64/65 headless, zero skips; A02 sole ordinary FAIL. Production,
ABI3, PAL, private ROM/cache and other worktrees unchanged. No commits/remote
publication. Other native platforms NOT RUN; historical Linux GL issue open.
See [Phase 4 report](docs/MODDING_SDK_V2_PHASE4.md),
[CHR audit](docs/MOD_CHR_RESOURCE_AUDIT.md) and
[variable text study](docs/MOD_VARIABLE_TEXT_RESEARCH.md).

## Modding SDK v2 Phase4B — bounded composition (2026-10-14)

Independent dirty `feature/modding-sdk-v2-phase4b`, all92 Phase4 files recovered.
Typed display-only NES-background text/glyph resources implemented; no globally
free CHR tile claimed. Authored Ñ visibly rendered as NIÑOS in the real private
module and SDL/OpenGL via existing Mods UI. Original CPU/RAM/PCM/PPU/MMC1 traces
match over1800 units, disable and safe-mode verified. Full macOSARM64 suites:
75/76 desktop,67/68 headless, zero skips; A02 sole ordinary FAIL. ABI3/emitter/
PAL/audio/scheduler and private ROM/module unchanged. Others NOT RUN; historical
Linux GL issue open. Local uncommitted/unpublished demo only. See
[Phase4B](docs/MODDING_SDK_V2_PHASE4B.md) and
[decision](docs/MOD_CHR_PHASE4B_DECISION.md) for explicit opacity/priority contract.

## Gameplay & Visual SDK Phase5 — local isolated continuation (2026-10-14)

`feature/gameplay-visual-sdk-phase5` preserves recovered SDK1–4B and selectively
reuses the approved SuperJump adapter. Bounded API2 jump request, independent
text/rectangle UI and validated own PNG foreground layers are implemented. Three
ZIPs install via the existing manager. Native private PAL scenario: original
58px ascent, enhanced94px; visual-only CPU/RAM/raster/PCM traces unchanged.
Full macOS ARM64 suites:79/80 desktop,71/72 headless, A02 sole FAIL, zero skips.
Own-image OpenGL probe16PASS; real private game composition inspected. Super
Speed/other-room jumps, true semantic parallax/2.5D and other native platforms
remain pending. No ABI3/PAL/emitter/hardware/beta changes or remote publication.
See [Phase5 report](docs/GAMEPLAY_VISUAL_SDK_PHASE5.md) for limits and local demos.

## Mod Manager Completion01 — isolated local delivery (2026-10-14)

feature/mod-manager-completion preserves all136 dirty Phase5 recovery hashes.
Main menu/context Back/details, bounded typed API2 configuration and profiles
implemented on the existing store. Three original ZIPs unchanged; additional
configurable jump example uses existing adapter. Actual macOS ARM64 final complete
suites81/82 desktop,73/74 headless, A02 sole FAIL, no skips. SDL/OpenGL30 checks,
private2,400-unit all-mod trace identical to Phase5, copied-app safe startup passed.
Other native platforms/physical Deck NOT RUN; Linux historical GL issue open.
No commit/publication/main/beta/core/ABI/PAL changes. See
[Completion01](docs/MOD_MANAGER_COMPLETION_01.md) and
[user guide](docs/MOD_MANAGER_USER_GUIDE.md). API1/API2 ZIP compatibility retained;
upgraded state.json is not readable by pre-Completion hosts: isolated demo storage.

## Local continuation: Mod Manager Completion02 (2026-10-14)

Independent feature/mod-manager-completion-02 preserves all146 recovered dirty
files from Completion01, based on historical95c194afac4574be9bf8b495341517389a73988b.
No commit or remote publication. Profiles now support delete/export/import,
review confirmations and exact version requirements; configuration supports
restoring defaults and unchanged settings do not restart Lua. Experimental
default store mods-completion02 is separate from the public beta. Full native
macOS ARM64 Release desktop/headless suites retain A02 as realFAIL; platform
counts, package identity and remaining checks are in docs/MOD_MANAGER_COMPLETION_02.md.
Windows, Intel, Linux and Steam Deck certification remains pending execution.

## Completion03 integration (2026-10-14)

feature/mod-manager-beta independently recovers the complete Completion02 tree.
Publication and four-platform CI are authorized for this branch only. See
docs/MOD_MANAGER_COMPLETION_03.md for exact executed results; prior delivery
numbers above remain historical, not certification of the new commit.
No new gameplay mechanics, beta/main changes, tag or release are authorized.


## Mod Manager Completion03 — native integration (2026-10-15)

Published implementation: `feature/mod-manager-beta`,
`10fbb1d8ad829d20f3872f47ced16a727002c126`.
[Native Actions run](https://github.com/inthe92/addams-family-recompiler/actions/runs/38707087575).
All four native builds and full CTest inventories executed:85 entries per platform,
84PASS/1FAIL A02/0omitted. All four diagnostic payloads have matching version0.9.0,
ABI3, clean commit and source fingerprint; downloaded hashes match native reports.
Original four authored example ZIP hashes retained. No release/tag/main changes.

**Not full cross-platform graphical certification:** Windows manager GL FAILS
because glCreateShader is unavailable on the hosted environment; native portable
importer/helper/ABI checks PASS, normal UI remains BLOCKED there. Linux manager GL
57PASS under Mesa/Xvfb, but its separate historical high-refresh pixel probe FAILS.
Both Macs manager GL57PASS; actual packaged main -> Mods -> Back -> main/close
also observed locally on ARM64 without ROM. Real gameplay/physical controller/Deck
acceptance remains NOT RUN in this integration. See
[Completion03](docs/MOD_MANAGER_COMPLETION_03.md) for exact artifacts/hashes,
source recovery, portability fixes and remaining acceptance. Later report-only
commits do not change the embedded SHA of those explicitly identified packages.
