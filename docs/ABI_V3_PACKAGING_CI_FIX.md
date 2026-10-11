# ABI v3 — corrección de empaquetado CI

Fecha: 2026-10-11. Rama: `verify/phase-3-1-bus-timing`.

## 1. Trazabilidad

- SHA inicial: `c67a201f9e8aa2933f1295067b16214b53948ffe`.
- SHA final: se completa tras el commit de esta corrección.
- Workflow: `.github/workflows/build.yml`.
- No se modificó `main`, ni se crearon tags, releases o pull requests.

## 2. Causas

En macOS, el smoke ABI v3 se ejecutaba desde `/tmp/nes-smoke`, pero el workflow solo copiaba `tools` y `runtime-tools`; faltaba `include/recomp/module_abi.h` (y el resto de headers necesarios para compilar el fixture). Por eso fallaba antes de compilar con `FileNotFoundError`.

En Linux, compilación y CTest estaban en el mismo paso. El fallo conocido A02 hacía fallar ese paso y bloqueaba el empaquetado, aunque la compilación y la recogida de resultados fueran utilizables.

## 3. Cambios concretos

- macOS y Linux copian `include` y `game/include` al recurso temporal del smoke.
- Antes de compilar el fixture se comprueba explícitamente la presencia de `module_abi.h`, `runtime_services.h`, `cpu_state.h` y `game/compiled_program.h`.
- Linux separa compilación, CTest completo, validación JUnit/inventario y empaquetado.
- El AppImage depende explícitamente de compilación, toolchain y herramientas de empaquetado; no depende del éxito de CTest.
- A02 continúa produciendo fallo global de CI. No se usa `continue-on-error`, no se eliminan pruebas y no se cambia lógica de producción.

## 4. Verificación local

- YAML del workflow: válido, comprobado con Ruby/Psych.
- `tests/verify_ctest_suite_checks.py`: 6/6.
- Smoke ABI v3 sintético: correcto; compilación, carga, ABI v3 y orden de poll correctos.
- `tests/windows_package_checks.py --self-test`: 11/11.
- `git diff --check`: correcto.
- No se repitió la batería nativa completa: el cambio afecta únicamente a scripts y condiciones del workflow.

## 5. Verificación remota

Pendiente de la nueva ejecución de GitHub Actions del SHA final. Deben revisarse los cuatro jobs: Windows x64, macOS ARM64, macOS x86_64 y Linux x64, incluidos inventarios, JUnit y artefactos.

## 6. Estado por plataforma

La evidencia anterior conserva Windows x64 como paquete ZIP verificado, con 31 entradas CTest, 82/82 IRQ y A02 como único fallo. macOS ARM64, macOS Intel y Linux necesitan la nueva ejecución para considerar verificados sus paquetes de este SHA.

## 7. Paquetes

- Windows x64: ZIP verificado en la ejecución anterior.
- macOS ARM64: pendiente de ZIP, firma ad hoc, extracción, ejecución y smoke en CI.
- macOS Intel: pendiente de ZIP, firma ad hoc, extracción, ejecución y smoke en CI.
- Linux x64: pendiente de AppImage, dependencias, ejecución real, smoke y auditoría.

## 8. Fallos

El fallo conocido A02 debe seguir siendo el único fallo de CTest: 31 entradas ejecutadas por plataforma, 30 aprobadas, 1 fallida, 0 omitidas y 82/82 casos IRQ aprobados. Cualquier fallo distinto debe investigarse antes de introducir cambios adicionales.

## 9. Limitaciones pendientes

La corrección local no sustituye la evidencia de los runners. Tampoco resuelve el contrato A02 ni constituye una validación de juego real con una ROM comercial. La nueva alpha conjunta no debe recomendarse hasta revisar los cuatro paquetes y sus comprobaciones obligatorias.

## 10. Recomendación alpha

No publicar todavía una alpha conjunta. Si la nueva ejecución conserva únicamente A02 y los cuatro paquetes pasan todas sus comprobaciones, podrá considerarse una alpha experimental ABI v3 con las limitaciones documentadas; A02 debe seguir visible como fallo conocido.
