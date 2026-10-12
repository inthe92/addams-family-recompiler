# Preparación del entorno

## Incorporación rápida

1. Clona el repositorio y entra en `addams-family-recompiler/`.
2. Abre esa carpeta en Codex. Lee `AGENTS.md`, `PROJECT_STATUS.md` y después `README.md`.
3. Instala CMake 3.20+, un compilador C/C++17 y Python 3.9+.
4. En Windows usa MSYS2 UCRT64 con Ninja y las herramientas MinGW indicadas por `.github/workflows/build.yml`. En Linux instala CMake/Ninja, compilador, Python y paquetes de desarrollo OpenGL/SDL. En macOS usa Apple Clang y CMake.
5. Configura y compila sin ROM:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

6. Ejecuta `python3 -B tools/public/audit_public.py .` y `python3 -B tests/public_import_checks.py`.
7. Comprueba `git status --short`, crea una rama de tarea y realiza cambios pequeños.
8. Abre un PR con comandos, resultados y limitaciones. No subas ROMs ni artefactos privados.

## Paquetes y ROM privada

El empaquetado público usa `tools/public/fetch_runtime_tools.py` y `package_platform.py`; consulta [README.md](README.md) y [docs/PUBLISHING.md](docs/PUBLISHING.md). Una importación real requiere una ROM compatible proporcionada por el usuario y una caché privada fuera del repositorio. CI no contiene ROM y los smoke tests sintéticos no prueban una partida.

## Diagnóstico

Si CTest falla, conserva la salida completa y comprueba primero si es A02. Si falla la auditoría pública, no empaquetes ni publiques. Para diferencias de plataforma, reporta sistema, arquitectura, compilador, CMake, commit y si hubo ejecución nativa. Los informes anteriores en `docs/` no deben sobrescribirse: añade o actualiza documentación de estado con enlaces a ellos.
