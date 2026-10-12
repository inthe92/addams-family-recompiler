NES Native Lab v0.9.0-beta — Windows x64 portable package
=======================================================
Extract the ENTIRE ZIP into a writable folder, then open nes_native.exe.
Keep all companion folders with the executable. Do not run inside the ZIP.

Select your own compatible European PAL NES ROM (En/Fr/De). The application
checks its SHA-256 before importing. No game or ROM download is included.
Python and Zig are bundled; you do not need to install development tools.
The first local compilation may take a few minutes.

Controls: arrow keys, Z (jump), X (action), Enter (start/inventory), right
Shift (select). F11 toggles fullscreen. Escape opens the exit menu; Cancel
is selected by default. F1 or gamepad Y dismisses the temporary help.
A supported SDL controller can be used. GOD is OFF by default.

This is an experimental beta. A02 remains a known PAL APU timing/reference
failure in CI. Compatible-ROM gameplay on consumer PCs still needs testing.
This Windows application does not have an Authenticode signature.

Import errors include details and the diagnostic log location, normally
%APPDATA%\nes-native-lab\user-import\import.log. Never upload your ROM or
private game module/cache when reporting a problem.
Keep LICENSE, THIRD_PARTY.md, licenses/ and toolchain notices intact.
