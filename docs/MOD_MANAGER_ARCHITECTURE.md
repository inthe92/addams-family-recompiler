# Mod Manager — architecture, API v1

2026-10-13. Local development on `feature/mod-manager-v1`, based on `80af76aa5e4408f37dcb0e7047355c21be186ae8`. No commit, push, CI dispatch or release. The original checkout and its pending reports are retained separately.

The manager is a frontend feature, not part of the generated game module or ABI v3:

```text
SDL Mods panel -> C++ Manager -> local Python package helper -> private user storage
                       |
                  one Lua state per enabled package
                       |
               immutable host observation / API v1
```

- `tools/mods/packages.py`: ZIP/manifest validation, bounded extraction, installation, activation and removal. Python standard library only; uses the interpreter already shipped for ROM importing. Never executes Lua or downloads anything.
- `runtime/src/mod_process.cpp`: host-only subprocess transport using argv/CRT quoting, no shell, output capture and a 30-second wait limit. Derived from the project's existing process transport, without modifying the ROM importer.
- `mod_manager.cpp`: sorted package reconciliation, independent script states, lifecycle, persistent failure deactivation, bounded private logging and unclean-session recovery.
- `mod_lua.cpp`: read-only API, allocator/instruction limits, controlled libraries and protected calls.
- `mod_panel.cpp`, `mod_frontend.cpp`: existing bitmap font/OpenGL UI, responsive layout, keyboard/mouse/controller input and native ZIP picker on macOS/Windows. Linux uses drop-to-install.
- `game/include/game/mod_observation.hpp`: separate Addams PAL observer. Reads the existing diagnostic room field, current PPU frame and CPU cycle through side-effect-free getters. Does not install services, access PPU registers or change RAM.

Lua **5.5.1** is reused from the existing pinned `third_party/lua-5.5.1.tar.gz`, SHA-256 `1c4b4068d67061f2a2231ad2b5422e77acea1487ea9890f6320af614f4373dce`. CMake verifies it and links it statically. No new interpreter dependency, font, ZIP library, UI framework or worker thread was introduced. Existing Lua notices remain in the packages. See the [Lua 5.5 manual](https://www.lua.org/manual/5.5/manual.html) for allocator, protected-call and hook contracts; this implementation uses its three-argument `lua_newstate` with a fixed seed.

Managed scripts receive values, never Bus/CPUState/RuntimeServices or a pointer to live game memory. Optional player/room fields allow future observers without changing the script-facing function names. The manager itself has no ROM identity or game addresses. The initial PAL update interval is fixed to the existing host contract; another timing model would need an explicit host adaptation, not automatic NES compatibility.

`on_update` runs inside the original `if(simulate)` body, after the existing trusted legacy mod hook and before GOD/native execution. One call per executed NES unit; extra draws/swaps, pause, resize and overlays do not invoke it. CPU/PPU/APU/bus, the C emitter, `PresentationClock`, snapshot/cache behavior, PAL pacing and ABI v3 are unchanged. The frontend timing counter `mod_hook_calls` retains its legacy hook-call meaning; it is not a count of all managed Lua callbacks.

F2 or controller right-stick click opens Mods. The existing pause and input barrier stop simulation/audio and prevent controls leaking back into the game until neutral. F1, F11, game controls and quit confirmation retain their roles. Focus and normal close events still reach the original frontend. The panel never modifies the NES framebuffer.

`--no-mods` disables both managed loading and the old `--mod` script path; the managed frontend creates no storage, launches no helper and loads no scripts in this mode. The old trusted developer `--mod` API is retained for compatibility and **is not the managed read-only sandbox**. It is not exposed in the Mods list.
