# High refresh rate — Phase C: eficiencia y validación multiplataforma

Fecha: **2026-10-13**. Estado: **trabajo local terminado; validación externa pendiente**. No se certifican cuatro plataformas ni refresco físico. No hay interpolación, nuevos hilos, commit, push, ejecución remota, paquete nuevo o modificación de la beta publicada.

## 1. Base y conservación

Rama `diagnostic/high-refresh-phase-a`, HEAD `bafd0cf5ef2007daa24c133c3e45dd5d55a8d985`. A/B y otros informes estaban pendientes de commit. Se leyeron íntegramente AGENTS.md, PROJECT_STATUS.md y HIGH_REFRESH_RATE_AUDIT/PHASE_A/PHASE_B. Se guardó una copia temporal de los archivos iniciales para revisar **el incremento de C**, sin descartar trabajo.

Los informes históricos y el trabajo ajeno se conservan byte a byte. CPU, PPU, APU, bus, mapper, `next_pal_deadline`, emisor C, perfil ROM y ABI v3 mantienen sus hashes. Las funciones del planificador de B permanecen idénticas; su archivo recibe solo el identificador de publicación de snapshots. Versión declarada 0.9.0, presentación 256×240/4:3/nearest, GOD-off y modo original predeterminado permanecen. Ningún módulo privado necesita regenerarse por este cambio.

Compilación local: macOS ARM64, macOS 27.0.1, Apple Clang 21, SDL 2.32.8, OpenGL 4.1 Metal. Se recompilaron los targets desktop/headless de los builds existentes; no se presentan como dos compilaciones limpias adicionales.

Identidad final del frontend local: ABI 3, HEAD anterior, `dirty=true`, source_sha256 `6575e2ed2462d2feb8fe8c21e83ffa4505e705d08c3ee6a397406b6ab7287587`. Es el árbol de desarrollo, **no el binario beta publicado**. El nuevo recolector exige que la identidad del build coincida con las fuentes actuales. En CI exige además el SHA del job y árbol limpio.

## 2. Medición previa y decisión sobre la caché

Primero se añadieron métricas de conversión y subida, sin caché, y se ejecutó el probe OpenGL con imágenes propias. En dos segundos por tasa:

| Objetivo | Dibujos/swaps | Nuevas imágenes propias | Conversiones | Subidas | Conversión (ms) | Llamada de subida, host (ms) |
|---|---:|---:|---:|---:|---:|---:|
| 60 | 120 | 100 | 120 | 120 | 27,702 | 22,924 |
| 90 | 180 | 101 | 180 | 180 | 41,243 | 33,702 |
| 120 | 240 | 100 | 240 | 240 | 56,508 | 45,061 |

A 120 se procesaban 140 rasters repetidos innecesariamente. Existe un coste medido, además de la duplicación contada; se justifica una caché acotada. Estos updates son callbacks del scheduler con pixels propios, **no ejecuciones de CPU NES**.

## 3. Caché implementada y límites

Nuevo overload C++ `GLPresenter::present(const FrameSnapshot&)`, utilizado exclusivamente en la presentación/pausa experimental. Retiene la textura NES y evita `original_rgba`/`glTexSubImage2D` cuando se repite la misma publicación. Sigue limpiando/dibujando el quad, respetando viewport/nearest y dibujando la UI en su textura separada.

Cada `FrameSnapshot::complete` obtiene un identificador de publicación de 64 bits, único durante el proceso, mediante un contador estándar `std::atomic` relaxed. No se crea ningún hilo. Este ID es información **del frontend**, sin relación con ciclos o IRQ. El número NES/PPU, ciclo CPU y tick host existentes siguen siendo etiquetas de observación.

La clave no depende de la dirección del objeto ni solo del número NES: un reinicio de secuencia, otro origen o una republicación con iguales metadatos reciben otro ID. Una copia inmutable puede conservar el ID; modificarla mediante `complete` crea una publicación nueva. No se copian pixels adicionales para comprobar un hit ni se serializa este identificador en módulos ABI.

Reglas:

- Nueva publicación: convertir/subir una vez cuando hay un drawable útil, después dibujar.
- Mismo snapshot: redibujar sin reconversión ni subida.
- `present(VideoState)` o buffers crudos: ruta original, siempre convertir/subir e invalidar la clave retenida.
- Snapshot inválido anterior al arranque: raster inicial por la ruta original.
- Resize/F11 con el mismo contexto: actualizar viewport, conservar textura.
- Nuevo GLPresenter: nueva textura y clave vacía, primer snapshot requiere subida.
- Overlay: textura propia; no invalida ni sustituye la textura NES.

No se modifica gestión de pérdida real del contexto GPU. Si un contexto se destruye, debe reconstruirse el presenter como antes; el test de recreación usa un nuevo presenter sobre el contexto disponible, no una simulación de reset del driver. No se cachea el bitmap UI ni se cambia vsync, shaders, calendario PAL, audio, entradas o hooks.

## 4. Métricas

Se reutiliza `--timing-diagnostic`, off por defecto. El JSON schema 1 incorpora campos aditivos:

- `nes_pixel_conversions`: conversiones NES completadas.
- `nes_texture_uploads`: llamadas de subida de la textura NES.
- `cached_nes_draws`: dibujos NES completados reutilizando esa textura.
- Etapas `pixel_conversion` y `texture_upload`: tiempo monotónico del host en esas operaciones.

Se conservan todas las métricas A/B: unidades NES/PPU, slices, hooks, PCM/lotes, draws NES/UI, swaps y tiempos de simulación/audio/render/swap/espera/captura. Conversión/subida están **incluidas en render**: no sumar esos subtotales de nuevo. No son GPU queries; una llamada de driver puede diferir del trabajo GPU posterior. No cuentan subidas del overlay.

El frontend/probe registran también el refresco informado por SDL, resultado de pedir swap interval 0 e intervalo consultado. La solicitud sigue siendo la misma. En este Mac: SDL informó 120 Hz, petición exitosa e intervalo 0. Eso **no mide scanout, ProMotion/VRR efectivo, tearing ni latencia visible**.

### Probe gráfico final con caché

| Objetivo | Dibujos/swaps | Imágenes propias | Conversiones/subidas | Dibujos/s del host | Render host (ms) | Swap (ms) | Espera (ms) |
|---|---:|---:|---:|---:|---:|---:|---:|
| 60 | 120 | 100 | 100 | 59,994 | 18,626 | 408,229 | 1.557,504 |
| 90 | 180 | 100 | 100 | 89,972 | 50,962 | 237,985 | 1.687,754 |
| 120 | 240 | 100 | 100 | 119,957 | 31,352 | 89,034 | 1.863,286 |

Cada intervalo fue de aproximadamente 2 s. A 120, conversión 6,275 ms y llamada de subida 8,102 ms en la ejecución final. Las duraciones variaron entre ejecuciones/carga del host; los conteos 240 dibujos/100 subidas son la evidencia estable del trabajo evitado. No se deduce una mejora global de FPS a partir de comparar duraciones de sesiones distintas.

Microbenchmark emparejado final, misma ventana/contenido y cinco lotes de 200 dibujos por ruta, tras calentamiento:

| Ruta | Mediana por lote de 200 (ms) |
|---|---:|
| Cruda, siempre convertir/subir | 60,0265 |
| Snapshot retenido | 5,6593 |

Es repetición de una imagen estática, **sin swaps**, con `glFinish` solo en el arnés al cerrar lotes para incluir el trabajo pendiente. No se añade `glFinish` a producción. No representa el rendimiento completo del juego ni una multiplicación de sus FPS. Una ejecución emparejada anterior en el mismo tamaño dio 40,352/4,489 ms; hay variabilidad real. Se conservan ambos registros.

## 5. Pruebas y determinismo

| Verificación local | Ejecutadas | Aprobadas | Fallidas | Omitidas |
|---|---:|---:|---:|---:|
| CTest desktop | 51 | 50 | 1: A02 | 0 |
| CTest headless | 46 | 45 | 1: A02 | 0 |
| Contratos de temporización/presentación/evidencias | 63 casos por configuración | 63 | 0 | 0 |
| Probe OpenGL real | 23 casos | 23 | 0 | 0 |

C aporta 11 casos portables: 3 de identidad de publicación, 1 de métricas y 7 del recolector/CI. También 7 casos GL de caché/invalidation/readback, conservando los 16 anteriores. Las comparaciones nativas de B mantienen sus expectativas.

Los 4.736 casos nativos, 82 IRQ, PAL 07/08, A01/A03, $2002, RAM/MMC1/bus/PPU, DMA/NMI, ABI/importador/caché y adaptadores pasan. A02 continúa como prueba ordinaria fallida. CTest retorna 8; el gate JUnit retorna 1 **solo por A02**. Sin WILL_FAIL, skips, filtros de CI ni continue-on-error.

El diferencial genera/ejecuta C propio y compara por unidad registros, RAM/pila, raster, OAM, PCM y trazas de read/write/advance/poll/boundary. Las nueve combinaciones 60/90/120 × normal/suspensión/pausa siguen iguales a la referencia. Cambiar presentaciones no introduce instrucciones, hooks ni audio adicional.

Ahora ese arnés usa reloj host `steady_clock` real para medir sus etapas, mientras el calendario de ejecución de prueba sigue siendo virtual. Emite nueve observaciones vinculadas a tasa/perturbación. No se les atribuyen swaps OpenGL ni FPS físicos; sus counters de dibujo/captura son operaciones sintéticas del arnés.

Ejemplo sin perturbación del C sintético, medido en la suite desktop:

| Objetivo virtual | Unidades NES | Lotes PCM | Muestras PCM | Hooks por frame, cada tipo | Simulación host (ms) | Extracción audio (ms) |
|---|---:|---:|---:|---:|---:|---:|
| 60 | 12 | 12 | 11.301 | 12 | 14,569 | 0,000250 |
| 90 | 12 | 12 | 11.301 | 12 | 14,400 | 0,000125 |
| 120 | 12 | 12 | 11.301 | 12 | 14,363 | 0,000125 |

La etapa de simulación incluye las trazas del arnés; la de audio aquí solo extrae muestras, sin dispositivo SDL. No extrapolar esos tiempos a una ROM o dispositivo audible. PCM y estado se comparan exactamente dentro de cada plataforma/build; no se inventa equivalencia flotante entre arquitecturas.

### Frontend completo privado

La ROM local previamente configurada se validó con el perfil existente; módulo ABI v3 privado reutilizado, sin descarga/regeneración/publicación. Cinco sesiones de 120 unidades sin inputs ni ayudas, con mute y WAV/captura privados:

| Modo | NES/lotes audio/hooks | Dibujos/swaps | Subidas | Hits de caché |
|---|---:|---:|---:|---:|
| Original | 120 | 120 | 120 | 0 |
| 60 experimental | 120 | 144 | 120 | 24 |
| 90 experimental | 120 | 215 | 120 | 95 |
| 120 experimental | 120 | 286 | 120 | 166 |
| Unthrottled + opción 120 | 120 | 120 | 120 | 0 |

Todas: exit 0, una captura, 114.965 muestras, **WAV y captura final idénticos byte a byte**. Confirma también que unthrottled conserva la ruta original, sin activar la caché experimental. Arranque hasta selector de idioma; no partida completa ni evaluación audible.

Tiempos totales de host de esas sesiones (ms):

| Modo | Simulación | Audio/take/WAV | Render | Swap | Espera |
|---|---:|---:|---:|---:|---:|
| Original | 410,638 | 7,167 | 77,116 | 71,367 | 1.805,549 |
| 60 | 435,660 | 7,881 | 92,723 | 88,831 | 1.707,221 |
| 90 | 428,522 | 7,922 | 122,946 | 123,453 | 1.653,145 |
| 120 | 340,937 | 6,757 | 124,417 | 174,947 | 1.697,528 |
| Fast + 120 | 148,073 | 1,325 | 23,096 | 50,450 | 0 |

Son muestras de startup con cargas variables y ventanas de observación completas, no pruebas estadísticas de overhead/latencia. Los contadores son separables; no dividirlos por la duración completa para anunciar FPS estables si hay arranque/pausa/finalización.

### Ventana, pausa y foco

- Resize/F11 y pixels/4:3/nearest: comprobados con GL real y lectura de pixels, incluyendo cache hits después de resize/fullscreen.
- Aplicación privada: F11 entrar/salir, Escape, Cancel mediante Escape, reanudación y Exit explícito; exit 0. CPU 23.598.167 y PPU 710 idénticos en pausa/reanudación; audio_paused 1→0.
- Sesión privada con audio real: 5.421 unidades/audio/hooks, 14.539 dibujos/swaps, 1.588 dibujos pausados, 5.413 subidas y 9.126 hits. Cero muestras omitidas por SDLAudioOutput en esa sesión. Algunas publicaciones quedaron reemplazadas antes de mostrarse; no se saltaron pasos NES.
- Sus swaps acumularon 33,722 s en una sesión de 123,385 s, con máximo ~86,945 ms. Esto demuestra que eliminar uploads no elimina bloqueos de swap; no justifica automáticamente crear hilos o cambiar vsync.
- FOCUS_LOST/GAINED y liberación de entradas: regresiones SDL automatizadas aprobadas. El intento de cambio de foco entre aplicaciones por automatización **no produjo una nueva transición SDL observable en el log**; no se cuenta como comprobación física de pérdida de foco ni demuestra un fallo del core. Queda pendiente repetirla manualmente, además de mandos reales y suspensión del sistema.

Todos los derivados privados permanecen fuera de Git/artefactos públicos. El informe contiene solo medidas agregadas.

## 6. CI preparada y resultados pendientes

Nuevo `tests/high_refresh_report.py` recoge el inventario/JUnit ya ejecutado, sin repetir la batería CPU. Verifica nueve observaciones nativas, tasas/perturbaciones completas, contadores y salida gráfica. Guarda JSON y log sintético con hash del probe ejecutado. Rechaza reportes incompletos, skips, nuevas fallas, fuente/build incompatibles o mediciones no finitas.

La recogida puede finalizar correctamente **aunque la suite siga fallida por A02**: el JSON mantiene `fully_passed=false`, `failed_names=[A02]` y el workflow conserva el fallo de CTest y del gate anteriores. No equivale a aprobar toda la CI.

| Plataforma | Ejecución de C en esta tarea | GL real en esta tarea | Preparación externa |
|---|---|---|---|
| macOS ARM64 | Sí, desktop/headless; A02 único fallo | 23/23 y medidas anteriores | macos-15: CTest completo y probe gráfico; no ejecutado remotamente |
| macOS Intel | **No ejecutada** | **No ejecutado** | macos-15-intel, mismo procedimiento; pendiente |
| Windows x64 | **No ejecutada** | **No ejecutado** | windows-latest/MSYS2: CTest/perfiles nativos; gráfico marcado not_executed, requiere PC/contexto GL 3.2 real |
| Linux x64 | **No ejecutada** | **No ejecutado** | ubuntu-22.04: CTest; probe bajo Xvfb/Mesa software, no monitor físico; pendiente |
| Steam Deck | **No ejecutada** | **No ejecutado** | Prueba local con artefacto experimental del mismo SHA; procedimiento siguiente |

CI mantiene los cuatro jobs/arquitecturas existentes. Los pasos de evidencias se ejecutan después de A02 si el build fue correcto y conservan resultados en `ctest-*`: high-refresh-evidence.json, high-refresh-gl.log, inventario/JUnit/log de tests. Linux añade únicamente Xvfb/xauth/Mesa DRI para la prueba con pantalla virtual; no se instalaron dependencias nuevas en el Mac ni se modifica el paquete de usuario.

YAML parseado localmente y siete contratos de evidencias/CI aprobados. **Esto no es una ejecución de GitHub Actions.** Si el runner macOS carece de pantalla/contexto o el probe falla, se conserva el fallo y log; no se reemplaza por aprobación artificial. Windows no se declara gráficamente validado por compilar.

### Comandos preparados, aún no autorizados ni ejecutados

Después de revisar/stagear selectivamente solo A–C (excluir informes ajenos, ROM/módulos/cache/logs), y **únicamente con autorización explícita**:

```sh
git diff --check
git diff --cached --check
git diff --cached --stat
# Revisar todo el diff staged y comprobar el remoto antes de confirmar.
git commit -m "Add experimental PAL-independent presentation diagnostics and texture retention"
git push -u origin diagnostic/high-refresh-phase-a
# El push lanza el workflow existente para las cuatro plataformas.
gh run list --repo inthe92/addams-family-recompiler \
  --workflow build.yml --branch diagnostic/high-refresh-phase-a
# Seleccionar la ejecución cuyo headSha coincide con el commit recién publicado.
gh run view <RUN_ID> --repo inthe92/addams-family-recompiler --json headSha,jobs,conclusion,url
gh run download <RUN_ID> --repo inthe92/addams-family-recompiler --dir <evidencia-temporal>
```

No se prepara push a main, tag o release. Antes del commit siguen siendo necesarios auditoría pública/privacidad del conjunto staged y compatibilidad del remoto. Esta tarea no stagea ni confirma nada. La ejecución externa requiere autorizar esa publicación; los resultados de la beta/commits antiguos no sustituyen los de A–C.

## 7. Reproducción local

```sh
cmake -S . -B build-hr -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-hr --parallel 3
ctest --test-dir build-hr --show-only=json-v1 > build-hr/ctest-inventory.json
ctest --test-dir build-hr --verbose --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit "$(pwd)/build-hr/ctest-results.xml"
python3 -B tests/verify_ctest_suite.py --inventory build-hr/ctest-inventory.json \
  --junit build-hr/ctest-results.xml
# En escritorio gráfico real (Windows: mismo probe con .exe):
python3 -B tests/high_refresh_report.py --build build-hr \
  --output build-hr/high-refresh-evidence.json --graphics run --graphics-label local-display
# Sin display: --graphics not-executed; queda explícitamente pendiente.
```

Para headless: NES_ENABLE_DESKTOP=OFF, mismo CTest completo y recolector con `--headless --graphics not-executed`. No requiere ROM. El recolector tiene éxito como recogida si únicamente falla A02; CTest/gate mantienen su fallo.

## 8. Steam Deck: protocolo pendiente

Utilizar modo escritorio y **un AppImage experimental nuevo del commit A–C**, una vez autorizado/construido en CI. La beta publicada no contiene estas fases; no sirve para verificarlas. No usar Proton ni un paquete antiguo para simular validación Linux nativa.

1. Descargar el artefacto `frontend-linux-x86_64-AppImage` del run correcto, comprobar su SHA-256 y extraer el contenedor de GitHub Actions. Copiar AppImage/checksum a una carpeta temporal nueva, preferiblemente con espacios. No reemplazar archivos/configuración existentes.
2. `chmod u+x` al AppImage local; ejecutar `--build-info` y registrar commit/source_sha256/ABI 3. Deben coincidir con la CI experimental. Guardar SO/GPU/controlador, sesión X11/Wayland, tamaño de ventana y modo de monitor configurado.
3. Si FUSE impide el arranque, registrar ese fallo por separado y probar `--appimage-extract`/AppRun en otra carpeta limpia. No confundir extracción con ejecución directa ni presentar esa alternativa como éxito de FUSE.
4. Para prueba sin ROM: ejecutar los probes/contratos desde un build de pruebas del mismo SHA en un entorno de desarrollo separado, siguiendo los comandos anteriores. El AppImage de usuario no incluye los ejecutables de tests; no se promete un benchmark sintético dentro de él. No instalar compilers globales en SteamOS para este protocolo.
5. Para uso real, utilizar exclusivamente la ROM propia compatible. Importar primero en cache privada; conservar ROM, módulo, WAV, capturas y logs fuera del repo. Comparar startup sin inputs en original/60/90/120 con iguales `--frames 120`; después hacer sesiones jugables separadas de 5–10 minutos, sin llamar a eso una partida completa.

Ejemplo tras obtener un artefacto correcto; reemplazar placeholders. Las variables son de esta prueba:

```sh
NES_HR_DIR=$(mktemp -d -t 'nes-high-refresh.XXXXXX')
NES_HR_APP="/ruta/NES-Native-Lab-Linux-x86_64.AppImage"
NES_OWN_ROM="/ruta/ROM-propia-compatible.nes"
"$NES_HR_APP" --build-info > "$NES_HR_DIR/build-info.json"
for hz in 60 90 120; do
  "$NES_HR_APP" --rom "$NES_OWN_ROM" --import-cache "$NES_HR_DIR/private-cache" \
    --experimental-present-hz "$hz" --timing-diagnostic --no-god --frames 120 \
    > "$NES_HR_DIR/session-$hz.log" 2>&1
done
# Repetir sin --experimental-present-hz para referencia original.
# Repetir --unthrottled: tiene prioridad y debe producir un draw por unidad NES.
```

Durante la sesión jugable comprobar resize/F11, Escape/Cancel/Exit, pérdida/recuperación de foco, pausa y mando, y que no queda movimiento pulsado. Registrar drops/skips, bloqueos, tearing y calentamiento; no deducir latencia audible/visible solo del contador. Un objetivo 120 no convierte una pantalla configurada a 60/90 en 120 Hz visibles. SDL reported_hz tampoco certifica su refresco dinámico.

Revisar al final la línea Timing diagnostic y la línea Presentation display. Compartir solo esos agregados saneados y build-info, no el log completo con rutas/importación ni contenido del juego. Si se comparan WAV, exigir iguales inputs por paso y misma plataforma/build; capturas con UI pueden diferir por su reloj real. Conservar las evidencias privadas y no subirlas a CI.

## 9. Archivos modificados en C

- `.github/workflows/build.yml`: recogida de evidencias por plataforma y display virtual Linux.
- `CMakeLists.txt`: un contrato CTest de evidencias nuevo.
- `app/desktop.cpp`: overload con snapshot y diagnóstico opt-in de pantalla/swap; sin cambios de unidad NES.
- `runtime/include/runtime/gl_presenter.hpp`, `runtime/src/gl_presenter.cpp`: clave/retención y métricas, conservando la ruta original.
- `runtime/include/runtime/presentation_clock.hpp`, `runtime/src/presentation_clock.cpp`: identidad única de publicación, planificador intacto.
- `runtime/include/runtime/frontend_timing.hpp`, `runtime/src/frontend_timing.cpp`: tres contadores y dos subtimings aditivos.
- `tests/presentation_gl_checks.cpp`: siete invalidaciones/readbacks y benchmark emparejado; perfiles por tasa.
- `tests/frontend_timing_native_checks.cpp`: perfiles host reales del replay con calendario virtual existente.
- `tests/frontend_timing_checks.cpp`, `tests/presentation_clock_checks.cpp`: métricas/identidad adicionales.
- `tests/frontend_timing_source_checks.py`: protege las rutas original/snapshot/pausa.
- `tests/verify_ctest_suite.py`, `tests/verify_ctest_suite_checks.py`: entrada/recuentos obligatorios actualizados.
- Nuevos `tests/high_refresh_report.py`, `tests/high_refresh_report_checks.py` y este informe.

No se alteran los informes A/B ni el resto de modificaciones preexistentes. No hay dependencias de Python adicionales; atomic/chrono son biblioteca estándar. La única instalación prevista nueva es la pantalla virtual de CI Linux, no una dependencia del producto.

## 10. Límites y criterios para investigar interpolación

La caché está verificada localmente; **C no está certificada multiplataforma**. Faltan CI del nuevo SHA en Windows/macOS Intel/Linux/macOS 15, OpenGL Windows real, GPU/monitor Linux/Deck reales, foco físico manual, mandos/suspensión y sesiones representativas con audio. Las duraciones de swaps muestran que no basta eliminar uploads para garantizar 120 bajo toda carga.

Antes de estudiar interpolación visual:

1. Publicación de rama autorizada y cuatro ejecuciones/artefactos de idéntico SHA, sin nuevas regresiones ni omisiones, A02 visible e independiente.
2. Pruebas manuales nativas de viewport, foco/pausa/input/audio, y medidas de pacing/coste/latencia por plataforma; distinguir host, swap y monitor. No prometer 120 visibles sin medición apropiada.
3. Conservar el diferencial determinista y la ruta original, incluida PCM exacta dentro del mismo build, hooks y trazas. No simular por adelantado ni cambiar 50,007.
4. Fase separada de observación: identidad/generación de actores, cámara y épocas de escena coherentes con el raster; cobertura de apariciones, animación discreta, colisiones, cambios de cuarto y prioridades. Datos actuales no certifican esos endpoints.
5. Definir cortes/fallback a raster real, presupuesto de latencia aceptable y tests de endpoints/discontinuidades antes de cualquier lerp. No mezclar rasters completos, revelar zonas ocultas, activar panorámico o anunciar suavidad demostrada sin evidencia visual.

Siguiente paso: solicitar autorización para publicar **solo la rama de verificación A–C** y ejecutar CI. Después, cerrar las pruebas manuales que CI no puede demostrar. No iniciar todavía interpolación ni distribución pública del modo experimental.
