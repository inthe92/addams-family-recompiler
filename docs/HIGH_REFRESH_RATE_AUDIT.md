# Auditoría de presentación a 60, 90 y 120 FPS

Fecha: 2026-10-12. **Auditoría exclusivamente; no implementación.**

## Conclusión

Es viable separar la presentación del avance NES **sin cambiar ABI v3 ni regenerar el C del juego**, comenzando por mostrar el último fotograma PAL completo a una frecuencia independiente. La PPU seguiría produciendo aproximadamente 50,007 fotogramas distintos por segundo. Presentar ese contenido 120 veces por segundo no lo convierte en 120 posiciones distintas de movimiento ni garantiza suavidad o ausencia de tearing.

La interpolación correcta de cámara/personajes/enemigos **no está demostrada**. El renderizador activo recibe un raster ya compuesto; los observadores experimentales contienen datos útiles pero incompletos, específicos de esta ROM y desconectados del frontend público. La recomendación es separar primero los relojes, comprobar equivalencia determinista y medir la presentación real. Investigar interpolación sería una tarea posterior, con aceptación visual y de latencia propia.

## 1. Estado y alcance

- Checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` del workspace actual.
- Rama: `beta/0.9.0-english-ui`; HEAD: `bafd0cf5ef2007daa24c133c3e45dd5d55a8d985`.
- Versión 0.9.0 / etiqueta pública v0.9.0-beta; ABI v3.
- Ocho archivos preexistentes pendientes, incluidos informes sin seguimiento y `docs/ABI_V3_PACKAGING_CI_FIX.md` modificado: preservados mediante comparación SHA-256.
- Leídos `AGENTS.md`, `PROJECT_STATUS.md`, `docs/ARCHITECTURE.md`, `docs/ORIGINAL_PRESENTATION_1_0.md`, fuentes y pruebas pertinentes. `ARCHITECTURE.md` conserva una descripción histórica del panorámico; la integración efectiva actual en CMake y `app/desktop.cpp` confirma que está desactivado. `PROJECT_STATUS.md` corresponde a la preparación previa a publicación; no se modifica aquí.
- Único archivo creado: este informe. Sin cambios de rama, producción, tests, configuración, ROM, módulos, paquetes, beta, commits o remoto. No se utiliza ninguna ROM en los experimentos de esta auditoría.

Las referencias de líneas siguientes corresponden al HEAD anterior. [Código de referencia completo](https://github.com/inthe92/addams-family-recompiler/tree/bafd0cf5ef2007daa24c133c3e45dd5d55a8d985).

## 2. Relojes actuales y dependencias de 50 Hz

| Componente | Funcionamiento observado | Dependencia actual / consecuencia |
|---|---|---|
| Bucle principal | `app/desktop.cpp:115–168`: eventos → entrada → hooks → ejecución hasta nuevo frame PPU → audio → dibujo → swap → espera | Una iteración activa = un frame NES completo = una presentación. El trabajo y los bloqueos se suman en el mismo hilo. |
| Tiempo de host | `original_presentation.cpp:33–36`, `next_pal_deadline` | Incrementa el deadline en `frequency/50.007`; conserva el calendario entre iteraciones, salvo retraso superior a 100 ms, que lo reancla al presente. No es un acumulador independiente de presentación. |
| CPU / servicios | `runtime_services.h:70–102`; `Bus::advance`, `sync_devices`, `boundary` | El C ejecuta instrucciones, accesos temporizados, avance único, muestreo IRQ y límite de instrucción. La frecuencia de GPU no participa. |
| PPU | `video_state.cpp:156–180` | 312 líneas × 341 dots; 16 ticks por ciclo CPU / 5 por dot, con resto conservado. En línea 241/dot 1 congela frame/background/styles, incrementa `frames()` y afirma vblank/NMI. |
| Salida de bloque | `bus.cpp:114–124`; `emit_blocks.py:200–206` | `boundary` atiende DMA/NMI/IRQ antes de devolver el estado de frame pendiente. El frontend repite slices hasta que cambia `frames()`. No termina necesariamente en el dot exacto: conserva el final de instrucción y posibles ciclos de entrega de interrupción/DMA. |
| Audio NES | `audio_state.cpp:166–192` | Se sintetiza por ciclo CPU; reloj `53203425/32` Hz y acumulador racional hacia 48.000 muestras/s. Es independiente de la GPU. |
| Entrega de audio | `desktop.cpp:161`; `sdl_audio_output.cpp:24–40` | Una extracción/submisión por frame NES; dispositivo consume PCM a su propio reloj. La entrega puede bloquear el hilo principal. |
| Controles | `desktop.cpp:124–147`, `frontend_events.cpp` | Eventos y estado SDL se procesan una vez por iteración activa; los botones NES se fijan antes de ejecutar ese frame. Frecuencia de sondeo cercana a 50 Hz, no lectura del mando a cada instrucción del host. |
| Hooks | `desktop.cpp:149–160` | `mods.frame`, `god.begin_frame`, diagnóstico por slice y por frame pertenecen al avance NES; aumentar sus llamadas a 120/s cambiaría comportamiento/diagnóstico. GOD permanece desactivado por defecto. |
| Imagen | `GLPresenter::present`, `gl_presenter.cpp:85–100` | Convierte el raster 256×240 a RGBA, actualiza una textura y dibuja un quad 4:3 con nearest. No hay scene graph, objetos o cámara interpolable en esta ruta. |
| Interfaz y pausa | `frontend_ui.cpp:4–8`, `desktop.cpp:139–145` | Banner y fade usan tiempo real monotónico. Durante menú/foco perdido no se ejecuta NES: se dibuja el último frame y se espera 10 ms, sin un objetivo de FPS de presentación. |
| Captura/límite | `desktop.cpp:121,164,171–173` | `presented`, `--frames`, captura final y métricas se apoyan hoy en equivalencia entre presentación activa y frame NES. Deben separarse sin acortar ejecuciones de prueba. |

**No cambiar `50.007` por 60/90/120 y no llamar al juego una vez por swap.** Tampoco introducir `Bus::advance` desde el reloj gráfico ni redondear a un número fijo de ciclos por frame.

Las constantes actuales dan 33.247,5 ciclos CPU por frame PAL completo y aproximadamente 50,0069789 Hz por cálculo; el pacing de host usa 50,007. La diferencia aritmética equivale a unos 1,52 ms por hora, sin considerar otras fuentes de jitter. No se ha medido deriva audible ni se propone corregir esa diferencia en esta tarea. El primer frame, la fase CPU/PPU y el final de instrucción impiden exigir que cada slice/frame observado consuma exactamente 33.247 o 33.248 ciclos. [NESdev, tabla de ciclos PAL](https://www.nesdev.org/wiki/Cycle_reference_chart) documenta la relación 3,2 dots/ciclo y 312 líneas; su consulta directa devolvió 403, pero el resultado indexado disponible incluyó la tabla. Las cifras específicas de esta auditoría se derivan también de las constantes del código, no de una nueva medición de hardware.

## 3. SDL, OpenGL, pantalla y audio

### Presentación y sincronización vertical

El programa solicita `SDL_GL_SetSwapInterval(0)` y no comprueba su resultado ni consulta el intervalo efectivo, modo/Hz del monitor o tiempos de swap. Por tanto, se demuestra la solicitud de desactivar sincronización vertical, **no** la cadencia física ni el comportamiento de compositor/driver. El comentario de Cocoa explica una precaución histórica frente a bloqueos en segundo plano; no se ha reproducido ni confirmado ese bloqueo en esta auditoría.

Según [SDL2 SetSwapInterval](https://wiki.libsdl.org/SDL2/SDL_GL_SetSwapInterval), 0 solicita actualizaciones inmediatas, 1 sincronización vertical y -1 sincronización adaptativa cuando se admite. Un intervalo es una política de swap, no un límite portátil arbitrario de 60/90/120 FPS. La especificación [Khronos WGL_EXT_swap_control](https://registry.khronos.org/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt) expresa una periodicidad mínima en periodos del monitor para Windows; no certifica tiempos efectivos del programa en macOS/Linux. [SDL_GL_SwapWindow](https://wiki.libsdl.org/SDL2/SDL_GL_SwapWindow) actualiza la ventana de doble buffer, sin informar por sí sola cuándo el usuario vio el frame.

Hay que distinguir cuatro medidas: frames NES producidos, draws del frontend, swaps completados y frames realmente mostrados. Dibujar a 120 en un monitor fijo de 60 Hz no entrega 120 imágenes completas visibles. En una pantalla fija de 120 Hz, limitar a 90 no produce una cadencia uniforme mediante un divisor entero del retrace. VRR, compositor, monitores múltiples y modos fraccionarios requieren evidencia real.

Una futura selección automática podría consultar [SDL_GetWindowDisplayIndex](https://wiki.libsdl.org/SDL2/SDL_GetWindowDisplayIndex) y [SDL_GetCurrentDisplayMode](https://wiki.libsdl.org/SDL2/SDL_GetCurrentDisplayMode) al iniciar/mover la ventana/cambiar pantalla completa. El campo de refresco de [SDL_DisplayMode](https://wiki.libsdl.org/SDL2/SDL_DisplayMode) es entero y puede valer cero; no basta para medir ProMotion/VRR o confirmar 59,94/119,88 Hz. `SDL_GL_GetSwapInterval` tampoco confirma scanout y admite un valor por defecto cuando no puede determinarlo. Mantener una alternativa de pacing de host y registrar capacidades/fallos resulta necesario.

### Audio y bloqueos

El audio no se obtiene de FPS. Cambiar el ritmo de `take_samples`, duplicar bloques PCM en draws repetidos o repetir filtros alteraría su salida. Extraer/enviar exclusivamente lo generado por cada avance NES preserva el flujo; WAV debe registrar ese mismo flujo una sola vez.

La cola actual limita a 250 ms, espera hasta 150 ms por espacio y después descarta el lote si sigue lleno. La primación es un umbral de 40 ms de muestras; el buffer solicitado es 1.024 muestras (~21,33 ms a 48 kHz). Son parámetros del código, **no mediciones de latencia audible total**. La APU puede acumular como máximo un segundo de muestras antes de incrementar `dropped_samples`. Ninguno de esos bloqueos/caps depende de los Hz del monitor.

Un bloqueo de 150 ms en `submit` supera los presupuestos gráficos de 16,67/11,11/8,33 ms. Por ello, separar deadlines no basta para garantizar 120 FPS sin medir CPU, cola y swaps. [SDL_QueueAudio](https://wiki.libsdl.org/SDL2/SDL_QueueAudio) copia el PCM y el dispositivo consume la cola; si se vacía, SDL reproduce silencio. Un reloj físico de audio y el host pueden divergir. Esta auditoría no propone cambiar APU/resampler ni usar la cola como autorización para ejecutar más lógica NES. Si el bloqueo se confirma, una cola/delegación de entrega en el frontend sería un experimento específico posterior, conservando orden y muestras.

### Coste gráfico previsto

Una imagen RGBA ocupa 245.760 bytes (240 KiB); dos snapshots de índices ocupan 122.880 bytes (120 KiB). Subir RGBA en cada draw supone nominalmente 14,06/21,09/28,13 MiB/s a 60/90/120. Son cálculos de payload, **no benchmarks** ni tráfico físico medido. Retener textura por número de frame y actualizarla solo al cambiar el raster evita conversión/subida redundante; todavía habría draws/swaps y trabajo de UI. No se ha medido consumo, temperatura ni margen GPU/CPU a 120.

## 4. Información disponible para interpolar

| Datos | Disponibles | Limitación |
|---|---|---|
| Imagen final | `VideoState::frame()`, 256×240 congelado en vblank | Sprites, fondo y HUD ya están mezclados. No aporta identidad, velocidad, colisión ni pixels futuros. |
| Fondo | `background_frame()` | Ofrece raster de fondo de ese frame, útil para investigar composición; no reconstruye por sí solo prioridad y sprites en todos los casos. |
| Estilo por línea | `VideoLineStyle`: scroll/fine X, palette, máscara, control y offsets CHR | Muestreo al inicio de línea (`draw_pixel`, x=0), no un historial completo por dot de todos los cambios. Es desplazamiento PPU, no cámara semántica única. |
| OAM | Getter `oam()` de 256 bytes | Estado vivo, no historial OAM congelado/etiquetado junto a cada frame. 64 registros no equivalen a 64 actores estables: metasprites, reciclaje, ocultaciones, wrap y prioridad. |
| Cámara/poses experimentales | `ActorView`, `ActorPoseSnapshot`, `WorldView::raster_camera` | Cámara RAM $74/$75, posiciones relativas, slot/tipo/pose y asociación con DMA. Son observaciones específicas de esta ROM, con rutas/poses rechazadas y cobertura incompleta. |
| Transiciones | RAM observada por diagnóstico: pantalla, habitación, layout, muerte; estabilización por dos muestras | Señales candidatas para cortar interpolación, no una garantía universal de escena estable. No usar únicamente la confirmación tardía de dos frames. |

`CMakeLists.txt:115–118` compila ActorView/WorldView en `experimental_world_view` para tests; `user_game_view` y `desktop_presenter` del producto no los enlazan. El frontend conecta GOD y diagnóstico, no ActorView. Su existencia no demuestra que el juego publicado entregue todos los actores ni que podamos activar ese compositor: WorldView usa ancho 427 y extensión de mapas, incompatible con mantener la presentación original.

Referencias: `game/src/actor_view.cpp:15–76` (captura en PCs concretos, hasta 12 slots, asignación al próximo frame y contraste con DMA), `game/include/game/actor_pose.hpp` (metasprites y coordenadas relativas), `game/src/world_view.cpp:9–24,54–86` (cámara lógica vs scroll del raster y composición lateral). El código ya reconoce que RAM puede contener el próximo scroll mientras el frame visible conserva el anterior. Emparejar el raster N con RAM/OAM leídos después sin demostrar su fase puede desplazar sprites respecto al fondo.

**Conclusión sobre interpolación:** hay datos parciales para investigación selectiva; faltan identidad persistente por aparición, pares coherentes de escena/cámara/actores, reglas de corte verificadas, recursos/prioridad congelados en el instante correcto y cobertura de todos los estados. No hay una fuente validada de posiciones futuras. No se usaron recursos de la ROM para probar cobertura aquí.

## 5. Alternativas

| Alternativa | Lógica/ABI | Resultado esperado | Complejidad y riesgo |
|---|---|---|---|
| Mantener el modo actual | Intactos | ~50 presentaciones activas/s; fidelidad actual | Baja; conserva dependencia de trabajo gráfico en el hilo. |
| Repetir último raster completo a frecuencia independiente | Intactos; frontend solamente | Mayor frecuencia de UI/presentación, pixels originales y movimiento todavía PAL | Baja–media; opción recomendada. Persisten repeticiones desiguales y riesgo de pacing/tearing. |
| Mezclar dos imágenes completas | Puede dejar simulación intacta | Ghosting/doble contorno, mezcla de HUD/animación; no es interpolación de posiciones | Media; desaconsejada para fidelidad y pixel art. |
| Interpolar actores/cámara con metadatos observados | ABI puede mantenerse mediante observadores C++ que deleguen todos los servicios v3 | Movimiento visual intermedio solo donde esté verificado | Alta; datos/oclusiones/transiciones incompletos. No activar WorldView ni mapas laterales. |
| Extrapolar posiciones conocidas | Simulación podría mantenerse | Reduce retraso aparente pero puede mostrar posiciones imposibles antes de corregirse | Alta; saltos, colisiones, muertes y apariciones no predecibles. No recomendada como primera fase. |
| Ejecutar NES a 60/90/120 o cambiar clocks | Cambia lógica/calendario | Acelera/modifica juego, IRQ, música y entrada | Fuera de alcance; rechazada. |
| Hilo NES separado y presentación principal | Sin necesidad de ABI nueva si se conserva propiedad exclusiva de runtime | Aísla bloqueos de GL; no elimina coste CPU/audio | Media–alta; opción condicional tras medidas. Sin acceso concurrente a Bus/PPU/RAM. |

Con retención del último frame, un análisis racional ideal de 10 segundos produce 500 avances PAL y 600/900/1.200 oportunidades gráficas, excluyendo el arranque y sin simular CPU. La retención de frames completos varía entre 1–2 refreshes a 60/90 y 2–3 a 120. Ratios medios: 1,199832 / 1,799748 / 2,399664. No se han medido esos FPS en la aplicación ni en una pantalla física. **120 no es un múltiplo entero de 50,007.** Una pantalla compatible cercana a 100 Hz o VRR podría reducir irregularidad, pero no se ha probado y tampoco genera posiciones nuevas.

El análisis usa `F(n) = floor(n × 50007 / (R × 1000))` para la etiqueta del último frame en la oportunidad n de una pantalla ideal de R Hz, sin costes ni jitter. Puede reproducirse con aritmética entera para `n = 0..10R−1`, contando cuánto se repite cada etiqueta; no representa el arranque ni ejecuta un planificador de producción.

Interpolar causalmente dos estados ya conocidos suele mostrar la escena entre N−1 y N mientras la simulación está en N: introduce aproximadamente un periodo PAL (~20 ms) de retraso visual respecto al último estado, además de colas existentes. La fórmula debe usar timestamps de los estados, no solo el número de swaps. No es posible utilizar de forma causal un estado N+1 todavía no calculado sin retraso o predicción. Tampoco debe ejecutarse la simulación por adelantado para obtenerlo: cambia la relación con entrada/audio. Aun sin cambiar colisiones, posiciones visuales intermedias pueden dar una percepción diferente de contacto/plataformas.

## 6. Arquitectura mínima propuesta

Primera implementación propuesta: **un único hilo, dos deadlines monotónicos y un snapshot inmutable**; conservar SDL/OpenGL en el hilo principal. No crear un segundo motor.

```text
Eventos del host ──► estado de entrada/foco/interfaz
                         │ muestreo una vez por avance NES
Deadline PAL ──► unidad de ejecución actual ──► CPU/bus/PPU/APU originales
                         │                      │
                         │                PCM por ciclos ──► cola SDL
                         └─► snapshot del último frame completo
Deadline gráfico ──► dibujo del snapshot + interfaz ──► swap
```

Contrato recomendado:

1. Extraer conceptualmente la unidad actual de avance: aplicar el mismo muestreo de entrada y hooks, ejecutar el mismo bucle de slices hasta cambio de `video.frames()`, conservar orden de diagnóstico/`consume_frame`/PCM. Preservar el final de instrucción y servicios existentes; no forzar alineación CPU al inicio del raster ni añadir lecturas de registros para observarlo.
2. El calendario NES conserva el periodo de host actual, separado del deadline gráfico, incluida la primera unidad inmediata y su frame inicial parcial. No inferir timestamps del raster únicamente multiplicando `frames()` por 20 ms: conservar origen/fase y etiquetar el instante de observación. El renderizador nunca llama a `advance`, `poll_irq`, `boundary`, `read8`, `consume_nmi` ni `consume_frame`. Sigue produciéndose el número de pasos debido al reloj PAL, no a los swaps.
3. Copiar al terminar esa unidad el raster congelado y su número/secuencia/ciclo de observación. La primera fase requiere solo pixels y etiqueta, sin datos de escena. El renderer recibe ese snapshot, no un Bus mutable. Mantener originales 256×240, proporción 4:3 y barras negras.
4. Sondear eventos con mayor frecuencia, pero aplicar botones NES una vez en el mismo punto lógico de la unidad. Para demostrar invariancia se exige la **misma secuencia de botones por paso NES**. Sondear más rápido puede cambiar qué acción humana llega a un paso: eso es una diferencia de entrada/latencia, no prueba de igualdad determinista. No añadir retención automática de pulsaciones breves ni predicción de input.
5. En un turno pueden vencer cero, uno o varios pasos NES y cero/una oportunidad gráfica. Si hay atraso, omitir oportunidades de presentación antes que pasos de lógica; limitar trabajo por turno para atender UI. Definir explícitamente la política de retrasos y conservar el reanclaje actual para pausas largas. No ejecutar un gran catch-up tras menú/foco/suspensión ni borrar ciclos acumulados del hardware. Si falta capacidad sostenida, registrar ralentización: no puede garantizarse tiempo real por decreto.
6. Subir textura únicamente para una nueva secuencia. Redibujar el último estado y la UI cuando corresponda; no repetir PCM, hooks o diagnóstico por render. Separar contadores de frames NES/draws/swaps; `--frames` y captura final conservan su significado de ejecución NES, incluso a 120 presentaciones/s y con `--unthrottled`.
7. Pausa/foco perdido congela simulación y dispositivo de audio según el contrato actual; el reloj de UI sigue monotónico. Reanclar deadlines de host al reanudar, limpiar entradas y respetar barrera de liberación. Redimensionado/F11/monitor nuevo cambian viewport/pacing gráfico, nunca ciclos NES.
8. El reloj se lee mediante contador de alta resolución y su frecuencia ([SDL_GetPerformanceCounter](https://wiki.libsdl.org/SDL2/SDL_GetPerformanceCounter)). Preferir deadlines absolutos/incrementales para evitar sumar el coste de cada draw al periodo. [SDL_Delay](https://wiki.libsdl.org/SDL2/SDL_Delay) puede dormir más de lo pedido: no prometer precisión de 8,33 ms por usar `Delay(1)` ni implementar un busy-wait permanente.

Vsync debe elegirse y medirse como política de presentación: solicitar 1 cuando convenga a la pantalla, comprobar éxito/intervalo, registrar tiempos; alternativa con 0 y límite de host cuando no sea fiable. No añadir a ciegas espera PAL completa + espera gráfica + espera de retrace, porque sumaría latencia o causaría pérdida de oportunidades. La llamada de swap o el audio aún pueden bloquear un único hilo; la opción de hilo NES solo se consideraría si las mediciones muestran que impiden el objetivo. En esa opción el worker sería dueño exclusivo de CPU/bus/audio de simulación; main solo snapshots/eventos por cola. No hay garantías de rendimiento todavía.

## 7. Riesgos y mitigaciones necesarias

| Riesgo | Impacto | Verificación / política |
|---|---|---|
| Hook/PCM/avance ligado al draw por error | Alto: lógica, audio o progreso cambian | Replay idéntico por frame y trazas de llamadas; cero mutaciones desde presentación. |
| Latencia de interpolación vs repetición | Alto para saltos/contacto | Medir evento→paso→swap y, aparte, visible mediante equipo externo; no anunciar mejora solo con FPS. |
| Fin de salto, colisión, teleport, muerte/respawn | Alto: imagen muestra estados inexistentes | Si se experimenta con interpolación, cortar ante discontinuidades; no predecir física. |
| Cambio de habitación/scroll wrap | Alto: objetos/fondos mezclados | Época de escena + transición detectada inmediatamente; descartar historia, mostrar raster real. Validar más que el ID estable tardío. |
| Metasprite/slot/OAM reciclado y animación | Alto | Identidad/generación y agrupación verificadas; tile/pose/máscara/paleta son estados discretos, no números para lerp. |
| Prioridad, sprite-zero, ocho sprites por línea, parpadeo | Alto para fidelidad | No cambiar evaluación PPU ni sus flags. Un compositor visual no puede inventar sprites que el raster original ocultó; fallback original. |
| Bordes de cámara, HUD y efectos raster | Alto | No revelar áreas fuera del 256×240 original, no trasladar HUD, no asumir una cámara única por frame; cortar/fallback si faltan pixels. |
| Cola de audio/vsync/GL bloqueando | Medio–alto | Instrumentar duración, cola, underflow/drop y lateness; no usar FPS para cambiar reloj NES. |
| Refresco fraccionario/VRR/compositor/foco | Medio | Medir cadencia efectiva por plataforma y política, fallback/pause seguro; ninguna certificación física con CI headless. |
| Concurrencia prematura | Alto | Preferir un hilo; si se separa, snapshots inmutables y propiedad exclusiva, sin carreras sobre VideoState/cartridge/RAM. |
| Coste/batería/temperatura | Medio, especialmente Steam Deck | No repetir uploads; medir tras desacoplar; evitar render ilimitado. Sin estimación porcentual de rendimiento. |
| A02 | Sigue abierto | Conservar el fallo y su expectativa. La frecuencia gráfica no valida ni resuelve el contrato PAL de $4015. |

## 8. Pruebas propuestas para una implementación futura

No se añaden ni cambian tests en esta tarea. Las siguientes pruebas serían los criterios de aceptación, en un cambio posterior autorizado:

| Prueba reproducible | Entrada / procedimiento | Resultado exigido |
|---|---|---|
| Planificador con reloj falso | Secuencia racional de timestamps, 50,007 PAL y render 60/90/120, 59,94/119,88, cambio de monitor, jitter y pausas; al menos 10 min virtuales | Mismo número/orden de pasos NES para mismo calendario/input; oportunidades gráficas independientes. Sin acumulación tras pausa, ciclos añadidos o frame-limit basado en swaps. |
| Replay nativo diferencial | Programas propios 6502 → emisor actual → C nativo → runtime actual; baseline y futuro frontend con iguales botones por paso. Incluir lecturas/escrituras PPU/APU, RAM/espejos, bancos, ramas, IRQ/NMI y OAM/DMC DMA | Comparar después de cada unidad los registros A/X/Y/SP/P/PC, ciclos, RAM/stack, mapper, dots/frame/estado PPU, raster, OAM, contador/PCM APU y trazas de bus/instrucciones. Igualdad, no solo totales finales. |
| Continuación tras snapshots | Intercalar 0/1/muchas presentaciones entre mismos pasos; ejecutar después idéntico programa de lecturas/control, desde ambos recorridos | Iguales flags/latches/IRQ y resultados futuros; detecta mutación latente que un contador no descubre. No leer registros con efectos secundarios solo para calcular hashes. |
| Pureza de presentación | Snapshot sintético, repetir draws a diferentes schedules, resize/F11, UI y captura | Snapshot y estado NES intactos; imagen original sin extensión y sin mezcla en modo repetición. Comparar framebuffer de origen separado de la UI. |
| PCM y entrega | Guardar flujo sintetizado concatenado por paso y eventos de cola; audio normal/mute/unthrottled/focus/menu/catch-up | Igual número, orden y valores PCM para mismo binario/configuración. Sin duplicar lotes por render; reportar por separado entrega real, skips y underflow. |
| Entrada y hooks | Replay fijado por frame + pruebas separadas con timestamps host y pulsaciones cortas/held/repeat | Hooks exactamente una vez por paso; misma entrada latcheada en diferencial. No enviar controles durante menú; audio/input restaurados. No confundir entradas humanas distintas con divergencia del core. |
| Suite de regresión | CTest completo escritorio/headless, mismas expectativas | 4.736 casos nativos, 82 IRQ, PAL 07/08, A01/A03, $2002, DMA/NMI, RAM/MMC1/bus, importador/caché/ABI y UI intactos. A02 sigue fallando; ningún skip/WILL_FAIL/filtro que lo oculte. |
| Cadencia nativa real | Windows/macOS ARM64/Intel/Linux/Deck; pantallas y modos conocidos; registrar CPU-step/draw/swap/espera/cola durante sesión y estrés | Medidas publicadas por plataforma, incluyendo percentiles y misses; comprobar visualmente tearing/stutter y usar captura externa para latencia/scanout si se afirma precisión física. Un contador 120 no basta. |
| Interpolación, solo fase separada | Escenas sintéticas de metasprites, spawn/despawn, slot reuse, salto/colisión, wrap, cambio de cuarto, HUD, prioridad y palette/tile changes | Snap a estado real en discontinuidades; endpoints correctos, sin recursos fuera de viewport ni mutación NES. Demostración de cobertura antes de usar una ROM privada local para validación visual. |

Limitación del diferencial: el runtime no expone una serialización completa de todos los estados privados (latches PPU, historial IRQ, filtros/envelopes APU, controller shift, estado MMC1 temporal). Hash de `CPUState` crudo o memoria de objetos C++ no es válido por padding/punteros; tampoco bastan getters de ciclos. Comparar campos explícitos/trazas y continuaciones es viable sin ABI nueva. Si se exige identidad de todo el estado interno, haría falta acceso de diagnóstico **solo para builds de pruebas**, con serialización explícita de campos, evaluado aparte y sin llamadas de bus con efectos secundarios. Igualdad byte a byte de PCM se exigiría dentro de la misma plataforma/configuración; entre compiladores/arquitecturas se debe justificar la referencia flotante, sin inventar tolerancias para ocultar cambios.

## 9. Comprobaciones ejecutadas en esta auditoría

Mac ARM64, macOS 27.0.1, OpenGL 4.1 Metal. Se reutilizaron binarios locales existentes de las mismas fuentes de producción, sin recompilar ni alterar paquetes. Evidencia temporal fuera del repo; no captures del juego.

| Comprobación actual | Resultado | Qué demuestra / qué no |
|---|---|---|
| 9 entradas CTest pertinentes | 8 PASS, 1 FAIL (A02), 0 omitidas; exit 8 | Geometría 18 casos, pixels 7, pacing 7, banner 9, pausa audio 2, A01 1, A03 1 y SDL queue 1 pasan. A02 1 falla. No es el planificador propuesto. |
| `presentation_gl_checks` existente | 12/12 PASS, exit 0 | Dibujo real/resize/fullscreen/readback y UI; repetir presentaciones no cambia ciclos CPU/APU, frame PPU o raster observado. No demuestra todo el estado interno ni FPS físicos. |
| Análisis racional temporal aislado | Cálculos reproducidos para 60/90/120 | Ratios/retenciones/coste de payload de la tabla. No ejecuta CPU ni el juego: no se cuenta como una prueba del runtime a esas frecuencias. |
| Suite nativa completa del mismo SHA, evidencia anterior | Run 38290678505: 43 entradas/platform, 42 PASS + A02 FAIL, 0 omitidas | Referencia ya verificada, no reejecutada en esta auditoría. [CI](https://github.com/inthe92/addams-family-recompiler/actions/runs/38290678505). No contiene un frontend high-refresh. |

La prueba de pacing actual (`tests/presentation_checks.cpp:62–78`) llama geometría entre deadlines y usa 50/60/120/144, no un scheduler gráfico real; no cubre 90 explícitamente. El probe GL (`tests/presentation_gl_checks.cpp`) repite 60 y 120 draws **sin espaciar a esos Hz**. No convertir sus nombres de frecuencia en una afirmación de fluidez a 120 FPS. A02 reprodujo estado 64 / IRQ posterior 0 frente a esperado 1; no se cambió nada para hacerlo pasar.

Comandos ejecutados (directorio de build local existente):

```sh
ctest --test-dir /private/tmp/nes-original-presentation-tj5sp9gi/desktop \
  -R '^(original_presentation_(geometry|pixels|pacing)|frontend_ui_banner|frontend_audio_pause|apu_sdl_queue_contract|apu_status_(open_bus|irq_collision)_reference|apu_frame_mode_delay_reference)$' \
  --verbose --output-on-failure --output-junit <directorio-temporal>/relevant-tests.xml
/private/tmp/nes-original-presentation-tj5sp9gi/desktop/presentation_gl_checks
git diff --check
```

El filtro anterior limita deliberadamente la **auditoría** a comprobaciones pertinentes e incluye A02; no se modificó la suite/CI ni se propone filtrar la futura verificación completa. El artefacto local usa el mismo emisor/runtime de HEAD; no se presenta como compilación limpia adicional ni como ejecución de Windows/Linux/Intel realizada hoy. Las lecturas oficiales SDL y Khronos son referencias de API, no mediciones de drivers. Ninguna prueba jugable/benchmark high-refresh se ejecutó.

## 10. Implementación recomendada por fases

| Fase futura | Trabajo mínimo | Complejidad / riesgo | Condición para avanzar |
|---|---|---|---|
| A. Instrumentación y contrato | Separar métricas NES/draw/swap; cronometraje CPU, audio, swap y espera; fijar replay/input/pause/frame-limit | Baja–media, bajo para lógica si solo observa | Conocer bloqueos y contar trabajo sin alterar trazas. No prometer 120. |
| B. Deadlines independientes, repetición | Reorganizar `app/desktop.cpp`, snapshot frontend y estado de pacing de host; misma unidad NES y PCM, UI frecuente | Media, riesgo de scheduling/entrada | Diferencial determinista + suites; 60/90/120 en reloj falso y errores/pausas correctos. ABI/recompilación intactos. |
| C. Eficiencia y pantallas reales | Retener textura por frame en `gl_presenter.cpp/.hpp`, capacidad/política vsync, medición en cuatro targets y Deck | Media, riesgo de driver/cadencia | Rendimiento/cadencia/latencia medidos y sin nuevas regresiones. Audio-thread solo si el bloqueo está demostrado y autorizado. |
| D. Observación de escena, sin interpolación | Evaluar puente privado/solo diagnóstico de poses/cámara, etiquetas de frame, épocas, prioridades y cobertura | Alta, riesgo de fase/identidad | Coherencia con raster y escenas/transiciones verificadas. No conectar WorldView panorámico al producto. |
| E. Interpolación visual selectiva opcional | Solo componentes verificados, historial propio, fallback original inmediato | Alta, riesgo visual/latencia; no garantía actual | Evidencia sintética + validación privada/local, latencia aceptada y determinismo conservado. No habilitar por defecto por tener 120 swaps. |

**Sin cambio de ABI/C generado:** fases A–C, snapshots de raster, reutilización de textura, manejo de reloj/ventana/UI, métricas y pruebas de replay. Un eventual observador de escena puede técnicamente usar callbacks y datos exportados ya existentes, manteniendo `read8/write8/advance/boundary/poll_irq` y su orden; esa posibilidad no prueba cobertura ni valida interpolación. Nuevas exportaciones desde el módulo generado exigirían revisar compatibilidad y caché y quedan fuera de la propuesta mínima. No se recomienda introducirlas ahora.

El próximo experimento útil es un scheduler frontend con reloj falso y presentación por repetición, comparado con la unidad PAL existente mediante programas nativos sintéticos. Solo después de demostrar invariancia y medir cadencia real se decide si hace falta separar hilos o estudiar interpolación. La beta publicada permanece intacta y A02 sigue abierto.
