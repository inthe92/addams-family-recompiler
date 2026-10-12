# Estado técnico comprobado

Fecha de auditoría: 2026-10-12. Este documento resume el checkout actual; los informes detallados e históricos enlazados conservan su contexto original.

## Estado actual

- Versión declarada: **0.2.1**, alpha técnica. No es una versión 1.0.
- El frontend público compila sin ROM y genera el módulo privado desde una ROM local compatible.
- ABI declarada: **v3**, en `include/recomp/module_abi.h`.
- Plataformas objetivo: macOS arm64/x86_64, Windows x86_64 y Linux x86_64/AppImage. La compilación o inspección de un paquete no prueba ejecución nativa.
- Mods Lua están habilitados por defecto mediante una API restringida; no son un sistema de mods genérico para otros juegos.
- El lanzador 3D es un prototipo separado en `launcher/`, compilable mediante el objetivo `nes_launcher_prototype`. No está integrado con el juego.

## Pruebas y discrepancias

La CI construye sin ROM, ejecuta CTest, audita el contenido público y verifica paquetes por plataforma. Los contratos cubren CPU/generación, IRQ, bus, dispositivos, ABI, diagnóstico, audio, SDL y estado del lanzador. La cobertura no demuestra corrección completa del juego, audio, rutas ni hardware.

**A02 sigue visible y pendiente:** `apu_status_irq_collision_reference` falla en la colisión de IRQ de frame y lectura de `$4015`. No debe ocultarse ni marcarse como pasado.

El smoke ABI v3 y los diagnósticos de gameplay son automatizados; no equivalen a una partida completa. Siguen pendientes, entre otros, pruebas interactivas completas, rutas/muertes, sonido, activación de enemigos en widescreen, habitaciones verticales/fijas y ejecución nativa completa en cada sistema.

## Documentación de referencia

[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/TECHNICAL_BASELINE.md](docs/TECHNICAL_BASELINE.md), [docs/TESTING_BASELINE.md](docs/TESTING_BASELINE.md), [docs/PLATFORM_VERIFICATION.md](docs/PLATFORM_VERIFICATION.md), [docs/VERIFICATION.md](docs/VERIFICATION.md) y [docs/PUBLISHING.md](docs/PUBLISHING.md). Los informes ABI/AppImage y gameplay son históricos y deben leerse con su SHA y fecha.
