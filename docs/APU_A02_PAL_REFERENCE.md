# A02 — Referencia independiente de sincronización PAL

Fecha: **2026-10-11**. Resultado: **A02 pendiente; no se modifica producción ni su expectativa**. Hay evidencia PAL publicada sobre lecturas y entrada al manejador, pero no demuestra la igualdad inmediata entre el flag legible y la fuente IRQ exigida por A02. Las nuevas sondas descubren además una discrepancia de muestreo de CPU que debe investigarse por separado antes de decidir una corrección.

## 1. Checkout y conservación

- Repositorio activo: `outputs/public-alpha-0.2.0/addams-family-recompiler`, dentro del workspace de esta conversación. El nombre del directorio es histórico.
- Rama: **verify/phase-3-1-bus-timing**.
- HEAD: **35f5ce38562fcc62d47c8dcd392517bd203a86d1**; origin: `https://github.com/inthe92/addams-family-recompiler.git`.
- Estado inicial: ningún cambio tracked/staged; tres documentos untracked: `APU_A02_DIAGNOSIS.md`, `NESRECOMP_COMPARISON.md`, `RELEASE_1_0_READINESS.md`. Se conservan íntegros.
- Contexto contrastado: diagnóstico A02, auditoría APU, comparación NESRecomp, `audio_state.cpp`, `bus.cpp`, `apu_audio_checks.cpp` y contrato de servicios. Los informes anteriores son históricos: la auditoría original enumeraba A01/A03 como fallos antes de sus correcciones.
- Único archivo añadido al repositorio: **este informe**. Experimentos, código sintético, fuentes externas y logs están en el workspace circundante, `outputs/a02-pal-reference-2026-10-11/`, fuera del repositorio público.
- Se reutiliza el build Release macOS ARM64 `/private/tmp/nes-technical-baseline-build`. Los siete archivos relevantes de runtime/bus/emisor/ABI/test coinciden por SHA-256 con el snapshot del diagnóstico anterior. El diff de fuentes pertinentes entre `64dd1064d4a24241554f2ce5d94aa92255bd688f` y HEAD está vacío. No se presenta como una compilación completa nueva de HEAD.

No se cambia de rama, no se instala ninguna dependencia, no se usa ROM comercial y no se hace commit, push ni modificación de la alpha publicada.

## 2. Seis observables diferentes

| Observable | Nuestro runtime | NESRecomp de ciclo | Mesen2, inspección de fuente |
|---|---|---|---|
| 1. Bit 6 devuelto por $4015 | Muestreo de `frame_irq_` antes de borrarlo | `apu.frame_irq` | `ApuFrameCounter::GetIrqFlag()` |
| 2. Flag interno legible después | Mismo booleano, borrado inmediatamente | Flag separado; limpieza pendiente | `_irqFlag` y `_irqFlagClearClock` separados |
| 3. Fuente IRQ de la APU | `irq_pending()` = frame OR DMC | `frame_irq_out` OR DMC | `IRQSource::FrameCounter` en CPU |
| 4. Línea observada por CPU | No existe un estado separado de línea física/muestreada | `hw.irq_line`, actualizado en tick PAL 9 | Fuentes CPU y muestreo en `EndCpuCycle` |
| 5. Resultado/instante del poll | Consulta de fuente actual en `Bus::boundary` después de advance; máscara I explícita | `cpu_poll` antes del acceso, guarda `cpu.do_irq` | `_prevRunIrq` guarda resultado anterior; `Exec` lo utiliza |
| 6. Final de instrucción | `recomp_finish`: PC, avance total, boundary | `CYC_DONE`, siguiente fetch consulta latch | Final de `Exec`, después de ciclos de la instrucción |

En las tablas, IRQ=1 significa afirmación lógica; el pin físico /IRQ es activo bajo. No se representa su voltaje medido.

En nuestras sondas, DMC está desactivado, por lo que `irq_pending()` equivale al único booleano frame. **Es un alias deducible del código, no una segunda medición independiente del flag.** No se inventa una línea CPU o un latch que nuestro modelo no contiene. Un `$40` retornado tampoco obliga a que los observables 2–5 valgan 1.

**Convenciones temporales.** PAL utiliza 16 relojes maestros por ciclo CPU y 5 por dot PPU; get/put alternan ciclos CPU completos (un par ocupa dos ciclos), mientras φ1/φ2 son fases dentro del ciclo CPU. Ninguno es sinónimo de un tick maestro. La paridad del contador CPU depende de su origen; no se identifica automáticamente con get/put. [NESdev: ciclos](https://www.nesdev.org/wiki/Cycle_reference_chart).

Nuestro offset entero no especifica la fase maestra del acceso. Mesen PAL divide el ciclo en 8+8 ticks y realiza avance/dispositivos antes del acceso; mantiene historial al terminar cada ciclo. NESRecomp ejecuta sus ticks restantes al comenzar el próximo acceso: `hw_cycle_finish` deja tick=1; el próximo `hw_cycle_start` completa hasta 16 y vuelve a tick=0. Su poll usa la línea muestreada durante el ciclo anterior. Su `hw_cycles`, contador APU y `hw.tick` no son intercambiables con nuestro anchor ni con `AudioState::cycles()`.

En las tablas, **T es el índice no envuelto del secuenciador al observar la lectura**, no una afirmación de igualdad de reloj maestro entre implementaciones. En el runtime fresh T sí coincide con ciclos CPU avanzados. NESRecomp arranca con contador APU=1 y `hw.cycles=0`; tras el tercer evento el contador visible se envuelve a 0. Tras escribir $4017, se normaliza cada ejecución a su propio origen de secuenciador y se conserva su contador absoluto en JSONL. No se alinea una referencia sólo buscando un `$40`.

## 3. Fuentes y fuerza de la evidencia

### PAL real: pruebas publicadas de Shay Green/blargg

Fuente fijada en **christopherpow/nes-test-roms@95d8f621ae55cee0d09b91519a8989ae0e64753b**. Se descargan únicamente texto y ensamblador, **ningún .nes**.

El [readme PAL](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/readme.txt) declara que las pruebas fueron ejecutadas y aprobadas en una NES PAL. Distingue tres afirmaciones consecutivas del flag, empezando a 33.255 ciclos desde una escritura sincronizada de $4017=$00, y una entrada mínima al manejador a 33.257 desde esa escritura. Es una declaración del autor sobre hardware, no una ejecución repetida aquí ni una traza interna de silicio. No identifica en ese documento la revisión concreta del chip.

- [07.irq_flag_timing.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/07.irq_flag_timing.a): lectura a W+33.254 debe dar bit 6=0; W+33.255 debe dar 1. Una lectura a W+33.256 seguida de otra LDA debe observar reafirmación; empezar en W+33.257 debe limpiar sin nueva reafirmación. Son lecturas CPU separadas por instrucciones, no una consulta inmediata del pin IRQ.
- [08.irq_timing.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/08.irq_timing.a): dos alineaciones de instrucción, separadas por un ciclo, verifican qué valor de A guarda el manejador. Observa entrada efectiva, no el estado de `irq_pending()` tras leer.
- [03.irq_flag.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/03.irq_flag.a) comprueba limpieza por lectura y por inhibición; [sync_apu.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/sync_apu.a) elimina jitter del divisor mediante lecturas y una rama con diferencia de un ciclo. Sin esa sincronización, W+33.255 no debe trasladarse sin más a T=33.252.

Estas fuentes **mejoran el diagnóstico anterior**: sí existe una referencia PAL declarada como probada en hardware para resultados de programa. Falta el caso exacto de colisión con observación separada de flag, fuente, pin y latch; no falta toda evidencia PAL.

La [medición PAL de thefox, 2015](https://forums.nesdev.org/viewtopic.php?t=13604) obtuvo un intervalo aproximado de 33.252 ciclos, pero explícitamente no sincronizó con precisión la APU. Confirma frecuencia aproximada, no la carrera. El [experimento original NTSC de blargg](https://forums.nesdev.org/viewtopic.php?t=635) aporta metodología y resultados NTSC; **no se convierte en demostración PAL**.

[NESdev APU/status](https://www.nesdev.org/wiki/NES_APU) documenta lectura que limpia frame IRQ, excepción de afirmación coincidente y bit legible. La página no especifica una igualdad inmediata con el latch de CPU para nuestra fase entera. [NESdev interrupciones](https://www.nesdev.org/wiki/Interrupts) distingue detector y decisión de instrucción: importa el muestreo anterior al último ciclo, con excepciones de ramas. Se consultaron textos indexados cuando la recuperación directa de NESdev falló.

### Mesen2: implementación independiente, no ejecutada

Commit **b9fa69ddc6d0a331fb103fdb5eef6904305703c2**. Los tres archivos cached coinciden con sus blobs del commit, comprobado mediante GitHub API (`mesen-source-integrity.json`).

- [ApuFrameCounter.h](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/APU/ApuFrameCounter.h): `Run` usa los eventos PAL 33252/33253/33254; `GetIrqFlag` mantiene flag y limpieza diferida en reloj maestro; `WriteRam` inhibe inmediatamente la fuente y el flag, demora reset según paridad.
- [NesApu.cpp:GetStatus/ReadRam](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/APU/NesApu.cpp): avanza APU, obtiene el bit y limpia **incondicionalmente la fuente FrameCounter** al leer $4015. El flag retornado y la fuente posterior pueden diferir. Por inspección, esto contradice la supuesta necesidad universal de fuente inmediata=1; no demuestra nuestro clearing compartido.
- [NesCpu.cpp:StartCpuCycle/EndCpuCycle/Exec](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/NesCpu.cpp): el historial `_prevRunIrq` puede mantener una decisión aun cuando la fuente haya cambiado al terminar la instrucción.

El código de limpieza remite a AccuracyCoin, cuya referencia de hardware es NTSC. No se ejecuta Mesen2 ni AccuracyCoin; no se atribuye a PAL un resultado NTSC. La expresión `masterClock & 1` de Mesen no se traduce automáticamente a nuestra paridad de CPU.

### NESRecomp: referencia de software ejecutada

Checkout independiente y sin modificaciones, commit **2b4fcdb98c9e702be3fb365cfc645003c50b5fc1**.

- [hw_apu.c](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/runner/cyc/hw_apu.c): `apu_read_status`, `apu_cycle`, `apu_sample_irq`, `apu_irq_output` separan flag, solicitud de limpieza, salida y línea muestreada. Limpieza en la rama `apu.put`; eventos pueden reafirmar después. En `apu_sample_irq` se copia salida a línea **antes** de propagar el flag a salida.
- [hw_machine.c](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/runner/cyc/hw_machine.c): `run_pal_tick` reloj APU en tick 0 y muestreo IRQ en 9; start/finish conservan fases.
- [cpu6502.h](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/runner/cyc/cpu6502.h): `cpu_poll`, `cpu_read`, `cpu_fetch_rom` conservan `do_irq` hasta la consulta del siguiente fetch. La secuencia observacional fija LDA reproduce los accesos del caso $AD en [cpu6502_interp.c](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/runner/cyc/cpu6502_interp.c).

Se enlazan objetos existentes con un driver de observación externo, no se añade intérprete ni se utiliza fallback en el experimento. Su test `region_test.c` comprueba eventos PAL con callbacks directos, pero no certifica esta carrera completa. La sonda anterior de comparación llamaba `apu_sample_irq` manualmente: **las nuevas sondas no lo hacen**, utilizan el reloj real de este backend. Coincidir con otra implementación no certifica hardware ni independencia de sus supuestos.

### MiSTer: útil para distinguir estados, no un oráculo PAL certificado

**MiSTer-devel/NES_MiSTer@49a0a662e244469ca77b2155746a066df704ffae**, [rtl/apu.sv:FrameCtr](https://github.com/MiSTer-devel/NES_MiSTer/blob/49a0a662e244469ca77b2155746a066df704ffae/rtl/apu.sv). Se inspecciona RTL sin instalar Verilator ni ejecutarlo. Distingue `FrameInterrupt`, `frame_interrupt_buffer`, `irq` y `irq_flag`, con fases `aclk1/aclk2` y prioridad entre lectura, afirmación e inhibición. **Su propio comentario identifica los valores LFSR PAL como especulativos**, basados en ciclos observados y en lógica/netlists 2A03. No constituye medición nueva de un 2A07.

El [harness MiSTer de NESRecomp](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/tools/cyc/mister_apu/apu_harness.sv) fija `PAL=0` y reloj CPU de 12 ticks. Sus coincidencias NTSC no validan A02 PAL. No se ejecuta ni se presenta como una segunda prueba de hardware independiente.

## 4. Experimentos ejecutados y tabla temporal

Host real: macOS ARM64. **36 schedules nuestros y 36 externos**, cada uno con estado reiniciado. Se observan T=33250–33255: fresh con I=0/1, reset $4017=$00 en ambas paridades, inhibición $4017=$40 en ambas paridades. Nuestra ruta compila y ejecuta C producido por el `emit_bank` actual a partir de tres bytes **autorizados sintéticos: AD 15 40**, con IRQ vector sintético $9000. El driver reenvía cada callback una vez y registra tras lectura, tras avance, antes/después de boundary y retorno. No se lee extra $4015 para observar estado.

La ruta externa ejecuta la secuencia fija fetch/operandos/lectura con los helpers originales, registra `counter`, `put`, flag, fuente, línea y latch antes/después del poll y la lectura, al finish y en el siguiente fetch. Si no hay IRQ inmediata, ejecuta los accesos de un NOP y observa su poll; si hay, ejecuta entrada IRQ. **Es una sonda de máquina/ciclos, no una validación nueva del generador externo ni una partida.**

Los resets se escriben con contador CPU externo 10/11 y `put` observado 0/1; los dos delays convergen al mismo origen posterior en ese modelo. Fresh y reset proporcionan además ambas fases `put` para los mismos índices del secuenciador. Nuestro modelo sólo ofrece paridad de ciclos, no un registro explícito get/put. Las lecturas consecutivas T cubren paridades alternas. No se fuerzan fases escribiendo estado privado.

### Nuestro C generado y runtime, I=0

| T de lectura | $4015 | Flag/fuente inmediatamente después | Fuente tras completar 4 ciclos | IRQ entregada por boundary (I=0) | PC tras boundary |
|---:|---:|---:|---:|---:|---:|
| 33,250 | `$00` | 0 | 0 | 0 | `$8003` |
| 33,251 | `$00` | 0 | 1 | 1 | `$9000` |
| 33,252 | `$40` | 0 | 1 | 1 | `$9000` |
| 33,253 | `$40` | 0 | 1 | 1 | `$9000` |
| 33,254 | `$40` | 0 | 0 | 0 | `$8003` |
| 33,255 | `$40` | 0 | 0 | 0 | `$8003` |

Fuente tras completar 4 ciclos se registra **antes de entregar interrupciones**. Boundary, si entrega, añade 7 ciclos y tres bytes de pila; PC cambia a $9000 y SP de $FD a $FA. Por eso el retorno final de esos schedules consume 11 ciclos, no porque LDA haya cambiado de duración. Con I=1, los seis schedules fresh retornan PC=$8003, SP=$FD, cero IRQ; A=$00 antes del evento y $40 desde T=33252. Los JSONL conservan P y ambos relojes antes y después.

Caso exacto A02: anchor=33249, lectura/estado APU=33252, valor=$40, fuente inmediata=0. Final de LDA: 33253, A=$40, fuente=1. Con I=0 nuestro boundary entrega IRQ; con I=1 no. **La foto inmediata de A02 no demuestra una interrupción perdida en ese schedule.**

### NESRecomp PAL, I=0, origen fresh propio

| T del secuenciador | get/put al leer (put) | $4015 | Flag después | Fuente después | Línea muestreada después | Latch CPU después del poll | IRQ en siguiente fetch |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 33,250 | 1 | `$00` | 0 | 0 | 0 | 0 | 0 |
| 33,251 | 0 | `$00` | 0 | 0 | 0 | 0 | 0 |
| 33,252 | 1 | `$40` | 1 | 1 | 0 | 0 | 0 |
| 33,253 | 0 | `$40` | 1 | 1 | 1 | 1 | 1 |
| 33,254 | 1 | `$40` | 1 | 1 | 1 | 1 | 1 |
| 33,255 | 0 | `$40` | 1 | 1 | 1 | 1 | 1 |

`put` es el estado del backend al observar, no una asignación universal de paridad. En T=33254 el contador ya vale 0. Al finalizar esa LDA, flag=0 y fuente=0, pero línea muestreada=1 y latch=1; en el siguiente fetch, incluso con línea ya=0, se inicia IRQ. Es un ejemplo ejecutado de por qué los seis observables no son equivalentes.

En T=33252 la fuente puede ser 1 y la línea muestreada 0; el latch sigue 0 al terminar LDA. Tras el NOP siguiente, latch=1. La tabla no afirma que el backend haya simulado un instante de silicio equivalente a nuestro callback: expone sus fases y su decisión.

**Diferencia adicional reproducida entre modelos:** en T=33251/33252 nuestro boundary entrega inmediatamente; NESRecomp no entrega en ese siguiente fetch. En T=33254/33255 nuestro boundary no entrega; NESRecomp conserva una IRQ ya muestreada y sí entra. Esto apunta a muestreo tardío/ausencia de historial en nuestra CPU de boundary, además del clearing APU. No se demuestra una pérdida física en PAL mediante comparación de software, y no se corrige en esta tarea.

### Inhibición y paridad

Con $4017=$40 antes del recorrido y en ambas paridades, nuestro runtime retorna bit 6=0, fuente=0 y no entrega IRQ en los doce schedules. NESRecomp retorna bit 6=1 en T=33252/33253 incluso inhibido, pero **fuente=0, línea=0, latch=0 y no entrada IRQ**; en T=33254 retorna 0. Esta diferencia también aparece estructuralmente en `Mesen::Run`, que mantiene flag transitorio aun inhibido y lo borra en el último paso.

Es otra diferencia software que exige prueba PAL específica. El test PAL básico de inhibición observa fuera de esa ventana, por lo que no la resuelve. No se declara que nuestro resultado o el externo sean correctos sólo por coincidencia con Mesen. No se investigan DMC/mapper ni IRQ simultáneas en esta fase.

## 5. Resultado verificable y límites

| Comprobación de esta tarea | Ejecutada | Resultado |
|---|---:|---|
| CTest `apu_status_irq_collision_reference` sin filtros internos ni cambio de expectativa | 1 entrada / 1 caso | **FAIL**, CTest exit 8, binario exit 1; ciclo 33252/status64/IRQ0 frente a esperado1 |
| C sintético de nuestro emisor + driver observacional | 36 schedules, 276 snapshots | Compilado/enlazado y ejecutado, exit 0; sin assertions de fidelidad de hardware |
| Driver de fases NESRecomp + objetos existentes | 36 schedules, 336 snapshots | Compilado/enlazado y ejecutado, exit 0; cero fallos de helpers de fixture |
| Integridad de tres fuentes Mesen | 3 blobs | Coinciden exactamente con commit fijado |
| Mesen completo / RTL MiSTer / programas PAL de blargg | 0 ejecuciones | Sólo inspección; no se ensamblan ni descargan ROMs de test |
| PAL físico / pin IRQ / reloj maestro medido | 0 ejecuciones | **Pendiente**, sin equipo/traza local |
| CTest completo, headless y cuatro plataformas | No repetidos | Fuentes/configuración intactas; no se presentan resultados anteriores como nuevos |

Los 72 schedules son **experimentos observacionales**, no 72 tests de precisión aprobados. Se obtienen 612 snapshots. A02 sigue siendo el único caso de contrato formal ejecutado aquí y sigue fallando. Los compiladores de los drivers terminan con 0; el enlace externo conserva una advertencia de alineación de sección `__DATA,__common`, sin fallo ni modificación de librerías. Una primera compilación del driver externo requirió añadir el include original `cpu6502.h` al driver, fuera de ambos repositorios.

Evidencia local: `starting-state.json`, `final-state.json`, `a02-ctest.log`, `synthetic_lda.c`, `our_probe.cpp`, `our-observations.jsonl`, `nesrecomp_phase_probe.c`, `reference-observations.jsonl`, `commands.txt`, stderr de ambas sondas, fuentes PAL/MiSTer y `mesen-source-integrity.json`. Los snapshots públicos existentes no se sobrescriben. Ningún byte del juego comercial está en los experimentos.

## 6. Respuestas expresas sobre A02

1. **¿`irq_pending()==true` inmediatamente está justificado?** No como expectativa general en la fase actual. La lectura bit6=1 está fundamentada en el evento de la fixture, pero la excepción del flag legible no especifica nuestro accessor de fuente. Mesen limpia esa fuente después de obtener el bit; NESRecomp mantiene una salida diferida y una línea distinta. La referencia PAL publicada mide resultados de programa, no ese booleano instantáneo.
2. **¿Qué debería comprobar A02?** Un contrato con fase/origen definidos: bit retornado, resultado de una segunda lectura programada, transición de la fuente y decisión de IRQ en una instrucción real con I=0/1. Flag y fuente deben comprobarse por separado; entrada al manejador se observa mediante vector/pila/ciclos. El test existente permanece intacto hasta autorizar y fundamentar ese reemplazo/ampliación.
3. **¿Mezcla observables?** Sí. Usa el mismo booleano de producción para flag/fuente y supone que la excepción de lectura garantiza fuente persistente. No observa pin, poll ni final de instrucción; no ejecuta una CPU en ese caso.
4. **¿Hay evidencia de IRQ realmente perdida por CPU?** En el schedule exacto T=33252, no: la sonda nativa entrega tras reafirmación a 33253. Hay una omisión **respecto al backend externo** en T=33254/33255, donde éste conserva el poll anterior; es evidencia de una divergencia software concreta y de un riesgo de pérdida, no una medición de interrupción perdida en una NES PAL real ni una partida.
5. **¿Separar flag legible y fuente IRQ?** Sí como distinción de contrato: fuentes y experimentos demuestran que no son el mismo observable. Eso no basta por sí solo: una decisión CPU ya muestreada también debe distinguirse de la fuente actual. Mantener un booleano compartido mediante una excepción de ciclo no resuelve ambos riesgos.
6. **¿Hay corrección segura definida?** Todavía no para cerrar A02 tal como está. Podemos especificar pruebas externas PAL fundamentadas y reconocer estados distintos; falta fijar el mapeo de fase/poll de nuestra ABI y confirmar el caso PAL exacto de lectura/limpieza/inhibición. No se selecciona entre Mesen y NESRecomp para hacer verde CI. **A02 continúa pendiente y visible.**

## 7. Trabajo mínimo necesario antes de cambiar producción

1. Reproducir los casos PAL publicados `07.irq_flag_timing` y `08.irq_timing`, con programas sintéticos equivalentes y origen sincronizado W explícito; preservar sus resultados de flag separado y entrada IRQ. Primero puede hacerse en un harness temporal usando el recompilador existente, sin incorporar un intérprete.
2. Preparar una prueba PAL específica de colisión: barrer W alrededor de los tres eventos, ambas fases de escritura $4017, dos LDA sucesivas, I=0/1, inhibición previa y en ciclos adyacentes. Guardar bytes retornados y marcador del manejador **antes y después** de la lectura. Debe incluir T=33254/33255, donde las sondas detectan pérdida respecto a un latch anterior.
3. Ejecutar ese mismo programa en un RP2A07 real, registrar revisión/origen de reloj y resultados. Para decidir el pin instantáneo o prioridad subciclo, acompañar con captura M2/reloj/IRQ; los estados internos no se leen directamente en hardware, se infieren con sondas de programa cuya perturbación esté controlada. Un programa PAL autoral probado sin traza ya mejora la evidencia, pero no certificaría todos los ticks maestros.
4. Mapear explícitamente acceso/poll/final a las convenciones de ABI v2 y a las tablas W de blargg. Resolver cómo conservar un poll previo sin inventar un IRQ por mirar únicamente la fuente al final. Comparar Mesen completo sólo si aporta un caso nuevo; no contar dos modelos coincidentes como dos mediciones de hardware.
5. Con el contrato acordado, autorizar una corrección acotada y regresiones separadas de flag, fuente, historial CPU e inhibición. Conservar A01/A03, ciclos/pila/vectores y casos tardíos. Hasta entonces, no modificar A02 ni usar excepciones CTest.

**Propuesta conceptual, no implementación aprobada:** distinguir latch legible de salida IRQ dentro de AudioState y resultado de poll CPU de fuente actual. La separación está fundamentada; las reglas exactas de limpieza y el punto de muestreo de nuestra instrucción todavía no. No se propone copiar RTL, usar un guard `if(ciclo==33252)` ni reescribir el planificador. Módulos potencialmente afectados tras validar contrato: AudioState, Bus/boundary y tests de acceso/IRQ; una necesidad de cambio ABI debe analizarse por separado.

## 8. Reproducción y estado final

```sh
ctest --test-dir /private/tmp/nes-technical-baseline-build   -R '^apu_status_irq_collision_reference$' --output-on-failure --verbose --no-tests=error
# Resultado observado: FAIL, exit 8. No WILL_FAIL, DISABLED, xfail ni exclusión.
```

Para reproducir las sondas locales, `commands.txt` conserva los comandos absolutos de compilación/enlace usados con los objetos existentes; después ejecutar `our_probe` y `nesrecomp_phase_probe` en la carpeta de evidencia. Los fuentes y logs están fuera del repositorio por alcance/licencias; este documento no pretende que sus binarios locales estén disponibles en CI o en otro equipo.

**Final:** investigación concluida dentro del alcance; A02 no resuelta. No se modifica producción, emisor, ABI, tests, workflow ni publicación. Siguiente paso recomendado: fijar y ejecutar la prueba sintética equivalente a las referencias PAL de lecturas/entrada IRQ, seguida de la colisión concreta en hardware PAL, antes de escoger una corrección.
