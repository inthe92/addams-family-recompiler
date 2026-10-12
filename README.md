# Addams Family Recompiler

[English](README.md) | [Español](README.es.md)

Experimental native recompilation of the European NES game, using a compatible ROM supplied by the user. The repository and downloadable applications include **no game ROM, translated game code, graphics, music, extracted maps or disassembly**. The application is currently called **NES Native Lab**.

**Technical alpha 0.2.1.** The game is not fully rewritten or verified from start to finish. Widescreen enemy activation and fixed-width/vertical rooms still have visible limitations.

## Download and run

Download the package for your system from [Releases](https://github.com/inthe92/addams-family-recompiler/releases). Release packages include Python and Zig for local import: you do not need to install a compiler, Python or development tools. A graphics driver supporting OpenGL 3.2 is required. First import can take several minutes; subsequent starts reuse the validated module.

| System | Package | First start |
|---|---|---|
| macOS Apple Silicon | `NES-Native-Lab-macOS-arm64.zip` | Extract, open `NES Native Lab.app`, select your ROM. |
| Windows x64 | `NES-Native-Lab-Windows-x86_64.zip` | Extract the entire ZIP; open `nes_native.exe`, select your ROM. |
| Linux / Steam Deck | `NES-Native-Lab-Linux-x86_64.AppImage` | Mark executable; drag your ROM into the import window, or use `--rom`. |

macOS builds use an ad hoc signature and are not notarized. Windows builds have no Authenticode signature. Hardware testing on Windows and Steam Deck is pending; please report your system and import log if it fails. macOS Intel builds are configured in GitHub Actions and are not part of the first locally built release.

### If import fails

Alpha 0.2.1 fixes launching Python from Windows paths containing spaces. The frontend records stdout and stderr from process startup, including failures before Python or its tools initialize. Extract the entire ZIP into a new folder and keep `tools` and `toolchain` beside the executable.

The error dialog shows the cause and the location of `import.log`. On Windows this is normally `%APPDATA%\nes-native-lab\user-import\import.log`. A SHA-256 mismatch means the input is not the compatible revision listed below.

### Steam Deck

1. Switch to Desktop Mode. Download the AppImage and enable **Is executable** in its file properties.
2. Open it and drag your `.nes` file from Dolphin into the import window. Wait for local compilation. Keep the ROM at that location because it is checked on every start.
3. Add the AppImage to Steam as a non-Steam game, then return to Gaming Mode. Use a gamepad layout, with **G** and **H** assigned to back buttons if desired.

No changes to the SteamOS read-only system are needed. The first drag-and-drop import is intended for Desktop Mode. If a distribution cannot mount AppImages, extract with `--appimage-extract` and launch `squashfs-root/AppRun`.

```sh
chmod +x NES-Native-Lab-Linux-x86_64.AppImage
./NES-Native-Lab-Linux-x86_64.AppImage --rom "/path/to/my game.nes"
```

## Compatible ROM

Only the verified European En/Fr/De revision of *The Addams Family* is accepted:

```text
Size: 262160 bytes
Mapper: MMC1 (1)
SHA-256: 6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880
```

Other regions, revisions and patched files are rejected. Renaming a file does not change compatibility. No game download is provided. The importer reads the original file without modifying it.

## How it works

Your ROM is authenticated, translated into C and compiled locally into a native `.dll`, `.so` or `.dylib`. The engine loads that private module. There is no 6502 instruction interpreter or FCEUmm/libretro fallback. NES memory, MMC1, video and audio behavior are still reproduced by the runtime.

Generated modules and resources contain game content and must remain private. Do not upload your import cache, ROM, previous embedded-content alpha, screenshots or recovered sources to this repository or a release.

## Controls

| Keyboard | Gamepad | Action |
|---|---|---|
| Enter | Start | Start / menus |
| Arrow keys | D-pad / left stick | Move / climb |
| Z | A | Jump |
| X | B | Doors / interaction |
| Right Shift | Back | Select |
| G | Assign with Steam Input | Toggle GOD; only with `--god` or `--diagnostic` |
| H | Assign with Steam Input | Grant and retain inventory; only with `--god` or `--diagnostic` |
| F11 | — | Toggle fullscreen |
| Escape | Guide, if available | Open exit confirmation |
| F1 | Y | Hide temporary controls banner |

GOD protects health/lives and attempts a room reload for some direct deaths. H is not teleportation. All routes, battles and death recovery cases are not yet verified.

## Beta 0.9.0: interface and original presentation

The application starts in a 960 × 720 window. It displays the entire original 256 × 240 frame at a visual 4:3 aspect ratio using nearest-neighbor filtering. It does not extend rooms or draw enemies outside that frame. Widescreen sources remain isolated for research and existing tests; the application has no switch to enable them.

Resizing prefers multiples of 320 × 240 (integer vertical scaling, 5:4 NES pixel aspect). Small windows and fullscreen use the largest fitting 4:3 rectangle, surrounded by black bars. `--fullscreen` starts fullscreen; F11 enters/exits it. PAL timing remains independent of window size and display refresh.

GOD is off by default in this build. `--god` or `--diagnostic` explicitly enable G/H debug controls; `--diagnostic --no-god` starts normal gameplay with logging. The application interface is in English; the original game language selector is preserved. Controls appear in a banner for 10 real seconds; F1 or gamepad Y dismisses it. Escape, normal window close and Cmd+Q open an exit confirmation with Cancel selected by default. Gameplay and device audio pause during the dialog; Cancel resumes with held inputs gated until release. Previous alpha packages do not contain these changes. See [the verification report](docs/ORIGINAL_PRESENTATION_1_0.md).

## Build without a ROM

Build requirements: CMake 3.20+, C/C++17 compiler and Python 3.9+. Verified SDL2/Lua source archives are included. On Windows use Ninja in MSYS2 UCRT64. Linux needs OpenGL and SDL platform development packages; see the CI recipe.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

For a standalone package, fetch the pinned import tools for the target and package the frontend. This downloads development runtimes, never a game:

```sh
python3 -B tools/public/fetch_runtime_tools.py --destination runtime-tools --cache runtime-cache --system macos-arm64
python3 -B tools/public/package_platform.py --build build --output dist --toolchain runtime-tools
```

Other target IDs: `macos-x86_64`, `windows-x86_64`, `linux-x86_64`. AppImage creation runs on Linux using `linuxdeploy` and `appimagetool`:

```sh
sh tools/public/package_linux.sh build
```

GitHub Actions builds the content-free engine for Linux x64, Windows x64 and both Mac architectures, runs synthetic tests, bundles verified import tools and uploads artifacts. It does not automatically publish releases or use a secret ROM. The optional local `import_probe` checks boot with your own ROM; CI never contains one.

See [architecture](docs/ARCHITECTURE.md), [publishing](docs/PUBLISHING.md), [verification](docs/VERIFICATION.md) and [third-party notices](THIRD_PARTY.md).

## License and compatibility names

Original engine/tool contributions use MIT. Dependencies retain their own licenses. This grants no rights to the commercial game or locally generated content. Game names identify compatibility; this independent project is not affiliated with or endorsed by its rights holders. Excluding game content is a technical safeguard, not a legal guarantee against claims.
