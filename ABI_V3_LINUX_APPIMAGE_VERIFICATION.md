# ABI v3 Linux AppImage verification

Estado verificado en la rama `verify/phase-3-1-bus-timing`.

- SHA de la ejecución: `020540a89de00149e5a591a9c790ea74c965e490`
- GitHub Actions: https://github.com/inthe92/addams-family-recompiler/actions/runs/38174294730
- Resultado de la matriz: `failure` únicamente por el fallo conocido A02 de CTest. Los pasos de empaquetado/verificación de Windows, macOS arm64, macOS x86_64 y Linux terminaron correctamente.
- No se creó release, tag ni pull request.

## Cambios

Se añadió `tests/linux_appimage_checks.py` y se incorporó al job Linux inmediatamente después de generar el AppImage. El informe JSON se conserva aunque CTest falle. El workflow no usa `continue-on-error`, `WILL_FAIL`, `DISABLED` ni exclusiones; A02 sigue siendo visible y mantiene roja la conclusión global.

## Linux x86_64

AppImage: `NES-Native-Lab-Linux-x86_64.AppImage`

SHA-256: `5eb12444a4d853ff6ed1cde0a042c4fa119e4564d43e15833fcdf99edaf5f14a`

Resultado del verificador: **PASS**.

- Integridad y arquitectura: PASS; ELF 64-bit x86-64, 109,369,848 bytes.
- Extracción: PASS mediante `--appimage-extract`. Esta evidencia es de extracción, no se presenta como ejecución directa.
- Estructura interna: PASS; 21.612 miembros y recursos ABI públicos presentes.
- Dependencias dinámicas: PASS; `readelf` y `ldd` no encontraron dependencias sin resolver.
- Python/Zig incluidos: PASS; Python 3.11.17 y Zig 0.17.0.
- Módulo C sintético: PASS; compilación, carga con `ctypes`, retorno esperado y ABI v3.
- Orden de servicios ABI v3: PASS; el smoke test valida avance, poll IRQ y boundary en orden.
- Contenido: PASS; sin ROMs, módulos comerciales ni cachés privados; licencias y `THIRD_PARTY.md` presentes.
- Ejecución directa: PASS; el AppImage se ejecutó desde una ruta con espacios y recorrió la ruta real de importación, rechazando una entrada sintética inválida con el diagnóstico esperado (código 1).

## CTest, IRQ y A02

El artefacto Linux contiene 31 casos JUnit, con 30 aprobados y 1 fallo. Los grupos IRQ suman **82/82**:

- contratos IRQ: 36/36
- controles de borde IRQ: 46/46

A02 permanece visible y sin modificar: `status_irq_collision` falla porque la IRQ de frame en el mismo ciclo no sobrevive al borrado de estado. No se ocultó ni se excluyó.

## Otras plataformas

En la misma ejecución, Windows x64, macOS 15 arm64 y macOS 15 x86_64 completaron sus pasos de empaquetado y verificación de paquete. Sus jobs siguen apareciendo como fallidos globalmente por el mismo A02 de CTest; no se introdujeron cambios en sus verificadores.

## Limitaciones y recomendación

La ejecución directa demostrada es la ruta sin ROM privada: se prueba el binario empaquetado y su diagnóstico de entrada inválida, no una partida ni una importación comercial. La ejecución real de una ROM compatible sigue fuera del alcance de esta exportación pública.

Por tanto, **no recomiendo preparar todavía una alpha pública ABI v3** bajo la condición de aceptación indicada: Linux ya cumple el AppImage y 82/82 IRQ, pero la matriz no está globalmente verde porque A02 continúa pendiente. Puede prepararse una alpha experimental interna o explícitamente marcada como bloqueada por A02, sin presentarla como verificación final completa.
