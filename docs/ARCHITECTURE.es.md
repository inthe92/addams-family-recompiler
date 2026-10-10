# Arquitectura

El frontend público compila `nes_runtime`, `user_game_view`, `user_rom` y `desktop_presenter`. No existe un target `game_recompiled` en esta edición. Ninguna biblioteca del motor incluye bancos de instrucciones originales o arrays de recursos PRG/CHR.

```text
Aplicación distribuida + herramientas genéricas + perfil de direcciones
                            |
                   ROM propia del usuario
                            |
                 validación SHA-256 completa
                            |
                 generación C y recursos local
                            |
                       Python + Zig incluidos
                            |
              módulo local .dylib / .so / .dll
                            |
                   runtime NES + SDL/OpenGL
```

`compatible_profile.json` contiene una identidad SHA-256, un tamaño y direcciones de inicio de instrucciones. No contiene opcodes, operandos, bytes de ROM, textos, mapas ni imágenes. Es metadata de análisis para una revisión concreta, no el programa original. Los mnemonicos y operandos se obtienen de la ROM importada.

La ABI de servicios C conserva registros, flags, accesos de memoria y límites de instrucciones. Las instrucciones se traducen a C antes de ejecutar el juego; no se decodifican en el bucle del runtime. Las direcciones no cubiertas detienen la alfa. Los saltos de interrupción instalados en RAM tienen una traducción acotada; no hay fallback a un emulador.

Los parches GOD, objetos y presentación son código propio ligado a campos de RAM de esta revisión. No son una API genérica para cualquier juego NES. La composición panorámica conserva el raster central y utiliza mapas generados localmente para los laterales horizontales. Las habitaciones verticales no disponen de geometría adicional validada.

La caché del usuario contiene su contenido generado y un recibo con hashes. Se invalida si cambian el generador, el perfil, las cabeceras de ABI, la arquitectura o el módulo. La ROM se valida incluso cuando existe una importación previa. El compilador sólo recibe C generado desde una imagen admitida. No se utiliza una shell para lanzar el importador o el compilador.

Ubicaciones predeterminadas:

- macOS: `~/Library/Application Support/nes-native-lab/user-import/`
- Linux: directorio de datos de SDL, normalmente `~/.local/share/nes-native-lab/user-import/`
- Windows: directorio de datos de SDL bajo `%APPDATA%`, `nes-native-lab/user-import/`

`--import-cache` permite indicar otro directorio privado. `--rom` cambia la ROM seleccionada. `--import-only` prepara el módulo sin abrir el juego. El importador no escribe dentro de la aplicación ni descarga una ROM.
