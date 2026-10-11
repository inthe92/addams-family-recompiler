# A02-D — integración y verificación de ABI v3

Fecha: 2026-10-11. **Compilación limpia y paquete ARM64 verificados localmente;
verificación nativa de las otras tres plataformas pendiente. A02 sigue fallando.**

## 1. Estado inicial y conservación

Checkout: `outputs/public-alpha-0.2.0/addams-family-recompiler` del workspace.
Rama: `verify/phase-3-1-bus-timing`.
HEAD: **`35f5ce38562fcc62d47c8dcd392517bd203a86d1`**.
Origin: `https://github.com/inthe92/addams-family-recompiler.git`.
Los resultados corresponden a **HEAD más los cambios locales**, no al commit
publicado por sí solo. ABI v3 y las regresiones IRQ todavía no están committed.

Se leyeron íntegramente `CPU_IRQ_POLL_FIX.md` y
`CPU_IRQ_POLL_REGRESSIONS.md`; sus resultados históricos se conservan.
Antes de esta fase había 13 archivos tracked modificados:

```text
CMakeLists.txt
game/src/actor_view.cpp
game/src/gameplay_diagnostic.cpp
game/src/god_mode.cpp
include/recomp/module_abi.h
include/recomp/runtime_services.h
runtime/include/runtime/bus.hpp
runtime/src/bus.cpp
tests/gameplay_diagnostic_checks.cpp
tests/import_cache_checks.py
tests/module_abi_checks.cpp
tests/opcode_codegen_checks.cpp
tools/recompiler/emit_blocks.py
```

También había 11 archivos sin seguimiento, todos conservados:

```text
docs/APU_A02_DIAGNOSIS.md
docs/APU_A02_PAL_PROGRAM_TESTS.md
docs/APU_A02_PAL_REFERENCE.md
docs/CPU_IRQ_POLL_FIX.md
docs/CPU_IRQ_POLL_REGRESSIONS.md
docs/NESRECOMP_COMPARISON.md
docs/RELEASE_1_0_READINESS.md
tests/cpu_irq_edge_checks.cpp
tests/cpu_irq_poll_checks.cpp
tests/emit_irq_edge_fixtures.py
tests/emit_irq_poll_fixtures.py
```

Se guardaron estado Git y SHA-256 de todos los archivos iniciales fuera del
checkout. No se cambió de rama ni se hicieron commits, pushes, tags, releases o
ejecuciones remotas. No se modificó producción en esta fase.
La comparación final de los 131 archivos preexistentes sólo detectó los cinco
cambios de integración/tests listados en el apartado 8; no hubo eliminaciones.
Los dos archivos nuevos son este informe y el test del gate. La auditoría del
export final aprobó **133 archivos**; HEAD, rama y staging permanecen intactos,
y `git diff --check` terminó con 0.

## 2. Compilaciones nuevas

Host ejecutado: **macOS 27.0.1 ARM64**. Release, Ninja, mods ON, testing ON,
arquitectura arm64, deployment target 15.0. Se crearon dos directorios inicialmente
vacíos, sin binarios, objetos o fixtures recuperados de builds anteriores:

```text
/private/tmp/nes-abi3-integration-to3vo36c/desktop
/private/tmp/nes-abi3-integration-to3vo36c/headless
```

| Configuración | Configure | Build completo | Resultado |
|---|---:|---:|---|
| Escritorio ON | 0 | 0 | Frontend `.app`, importador/probe, runtime, vistas y todos los targets de pruebas |
| Escritorio OFF | 0 | 0 | Runtime, vistas y pruebas compatibles; sin enlace SDL/OpenGL |

SDL2 y Lua se extrajeron de los archivos locales con sus hashes verificados.
No se instalaron dependencias globales ni se descargó nada. Los generadores
produjeron C sintético nuevo en cada build mediante el recompilador actual.
La búsqueda en CMake, generadores, runtime y vistas no encontró dependencias
operativas de los antiguos directorios temporales o árboles de recuperación.
Los paths del propio build nuevo y los ejemplos históricos de documentación
no son dependencias de esos árboles.

Después de añadir la prueba del verificador de CI, CMake regeneró los builds
nuevos. Los objetos de producción siguen siendo los compilados desde cero en
esta fase; no se reutilizaron los binarios de la fase A02-C.

## 3. CTest completo

Las ejecuciones finales no usaron filtros. Se retuvo suficiente salida para
verificar los recuentos en JUnit.

| Configuración | Entradas ejecutadas | Aprobadas | Fallidas | Omitidas | Exit CTest |
|---|---:|---:|---:|---:|---:|
| Escritorio | **31** | **30** | **1: A02** | **0** | **8** |
| Headless | **29** | **28** | **1: A02** | **0** | **8** |

La entrada adicional respecto a A02-C es `ctest_verification_contract`, que
ejecuta seis pruebas del verificador de CI. No se eliminó ninguna entrada.
Headless conserva las dos exclusiones de configuración preexistentes:
`module_abi_contract` (loader SDL) y `apu_sdl_queue_contract`. No son tests
desactivados durante la ejecución; esos targets no pertenecen a headless.

En **cada configuración** pasan:

- **82/82 casos IRQ**, en 11 entradas: 24 controles anteriores y 58 adicionales;
  PAL 07/08, IRQ prematura/perdida, CLI/SEI/PLP/RTI, ramas y cruces, NMI/DMA y
  cinco variantes de adaptadores.
- **4.736 ejecuciones nativas**, las 151 codificaciones oficiales, registros,
  flags, accesos ordenados, ciclos y descriptores de poll.
- Dos programas completos de pila, 14 casos temporizados del bus y dos
  regresiones `$2002` (lecturas 25.683 y 58.930).
- 17 casos de dispositivos: RAM 3, MMC1 4, bus 5 y PPU 5.
- 17 grupos APU core, A01, A03 y sus límites adyacentes.
- Tres pruebas públicas del importador, cinco de caché/ABI, cinco del harness
  diagnóstico y ocho casos de diagnóstico de juego.
- Seis pruebas nuevas del gate de inventario/JUnit.

Escritorio también pasa cuatro casos de carga real de módulos ABI y la prueba
SDL de cola de audio. Los recuentos de grupos, casos internos y entradas CTest
son unidades diferentes; no se suman como si fueran cobertura homogénea.

**A02 original permanece byte a byte intacto**: `$4015` devuelve `$40`, su fuente
instantánea posterior es 0 y el test exige 1. No hay `WILL_FAIL`, `DISABLED`,
exenciones de skip ni cambios de expectativas. El gate también devuelve **1**,
identificando A02 y los recuentos reales, sin declarar aprobada la suite.
No aparecen nuevas regresiones del motor. No se repitió ADC/SBC exhaustivo.

## 4. Importación, caché y adaptadores

`NES_RECOMP_ABI_VERSION` es **3**. El loader nuevo rechaza módulos sin versión,
v1 y **v2**, y acepta v3, antes de utilizar sus entry points/recursos.
Las cinco pruebas de caché confirman reutilización v3 correcta e invalidación
por v1, v2, versión ausente o generador diferente. Estas pruebas de decisiones
de caché usan validación/compilación simuladas; no son una importación comercial.

El código generado comunica su plan al servicio `poll_irq` después de avanzar
una vez y antes de `boundary`. Se revisaron todos los proveedores/adaptadores
de producción: Bus → ActorView → GodMode → GameplayDiagnostic. Las cinco
variantes de integración ejecutan C generado y comprueban entrega única tras
limpiar `$4015`, PC, SP, flags, ciclos y ausencia de entrega obsoleta tras RTI.
GOD ON/OFF y diagnóstico están incluidos. No se cambiaron sus reglas.

No se encontró una ruta actual del frontend que omita el servicio. Subsisten
los límites documentados de llamadas manuales de diagnóstico a `boundary` y
proveedores sintéticos sin callback; no se confunden con la cadena del juego.
Los módulos anteriores deben **regenerarse mediante reimportación**; recompilar
C antiguo contra headers nuevos no garantiza compatibilidad semántica.

## 5. Paquete macOS ARM64 ejecutado

Se reutilizaron exclusivamente Python/Zig locales de la extracción previamente
verificada, con `target.json` de macos-arm64 y sus recibos de procedencia.
El empaquetador existente tomó **el frontend nuevo**, los headers ABI v3 y el
emisor actual. Se generó un ZIP nuevo local, sin modificar paquetes anteriores
ni publicarlo. El empaquetador no necesitó cambios.
Se compararon byte a byte los headers y herramientas del paquete con el checkout
actual; coinciden. También se comprobó la presencia de los avisos de licencia
obligatorios y el recibo de las herramientas en el ZIP.

```text
Archivo: NES-Native-Lab-macOS-arm64.zip
Bytes: 132628798
SHA-256: 64fba4bbf6bcae4cc40592f31807ac381b7bc69d29c713ebaff448cf31533761
```

Resultados de `macos_package_checks.py`, exit **0**:

- CRC y rutas ZIP correctos; extracción nueva fuera del checkout y del build.
- Firma ad hoc `codesign --verify --deep --strict` aprobada antes/después de
  extracción y nuevamente después de ejecutar las herramientas.
- Arquitectura arm64 y mínimos inspeccionados en **13 binarios Mach-O**:
  frontend 15.0; Python y sus bibliotecas 11.0; Zig 15.0. Se registraron las
  dependencias con `otool -L`.
- Python **3.11.17** y Zig **0.17.0** incluidos ejecutados.
- Programa propio BNE +0 decodificado por el emisor empaquetado, compilado
  con Zig a módulo C ABI v3 y cargado dinámicamente. Comprueba registros/PC/SP,
  tres ciclos y orden advance → poll(3,2) → boundary. Conserva el control de
  retorno 42 anterior. Este probe no pretende sustituir el runtime real probado
  en CTest ni certificar una partida.
- Tres escenarios del proceso de importación: rutas con espacios, stdout/stderr
  al fallar temprano y fallo al lanzar Python. No generan contenido del juego.

La auditoría de contenido del paquete aprobó **21.608 archivos**, sin ROM ni
módulos de juego incluidos. Se conservan LICENSE, THIRD_PARTY, avisos SDL/Lua y
distribuciones de herramientas. La auditoría excluye del escaneo propio los
binarios upstream de `toolchain`; no se realizó comparación contra bytes de una
ROM privada. Es un control técnico de contenido, no certificación jurídica.

La compatibilidad realmente ejecutada es macOS **27.0.1 ARM64**. Declarar mínimo
15.0 no demuestra arranque en macOS 15; eso requiere el runner nativo pendiente.
La firma es ad hoc, **no notarización**. El ZIP es evidencia local de integración,
no una nueva versión pública; conserva los metadatos de versión existentes.

## 6. ROM privada y aplicación jugable

No se encontró una ROM recordada disponible en la configuración local del
importador (`rom_path.txt` en las preferencias de nes-native-lab), ni una entrada
original configurada para esta prueba. Los árboles históricos contienen imágenes
de recuperación/derivadas, que **no se usaron como sustituto**. No se buscó una
ROM para descargar ni se alteró el perfil de compatibilidad.

Por tanto, **no se reimportó la ROM comercial, no se arrancó el juego real y no
se comprobaron menú, idiomas, controles, gráficos, audio o transiciones mediante
una partida**. Se compiló la aplicación completa y se ejecutaron el loader,
adaptadores, importación con errores sintéticos y herramientas empaquetadas.
No hay logs de IRQ/ABI de una sesión real que se puedan declarar revisados.

Para la prueba privada pendiente, usar una ROM propia validada por el perfil
europeo En/Fr/De, un directorio nuevo de caché fuera de Git y el paquete ABI v3:

```sh
"/ruta/NES Native Lab.app/Contents/MacOS/nes_native" \
  --rom "/ruta/privada/compatible.nes" \
  --import-cache "/ruta/privada/cache-abi3-nuevo" --no-god --diagnostic
```

Comprobar que el recibo local `import.json` contiene `abi_version: 3`, que un
segundo arranque valida la caché, y que el diagnóstico termina sin errores de
servicio/ABI/direcciones no cubiertas. Probar idioma, menú, entrada, audio y
varias transiciones; después realizar otra sesión con GOD y diagnóstico.
No compartir la ROM, módulo ni caché. Una sesión asistida no prueba una partida
normal completa.

## 7. Estado de las cuatro plataformas y CI

| Plataforma | Compilación/ejecución ABI v3 en esta fase | Paquete | Pendiente |
|---|---|---|---|
| macOS ARM64 | **Ejecutada**, ambos builds completos y CTest | **ZIP nuevo extraído y verificado** en macOS 27.0.1 | Runner macos-15, partida privada |
| macOS x86_64 | **No ejecutada** | No generado para ABI v3 | Runner macos-15-intel, CTest/JUnit y paquete Intel |
| Windows x64 | **No ejecutada en Windows** | Ningún ZIP ABI v3 certificado | Runner nativo, paquete y prueba doméstica |
| Linux x64 | **No ejecutada** | Ningún AppImage ABI v3 certificado | Runner ubuntu-22.04 y validación de distribución |

No se hizo compilación cruzada ni se presentó como ejecución nativa. Las
compilaciones/ZIP de versiones ABI anteriores no validan estos cambios locales.

Se revisó `.github/workflows/build.yml`: macos-15/arm64,
macos-15-intel/x86_64, Windows UCRT64 x64 y Ubuntu 22.04 x64. Todos construyen
todos los targets con testing/desktop/mods ON y ejecutan CTest sin filtros.
Los fixtures C se generan mediante Python local y CMake, sin ROM ni árboles
de recuperación. Windows conserva el enlace estático de las nuevas pruebas.

El gate exige ahora todas las entradas, incluidos 82 casos IRQ, APU y ABI v3.
No se añadió `continue-on-error`. macOS/Windows conservan sus pasos independientes
de empaquetado/evidencia aunque A02 falle, manteniendo rojo el resultado global.
Linux conserva su estructura previa: A02 hace fallar el paso build/CTest y sus
pasos posteriores de empaquetado quedan bloqueados. No se declara un AppImage
validado ni se amplía esta fase para cambiar ese flujo.

Los checks de paquete macOS/Windows usan ahora el smoke de C generado ABI v3.
Windows conserva la limpieza de PATH para excluir las herramientas instaladas,
la inspección PE/DLL, el picker/errores/cierre y auditoría de contenido.
Sus **11 tests de formatos PE/ZIP** se ejecutaron aquí y pasaron, pero en macOS.
Además se ejecutó el código exacto de su subproceso de caché con los recursos
macOS extraídos: **5/5**, incluida v2. Ninguna de esas comprobaciones certifica
el arranque de un EXE en Windows.

No se disparó GitHub Actions: el código todavía es local y publicar/disparar
workflows requiere autorización posterior. Un dispatch del HEAD remoto antiguo
no verificaría esta corrección. Tras revisión y autorización de commit/push de
los archivos pertinentes en esta rama, el push existente ejecutará las cuatro
plataformas. Inspeccionar la ejecución del **nuevo SHA**, no otra histórica:

```sh
gh run list --repo inthe92/addams-family-recompiler --workflow build.yml \
  --branch verify/phase-3-1-bus-timing --commit <SHA-nuevo>
gh run view <run-id> --repo inthe92/addams-family-recompiler --json headSha,jobs,conclusion
gh run download <run-id> --repo inthe92/addams-family-recompiler --dir <evidencia-local>
```

Exigir build completo, 31 entradas ejecutadas por desktop, 30 aprobadas/A02
fallida, cero skips, JUnit e inventario concordantes y ZIP/evidencias propios
de cada runner. Un fallo adicional obliga a investigar, no a excluir tests.
Los comandos locales headless de abajo permiten comprobar sus 29 entradas en
otros hosts; no se añadieron cuatro builds remotos headless redundantes.

## 8. Incidencias y cambios mínimos de esta fase

| Archivo | Cambio / comprobación |
|---|---|
| `tests/verify_ctest_suite.py` | Recuentos actuales de caché 5 y loader ABI 4; exige IRQ/APU; distingue `status="fail"` ejecutado de un skip y rechaza todo fallo; opción de inventario headless. |
| `tests/verify_ctest_suite_checks.py` | Seis tests sintéticos del gate: inventarios completos, IRQ ausente, exenciones, recuentos antiguos/salida recortada, A02 fallida y ejecución ausente/omitida. |
| `CMakeLists.txt` | Una entrada adicional para esos seis tests; conserva toda la integración previa. |
| `.github/workflows/build.yml` | Sólo amplía a 65.536 bytes la salida retenida por CTest en los tres pasos de ejecución. |
| `tests/windows_package_checks.py` | El subproceso empaquetado exige ahora los cinco casos de caché existentes, incluyendo v2, en vez de cuatro. |
| `tests/public_toolchain_smoke.py` | Conserva retorno 42 y añade compilación/carga/ejecución de una rama propia emitida con los headers/servicios ABI v3. |
| `docs/ABI_V3_PLATFORM_VERIFICATION.md` | Este informe. |

Fallos de infraestructura/harness encontrados y conservados como evidencia:

1. Gate anterior rechazaba el recuento nuevo de caché; posteriormente se
   confirmó el recuento antiguo del loader y la falta de exigencia IRQ/APU.
2. JUnit truncaba PAL07/ramas/edges a 1.024 bytes, perdiendo los resúmenes.
   Se conservó más salida; las expectativas de CPU no cambiaron.
3. CTest marca una ejecución fallida como `status="fail"`; el gate antiguo
   la describía como no ejecutada. Ahora la cuenta y falla explícitamente.
4. Primer intento de smoke Zig: caché global fuera del sandbox, PermissionDenied.
   Se usó `ZIG_GLOBAL_CACHE_DIR` dentro de la evidencia temporal, sin permisos
   globales. Segundo intento: el nuevo fixture compilado sin optimización requería
   un símbolo UBSan con `-nostdlib`. El test se alineó con **`-O1`**, ya usado
   por el importador de producción. El tercer intento completo pasó.

No se ocultaron esas ejecuciones fallidas. No se cambió CPU, APU, PPU,
recompilador, ABI de producción, reglas del juego, gráficos o panorámico. Las
expectativas originales A02 y las 82 regresiones se mantuvieron intactas.

## 9. Reproducción y evidencia

Desde el checkout (builds inicialmente nuevos):

```sh
cmake -S . -B <nuevo-desktop> -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON -DNES_ENABLE_DESKTOP=ON
cmake --build <nuevo-desktop> --parallel 4
cmake -S . -B <nuevo-headless> -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DNES_ENABLE_MODS=ON -DNES_ENABLE_DESKTOP=OFF
cmake --build <nuevo-headless> --parallel 4
# En macOS añadir -DCMAKE_OSX_ARCHITECTURES=arm64 (o x86_64)
# y -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 a configure.
ctest --test-dir <build> --show-only=json-v1 > <inventory.json>
ctest --test-dir <build> --output-on-failure --verbose --no-tests=error \
  --test-output-size-passed 65536 --test-output-size-failed 65536 \
  --output-junit <results.xml>
python3 -B tests/verify_ctest_suite.py --inventory <inventory.json> --junit <results.xml>
# Añadir --headless al último comando para headless.
# CTest sale 8 y el gate 1 mientras A02 siga fallando.
python3 -B tests/windows_package_checks.py --self-test
git diff --check
```

Empaquetado local probado:

```sh
python3 -B tools/public/package_platform.py --build <nuevo-desktop> \
  --output <dist-fuera-de-git> --toolchain <toolchain-arm64-verificado>
ZIG_GLOBAL_CACHE_DIR=<cache-temporal> python3 -B tests/macos_package_checks.py \
  --build <nuevo-desktop> --output <dist-fuera-de-git> --arch arm64 \
  --report <evidencia.json>
python3 -B tools/public/audit_release.py "<dist-fuera-de-git>/NES Native Lab.app"
# En el runner macos-15 añadir --require-native-macos15 al package check.
```

Evidencias locales fuera de Git:
`/private/tmp/nes-abi3-integration-to3vo36c/`. Contienen snapshot inicial,
configure/build logs, inventarios, CTest/JUnit inicial y final, resultados del
gate antes/después, tests de Windows en macOS, auditorías y los tres intentos
del check macOS con sus diagnósticos. Los resultados finales son
`desktop-final-ctest.xml`, `headless-final-ctest.xml` y
`macos-package-verification-complete.json`. `results-summary.json` contiene
recuentos/hash del ZIP. Conservar esa carpeta si se necesita evidencia duradera.

## 10. Lista para el PC Windows del tester y decisión de publicación

Usar **un ZIP nuevo ABI v3 procedente del runner**, no el ZIP alpha anterior:

1. Comparar SHA-256 con el informe del artefacto; extraer en una carpeta nueva
   con espacios. Confirmar EXE, Python, Zig, tools, headers v3 y licencias.
2. En Windows 10/11 x64 sin herramientas de desarrollo instaladas, iniciar
   `nes_native.exe` desde otro directorio. Confirmar selector y cancelación,
   sin procesos persistentes ni DLL ausentes.
3. Con una ROM sintética inválida, comprobar rechazo claro y `import.log`;
   no debe producir un módulo. El runner automatiza los datos sintéticos.
4. Con su ROM europea compatible propia, ejecutar desde PowerShell:
   `& "C:\Pruebas con espacios\nes_native.exe" --rom "C:\ROMs\compatible.nes" --import-cache "$env:LOCALAPPDATA\NES-ABI3-Test" --no-god --diagnostic`.
   Usar una caché nueva; confirmar recibo ABI 3 y reutilización en el segundo
   arranque. No compartir caché ni módulo generado.
5. Revisar menú/idioma/controles/audio/gráficos/transiciones y salida limpia;
   repetir con GOD en sesión separada. Enviar sólo diagnóstico/errores revisados
   y los datos de versión del candidato. Windows sigue sin Authenticode.

**Podemos preparar una candidata alpha experimental**, con límites documentados,
gracias a las compilaciones y paquete ARM64 reproducibles. **No está certificada
una nueva alpha multiplataforma para publicar todavía**: faltan los tres runners,
la ejecución macOS 15 y el arranque real con ROM regenerada. No se publicó nada.

Para una RC faltan, como mínimo: esas verificaciones nativas y de paquetes
relocados; partida privada con ABI v3 (normal y asistida); resolver el contrato
PAL específico de A02 con evidencia independiente y eliminar su fallo sin
debilitarlo; revisión de las limitaciones conocidas de DMA/NMI y cobertura de
rutas del juego/panorámico que afecten a una experiencia completa; y una decisión
explícita sobre firma/notarización y requisitos de distribución. Los resultados
de esta fase no certifican precisión subciclo PAL ni una partida completa.

La siguiente acción recomendada es revisar los cambios locales y autorizar,
en otra tarea, su publicación controlada para obtener CI nativa del nuevo SHA;
en paralelo, hacer la prueba privada de arranque y transiciones. No avanzar
automáticamente a corregir A02 o publicar versiones.
