# v0.9.0-beta: English frontend and safe exit

Date: 2026-10-12. Preparation branch: `beta/0.9.0-english-ui`, based on `cec57a99aded032201de5e1a3d5c4960c066909d`. Publication requires new native CI packages from the final commit; previous packages are not reused.

## Scope and architecture

Application-owned prompts, titles, progress, import errors and diagnostic messages are English. The original ROM's English/French/German selector and text are unchanged. Native operating-system file-picker chrome may follow the user's system language. Primary frontend strings live in `runtime/include/runtime/ui_text.hpp`; Python importer diagnostics remain beside their validation code.

The public version is 0.9.0-beta, CMake/package version 0.9.0, and window/diagnostic label 0.9.0-beta. ABI remains v3. Updating importer messages changes the existing generator fingerprint and causes local caches to be verified/regenerated normally. No hardware, instruction generation, compatibility profile or ABI semantics change.

`frontend_ui` supplies an independent state model and an original small bitmap alphabet. SDL events map to that model; the existing OpenGL presenter draws a separate alpha-blended texture. No UI pixels enter the NES framebuffer. The game remains 256x240, presented at 4:3, initially 960x720, with nearest scaling, F11 fullscreen, PAL pacing and GOD disabled by default. Experimental widescreen remains outside the frontend.

The controls banner starts after the first completed game frame, uses SDL's monotonic performance clock and expires after 10 real seconds (0.15-second fade-in, 0.25-second fade-out). F1 or controller Y dismisses it without consuming NES controls. Labels reflect the actual bindings: arrows/D-pad/stick move; Z/controller A jumps; X/controller B acts; Enter/controller Start is START/inventory; Right Shift/controller Back is SELECT. Escape/Guide opens the exit menu.

## Exit, focus and audio

Escape, normal window close and Cmd+Q request an English confirmation with Cancel selected initially. Keyboard arrows/Tab or controller D-pad/stick change the choice; Enter/Space/controller A activates it, Escape/controller B cancels, and mouse clicks select an explicit button. Guide depends on the operating system delivering that controller button. Held/repeated, simultaneous and same-batch navigation/confirmation inputs cannot trigger an accidental exit. A neutral release arms confirmation. Cancel allows safe return even if movement is still held, but game input stays zero until released.

While the menu is open or focus is lost, the frontend does not execute game slices or advance the bus. It clears controller input, pauses the SDL audio device and redraws the last frame. Resume resets the presentation deadline and waits for neutral game controls; queued audio is retained. Neither APU state nor PAL clocks change. Exit uses existing resource destruction and leaves user files/caches intact. During import, confirmed exit waits for the active compilation to finish safely and joins the worker before closing; it is not an immediate compiler cancellation.

## Local executed checks

Native macOS ARM64, macOS 27.0.1, Apple Clang 21, SDL 2.32.8; existing build directories were reconfigured/rebuilt for these changes, not represented as clean CI builds.

| Verification | Executed | Passed | Failed | Omitted |
|---|---:|---:|---:|---:|
| Desktop CTest | 43 | 42 | 1 (unchanged A02) | 0 |
| Headless CTest | 38 | 37 | 1 (unchanged A02) | 0 |
| New frontend cases (desktop) | 56 | 56 | 0 | 0 |
| Real OpenGL synthetic presentation probe | 12 | 12 | 0 | 0 |

The six new CTest entries contain banner 9, exit model 18, bitmap/layout 8, English/import 6, SDL event mapping 13 and audio pause/resume 2 cases. Headless has the four applicable entries (41 cases); SDL loader/audio/window tests are desktop-only by construction, not skipped at execution. Existing 4,736 generated native instruction cases, 82 IRQ cases, two stack programs, $2002, A01/A03, device, DMA/NMI, ABI and importer/cache tests still pass. CTest and the inventory/JUnit gate intentionally return failure for `apu_status_irq_collision_reference`. No expected-failure exemptions, filters or changed A02 expectations.

A private local compatible ROM was freshly imported into temporary storage outside Git. Real application checks reached the original language selector and visually verified the banner, English title, Cancel-default dialog, keyboard cancellation, mouse Cancel, normal close protection, F11 fullscreen and explicit exit. Cmd+Q requested confirmation; exit returned 0. During a menu pause and resume the logged CPU cycle and PPU frame were identical, while audio-paused changed from 1 to 0. No private ROM, generated module, game image or private log is included here. This is startup/UI verification, not a full playthrough or an audio-fidelity certification.

## Reproduction

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 3
ctest --test-dir build --verbose --output-on-failure --output-junit build/ctest-results.xml
ctest --test-dir build --show-only=json-v1 > build/ctest-inventory.json
python3 -B tests/verify_ctest_suite.py --inventory build/ctest-inventory.json --junit build/ctest-results.xml
# Expected overall failure: A02 only. No tests should be skipped.
cmake -S . -B build-headless -DNES_ENABLE_DESKTOP=OFF -DBUILD_TESTING=ON
cmake --build build-headless --parallel 3
ctest --test-dir build-headless --verbose --output-on-failure
# A real graphical desktop is required for this additional synthetic probe:
./build/presentation_gl_checks
```

## Modified components and remaining verification

Review of SDL's Windows entry-point contract also added `SDL_MAIN_HANDLED` and `SDL_SetMainReady()` to the new audio harness, consistent with the existing SDL test targets. The preliminary CI run was cancelled to avoid producing superseded packages. Only results for the final commit qualify for publication.

Version/header and CMake; desktop app; frontend state/event/text/bitmap files; GL overlay; SDL audio pause; ROM selection/progress and importer messages; diagnostic version header; frontend tests and existing package-language expectations; CTest inventory gate; READMEs, Windows instructions and project status. Existing unrelated local reports are preserved and excluded from this task's commit.

Windows x64, macOS ARM64/Intel and Linux x64 must each perform a fresh native build, all 43 CTest entries and their existing package checks on the new SHA. Independent packaging steps remain enabled despite A02; the overall workflow must remain red for that failure. Validate package identity/version/ABI/icons, synthetic import/module loading and clean extraction before release. Physical controller navigation, Alt+F4, Linux/Steam Deck compatible-ROM play, resizing and platform-specific visual checks still need hands-on testing where no graphical session is available. No universal game compatibility or stable 1.0 claim is made.
