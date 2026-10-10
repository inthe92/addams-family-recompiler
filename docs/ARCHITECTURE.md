# Architecture

[Español](ARCHITECTURE.es.md)

The public frontend builds `nes_runtime`, `user_game_view`, `user_rom` and `desktop_presenter`. It has no `game_recompiled` target and no embedded instruction banks, PRG/CHR or extracted maps.

```text
Frontend + generic tools + instruction-address profile
                         |
                    User's ROM
                         |
                 Full SHA-256 validation
                         |
                  Local C generation
                         |
                Bundled Python + Zig
                         |
              Private native game module
                         |
                NES runtime + SDL/OpenGL
```

`compatible_profile.json` contains only schema, SHA-256 identity, size and instruction start addresses. Opcodes and operands come from the imported ROM. The generated C uses a C service ABI for CPU state, flags, memory and execution budgets. Instructions are translated before execution; the runtime does not decode them in an interpreter. Uncovered addresses stop the alpha. Selected RAM-installed interrupt jumps have bounded handling; there is no emulator fallback.

GOD, inventory and display changes are original patches tied to RAM fields of this revision. They are not a generic API for all NES games. Widescreen preserves the central raster and uses locally generated horizontal maps for side areas. Additional geometry in vertical rooms is not validated, and enemy activation still follows original visibility rules.

The private cache contains generated game content and a receipt with hashes. Generator/profile/ABI/architecture/toolchain changes or module changes invalidate it. The original ROM is checked on every start. Imports spawn processes through argument arrays, without a shell, and never download a ROM.

Default SDL user-data paths:

- macOS: `~/Library/Application Support/nes-native-lab/user-import/`
- Linux: normally `~/.local/share/nes-native-lab/user-import/`
- Windows: under `%APPDATA%`, `nes-native-lab/user-import/`

`--rom` selects the input; `--import-cache` overrides private output; `--import-only` prepares the module without opening the game. `import.log` records failures. Keep the ROM available at its selected path.
