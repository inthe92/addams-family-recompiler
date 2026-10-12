# Instrucciones para Codex y colaboradores

## Propósito y límites

NES Native Lab es una reimplementación nativa experimental para una revisión europea concreta de un juego NES. El repositorio público es **content-free**: no contiene ROM, código traducido, mapas, gráficos, audio ni módulos derivados. No se debe modificar la lógica del juego salvo que una tarea lo solicite expresamente y esté aprobada.

Antes de cambiar archivos, leer este documento y [PROJECT_STATUS.md](PROJECT_STATUS.md). Las afirmaciones de estado deben distinguir entre código presente, pruebas ejecutadas y trabajo pendiente.

## Arquitectura

El flujo es: ROM local proporcionada por el usuario → validación de tamaño/SHA → generación local de C → compilación local de un módulo privado ABI v3 → carga en el runtime NES. No hay intérprete 6502 ni fallback FCEUmm. `nes_runtime` contiene memoria, bus, MMC1, vídeo y audio; `user_game_view` observa/adapta datos del juego; `user_rom` carga el módulo; SDL/OpenGL presentan el frontend; Lua aporta mods restringidos. El lanzador 3D (`nes_launcher_prototype`) permanece separado del juego.

## Estructura

- `runtime/`, `include/recomp/`: runtime NES y ABI.
- `game/`: vistas, diagnóstico y parches explícitos del juego.
- `app/`: frontend principal.
- `launcher/`: prototipo 3D independiente.
- `tools/recompiler/`: importador y generadores.
- `tools/public/`: auditoría y empaquetado sin contenido.
- `tests/`: CTest, contratos sintéticos y verificadores de paquetes.
- `docs/`: informes técnicos e históricos; no duplicarlos sin necesidad.
- `.github/workflows/`: CI multiplataforma.

## Desarrollo y comandos

Desde esta carpeta:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
python3 -B tools/public/audit_public.py .
python3 -B tests/public_import_checks.py
```

El build requiere CMake 3.20+, C/C++17 y Python 3.9+. Linux necesita las dependencias OpenGL/SDL indicadas en la CI; Windows usa MSYS2 UCRT64 y Ninja. macOS usa Apple Clang. No interpretes una compilación cruzada como ejecución nativa.

## Seguridad y ROM privadas

Nunca descargues, busques, subas o publiques ROMs comerciales, módulos, cachés, mapas, recursos extraídos, capturas con contenido o claves. Usa únicamente una ROM que el desarrollador posea y mantén todos los derivados en almacenamiento privado temporal fuera del repositorio. No pongas tokens, contraseñas ni secretos en commits, logs, issues o PRs.

## Git y regresiones

Preserva cambios locales existentes: inspecciona `git status` y `git diff` antes de editar, no uses `reset --hard`, no borres ramas y no reformatees archivos no relacionados. Trabaja en una rama por tarea, con cambios pequeños. Una regresión debe reproducirse, documentarse con comando/entorno/salida, corregirse con una prueba cuando sea posible y compararse con la base. A02 (`apu_status_irq_collision_reference`) debe seguir siendo un fallo visible; no uses exclusiones, `WILL_FAIL`, `DISABLED` ni `continue-on-error` para ocultarlo.

## Documentación de tareas

Actualiza el documento específico si cambia un hecho comprobado. Incluye fecha, commit, plataforma, comando, resultado y limitaciones. En PRs separa PASS, FAIL, no ejecutado y evidencia histórica. No presentes smoke tests, generación de artefactos o compilación como una partida completa ni como certificación multiplataforma.
