#!/bin/sh
set -eu
# Run on Linux after the public CMake build. linuxdeploy and appimagetool are
# packaging tools, not included in the frontend or fetched by the ROM importer.
python3 -B tools/public/package_platform.py --build "${1:-build}" --output dist --toolchain "${NES_BUNDLED_TOOLS:-runtime-tools}"
export APPIMAGE_EXTRACT_AND_RUN=1
"${LINUXDEPLOY:-linuxdeploy}" --appdir dist/NES-Native-Lab.AppDir \
  --desktop-file dist/NES-Native-Lab.AppDir/nes-native-lab.desktop \
  --icon-file assets/nes-native-lab.svg \
  --executable dist/NES-Native-Lab.AppDir/usr/bin/nes_native
"${APPIMAGETOOL:-appimagetool}" dist/NES-Native-Lab.AppDir "dist/NES-Native-Lab-Linux-$(uname -m).AppImage"
