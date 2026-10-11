NES Native Lab - Windows x64 portable test package
================================================

English
-------
1. Extract the ENTIRE ZIP into a writable folder. Do not run inside the ZIP
   or move nes_native.exe away from its companion folders.
2. Double-click nes_native.exe and select your own compatible NES ROM.
   Only the verified European En/Fr/De revision is supported. No ROM is
   supplied or downloaded. Modified ROMs and other revisions are rejected.
3. The first local import can take several minutes. Keep its window open.
   Python and Zig are included; no Python, Zig, CMake, MSYS2 or Visual Studio
   installation is required. Windows graphics drivers are still required.
4. For a normal session, open a terminal in this folder and run:
       .\nes_native.exe --no-god
   GOD defaults ON in this experimental alpha. G toggles GOD; H grants all
   items. Avoid G/H when testing an unaided game.
5. Controls: arrows, Z/X, Enter, right Shift. Escape closes gameplay.
   A supported SDL controller can also be used.
6. Import errors show the cause and log location. The normal per-user cache
   and import.log are under %APPDATA%\nes-native-lab\user-import.
   Preserve that log when reporting a problem; do not publish your ROM.

This is a CI test artifact, not a public release or a completed game.
Synthetic CI checks do not prove a full game session. A02 remains an open
APU timing/reference issue. First opening, compatible-ROM import, graphics,
audio and normal gameplay still need testing on consumer Windows systems.
There is no Authenticode signing/trust certification for this package.
Keep LICENSE, THIRD_PARTY.md, licenses/ and toolchain notices intact.

Espanol
-------
1. Descomprime TODO el ZIP en una carpeta con permiso de escritura.
   No ejecutes dentro del ZIP ni separes el EXE de las demas carpetas.
2. Abre nes_native.exe y selecciona tu propia ROM compatible de NES:
   revision europea En/Fr/De verificada. No se incluye ni descarga una ROM.
3. La primera importacion local puede tardar varios minutos. No cierres
   esa ventana. Python y Zig vienen incluidos; no hay que instalarlos.
4. Para jugar sin ayudas, abre una terminal en esta carpeta y ejecuta:
       .\nes_native.exe --no-god
   Esta alfa activa GOD por defecto. No pulses G ni H en una prueba normal.
5. Controles: flechas, Z/X, Enter, Shift derecho; Escape cierra la partida.
6. Si falla la importacion, conserva el import.log indicado en el dialogo.
   La cache habitual esta en %APPDATA%\nes-native-lab\user-import.

Es un artefacto de pruebas, no una version publica terminada. Las pruebas
sinteticas no demuestran una partida real. A02 sigue pendiente. Conserva
las licencias y los archivos que acompanan al programa.
