# Third-party notices / Licencias de terceros

The root MIT license covers original project contributions only. / La licencia MIT de la raíz sólo cubre las contribuciones propias.

## Source dependencies

- **SDL2 2.32.8**: zlib license. Full notice: `third_party/licenses/SDL2-LICENSE.txt`. Official source archive and SHA-256 are recorded in `third_party/manifest.json`.
- **Lua 5.5.1**: MIT license, copyright Lua.org, PUC-Rio. Full notice: `third_party/licenses/Lua-readme.html`. Source archive and hash are in the same manifest.

Application resources carry these notices in `licenses/`. Dependency archives preserve their original headers and platform notices.

## Import tools bundled in releases

The source repository contains download recipes and pinned hashes, not these binaries. Release applications redistribute verified standalone **CPython 3.11.17** and **Zig 0.17.0**, fetched from Python Build Standalone and ziglang.org. URLs, hashes, target and version provenance are in `tools/public/runtime_tools.json` and the bundle's `toolchain/runtime_tools.json`.

CPython and its bundled components retain their PSF/BSD/MIT/OpenSSL and other original notices in the Python distribution. Zig and its LLVM/Clang/compiler/runtime/standard-library components retain their original MIT, Apache-2.0 with LLVM exceptions and component notices in the Zig distribution. Keep the complete `toolchain/python` and `toolchain/zig` notice files with each package; the project MIT license does not replace them. The optional Python `share/terminfo` database is omitted because the importer does not use curses.

The Linux AppImage uses the unmodified official AppImage type-2 runtime. Its MIT notice and component notices accompany the release. Linux OpenGL dispatch libraries, when included, retain their original notices and source links in `licenses/`. System graphics drivers and operating-system frameworks are not bundled. Packaging tools are build tools, not part of the application.

No rights to a commercial game, its ROM, characters, music or generated resources are granted. / No se conceden derechos sobre el juego comercial, su ROM, personajes, música o recursos generados.
