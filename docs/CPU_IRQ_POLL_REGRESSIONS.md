# A02-B — regresiones de muestreo IRQ de CPU

Fecha: 2026-10-11. **Regresiones incorporadas; producción sin corregir.** Las pruebas conservan los fallos y A02 continúa como test ordinario fallido.

## 1. Estado inicial y archivos

Checkout: `<checkout>`.

- Rama: `verify/phase-3-1-bus-timing`.
- HEAD: **35f5ce38562fcc62d47c8dcd392517bd203a86d1**.
- Origin: `https://github.com/inthe92/addams-family-recompiler.git`.
- Inicialmente no había modificaciones tracked/staged. Había cinco documentos untracked: `APU_A02_DIAGNOSIS.md`, `APU_A02_PAL_REFERENCE.md`, `APU_A02_PAL_PROGRAM_TESTS.md`, `NESRECOMP_COMPARISON.md`, `RELEASE_1_0_READINESS.md`. Se conservan.
- Se leyeron íntegramente los tres informes A02 solicitados. Sus resultados históricos se distinguen de las ejecuciones de esta fase.

Cambios autorizados de esta fase:

| Archivo | Cambio |
|---|---|
| `tests/emit_irq_poll_fixtures.py` | Genera programas propios, comprueba bytes con el decoder actual y emite C nativo con `emit_bank`. |
| `tests/cpu_irq_poll_checks.cpp` | Ejecuta el C sobre Bus real; verifica lecturas, registros, vector, pila y ciclos. |
| `CMakeLists.txt` | Generación en el directorio de build, biblioteca/executable de pruebas, siete entradas CTest y enlace estático Windows coherente con los otros tests. |
| `docs/CPU_IRQ_POLL_REGRESSIONS.md` | Este informe. |

No se modifican pruebas anteriores, producción, ABI, recompilador, CI, rama, tags o releases. No se hacen commits/pushes ni se usan ROMs comerciales. No se copia código externo: se reutiliza el ensamblador y los programas sintéticos propios de la investigación anterior.

## 2. Referencias y contrato independiente

Referencia PAL fijada: **christopherpow/nes-test-roms@95d8f621ae55cee0d09b91519a8989ae0e64753b**:

- [07.irq_flag_timing.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/07.irq_flag_timing.a): cuatro ventanas de lectura, incluidas dos lecturas consecutivas que distinguen reafirmación y limpieza final.
- [08.irq_timing.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/08.irq_timing.a): el manejador debe guardar A=0 en dos alineaciones separadas por un ciclo; A=2 significa demasiado pronto.
- [sync_apu.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/sync_apu.a): escrituras de inhibición/inicio, lectura tras retardo y rama 2/3 ciclos para normalizar la paridad.
- [Readme PAL](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/readme.txt): el autor declara ejecución y aprobación en una NES PAL. **No son mediciones nuevas realizadas aquí**, ni una certificación de todos los estados internos del 2A07.

Los controles adicionales se fundamentan en [NESdev: CPU interrupts](https://www.nesdev.org/wiki/Interrupts), cuyo texto indexado se consultó nuevamente. Distingue detector de nivel, poll y entrega; para instrucciones ordinarias importa la línea del penúltimo ciclo. CLI/SEI/PLP conservan la máscara anterior durante su poll; RTI utiliza la restaurada. Las ramas tienen puntos distintos, con otro poll cuando cruzan página. NMI tiene prioridad si ambas interrupciones están pendientes. Esta documentación no fija por sí sola todas las subfases internas PAL.

Los valores esperados de registros/PC/pila y los ciclos de entrada son consecuencias del programa y del contrato 6502, **no resultados obtenidos de NESRecomp**. No se ejecuta ni copia Mesen2/NESRecomp en esta fase. El test inmediato A02 permanece intacto: las pruebas nuevas no exigen que fuente posterior y bit retornado sean iguales.

## 3. Programas y registro CTest

Se incorporan **11 programas sintéticos y 24 casos internos en 7 entradas CTest**. Hay dos semillas de arranque por variante. Sólo se generan dos archivos de build: `irq_poll_fixtures.c/.hpp`. No se almacenan C generado, binarios ni logs en el repositorio.

La ruta es bytes propios → `emit_blocks.decode` → `emit_blocks.emit_bank` → compilador C99 → runtime C++. No hay intérprete, CPU auxiliar ni dependencia de los experimentos externos. El constructor mantiene vivos los buffers PRG/CHR que referencia CartridgeData. El driver reenvía cada callback una vez, observa RAM sin efectos secundarios y no lee `$4015` para inspeccionar el estado.

**W** es la escritura sincronizada de `$4017=$00`, observada con offset 3. Se verifican las escrituras/lectura/limpieza de sincronización, duración de la instrucción crítica y paridad normalizada. El retardo propio usa cuenta independiente `1+131*N`, más NOP/BIT; se rechazan cruces de página en las ramas contadas. Para 08 se conserva la cuenta de 33.247 ciclos de la ventana original. Para las ramas de control sí se genera expresamente un cruce de página.

| Entrada CTest | Procedencia | Casos | Esperado | Observado |
|---|---|---:|---|---|
| `cpu_irq_poll_pal08_alignment0` | Contrato PAL publicado | 2 | IRQ después de LDA #0; A guardado=0 | **2 fallidos**, A guardado=2 |
| `cpu_irq_poll_pal08_alignment1` | Contrato PAL publicado | 2 | Igual, desplazado un ciclo | **2 fallidos**, A guardado=2 |
| `cpu_irq_poll_pal07` | Contrato PAL publicado | 8 | Bits 0; 1; 1,1; 1,0 | 8 aprobados |
| `cpu_irq_poll_held_clear` | Contrato general CPU | 2 | CLI; LDA `$4015`: conservar decisión IRQ | **2 fallidos**, ninguna entrega |
| `cpu_irq_poll_held_no_clear` | Control general CPU | 2 | CLI; LDA #`$40`: una entrega | 2 aprobados |
| `cpu_irq_poll_held_masked` | Control general CPU | 2 | I=1; lectura: ninguna entrega | 2 aprobados |
| `cpu_irq_poll_branches` | Control general CPU, fuente estable | 6 | BNE no tomada/tomada/cruce: 2/3/4 ciclos; una entrega | 6 aprobados |

Se comprueban A/X/Y/P, PC final, SP restaurado, número de IRQ, ausencia de NMI/DMA accidental y coincidencia de reloj CPU/APU. Cuando hay entrada esperada: vector `$9000`, SP=`$FA`, PC de retorno apilado, P con B=0/U=1, I de entrada, registros y ciclo exacto; también escritura del marcador del manejador. IRQ entra en siete ciclos; STA/BIT/RTI del manejador consume otros trece.

### Resultados concretos antes de corregir

En 08, el PC de retorno esperado es `$802C`, después de LDA #0; se apila **`$802A`**, antes de ejecutarla. A de entrada esperado=0, observado=2. Ambas semillas fallan en ambas alineaciones:

| Alineación | W | Entrada a vector esperada, desde W | Observada | P apilado esperado / observado |
|---:|---|---:|---:|---|
| 0 | 33318 / 33320 | 33264 | **33262** | `$22` / `$22` |
| 1 | 33318 / 33320 | 33265 | **33263** | `$22` / `$20` |

El ciclo del vector se deriva del punto de programa esperado más los siete ciclos de entrada; no se confunde con una medición física del pin. El total del programa permanece correcto: W+33321/33322. El defecto está en el momento y estado de la entrada, no en una suma global distinta de ciclos.

En 07, W=33310: lecturas en W+33254→`$00`, W+33255→`$40`, W+33256 y W+33260→`$40/$40`, W+33257 y W+33261→`$40/$00`. I=1; sin IRQ. La fuente instantánea se registra, **sin imponer una expectativa subciclo no demostrada**.

En IRQ sostenida, W=33314/33316: la fuente está activa al comenzar la instrucción posterior a CLI. LDA `$4015` lee en W+33266, devuelve `$40` y limpia la fuente; debería conservarse la decisión previamente muestreada. Se esperan una entrega, marcador `$40`, PC de retorno `$805E`, entrada W+33274 y final W+33301. Se observan cero entregas, marcador `$A5` y final W+33281. Los veinte ciclos ausentes son entrada más manejador. El control sin lectura entrega correctamente; con I=1 no entrega.

Las ramas pasan con fuente sostenida antes de CLI: retorno `$80D3` sin cruce y `$8102` con cruce, entradas W+33277/33278/33279. **Esto no certifica la llegada/limpieza de IRQ dentro de una rama**: no se introduce una expectativa de hardware para una fase no confirmada.

## 4. Verificación ejecutada

Host: macOS ARM64. Se compilaron los targets existentes y nuevos en escritorio (`/private/tmp/nes-technical-baseline-build`) y headless con mods activados (`/private/tmp/nes-b10-headless-build`). Ambas compilaciones completas finalizaron con **0**. No se ejecutó CI ni Windows/Linux/Intel.

| Ejecución | Entradas | Aprobadas | Fallidas | Omitidas en ejecución | Exit |
|---|---:|---:|---:|---:|---:|
| Nuevas regresiones primero, escritorio | 7 | 4 | 3 | 0 | 8 |
| APU anterior, escritorio | 6 | 5 | 1, A02 | 0 | 8 |
| CPU/bus/PPU anteriores, escritorio | 8 | 8 | 0 | 0 | 0 |
| CTest completo escritorio | **26** | **22** | **4** | **0** | 8 |
| Nuevas regresiones primero, headless | 7 | 4 | 3 | 0 | 8 |
| CTest completo headless | **24** | **20** | **4** | **0** | 8 |

Los tres fallos nuevos son las dos alineaciones 08 y `held_clear`; el cuarto es **A02 preexistente**. Los 24 casos nuevos producen **18 aprobados/6 fallidos** en cada configuración. Los subcasos y las entradas CTest son unidades diferentes; los selectores se solapan con la suite completa y no se suman como cobertura única.

Pasan nuevamente los 4.736 casos de instrucciones, los dos programas de pila, los 14 casos temporizados de bus y las dos regresiones `$2002`. APU core pasa sus 17 grupos, incluidos máscaras CLI/SEI/PLP/RTI y DMC DMA/IRQ con ambas paridades. Pasan A01, A03, límites A03, RAM/MMC1/bus/PPU y NMI de boundary. No aparece un fallo nuevo en las 19 entradas anteriores de escritorio ni en las 17 anteriores headless.

Headless tiene dos entradas menos por las condiciones **ya existentes** de CMake: loader dinámico/ABI y cola SDL sólo pertenecen a escritorio. No se han eliminado ni omitido pruebas para esconder IRQ. El inventario JSON confirma ausencia de `WILL_FAIL`, `DISABLED` y propiedades de skip en A02 y las siete nuevas.

**Problemas del harness de esta fase: cero errores de ejecución**. Un primer intento de build headless apuntó a una carpeta histórica sin `build.ninja` y terminó con 1; era un problema de infraestructura, no del código. Se reutilizó el build headless válido del mismo checkout sin modificar esa carpeta antigua. El log del intento fallido se conserva. La generación independiente repetida produjo C/header idénticos byte a byte. No se presenta ninguna ejecución fallida como aprobada.

No se repitió ADC/SBC exhaustivo, no se probó silicio PAL ni una partida. La prueba PAL 03 del informe anterior no se incorporó ni volvió a ejecutar como programa nuevo porque el alcance solicitado prioriza 07/08 y los controles de CPU.

## 5. Causa y diseño mínimo recomendado, sin implementación

Orden actual comprobado en código:

1. `Bus::read8(address,offset)` sincroniza dispositivos hasta el acceso.
2. `AudioState::read_status` obtiene bit 6 y borra `frame_irq_`; `irq_pending()` comparte ese estado con la fuente de cuadro.
3. `recomp_finish_polled` actualiza PC, avanza los ciclos totales y llama a `boundary` con la máscara I correspondiente.
4. `Bus::boundary` ejecuta DMA pendiente, prioriza NMI y consulta **la fuente IRQ viva en ese instante**.

No hay resultado CPU conservado del muestreo anterior. Por ello puede entregar una afirmación que llegó demasiado tarde para ese poll o cancelar otra que sí estaba presente antes del acceso final. La correcta cuenta total de ciclos no corrige esa pérdida de información.

**Diseño acotado recomendado:** distinguir fuente viva, salida/detector CPU y decisión de la instrucción; conservar historia suficiente durante la sincronización de dispositivos, incluso cuando una lectura avanza varios ciclos; consumir una decisión tomada con la máscara correcta al terminar la instrucción, sin reconstruirla a partir de la fuente posterior. La limpieza del registro no debe borrar una decisión ya tomada. Tampoco debe mantenerse un latch permanente de «hubo IRQ alguna vez» ni introducir un guard de calendario para 33252.

CLI/SEI/PLP ya transmiten I anterior desde `emit_semantics`; RTI transmite I restaurada mediante el finish normal. Debe conservarse esa distinción y separar máscara del punto temporal. Los tests anteriores validan esos controles actuales, pero no todas las carreras en el onset PAL. La segunda alineación 08 exige además fijar la propagación entre flag APU y detector: no basta elegir sin demostración «último ciclo menos uno» sobre el booleano compartido.

### ¿Puede mantenerse la ABI v2?

**Para los casos ordinarios, hay una vía plausible dentro de Bus**: offsets de acceso y `advance(total)` permiten reconstruir la ventana de la instrucción y guardar sus muestras anteriores. No exige necesariamente cambiar CPUState o la estructura de servicios. Su orden de fase debe validarse contra ambas alineaciones y la pérdida sostenida antes de implementarse.

**Para una corrección general con ramas, la ABI actual no aporta toda la información necesaria.** Ejemplo: BNE tomada sin cruce y JMP absoluto pueden emitir únicamente `advance(3)` y `boundary` con idéntico PC de destino/máscara/registros, sin lecturas explícitas de opcode. Sus puntos de poll son diferentes. No pueden distinguirse con duración y estado final solamente; deducirlo mediante un decoder nuevo de ROM sería una solución ajena al objetivo.

Se recomienda un pequeño descriptor estático de poll producido por el emisor para esas instrucciones, incluyendo múltiples puntos cuando corresponda. El runtime consulta su historia sin retroceder el reloj ni duplicar avance. Debe elegirse cómo transmitirlo **antes** de corregir: ampliar callback o codificarlo en un campo existente cambia el contrato semántico, aunque la firma binaria pudiera permanecer igual. Los módulos v2 antiguos no tendrían ese descriptor; no puede afirmarse compatibilidad completa y silenciosa sin comprobarlos o invalidarlos.

Por tanto: **no se garantiza aún una corrección completa sin cambiar el contrato ABI**. La modificación mínima conceptual está identificada; falta cerrar el transporte de información de ramas y su compatibilidad. Esta fase no toma esa decisión ni cambia versiones/cachés.

### Módulos afectados por una futura corrección

| Módulo | Necesidad potencial |
|---|---|
| `runtime/include/runtime/bus.hpp`, `runtime/src/bus.cpp` | Historia/detector, decisión CPU y consumo en boundary; distinguir avance de instrucción de entrada/DMA. |
| `runtime/include/runtime/audio_state.hpp`, `runtime/src/audio_state.cpp` | Si el contrato de propagación lo requiere, separar flag legible de salida de IRQ; conservar temporización de `$4017`. No se presupone que sea necesario modificar ambos ahora. |
| `tools/recompiler/emit_blocks.py`, `include/recomp/runtime_services.h` | Descriptores de poll para casos que el protocolo actual no distingue. |
| `include/recomp/module_abi.h`, importador/caché/loader | Sólo si cambia el contrato: versión/invalidation para evitar módulos antiguos silenciosamente incompatibles. |

## 6. Riesgos y criterios de aceptación futuros

- **A01:** no actualizar bus abierto externo al leer `$4015`; no añadir lecturas observacionales con efectos secundarios.
- **A03:** conservar inhibición inmediata, reset de modo diferido por paridad y los eventos adyacentes de `$4017`.
- **NMI:** conservar edge pendiente y prioridad; no volver a entregar un IRQ previamente descartado por NMI. La entrada actual tiene vector fijo y no modela todos los hijacks; el test de boundary no certifica colisiones subciclo.
- **DMA:** OAM/DMC avanzan dispositivos dentro de boundary antes de entregar interrupciones. No confundir esos ciclos con un nuevo poll ordinario ni sobrescribir la decisión de la instrucción; validar qué debe pasar durante stalls. DMC básico pasa; no hay prueba nueva de OAM/DMC/NMI/IRQ simultáneos.
- **Ramas/ABI:** las ramas estables pasan, pero las llegadas entre sus puntos de poll quedan pendientes. No convertir una ampliación semántica en módulos viejos aceptados por tener la misma estructura C.

Aceptación mínima de la futura corrección:

1. Las siete entradas nuevas y los 24 casos pasan con sus expectativas actuales, incluidos PC/pila/ciclos, sin excepciones de calendario.
2. A01/A03 y las pruebas anteriores de CPU/bus/PPU/interrupciones permanecen correctas; IRQ no se entrega con I=1 ni antes de su poll.
3. Añadir antes del cambio los casos de llegada/limpieza/inhibición antes y después de cada punto de poll de ramas, y máscaras CLI/SEI/PLP/RTI; cuando falte evidencia, declararla expresamente.
4. Probar prioridad NMI y stalls DMA sobre una decisión pendiente, distinguiendo límites del modelo actual; no prometer precisión subciclo completa.
5. Revisar la expectativa inmediata de A02 únicamente con autorización y contrato separado de flag/fuente/CPU. **Pasar estas regresiones no autoriza por sí solo cambiar A02.**
6. Si cambia el protocolo, probar rechazo/invalidation de módulos antiguos. No afirmar ABI intacta únicamente porque no cambie `sizeof(RuntimeServices)`.

## 7. Comandos y evidencias

Desde el checkout, la generación es automática y no necesita ROM:

```sh
cmake --build /private/tmp/nes-technical-baseline-build --target cpu_irq_poll_checks -j 4
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^cpu_irq_poll_' --output-on-failure --verbose --no-tests=error
# 7 entradas: 4 PASS / 3 FAIL; exit 8.
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^apu_' --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^(native_|synthetic_|ppustatus_)' --output-on-failure --verbose --no-tests=error
cmake --build /private/tmp/nes-technical-baseline-build -j 4
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error
# 26 entradas: 22 PASS / 4 FAIL; exit 8.
cmake --build /private/tmp/nes-b10-headless-build -j 4
ctest --test-dir /private/tmp/nes-b10-headless-build -R '^cpu_irq_poll_' --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-b10-headless-build --output-on-failure --verbose --no-tests=error
# 24 entradas: 20 PASS / 4 FAIL; exit 8.
```

En un checkout nuevo: `cmake -S . -B build -DBUILD_TESTING=ON`, `cmake --build build`, `ctest --test-dir build --output-on-failure`. Para headless configurar otro build con `-DNES_ENABLE_DESKTOP=OFF`. El target y las siete entradas se incluyen automáticamente en ambas configuraciones.

Para reproducir sólo la pérdida: `cpu_irq_poll_checks held_clear` desde el build, exit **1**; para las dos alineaciones usar `pal08_alignment0` y `pal08_alignment1`, exit **1**. Selector desconocido produce exit **2**, no una ejecución vacía aprobada. Los filtros anteriores seleccionan diagnósticos; **las suites completas se ejecutaron sin filtros**.

Los logs, JUnit, inventarios y snapshots se guardan fuera del checkout público en `<workspace>/outputs/cpu-irq-poll-regressions-2026-10-11/`. Incluyen el intento de build no viable y los resultados correctos del build válido. Las invocaciones registradas de CTest añaden `--output-junit` hacia sus archivos de evidencia.

La comparación final contra los 125 archivos preexistentes sólo detecta el cambio autorizado de integración en `CMakeLists.txt`. Los demás conservan sus hashes iniciales; rama y HEAD permanecen iguales. `git diff --check` termina con 0. Los archivos nuevos son las dos pruebas y este informe.

**Estado final:** batería integrada y defectos visibles; no se implementa la corrección. El siguiente paso es acordar el descriptor/contrato de muestreo y las regresiones de ramas antes de tocar producción.

Publication review: host-specific personal paths were replaced by `<checkout>` / `<workspace>` placeholders. Original reports and execution evidence remain preserved privately; historical results were not changed.
