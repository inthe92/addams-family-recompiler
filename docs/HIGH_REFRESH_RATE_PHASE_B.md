# High refresh rate — Phase B: repetición experimental de fotogramas completos

Fecha: 2026-10-13. **Implementada y verificada localmente en macOS ARM64.** No hay interpolación ni publicación. El modo original sigue siendo el predeterminado.

## Estado y conservación

Rama `diagnostic/high-refresh-phase-a`; HEAD `bafd0cf5ef2007daa24c133c3e45dd5d55a8d985`. Se continúa sobre la fase A sin commit, cambio de rama, reset ni modificaciones remotas. Se leyeron íntegramente AGENTS.md, PROJECT_STATUS.md y los informes HIGH_REFRESH_RATE_AUDIT.md/HIGH_REFRESH_RATE_PHASE_A.md.

Una copia temporal de los archivos inicialmente pendientes permite comparar esta fase con la A. Todos siguen presentes. Solo siete archivos preexistentes reciben cambios incrementales propios de B; los demás, incluidos ambos informes anteriores, conservan sus bytes. CPU, PPU, APU, bus, mapper, `next_pal_deadline`, emisor C, perfil ROM y ABI v3 conservan sus hashes. No se modifica la beta publicada, versión, módulos, importador, política de cachés, paquetes ni workflows.

## Activación y contrato

Nueva opción **`--experimental-present-hz 60|90|120`**. Valores distintos se rechazan; sin la opción continúa el camino original. `--timing-diagnostic` conserva su opt-in y formato de fase A.

```sh
<frontend-local> --experimental-present-hz 120 --timing-diagnostic \
  --rom <ROM-propia-compatible> --import-cache <cache-privada> \
  --import-tools <tools/recompiler> --no-god
```

`PresentationClock` es un planificador de **host**, sin referencias a Bus/CPU/SDL ni callbacks de ejecución. Usa ticks monotónicos del contador SDL y dos deadlines absolutos:

- NES: primera unidad inmediata y periodo original `frequency / 50.007`, avanzando mediante la función existente `next_pal_deadline`. No se modifican ciclos, divisores PAL, servicios o final de instrucciones.
- Presentación: `frequency / R`, para R=60/90/120. Cada dibujo usa el último snapshot completo. Oportunidades gráficas vencidas se descartan; no se reproducen swaps atrasados en ráfaga.

El mismo hilo atiende eventos y como máximo una unidad NES por vuelta. Puede haber vueltas sin avance NES y/o sin dibujo. Un atraso corto permite recuperar pasos PAL sucesivos, atendiendo eventos entre ellos. Si el host supera el deadline NES en más de 100 ms, se reancla al presente: una unidad normal, sin recuperar el backlog de una suspensión. Este umbral coincide con el pacing original, pero la detección **antes del paso** es una política explícita del modo experimental; el camino original conserva exactamente su función/espera. No se saltan frames/ciclos del hardware para acelerar la recuperación. Si no hay capacidad suficiente, el juego puede ralentizarse; esta fase no garantiza tiempo real bajo cualquier carga.

El reloj inyectado en pruebas usa los mismos métodos de producción. Su defensa ante un contador que retrocede conserva el último tick observado; no convierte el reloj host en un reloj NES.

## Unidad NES y snapshot

Se conserva una sola copia del cuerpo de simulación en `app/desktop.cpp`, compartida por ambos modos:

```text
botones → mods.frame → god.begin_frame → slices C/diagnostic.slice
→ diagnostic.frame → consume_frame → take_samples → submit → append WAV
→ inicio del banner al primer paso
```

Ese cuerpo solo se ejecuta cuando vence el deadline NES. Las presentaciones adicionales no aplican entradas, hooks, diagnóstico ni PCM. Procesar eventos con mayor frecuencia puede cambiar qué entrada humana alcanza un paso; la prueba determinista exige iguales botones por paso, no igualdad de entradas humanas en tiempos de pared distintos. Las acciones explícitas de depuración ya existentes siguen siendo acciones del usuario, no hooks repetidos por dibujo.

`FrameSnapshot` posee una copia de **61.440 bytes** del raster de índices 256×240. Se publica después de finalizar simulación/diagnóstico/audio; etiqueta secuencia NES, frame PPU, ciclo CPU e instante monotónico de observación. Estos campos no pretenden identificar la fase exacta de vblank. El acceso gráfico expone solo pixels constantes; la copia no comparte almacenamiento con VideoState. No se publica una imagen parcial ni datos de cámara/actores. Un único hilo garantiza que no se sobrescriba mientras se dibuja. Antes del primer frame, una pausa presenta el fondo inicial vacío; después conserva el último snapshot.

Se reutiliza el renderizador existente: 4:3, nearest, framebuffer original y overlay independiente. Cada dibujo todavía convierte/sube el raster mediante GLPresenter; **no se implementa una caché adicional de textura**. Esto mantiene pequeño el cambio, pero deja trabajo gráfico redundante medible. No cambian la política de vsync solicitada (0), los shaders ni la composición NES.

## Semántica conservada

- `--frames N`: cuenta unidades NES, nunca swaps. Cero sigue significando sin límite. La unidad final fuerza un dibujo fresco, aunque no venza el deadline gráfico; captura tras raster/overlay y antes de swap. Después conserva la espera hasta el siguiente deadline PAL y termina. Esa presentación forzada puede añadir una oportunidad gráfica fuera de calendario.
- `legacy_frame_units` y el texto histórico `Presented N frames` siguen indicando unidades NES. En el experimental ya no equivalen al número de dibujos. `render_passes`/`nes_draw_commands`/`buffer_swaps_completed` describen estos últimos.
- `--unthrottled`: **tiene prioridad y evita activar el planificador experimental**. Conserva un dibujo por avance, sin espera PAL activa, con la misma política de audio y espera de pausa original. La consola informa que la opción Hz queda bypassed. No limita la lógica fast a 120 Hz.
- Escape, Cancel, confirmación explícita, mandos y barrera de liberación continúan usando FrontendUI/FrontendEvents sin modificaciones. Pérdida de foco congela NES y audio como antes. Los deadlines se reanclan al reanudar; no se recupera tiempo de pausa.
- En pausa experimental se dibuja según el calendario gráfico y se mantiene audio pausado/botones neutralizados. Ninguna vuelta de pausa consume `--frames`, hooks, slices o muestras. El polling/espera del host utiliza Delay(1); la rama predeterminada conserva Delay(10).
- F11/redimensionado siguen afectando exclusivamente al viewport. Banner y diálogo mantienen su reloj real, sin alterar imagen 4:3 o velocidad PAL.

## Pruebas automáticas

Tres entradas CTest nuevas, obligatorias en escritorio y headless, sin excepciones de fallo:

| Entrada | Casos | Evidencia |
|---|---:|---|
| `frontend_presentation_schedule` | 12 | Planificador real con reloj virtual: 10 minutos a 60/90/120, jitter de sondeo, pausa larga/reanudación, suspensión, calendarios independientes, retroceso de reloj, configuración y CLI inválidas. |
| `frontend_presentation_snapshot` | 4 | Validez, metadatos, propiedad de la copia tras mutar el raster fuente, y 120 conversiones sin mutación. |
| `frontend_presentation_native` | 9 | C sintético generado por el emisor actual: 60/90/120 × ejecución normal/suspensión/pausa. Igualdad por frame de CPU/RAM/pila/PPU/mapper/PCM y trazas completas de read/write/advance/poll/boundary frente a ejecución original. NMI, DMA y hook Lua observables. |

**25 casos nuevos aprobados por configuración**, además de los 27 de fase A que permanecen aprobados. El diferencial nuevo ejecuta una referencia y nueve comparaciones de 12 unidades (120 unidades sintéticas), sin ROM ni intérprete. Igualdad del número/orden/ciclos de accesos y muestreos evita ocultar instrucciones extras detrás de un estado final coincidente. Los snapshots/getters no constituyen una serialización exhaustiva de todos los campos privados ni una nueva certificación PAL subciclo.

En 600 segundos virtuales, incluido el primer paso inmediato:

| Presentación | Unidades NES | Oportunidades gráficas |
|---|---:|---:|
| 60 Hz | 30.005 | 36.000 |
| 90 Hz | 30.005 | 54.000 |
| 120 Hz | 30.005 | 72.000 |

No son benchmarks de tiempo real. El caso de jitter sondea cada 7 ms y termina antes del mismo horizonte de 10 s: 500 pasos observados en las tres tasas, sin ráfagas de draws; su último sondeo es anterior al último deadline ideal del intervalo. El caso de suspensión prueba un salto de 5 s; el de pausa un intervalo largo con reanudación inmediata sin backlog.

Los cinco checks de integración de fase A se amplían para proteger el guard de simulación, snapshot después de PCM/WAV, ruta legacy de pausa, prioridad de unthrottled y dibujo final forzado. No se eliminan sus comprobaciones anteriores. Inventario y gate JUnit exigen las tres entradas/recuentos nuevos.

### Suites completas ejecutadas

macOS ARM64, macOS 27.0.1, Apple Clang 21, SDL 2.32.8, OpenGL 4.1 Metal. Se recompilaron todos los targets de los builds existentes de A (no se trata de otra compilación limpia):

| Configuración | Ejecutadas | Aprobadas | Fallidas | Omitidas |
|---|---:|---:|---:|---:|
| Escritorio | 50 | 49 | 1: A02 | 0 |
| Headless | 45 | 44 | 1: A02 | 0 |

CTest retorna 8; el gate JUnit retorna 1 **por A02 solamente**. Los 4.736 casos nativos, 82 IRQ/PAL 07/08, A01/A03, $2002, RAM/MMC1/bus/PPU, DMA/NMI, importador/caché/ABI y adaptadores continúan pasando. A02 conserva expectativas, ejecución y fallo ordinario; no se etiqueta como aprobado, esperado ni omitido.

Incidencia del arnés: la primera ejecución completa pasó las mismas pruebas pero CTest truncó salidas aprobadas a 1.024 bytes en JUnit. Se repitió la recopilación completa con los límites 65.536 ya utilizados por la CI; el gate verificó entonces todos los recuentos y comunicó solo A02. No se modificaron tests hardware ni workflow para ello.

## OpenGL y frecuencias observadas

El probe gráfico existente conserva 13 casos y añade tres, uno por tasa: **16/16 aprobados**. Verifica viewport/nearest/letterboxing, fullscreen, redimensionado, imagen inmutable, modal y métricas, y ejecuta el planificador real con pixels propios, swaps y readback. Bus/CPU/PPU/APU permanecen inmóviles en esos casos gráficos; los 100 updates de snapshot por sesión son callbacks sintéticos de host, **no pasos ejecutados de juego**.

Resultados de la ejecución final de 2 s por tasa:

| Objetivo | Dibujos/swaps | Tiempo host (s) | Dibujos/s observados | Updates de imagen propia |
|---|---:|---:|---:|---:|
| 60 | 120 | 2,00018 | 59,9945 | 100 |
| 90 | 180 | 2,00024 | 89,9890 | 100 |
| 120 | 240 | 2,00033 | 119,980 | 100 |

La primera ejecución restringida no pudo acceder a pantallas (`The video driver did not add any displays`); las comprobaciones reales anteriores se ejecutaron con acceso al escritorio. No se cambió SDL ni se presentó el primer fallo de entorno como una aprobación.

Estas cifras son llamadas de dibujo/swap en un escritorio real, **no scanout, latencia visible, percentiles, FPS garantizados ni certificación de un monitor 120 Hz**. La lógica de imagen sigue PAL. No hay 120 posiciones nuevas por segundo, interpolación ni demostración de mayor suavidad de movimiento.

## Verificación privada del frontend completo

Se utilizó exclusivamente la ROM privada local ya configurada, validada por el perfil existente, y su módulo ABI v3 compatible. No se descargó ni regeneró contenido comercial. Seis sesiones de 120 unidades, sin inputs de juego ni ayudas; se reutiliza el mismo frontend recién recompilado:

| Modo | Unidades NES/audio/hooks por frame | Dibujos/swaps | Tiempo de sesión instrumentado (s) |
|---|---:|---:|---:|
| Original | 120 | 120 | 2,732414 |
| Experimental 60 | 120 | 144 | 2,729772 |
| Experimental 90 | 120 | 215 | 2,732897 |
| Experimental 120 | 120 | 285 | 2,734048 |
| Unthrottled | 120 | 120 | 0,567919 |
| Unthrottled + opción 120 | 120 | 120 | 0,561527 |

Todas terminaron con código 0, una captura, 120 lotes de audio y WAV y **audio WAV/captura final byte a byte idénticos**. Los tiempos incluyen otros costes y finalización; no dividir estos recuentos por el tiempo total para anunciar una tasa estable. Se comprobó el arranque hasta el selector original de idioma, no una partida completa, movimiento o sonido percibido. Derivados y logs permanecen fuera del repositorio y no forman parte del informe público.

Una sesión adicional a 120 con dispositivo de audio comprobó Escape → Cancel mediante Escape → Escape → Exit explícito. CPU=35.201.546 y PPU=1.059 se mantuvieron exactamente iguales en pausa/reanudación; audio_paused pasó 1→0. Terminó con código 0, 2.284 unidades NES/hooks/audio y 8.585 dibujos/swaps, incluidos 3.155 dibujos pausados. No hubo muestras descartadas por SDLAudioOutput en esa sesión. Los 18.890 turnos de pausa no ejecutaron lógica NES. Foco y controles se cubren además mediante las regresiones SDL existentes; no se realizaron nuevas inspecciones visuales de todas las combinaciones de mando/foco/suspensión real del sistema.

## Archivos de fase B

Modificados sobre el estado guardado de A:

- `app/desktop.cpp`: opción experimental, guard de avance, publicación del snapshot, presentación y pausa con deadlines independientes; ruta original/fast conservada.
- `CMakeLists.txt`: planificador en biblioteca frontend, dos contratos de reloj/snapshot, diferencial scheduled y enlace estático MinGW consistente.
- `tests/presentation_gl_checks.cpp`: tres casos de ritmo real con pixels propios y resumen de 16 casos.
- `tests/frontend_timing_native_checks.cpp`: reutilización del fixture/harness existente con el planificador y nueve comparaciones nuevas.
- `tests/frontend_timing_source_checks.py`: integración incremental sin eliminar contratos de A.
- `tests/verify_ctest_suite.py`, `tests/verify_ctest_suite_checks.py`: inventario/recuentos obligatorios nuevos.

Nuevos:

- `runtime/include/runtime/presentation_clock.hpp`, `runtime/src/presentation_clock.cpp`: planificador de host y snapshot sin cambios ABI.
- `tests/presentation_clock_checks.cpp`: reloj virtual/snapshot/configuración.
- `docs/HIGH_REFRESH_RATE_PHASE_B.md`: este informe.

Los archivos restantes de fase A y el trabajo preexistente se conservan sin modificaciones propias de B. No hay dependencias nuevas.

## Reproducción

```sh
cmake -S . -B build-phase-b -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-phase-b --parallel 3
ctest --test-dir build-phase-b -R '^frontend_(presentation|timing)_' --verbose --output-on-failure
ctest --test-dir build-phase-b --show-only=json-v1 > build-phase-b/inventory.json
ctest --test-dir build-phase-b --verbose --output-on-failure \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit "$(pwd)/build-phase-b/results.xml"
python3 -B tests/verify_ctest_suite.py --inventory build-phase-b/inventory.json --junit build-phase-b/results.xml
# Suite/gate siguen retornando fallo por A02.
cmake -S . -B build-phase-b-headless -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build build-phase-b-headless --parallel 3
# Repetir CTest con los mismos límites, y el gate con --headless.
# En escritorio gráfico real:
./build-phase-b/presentation_gl_checks
```

macOS: ejecutable dentro de `nes_native.app/Contents/MacOS/nes_native`; Windows/Linux según generador. Para comparar tasas, ejecutar la misma ROM privada y secuencia de entradas con `--frames 120`, `--timing-diagnostic` y cada valor 60/90/120. La captura/WAV opcionales y todos los logs deben guardarse localmente fuera del repositorio. Compartir solo el resumen de métricas saneado, sin capturas/PCM/módulos ni rutas privadas. `--build-info` identifica estas compilaciones dirty/locales, no la beta pública.

Evidencias de esta ejecución en almacenamiento temporal local: logs de recompilación, CTest/JUnit/inventario, gate, probe gráfico y comparación privada separados. No se disparó CI ni se generaron paquetes.

## Limitaciones y siguiente fase

- Windows, Linux/Steam Deck y macOS Intel **no ejecutados** en esta tarea. CMake/CTest quedan preparados; falta CI y medición nativa de frontend/pacing en cada plataforma.
- Un hilo conserva orden y evita carreras, pero audio y swap pueden bloquearlo. Se siguen convirtiendo/subiendo rasters repetidos. Polling de 1 ms y mayor tasa de dibujo pueden aumentar consumo; no hay mediciones de batería/temperatura/overhead sostenido.
- Repetición desigual por 50,007 frente a 60/90/120, compositor, tearing, refrescos fraccionarios y VRR siguen pendientes. UI puede actualizarse más a menudo, pero el movimiento del juego tiene únicamente los estados PAL originales.
- La igualdad se ha demostrado con fixtures y entrada fija de startup; no certifica todas las habitaciones, sesiones largas, mandos o presión de audio. A02 sigue abierto e independiente.

Siguiente fase recomendada: **validación de pacing/coste en las cuatro plataformas**, con sesiones representativas, métricas de bloques audio/swap y comparación determinista, conservando el modo experimental. Evaluar después la retención de textura por secuencia si las mediciones justifican ese cambio. No iniciar interpolación o threading sin nueva autorización/evidencia. No se recomienda activar este modo por defecto ni actualizar la beta con estas pruebas locales solamente.
