# Addams Family Recompiler

[English](README.md) | [Español](README.es.md)

Recompilación nativa experimental del juego europeo de NES, con una ROM compatible aportada por cada usuario. El repositorio y las aplicaciones descargables **no incluyen la ROM, código traducido del juego, gráficos, música, mapas extraídos ni desensamblados**. La aplicación se llama actualmente **NES Native Lab**.

**Alfa técnica 0.2.0.** El juego todavía no está reescrito por completo ni verificado de principio a fin. El panorámico sigue teniendo limitaciones visibles con la aparición de enemigos y las habitaciones verticales o de ancho fijo.

## Descargar y abrir

Descarga el paquete de tu sistema desde [Releases](https://github.com/inthe92/addams-family-recompiler/releases). Los paquetes incluyen Python y Zig para la importación local: no necesitas instalar un compilador, Python ni herramientas de desarrollo. Hace falta un controlador gráfico con OpenGL 3.2. La primera importación puede tardar varios minutos; después se reutiliza el módulo validado.

| Sistema | Paquete | Primera apertura |
|---|---|---|
| macOS Apple Silicon | `NES-Native-Lab-macOS-arm64.zip` | Extrae, abre `NES Native Lab.app` y selecciona tu ROM. |
| Windows x64 | `NES-Native-Lab-Windows-x86_64.zip` | Extrae el ZIP completo, abre `nes_native.exe` y selecciona tu ROM. |
| Linux / Steam Deck | `NES-Native-Lab-Linux-x86_64.AppImage` | Dale permiso de ejecución y arrastra la ROM a la ventana de importación, o usa `--rom`. |

macOS lleva firma ad hoc, sin notarización. Windows no tiene firma Authenticode. Falta probar los paquetes en equipos reales de Windows y Steam Deck; si fallan, indica tu sistema y adjunta sólo el registro de importación. Las compilaciones para Mac Intel están configuradas en GitHub Actions y no forman parte de la primera entrega compilada localmente.

### Steam Deck

1. Cambia al modo escritorio. Descarga el AppImage y activa **Es ejecutable** en sus propiedades.
2. Ábrelo y arrastra tu archivo `.nes` desde Dolphin a la ventana de importación. Espera a que compile localmente. Mantén la ROM en esa ubicación porque se comprueba en cada apertura.
3. Añade el AppImage a Steam como juego externo y vuelve al modo juego. Usa una plantilla de mando y asigna **G** y **H** a los botones traseros si quieres.

No hace falta modificar el sistema de sólo lectura de SteamOS. La primera importación por arrastre está pensada para el modo escritorio. Si una distribución no puede montar AppImages, extráelo con `--appimage-extract` y abre `squashfs-root/AppRun`.

```sh
chmod +x NES-Native-Lab-Linux-x86_64.AppImage
./NES-Native-Lab-Linux-x86_64.AppImage --rom "/ruta/mi juego.nes"
```

## ROM compatible

Sólo se admite la revisión europea En/Fr/De verificada de *The Addams Family*:

```text
Tamaño: 262160 bytes
Mapper: MMC1 (1)
SHA-256: 6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880
```

Se rechazan otras regiones, revisiones y archivos parcheados. Cambiar el nombre no cambia la compatibilidad. No se proporciona una descarga del juego. El importador lee el archivo original sin modificarlo.

## Cómo funciona

Se valida tu ROM, se traduce a C y se compila localmente un módulo nativo `.dll`, `.so` o `.dylib`. El motor carga ese módulo privado. No hay intérprete de instrucciones 6502 ni fallback a FCEUmm/libretro. El runtime sigue reproduciendo la memoria, MMC1, vídeo y audio de la NES.

Los módulos y recursos generados contienen contenido del juego y deben permanecer privados. No subas la caché, ROM, alfa anterior con contenido incrustado, capturas ni fuentes recuperadas a este repositorio o una release.

## Controles

| Teclado | Mando | Acción |
|---|---|---|
| Enter | Start | Inicio / menús |
| Flechas | Cruceta / stick izquierdo | Movimiento / escaleras |
| Z | A | Salto |
| X | B | Puertas / interacción |
| Mayús derecha | Back | Select |
| G | Asignar con Steam Input | Cambiar GOD; activado al iniciar la alfa |
| H | Asignar con Steam Input | Conceder y mantener objetos y requisitos de acceso durante la sesión |
| Escape | — | Cerrar |

GOD protege salud y vidas e intenta recargar la habitación ante ciertas muertes directas. H no es teletransporte. Todavía no están verificadas todas las rutas, combates y recuperaciones ante muerte.

## Compilar sin ROM

Necesitas CMake 3.20+, compilador C/C++17 y Python 3.9+. Se incluyen archivos oficiales verificados de SDL2 y Lua. En Windows utiliza Ninja dentro de MSYS2 UCRT64. Linux necesita las dependencias de desarrollo de OpenGL y de la plataforma SDL; consulta el workflow.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Para empaquetar una aplicación autónoma, descarga las herramientas fijadas para el sistema y genera el paquete. Se descargan herramientas de desarrollo, nunca el juego:

```sh
python3 -B tools/public/fetch_runtime_tools.py --destination runtime-tools --cache runtime-cache --system macos-arm64
python3 -B tools/public/package_platform.py --build build --output dist --toolchain runtime-tools
```

Otros sistemas: `macos-x86_64`, `windows-x86_64`, `linux-x86_64`. Para generar AppImage en Linux, utiliza `linuxdeploy`, `appimagetool` y:

```sh
sh tools/public/package_linux.sh build
```

GitHub Actions compila el motor sin contenido para Linux x64, Windows x64 y ambas arquitecturas de Mac; ejecuta pruebas sintéticas, incorpora herramientas verificadas y genera artefactos descargables. No publica releases automáticamente ni utiliza una ROM secreta. La prueba local opcional `import_probe` comprueba el arranque con tu ROM; CI nunca recibe una.

Consulta [arquitectura](docs/ARCHITECTURE.es.md), [publicación](docs/PUBLISHING.es.md), [verificación](docs/VERIFICATION.es.md) y [licencias de terceros](THIRD_PARTY.md).

## Licencia y nombres

Las contribuciones propias del motor y las herramientas usan MIT. Las dependencias conservan sus licencias. Esto no concede derechos sobre el juego comercial ni el contenido generado localmente. El nombre del juego identifica compatibilidad; este proyecto independiente no está afiliado ni respaldado por sus titulares. Excluir contenido del juego es una medida técnica, no una garantía jurídica frente a reclamaciones.
