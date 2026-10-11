# A02 — programas PAL de flag e interrupción

Fecha: 2026-10-11. Investigación terminada; **ninguna corrección implementada**. A02 permanece fallando y visible.

## 1. Estado y alcance

Checkout: `<checkout>`.

- Rama: `verify/phase-3-1-bus-timing`.
- HEAD: **35f5ce38562fcc62d47c8dcd392517bd203a86d1**.
- Origin: `https://github.com/inthe92/addams-family-recompiler.git`.
- Estado inicial: tracked/staged sin cambios; cuatro informes untracked: `APU_A02_DIAGNOSIS.md`, `APU_A02_PAL_REFERENCE.md`, `NESRECOMP_COMPARISON.md`, `RELEASE_1_0_READINESS.md`.
- Se leyeron íntegramente diagnóstico y referencia PAL. No se cambia de rama, no se modifica producción, ABI, pruebas existentes ni CI; tampoco commits, pushes o publicaciones.
- Único archivo añadido al repositorio: este informe. Los experimentos están fuera del checkout público, en `<workspace>/outputs/a02-pal-programs-2026-10-11/`.

Host ejecutado: **macOS 27.0.1 / ARM64**. Se reutiliza `/private/tmp/nes-technical-baseline-build`, Release, escritorio. No se recompila el juego ni se utiliza ROM comercial. Los siete archivos pertinentes de runtime/bus/emisor/ABI/test coinciden por SHA-256 con el snapshot del diagnóstico que utilizó este build; véase `reused-build-provenance.json`. Esto no constituye una compilación completa nueva de HEAD ni una validación multiplataforma.

## 2. Referencias y fuerza de la evidencia

Referencia principal: **christopherpow/nes-test-roms@95d8f621ae55cee0d09b91519a8989ae0e64753b**, fuentes PAL de Shay Green/blargg:

- [03.irq_flag.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/03.irq_flag.a).
- [07.irq_flag_timing.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/07.irq_flag_timing.a).
- [08.irq_timing.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/08.irq_timing.a).
- [sync_apu.a](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/source/sync_apu.a).

El [readme PAL](https://github.com/christopherpow/nes-test-roms/blob/95d8f621ae55cee0d09b91519a8989ae0e64753b/pal_apu_tests/readme.txt) declara que esas pruebas se ejecutaron y aprobaron en una NES PAL. Es evidencia publicada del autor, **no una nueva medición nuestra**. No especifica revisión del chip ni proporciona una traza de fases internas. Se reutilizaron las fuentes locales cacheadas, comprobando nuevamente sus cinco blobs Git contra el commit fijado (`reference-integrity.json`); no se descargaron archivos `.nes`.

[NESdev, CPU interrupts](https://www.nesdev.org/wiki/Interrupts), consultado mediante texto indexado porque el acceso directo devolvió 403, distingue el detector de nivel y el poll de instrucción. El detector observa φ2; su resultado aparece en φ1 siguiente. Para las instrucciones ordinarias importa la línea del penúltimo ciclo, aunque la decisión se consume al finalizar. CLI conserva la inhibición anterior durante su propio poll. Las ramas tienen puntos de muestreo particulares. Esta documentación respalda el control de IRQ sostenida descrito abajo; no es una medición de la prioridad interna de lectura del 2A07 PAL.

Referencias adicionales conservadas del informe anterior:

| Proyecto y commit | Archivos / comportamiento relevante | Validación disponible |
|---|---|---|
| Mesen2 `b9fa69ddc6d0a331fb103fdb5eef6904305703c2` | [ApuFrameCounter.h](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/APU/ApuFrameCounter.h), `GetIrqFlag`; [NesApu.cpp](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/APU/NesApu.cpp), `GetStatus`; [NesCpu.cpp](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/NES/NesCpu.cpp), `EndCpuCycle/Exec` | Inspección de fuentes: flag separado de fuente; lectura limpia la fuente y CPU conserva historial `_prevRunIrq`. No ejecutado aquí. |
| NESRecomp `2b4fcdb98c9e702be3fb365cfc645003c50b5fc1` | [hw_apu.c](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/runner/cyc/hw_apu.c), `apu_read_status/apu_sample_irq`; [cpu6502.h](https://github.com/mstan/nesrecomp/blob/2b4fcdb98c9e702be3fb365cfc645003c50b5fc1/runner/cyc/cpu6502.h), `cpu_poll` | Sonda ejecutada en la fase anterior: separa flag, salida, línea y decisión CPU. No se ejecutaron estos nuevos programas sobre NESRecomp. |

La comparación anterior mostró entrega inmediata nuestra y diferida externa en T=33251/33252, y falta de entrega nuestra frente a una decisión externa conservada en T=33254/33255. Sus ticks PAL, get/put y origen no son intercambiables con nuestros offsets. La coincidencia de emuladores no se cuenta como otra medición de hardware. Esta fase añade evidencia de **programas PAL publicados**, no intenta igualar a NESRecomp.

## 3. Método: bytes propios → recompilador actual → runtime

`build_programs.py` es un ensamblador mínimo para estos experimentos, no un intérprete: construye bytes, direcciones, ramas y CFG de instrucciones propias. Cada instrucción se comprueba mediante `emit_blocks.decode` y se emite mediante **`emit_blocks.emit_bank` actual**, utilizando `opcodes_6502.py`. Se compila el C resultante y se enlaza con `libnes_runtime.a` existente. La ejecución llama exclusivamente al C generado con los servicios reales de Bus.

Se evita el filtro de hash de ROM comercial del importador usando la entrada sintética del emisor, como hacen las pruebas públicas de codegen. **No se valida aquí el importador completo ni una partida**. Tampoco se ejecutan los binarios originales de blargg: se reproducen sus contratos mediante programas propios, con los mismos ciclos en las ventanas relevantes.

Se generan **25 programas distintos, 72 ejecuciones y 3.419 sitios de instrucción**, en un único C y un único header de fixtures. PRG sintética de 32 KiB y CHR vacía; vector IRQ propio `$9000`, inicio `$8000`, subrutina de sincronización `$A000`. No hay bytes comerciales. No se fuerzan estados privados del secuenciador. Cada ejecución comienza con un Bus nuevo.

El driver limita cada llamada a una instrucción nativa y reenvía cada servicio exactamente una vez. Registra accesos a `$4017/$4015`, finalización de instrucciones, consulta de boundary, entrada al vector y escritura del manejador. Observa la RAM directamente para registrar la pila; **no realiza lecturas adicionales de `$4015` ni avanza dispositivos para tomar fotografías**.

Los delays propios cuentan ciclos de instrucciones oficiales: bucle de X=25 y Y=N, `1+131*N`, con relleno NOP=2/BIT zp=3. Se verifican distancias y ausencia de cruces de página en las ramas contadas. El bucle de la ventana 08 conserva su cuenta original: `4+101+252*131+130 = 33247`. Los retardos auxiliares de presentación se sustituyen por una cola breve; se detiene antes de otro cuadro. No se emulan pantalla ni pitidos del test original.

### Sincronización y observables

La subrutina ejecuta SEI, escritura `$4017=$40`, escritura `$4017=$00`, delay total de 33.251 ciclos, lectura `$4015`, AND `$40`, BNE con destino a la siguiente instrucción (2/3 ciclos), escritura `$4017=$40` y RTS. La lectura y la rama ajustan la paridad del divisor. **W** es el ciclo real del callback de la escritura posterior que inicia cada ensayo. Se conserva también la escritura de sincronización en JSONL.

Dos semillas de arranque añaden una diferencia de un ciclo antes de sincronizar; convergen al origen ajustado. Las pruebas básicas 03, sin esa normalización, escriben en ambas paridades (W=12/13). Las dos alineaciones 08 añaden otra diferencia de un ciclo antes del NOP de interés. Esto verifica estas paridades de programa; no identifica la fase física de reset de una consola.

| Observable | Medición realizada / limitación |
|---|---|
| Escritura de `$4017` | Valor, PC, offset=3, ciclo APU real W y todas las escrituras de sincronización. |
| Lectura de `$4015` | PC, offset=3 de LDA abs, ciclo real y valor retornado. Segunda LDA separada por cuatro ciclos cuando procede. |
| Flag y fuente posterior | `irq_pending()` después de lectura y avance. DMC apagado: es alias de `frame_irq_`, no dos mediciones independientes. |
| Antes del acceso | `before_read4015_callback` se registra **antes de sincronizar los tres ciclos del offset**; no representa la fase inmediatamente anterior a la lectura de datos. |
| Reconocimiento CPU | Consulta y entrega de `Bus::boundary` después del avance total. No existe en producción un resultado de poll previo observable. |
| Entrada al vector | PC, SP, P, pila y ciclos después de los siete ciclos de entrada; escritura del manejador registrada aparte. |
| Fases físicas | No hay pin /IRQ, φ1/φ2 ni get/put explícitos en nuestro modelo. Paridad CPU y offset entero no permiten asignarlos. No se inventa una fase maestra. |

PAL tiene 16 relojes maestros por ciclo CPU y get/put alternan ciclos completos; φ1/φ2 son fases dentro del ciclo. El experimento conserva ciclos CPU enteros. T es el contador de secuenciador no envuelto: tras una escritura sincronizada, la lectura W+33255 coincide con T=33252; al terminar T=33254 el contador visible vuelve a cero. No se mezcla W con el origen fresh del test A02.

## 4. Resultados de las referencias PAL

### 07: timing del flag — 8/8 ejecuciones aprobadas

Los dos arranques producen W=33310. I=1; ninguna entrada IRQ.

| Lectura desde W | T no envuelto | Bit 6 esperado / observado | Segunda lectura esperada / observada | Fuente inmediatamente tras leer |
|---:|---:|---|---|---:|
| 33254 | 33251 | 0 / 0 | — | 0 |
| 33255 | 33252 | 1 / 1 | — | 0 |
| 33256 | 33253 | 1 / 1 | W+33260: 1 / 1 | 0 |
| 33257 | 33254 | 1 / 1 | W+33261: 0 / 0 | 0 |

Se observan reafirmación entre lecturas y posterior limpieza. El acceso W+33255 retorna `$40` aunque la fuente inmediatamente posterior vale 0. El contrato de esta prueba publicada no exige que el accessor instantáneo de fuente retorne 1.

### 03: operación básica — 14/14 ejecuciones aprobadas

Se utiliza un retardo propio de 33.300 ciclos fuera de la ventana crítica, en vez de la rutina original de 20 ms. Dos arranques por caso:

| Programa | Esperado / observado |
|---|---|
| Inhibición `$4017=$40` | Bit 6=0 / 0 |
| Modo de cinco pasos `$80` | Bit 6=0 / 0 |
| Modo de cuatro pasos `$00` | Bit 6=1 / 1 |
| Lectura seguida de otra | Bits 1,0 / 1,0 |
| Escrituras `$00` y `$80` con flag ya activo | Bit 6=1 / 1 |
| Escrituras `$40` y `$00` | Bit 6=0 / 0 |
| Escrituras `$C0` y `$00` (control adicional) | Bit 6=0 / 0 |

### 08: momento de entrada al manejador — 0/4 aprobadas, 4 fallidas

El marcador del manejador debe guardar **A=`$00`** para ambas alineaciones. El código utiliza A=`$02` antes de la ventana y A=`$03` después. Se conserva la secuencia NOP; LDA #0; LDA #3. El manejador guarda A, lee `$4015` con BIT y retorna con RTI.

| Alineación | Final NOP desde W | Punto esperado: después de LDA #0 | Decisión observada | Entrada observada a `$9000` | A guardado esperado / observado |
|---:|---:|---:|---|---:|---|
| 0 | 33255 | W+33257 | Tras NOP, antes de LDA #0 | W+33262 | `$00` / **`$02`** |
| 1 | 33256 | W+33258 | Tras NOP, antes de LDA #0 | W+33263 | `$00` / **`$02`** |

Cada fila se repite con dos semillas: W=33318/33320. Las cuatro ejecuciones fallan con el marcador de **demasiado pronto**. PC apilado=`$802A`, dirección de LDA #0 todavía no ejecutada; el resultado esperado del programa requeriría retornar a `$802C`. SP pasa `$FD→$FA`. P apilado=`$22`/`$20`, B=0/U=1; pila desde SP+1: `22 2A 80` / `20 2A 80`. El manejador escribe el marcador dos ciclos después de alcanzar `$9000`.

**Esto demuestra una divergencia de entrada IRQ frente al contrato PAL publicado.** No debe compararse el comentario de mínimo 33257 del original directamente con nuestro PC ya cargado desde el vector: ese PC se observa después de siete ciclos adicionales. La conclusión se apoya en A guardado y PC apilado, no en una conversión especulativa de fases.

## 5. Ventana T=33251–33255 y controles

Se ejecutan **40 schedules observacionales**, sin clasificarlos como tests de hardware aprobados: cinco objetivos, inhibición activada/desactivada, I=0/1 y dos semillas. La rama que selecciona CLI conserva la paridad de W mediante BIT zp. Todos completan sin errores de ejecución.

Sin inhibición, ambos arranques dan estos resultados:

| T nominal de lectura | I=1: bit 6 / entradas | I=0: bit 6 / entradas | Instante de decisión observado con I=0 |
|---:|---|---|---|
| 33251 | 0 / 0 | 0 / 1 | Después de LDA; fuente aparece en su último avance |
| 33252 | 1 / 0 | 1 / 1 | Después de LDA, tras reafirmación |
| 33253 | 1 / 0 | 1 / 1 | Después de LDA, tras reafirmación |
| 33254 | 1 / 0 | 1 / 0 | La lectura limpia; boundary no conserva un poll anterior |
| 33255 | 1 / 0 | **0 / 1** | IRQ antes de LDA: se retrasa su acceso hasta T real=33275 |

En la última fila I=0 **la lectura no ocurre en el ciclo programado**: se ejecutan los 7 ciclos de entrada y STA/BIT/RTI (3+4+6), que añaden 20 ciclos. El manejador ya reconoció el flag; la lectura posterior retorna 0. El JSONL conserva ese T real; no se presenta como dato obtenido a T=33255. El contador visible entonces vale 21 debido al wrap anterior.

Con `$4017=$40`, los veinte schedules retornan bit 6=0, fuente=0 y ninguna IRQ, en ambos estados de I y ambas semillas. Esta observación no resuelve la diferencia transitoria de bit inhibido con NESRecomp señalada en el informe anterior: el test básico publicado mide fuera de la colisión y no aporta el oráculo para cada subfase.

### Control de IRQ sostenida: pérdida de un poll previo

Se añaden **seis ejecuciones con expectativa basada en el comportamiento de CPU documentado**, distintas de los contratos originales PAL:

| Secuencia, fuente ya activa | Entrada esperada / observada | Casos |
|---|---|---:|
| CLI; LDA `$4015` | 1 / **0** | 2 fallidos |
| CLI; LDA #`$40` (sin limpiar fuente) | 1 / 1 | 2 aprobados |
| I=1; NOP; LDA `$4015` | 0 / 0 | 2 aprobados |

Se espera hasta después de las tres afirmaciones de cuadro; la fuente permanece activa antes de CLI. En el caso que limpia: W=33314/33316; comienzo de LDA en W+33263, fuente=1 e I=0. La lectura de datos ocurre en W+33266, T visible=9, retorna `$40` y limpia fuente. No hay nuevos eventos en esos tres ciclos. Tras finalizar LDA y la cola no se entra al manejador; el marcador conserva `$A5` y SP=`$FD`.

Por el contrato documentado del detector/poll, CLI permite completar la instrucción siguiente, cuyo muestreo anterior a la lectura final ya encontraba IRQ activa. Esa decisión no debe desaparecer por el reconocimiento de `$4015` en el último acceso. El control sin lectura sí entrega IRQ; el control con I=1 no. **Pérdida demostrada respecto al contrato de CPU, usando una fuente PAL**, sin medir físicamente el pin de un 2A07. No se atribuye a una prueba original de blargg que no contiene esta secuencia.

## 6. Causa y evaluación de A02

Código concreto en este HEAD:

- `runtime/src/audio_state.cpp:97`, `read_status`: obtiene el bit y borra `frame_irq_`; `clock_frame_counter`, línea 129, lo reafirma en los tres ciclos PAL.
- `runtime/src/bus.cpp:33`, `read8(address,offset)`: avanza dispositivos hasta el acceso antes de leer.
- `include/recomp/runtime_services.h:50`, `recomp_finish_polled`: termina PC/ciclos y consulta boundary.
- `runtime/src/bus.cpp:91`, `boundary`: consulta **la fuente actual** después del avance, sin detector ni historial del poll anterior. `deliver_interrupt`, línea 82, realiza pila/vector y siete ciclos.
- `tools/recompiler/emit_blocks.py`, `emit_address/emit_semantics/emit_bank`: emite las lecturas temporizadas y la máscara I anterior para CLI/SEI/PLP. No mantiene un muestreo por ciclo.
- `tests/apu_audio_checks.cpp:324–326`: A02 exige fuente inmediatamente activa después de leer en 33252, sin CPU ejecutando ni decisión al terminar instrucción.

El punto de boundary puede aceptar una afirmación recién aparecida demasiado pronto (08) y cancelar una decisión que debía proceder de una fuente anterior (IRQ sostenida). La ausencia de historial explica ambos riesgos, pero **no demuestra por sí sola la regla exacta de propagación interna del 2A07**. La fase 1 de 08 exige comprobar también el desfase entre flag legible, salida APU y detector CPU; mover toda la APU un ciclo no es una corrección fundamentada.

**La expectativa inmediata de A02 no queda justificada por estas pruebas PAL.** El bit retornado sí tiene respaldo; un booleano de fuente inmediatamente posterior no representa el poll CPU ni necesariamente el latch legible. El caso exacto A02 puede recibir IRQ tras la siguiente reafirmación, aunque su snapshot falle. No se ha cambiado esa expectativa ni se declara universalmente falsa para todas las fases posibles: falta el contrato subciclo correspondiente.

## 7. Pruebas ejecutadas y evidencia

| Ejecución de esta fase | Aprobados | Fallidos | Observacionales / omitidos | Salida |
|---|---:|---:|---|---:|
| Nuevos programas: contratos 03/07/08 | 22 | 4 (08) | 0 | Incluidos en driver exit 1 |
| Nuevos controles: IRQ sostenida / máscara | 4 | 2 (limpieza pierde IRQ) | 0 | Incluidos en driver exit 1 |
| Nuevos schedules de colisión | No se califican | No se califican | 40 observacionales, 0 errores de ejecución | — |
| Coherencia de trazas: ciclos previstos, vector/pila, terminación | 98 checks | 0 | No son tests de precisión de hardware | 0 |
| CTest relevante APU + bus | 6 entradas | 1, A02 | 0 omitidas | 8 |
| CTest completo, escritorio | **18 entradas** | **1, A02** | **0 omitidas** | 8 |

Total nuevo: **72 ejecuciones; 32 casos con expectativas, 26 aprobados/6 fallidos, más 40 observacionales**. Los cuatro fallos 08 son un defecto de contrato PAL; los dos adicionales son el contrato de poll CPU. No se añadieron a CTest para respetar la restricción de mantener intactas las pruebas/configuración. La etiqueta textual del driver `PAL contract cases` engloba los 32: este informe distingue su procedencia.

CTest completo volvió a ejecutar los **4.736 casos nativos**, dos programas de pila, 14 casos de efectos temporizados, dos regresiones de `$2002`, RAM/MMC1/bus/PPU, importación/caché/ABI, diagnósticos de juego, APU y SDL dummy. El grupo APU core aprobó sus 17 casos, incluyendo DMC DMA/IRQ y máscaras CLI/SEI/PLP/RTI; A01, A03 y sus casos límite pasan. A02 imprime ciclo 33252, status64, fuente0 y falla sin ocultación. No aparece una nueva regresión en la suite existente. Los experimentos nuevos revelan cobertura que esa suite no tenía.

Las siete entradas relevantes se solapan con las diecinueve de la suite completa; **no son 26 pruebas únicas**. No se repitió ADC/SBC exhaustivo, headless, CI ni hardware externo. No se inventa cobertura completa de OAM/DMC simultáneos o NMI/IRQ: los programas nuevos mantienen NMI/DMA inactivos.

Una primera ejecución del driver experimental tuvo un error propio de vida útil de los buffers de cartucho y doce errores de ejecución. Se corrigió únicamente el driver externo, manteniendo esos logs con prefijo `initial-harness-`; no se cuentan como resultados válidos del runtime. Los ensayos intermedios `pal-only-` y `pre-final-` también se conservan; las tablas proceden de los archivos finales sin prefijo. No se borraron fallos para obtener una salida positiva: el driver final termina con **1**.

Evidencias locales: `build_programs.py`, `instruction-listings.json`, `program-metadata.json`, `program_runner.cpp`, `generated_programs.c`, `fixtures.hpp`, `program-trace.jsonl`, `program-results.txt`, `runner-exit.txt`, `commands.txt`, `measurement-checks.json`, `reference-integrity.json`, `reused-build-provenance.json`, logs y JUnit de ambas invocaciones CTest. No contienen datos del juego. El snapshot inicial conserva hashes de los 124 archivos preexistentes para comprobar su preservación.

### Reproducción local

Los comandos absolutos de compilación C99 y enlace C++17 con el runtime existente están en `commands.txt`. Desde el workspace:

```sh
experiment='<workspace>/outputs/a02-pal-programs-2026-10-11'
python3 "$experiment/build_programs.py"
# Ejecutar los dos comandos de compilación/enlace de commands.txt.
"$experiment/program_runner" > "$experiment/reproduction-trace.jsonl" 2> "$experiment/reproduction-results.txt"
# Resultado esperado con este código sin corregir: exit 1; 26/32 contratos pasan.
ctest --test-dir /private/tmp/nes-technical-baseline-build -R '^(apu_|synthetic_bus_contract)' --output-on-failure --verbose --no-tests=error
ctest --test-dir /private/tmp/nes-technical-baseline-build --output-on-failure --verbose --no-tests=error
# Ambas invocaciones CTest terminan con 8 debido a A02.
```

Las ejecuciones registradas usaron además `--output-junit` hacia `apu-irq-junit.xml` y `full-junit.xml` del directorio de evidencia. Los binarios/build locales no se incorporan al repositorio ni están disponibles automáticamente en otro equipo.

## 8. Respuestas y siguiente paso

1. **¿Fallo real respecto a PAL? Sí:** 08 guarda `$02` en las cuatro ejecuciones donde el contrato publicado, declarado aprobado en hardware PAL, exige `$00`. 03/07 pasan. No se certifica que hayamos reproducido íntegramente las ROMs originales ni medido silicio.
2. **¿Expectativa actual A02 justificada? No por la evidencia obtenida:** mezcla fuente instantánea y resultado de poll. Debe mantenerse visible hasta autorizar un contrato mejor definido, sin cambiarla en esta fase.
3. **¿IRQ perdidas o prematuras? Ambas en programas sintéticos:** prematura según el marcador PAL 08; perdida según el contrato de CPU para CLI; LDA `$4015` con fuente sostenida. No se demostró una pérdida en una partida ni en hardware local.
4. **¿Modificación mínima recomendada?** Representar la decisión CPU procedente del muestreo correcto, separada de la fuente viva y del flag legible; consumirla en boundary sin cancelarla por el último acceso. Mapear previamente propagación de fuente y detector a ciclos/fases, incluyendo la segunda alineación 08 y las ramas. No añadir `if(cycle==33252)`, una IRQ sintética al leer status ni retrasar todos los eventos arbitrariamente. No se implementa ni se da por decidido un cambio ABI.
5. **¿Podemos corregir con garantías razonables sin NES PAL local?** Sí para una corrección acotada cuya aceptación sean los contratos publicados y las regresiones de CPU, con límites explícitos. **No** para certificar todas las prioridades subciclo del 2A07 o convertir el booleano inmediato de A02 en un oráculo: esa evidencia sigue faltando.

Antes de producción: convertir estos contratos en regresiones autorizadas, fijar el instante de muestreo por instrucción, probar ambas alineaciones 08 y lecturas 07 consecutivas, repetir IRQ sostenida con I=0/1, inhibición antes/después del poll y las máscaras CLI/SEI/PLP/RTI; incluir ramas tomadas/no tomadas/cruce de página y DMA/NMI sin introducir IRQ falsas. Mantener ciclos/pila/vector y A01/A03. Un latch permanente de «alguna vez hubo IRQ» sería incorrecto: debe recordar la decisión pertinente, no todas las afirmaciones desde el inicio.

Para resolver **la carrera exacta** aún falta una prueba PAL que discrimine lectura/limpieza/inhibición en la misma subfase, con origen y revisión del 2A07 documentados; para afirmar el estado físico del pin se requiere captura de reloj/M2/IRQ. Esos límites no impiden demostrar el fallo de programa 08, pero impiden escoger reglas internas especulativas para hacer verde A02.

La comprobación final de los 124 hashes preexistentes no encontró ningún cambio; HEAD y rama permanecen iguales. `git diff --check` termina con 0. Los cuatro documentos untracked iniciales se conservan y este informe es el único archivo nuevo en el checkout.

**Parada:** diagnóstico ampliado con resultados reproducibles; producción intacta, A02 pendiente. Siguiente trabajo recomendado: autorizar el contrato de poll y sus regresiones antes de implementar una corrección acotada; conservar separada la investigación PAL subciclo.

Publication review: host-specific personal paths were replaced by `<checkout>` / `<workspace>` placeholders. Original reports and execution evidence remain preserved privately; historical results were not changed.
