# High refresh rate — Phase A: instrumentación y contrato

Trabajo iniciado el 2026-10-12; informe finalizado el 2026-10-13. Estado: **fase A implementada y verificada localmente en macOS ARM64**. No se implementa la fase B, renderizado independiente ni interpolación.

## Estado inicial y conservación

Base: `bafd0cf5ef2007daa24c133c3e45dd5d55a8d985`, inicialmente en `beta/0.9.0-english-ui`. Se creó la rama **local** `diagnostic/high-refresh-phase-a`, conforme a AGENTS.md. HEAD permanece igual; los cambios de esta fase no están confirmados ni publicados. Los nueve archivos preexistentes pendientes conservan sus SHA-256, incluido `HIGH_REFRESH_RATE_AUDIT.md`. Se leyeron AGENTS.md, PROJECT_STATUS.md y la auditoría completa.

No cambian CPU, PPU, APU, bus, mapper, emisor, perfil de ROM, servicios ni ABI v3. No se generan nuevos paquetes, se alteran releases o se usa el remoto. La beta pública v0.9.0 permanece intacta. Se conserva el raster original 256×240 presentado a 4:3, el GOD-off por defecto y A02 como fallo ordinario visible.

## Instrumentación

Nueva opción: **`--timing-diagnostic`**. Es independiente de `--diagnostic` de gameplay y no habilita GOD, objetos ni mods.

El recolector `nes::frontend::FrontendTiming` no conoce Bus, CPUState, RuntimeServices ni SDL. Recibe una fuente de ticks de host; en producción es `SDL_GetPerformanceCounter`, con frecuencia de `SDL_GetPerformanceFrequency`. No toma decisiones de ejecución, no contiene un scheduler ni callbacks de juego. Su reloj no es el reloj maestro NES.

Está desactivado por defecto. Desactivado no lee el reloj, no acumula datos y no escribe salida; los scopes se reducen a comprobaciones constantes de estado. No se afirma coste de ejecución exactamente cero. Activado conserva únicamente contadores y agregado de intervalos en arrays de tamaño fijo: sin logs, asignaciones de memoria o escritura de archivos por fotograma. Al terminar emite una línea inglesa `Timing diagnostic: {JSON}` en stdout. También puede emitirla al salir por excepción/dirección no cubierta, si la sesión ya empezó. No incluye rutas ROM, datos de memoria, imágenes ni PCM.

El diagnóstico comienza inmediatamente antes del bucle principal, tras inicialización/importación. No cronometra la importación ni la creación de ventana/contexto/audio. La salida final utiliza locale clásica para JSON; los fallos de formateo/IO no se convierten en excepciones del juego. Los intervalos se cierran también al salir por excepción; una operación fallida puede tener tiempo parcial sin contador de finalización. Un reloj que retrocede incrementa `invalid_clock_samples`, sin underflow ni replanificación.

### Contadores

| Campo JSON (`counts`) | Definición |
|---|---|
| `nes_frames_executed` | Unidades NES terminadas al salir del bucle original de slices y consumir el frame pendiente. No aumenta al redibujar. Incluye la primera unidad/frame parcial de arranque. |
| `ppu_frames_produced` | Suma de los incrementos observados de `video.frames()` durante esas unidades; se mantiene separado del contador de unidades. |
| `native_slice_calls` | Llamadas al dispatcher C nativo, incluso la última llamada fallida. No es un contador de instrucciones 6502; el presupuesto 1.000 se conserva. |
| `mod_hook_calls`, `god_hook_calls` | Invocaciones reales de `mods.frame` (cuando se compila Lua) y `god.begin_frame`. Pueden ser no-ops; no significan que se activó una ayuda. |
| `diagnostic_frame_calls`, `diagnostic_slice_calls` | Invocaciones del observador, no eventos escritos. El primero incluye llamadas adicionales por acciones explícitas G/H cuando ya están habilitadas por el modo de depuración. |
| `audio_batches`, `pcm_samples_collected` | Llamadas de entrega por unidad NES y muestras extraídas de APU, sin asumir un tamaño fijo por frame. Un lote vacío/mute sigue siendo una llamada. No equivalen a muestras audibles. |
| `wav_batches` | Append completados del WAV opcional; su finalización de cabecera al salir no se cuenta como lote. |
| `render_passes`, `paused_render_passes` | Pares de llamadas a presentación NES + overlay completados. Se cuentan también durante menú/foco perdido. Un drawable sin área útil puede no producir comandos GL. |
| `nes_draw_commands`, `ui_draw_commands` | Invocaciones reales de `glDrawArrays` en GLPresenter, separadas por capa. El total OpenGL es su suma, no `render_passes`. No certifican ejecución GPU ni pixels mostrados. |
| `buffer_swaps_completed` | Llamadas a `SDL_GL_SwapWindow` que retornaron. No mide scanout/compositor ni refresco físico. |
| `captures_completed` | Capturas finales solicitadas por CLI que completaron el readback/escritura. |
| `paused_iterations` | Vueltas por la rama de pausa; aumentan dibujos/swaps/espera pero no hooks, lógica o extracción PCM. |

`legacy_frame_units` conserva el contador anteriormente llamado `presented`, ahora `nes_frames`. Sigue incrementándose **después** de la presentación activa, en el mismo punto anterior. En una salida normal por `--frames`, coincide con las unidades NES terminadas. Si falla el audio/dibujo después de ejecutar una unidad, puede ser menor que `nes_frames_executed`: el diagnóstico no inventa una presentación exitosa.

### Intervalos y unidades

Para cada entrada de `stages` se guarda `intervals`, `ticks`, `seconds`, `min_ticks` y `max_ticks`. La media se obtiene como `seconds / intervals` si el denominador es distinto de cero. `ticks / clock_frequency` convierte a segundos. No hay percentiles ni trazas de cada frame.

| Etapa | Región observada |
|---|---|
| `simulation` | Muestreo de botones, hooks, ejecución nativa, diagnóstico y consumo del frame pendiente. Incluye sus costes de host, no es tiempo CPU del sistema ni duración emulada. |
| `audio` | `take_samples`, `SDLAudioOutput::submit` y append WAV opcional, incluidos los bloqueos existentes de la cola. No cambia esos bloqueos ni el dispositivo. |
| `render` | Conversión/subida/dibujo de raster y overlay, también en pausa. Tiempo de host/driver; no utiliza GPU queries ni `glFinish` en producción. |
| `swap` | Duración de la llamada de intercambio de buffers. |
| `wait` | Deadline/espera PAL original; en pausa incluye reanclaje y `SDL_Delay(10)`. `--unthrottled` sigue omitiendo únicamente la espera activa PAL. |
| `capture` | Readback/escritura de la captura final opcional, separado de render/swap. |

`wall_seconds` incluye eventos, pausa, costes no delimitados y finalización de sesión antes de formatear el resumen. Las etapas no cubren todo el trabajo y no deben sumarse para afirmar tiempo GPU, latencia visible o FPS garantizados. Activar medición añade overhead del host y puede influir en lateness/colas bajo carga; no altera ciclos/instrucciones NES para compensarlo. La invariancia se comprueba para iguales entradas por paso, no como igualdad nanosegundo a nanosegundo de acciones humanas.

## Contrato conservado

El orden del bucle permanece:

```text
eventos → estado de pausa/audio → [rama de pausa, si procede]
botones → mods.frame → god.begin_frame
→ slices C y diagnostic.slice hasta nuevo frame PPU
→ diagnostic.frame → consume_frame
→ take_samples → audio.submit → append WAV opcional
→ inicio del banner si primer frame → presentación/overlay
→ incremento legacy → captura final opcional → swap → espera PAL si no fast
```

Los observadores se insertan alrededor de esas llamadas sin moverlas. El renderer solo recibe un puntero opcional al recolector, no acceso nuevo a la simulación. Se mantiene `SDL_GL_SetSwapInterval(0)` sin cambiar políticas de vsync. No se extrae ni sustituye la unidad de ejecución actual.

- **`--frames N`:** misma condición de finalización y contador activo; cero continúa significando sin límite. Las vueltas de pausa nunca consumen el límite.
- **`--unthrottled`:** mismo avance NES; omite la misma espera PAL activa. La espera de pausa de 10 ms y la política de audio anterior permanecen.
- **Capturas:** mismo trigger en el último contador activo, después de raster/overlay y antes del swap. Se conserva formato/viewport/readback. La UI sigue usando tiempo real; no se promete igualdad de su fade si cambian instantes de captura.
- **Pausa/foco:** mismos controles, audio pausado y botones neutralizados; sin slices, hooks, `consume_frame` ni `take_samples`. Se conserva Cancel, reanudación con liberación de teclas y reanclaje de deadline.
- **PAL:** `next_pal_deadline` y 50,007 permanecen byte a byte; no nuevos deadlines gráficos, decisiones por FPS, hilos o pasos artificiales de bus.

## Pruebas incorporadas

Cuatro entradas CTest obligatorias nuevas, disponibles también en headless:

| Entrada | Casos | Alcance |
|---|---:|---|
| `frontend_timing_metrics` | 8 | Desactivación sin reloj/IO, duraciones exactas, stop único, agregados/min/max, contadores independientes, clock inválido/retroceso, scope parcial y fallo IO. |
| `frontend_timing_clock_contract` | 6 | Modelo **solo de tests** con reloj virtual: 10 minutos de pasos PAL y presentaciones 60/90/120, reanclaje largo, límite/captura/pausa en fast y normal. No es un planificador instalado en la app. |
| `frontend_timing_source_contract` | 5 | Posición/orden real de llamadas en app, rama de pausa sin ejecución, CLI/captura/fast, activación opt-in y parseo JSON emitido por el ejecutable C++ con reloj falso. |
| `frontend_timing_native_contract` | 8 | Fixture propio 6502 → emisor actual → C compilado → runtime actual. Comparación por frame y trazas completas de servicios con diagnóstico off/on y 0/1/3/7 presentaciones sintéticas intercaladas. |

Total: **27 casos nuevos por configuración**. El diferencial nativo ejecuta una referencia y ocho comparaciones de 12 unidades cada una (108 unidades NES por configuración), con lecturas PPU/controlador, audio Pulse, RMW RAM, NMI y DMA reales del runtime. Comprueba registros CPU, RAM/pila, raster/fondo/OAM, getters de PPU/mapper/APU, PCM a nivel de bits y orden/ciclos de read/write/advance/poll/boundary. Usa un hook Lua propio observable cuando Lua está habilitado, además de GOD/diagnóstico existentes. No hay ROM comercial, intérprete ni segundo motor. Los draws sintéticos son conversión/presentación de datos; no se presentan como swaps físicos a esos Hz.

Se añade además un caso al probe gráfico existente: **13 casos GL** en total. Cuenta comandos de NES/UI y cuatro swaps reales de prueba, dejando sin cambios los contadores NES/audio/hooks y el estado observado del bus. No se registra en CTest portable porque requiere escritorio OpenGL real.

El inventario/JUnit exige las cuatro entradas y sus recuentos. A02 no se filtra, omite ni marca WILL_FAIL. Las expectativas existentes permanecen intactas. Los checks de fuente son una protección de integración, no sustituyen las ejecuciones nativas o gráficas.

Incidencia de desarrollo: el primer modelo virtual terminaba en la última oportunidad gráfica de cada frecuencia, a distintos instantes; falló su expectativa PAL. Se corrigió el **límite de observación del arnés** a los mismos 600 s, manteniendo el esperado de 30.005 unidades (primera inmediata incluida) y 36.000/54.000/72.000 oportunidades gráficas. No se cambió producción ni una expectativa hardware para resolverlo.

## Resultados ejecutados

macOS ARM64, macOS 27.0.1, Apple Clang 21, SDL 2.32.8, OpenGL 4.1 Metal. Dos builds Release nuevos, fuera del repo, con dependencias vendorizadas y sin instalaciones globales.

| Verificación | Ejecutadas | Aprobadas | Fallidas | Omitidas |
|---|---:|---:|---:|---:|
| CTest escritorio completo | 47 | 46 | 1 (A02) | 0 |
| CTest headless completo | 42 | 41 | 1 (A02) | 0 |
| Casos nuevos en cada configuración | 27 | 27 | 0 | 0 |
| Probe SDL/OpenGL real | 13 | 13 | 0 | 0 |

Ambas compilaciones completas terminaron correctamente. CTest retorna 8 y el gate JUnit 1 **por A02**, sin errores nuevos. Se verificaron los 4.736 casos nativos, 82 IRQ, PAL 07/08, A01/A03, $2002, RAM/MMC1/bus/PPU, DMA/NMI, importador/caché/ABI y adaptadores existentes. Los warnings de compilación observados son de APIs GameController deprecadas en SDL vendorizado, sin modificar esa dependencia.

Con la ROM privada local ya configurada y autenticada por el perfil, se reutilizó exclusivamente su caché privada compatible ABI v3. Tres sesiones de **180 unidades**: (1) fast/mute sin diagnóstico; (2) fast/mute con diagnóstico; (3) PAL/audio con diagnóstico. Las tres salieron con código 0, resumen legacy de 180 y captura PPM 1920×1440. PCM WAV y captura resultaron idénticos entre las tres. La primera no emitió resumen de timing, las otras emitieron uno. No hubo ayudas activadas. Es startup/UI hasta selector original, **no una partida completa ni evaluación de fidelidad audible**. Todos los recursos/capturas/logs permanecen privados fuera del repo.

En las dos sesiones instrumentadas hubo 180 frames NES/PPU, 180 hooks de cada tipo por frame, 180 lotes audio/WAV, 180 pasadas NES/overlay, 180 comandos GL de cada capa, 180 swaps y una captura. Se recogieron 172.557 muestras PCM. En fast hubo cero esperas PAL; en normal, 180. Sin pausas en esas tres sesiones.

Mediciones disponibles de esas sesiones cortas, en milisegundos (total / máximo intervalo):

| Etapa | Fast + mute | PAL + dispositivo audio |
|---|---:|---:|
| Simulación | 227,038 / 1,815 | 650,412 / 6,286 |
| Audio/append WAV | 1,897 / 0,063 | 12,690 / 0,181 |
| Render de host | 33,125 / 2,413 | 115,370 / 2,336 |
| Swap | 64,038 / 9,848 | 126,341 / 9,661 |
| Espera | 0 / 0 | 2.655,238 / 17,709 |
| Captura final | 10,883 / 10,883 | 26,690 / 26,690 |

Tiempo de pared de sesión: 0,666370 s fast y 3,932136 s normal. Es un único conjunto corto con cargas/configuraciones distintas, no un benchmark comparativo de overhead ni garantía de alcanzar una tasa. La suma excluye parte del procesamiento de eventos y otros costes. Un máximo de swap >8,33 ms en esta muestra exige investigar/mejorar pacing antes de afirmar 120 FPS; no prueba imposibilidad ni identifica por sí solo la causa.

Una cuarta sesión privada comprobó Escape → Cancel → Escape → Exit con métricas activadas. CPU/PPU se mantuvieron iguales a ambos lados de la primera pausa y el audio cambió de pausado a reanudado. Terminó limpiamente con 1.457 unidades NES, 1.457 lotes audio/hooks y **3.560 vueltas de pausa**: 5.017 pasadas/swaps, sin gastar unidades NES por los redibujos. No se midió latencia audible/scanout ni se afirma una partida real completa.

## Reproducción

Compilación y suite (no requiere ROM):

```sh
cmake -S . -B build-phase-a -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-phase-a --parallel 3
ctest --test-dir build-phase-a -R '^frontend_timing_' --verbose --output-on-failure
ctest --test-dir build-phase-a --verbose --output-on-failure --output-junit build-phase-a/results.xml
ctest --test-dir build-phase-a --show-only=json-v1 > build-phase-a/inventory.json
python3 -B tests/verify_ctest_suite.py --inventory build-phase-a/inventory.json --junit build-phase-a/results.xml
# CTest completo/gate deben seguir exponiendo A02.
cmake -S . -B build-phase-a-headless -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build build-phase-a-headless --parallel 3
ctest --test-dir build-phase-a-headless --verbose --output-on-failure
# En un escritorio gráfico real, no en CI sin pantalla:
./build-phase-a/presentation_gl_checks
```

En macOS el frontend está en `build-phase-a/nes_native.app/Contents/MacOS/nes_native`; en Linux `build-phase-a/nes_native`, y en Windows el EXE según el generador. Para la ROM propia configurada:

```sh
<frontend-local> --rom <ROM-propia-compatible> --import-tools <tools/recompiler> \
  --import-cache <cache-privada> --no-god --frames 180 --timing-diagnostic \
  > <archivo-local-fuera-del-repo>
# Añadir --unthrottled --mute únicamente para diagnosticar ese modo.
# --capture y --audio-output siguen siendo opcionales y privados.
```

Buscar la línea `Timing diagnostic: ` y parsear el sufijo como JSON. Registrar también `--build-info` del mismo binario: los builds de esta fase son **dirty/locales**, no los paquetes beta publicados. El stdout completo puede contener rutas de importación; compartir solo el resumen de métricas saneado, sin ROM, módulo, WAV o captura del juego. Los logs de build/CTest y las sesiones privadas de esta ejecución están separados en almacenamiento temporal local.

## Archivos modificados o creados

- `app/desktop.cpp`: flag opt-in, métricas alrededor de llamadas existentes y nombre explícito del contador legacy.
- `runtime/include/runtime/frontend_timing.hpp`, `runtime/src/frontend_timing.cpp`: recolector acotado, reloj inyectable, scopes y resumen JSON.
- `runtime/include/runtime/gl_presenter.hpp`, `runtime/src/gl_presenter.cpp`: enlace opcional del observador y conteo en los puntos reales de `glDrawArrays`.
- `CMakeLists.txt`: biblioteca y cuatro tests, sin dependencias nuevas; fixture C generado únicamente en build.
- `tests/frontend_timing_checks.cpp`: métricas y contrato de reloj virtual.
- `tests/emit_frontend_timing_fixture.py`, `tests/frontend_timing_native_checks.cpp`: fixture propio y diferencial de C nativo/runtime.
- `tests/frontend_timing_source_checks.py`: contrato de colocación real de llamadas y JSON.
- `tests/presentation_gl_checks.cpp`: una comprobación gráfica adicional.
- `tests/verify_ctest_suite.py`, `tests/verify_ctest_suite_checks.py`: entradas/contajes obligatorios nuevos.
- `docs/HIGH_REFRESH_RATE_PHASE_A.md`: este informe. Los informes preexistentes no se modifican.

## Límites y criterios para fase B

Esta fase prepara métricas/contratos, **no desacopla todavía el bucle activo**: sigue ejecutando un frame NES y presentándolo una vez antes de la misma espera. No introduce snapshots, interpolación, opciones FPS, threads, cambios de audio, vsync o calendario PAL.

La comparación nativa cubre campos públicos, pixels/PCM y continuaciones/trazas a lo largo de 12 frames, no una serialización completa de todos los estados privados ni certificación PAL subciclo. Las comprobaciones de fuente protegen la integración, pero no reemplazan un futuro replay del scheduler real. El modelo de reloj se queda en tests: sus cifras no son rendimiento del frontend. No hay pruebas físicas de 60/90/120 ni resultados actuales Windows/Linux/macOS Intel; CMake y tests quedan preparados, sin disparar CI.

Para comenzar **una fase B experimental y autorizada**:

1. Conservar estos contadores y la unidad/orden de ejecución, con igual input por paso; `--frames`, captura y pausa deben seguir pasando los contratos.
2. Implementar después la repetición del último raster/snapshot inmutable con deadlines separados, sin cambiar ABI v3 ni C. No usar el número de draws como autorización de hooks/audio/CPU.
3. Ampliar el modelo a jitter, retrasos/suspensión, 59,94/119,88 y cambios de monitor; comparar la implementación real contra ese contrato, no solo contra counters alimentados por el arnés.
4. Repetir diferencial, suites completas y pruebas de GUI. Exigir cero fallos nuevos/omisiones; A02 continúa visible e independiente.
5. Medir sesiones representativas y colas/bloqueos por plataforma; diferenciar tiempo de host, swap y scanout. Solo con esa evidencia evaluar el objetivo físico de 120 FPS o una futura necesidad de separar hilos.

No hay bloqueo arquitectónico identificado para continuar con repetición de frames en fase B; faltan implementación y validación de ese scheduler. No se avanza automáticamente.
