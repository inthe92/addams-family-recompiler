cmake_minimum_required(VERSION 3.20)
project(nes_native_lab VERSION 0.2.0 LANGUAGES C CXX)
# Keep filesystem UTF-8 paths as std::string, including on newer GCC defaults.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(APPLE)
  enable_language(OBJC OBJCXX)
endif()
set(NES_ENABLE_MODS ON CACHE BOOL "Native Lua mod support")
set(NES_ENABLE_DESKTOP ON CACHE BOOL "Desktop application")
include(cmake/dependencies.cmake)

# The public build has no recovery source directory, generated game sources,
# embedded PRG/CHR, or game_recompiled target.
add_library(nes_runtime STATIC runtime/src/ram.cpp runtime/src/cartridge.cpp
  runtime/src/video_state.cpp runtime/src/audio_state.cpp runtime/src/wav_writer.cpp runtime/src/bus.cpp)
target_include_directories(nes_runtime PUBLIC include runtime/include)
target_compile_features(nes_runtime PUBLIC cxx_std_17)
if(NES_ENABLE_MODS)
  target_sources(nes_runtime PRIVATE runtime/src/mod_engine.cpp)
  target_link_libraries(nes_runtime PUBLIC lua_embedded)
  target_compile_definitions(nes_runtime PUBLIC NES_ENABLE_MODS=1)
endif()

add_library(user_game_view STATIC game/src/world_view.cpp game/src/actor_view.cpp game/src/god_mode.cpp)
target_include_directories(user_game_view PUBLIC game/include)
target_compile_definitions(user_game_view PUBLIC NES_USER_ROM=1)
target_link_libraries(user_game_view PUBLIC nes_runtime)

add_library(user_rom STATIC runtime/src/user_rom.cpp)
target_link_libraries(user_rom PUBLIC user_game_view SDL2::SDL2-static)
if(APPLE)
  target_sources(user_rom PRIVATE runtime/src/user_rom_mac.mm)
  target_link_libraries(user_rom PUBLIC "-framework Cocoa")
elseif(WIN32)
  target_link_libraries(user_rom PUBLIC comdlg32)
endif()

add_library(desktop_presenter STATIC runtime/src/gl_presenter.cpp runtime/src/sdl_audio_output.cpp)
target_link_libraries(desktop_presenter PUBLIC nes_runtime SDL2::SDL2-static OpenGL::GL)
add_executable(nes_native MACOSX_BUNDLE app/desktop.cpp)
target_link_libraries(nes_native PRIVATE user_rom desktop_presenter)
target_compile_definitions(nes_native PRIVATE NES_ALPHA_BUILD=1 SDL_MAIN_HANDLED=1)
set_target_properties(nes_native PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "NES Native Lab"
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.nes-native-lab.preview"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "0.2.0"
  MACOSX_BUNDLE_BUNDLE_VERSION "0.2.0")
if(WIN32)
  # Both SDL and the C++ runtime are static in the MinGW portable build.
  target_link_options(nes_native PRIVATE -static -static-libgcc -static-libstdc++)
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
  add_test(NAME public_import_contract COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/public_import_checks.py")
  add_executable(import_probe tests/import_probe.cpp)
  target_link_libraries(import_probe PRIVATE user_rom)
  if(WIN32)
    target_link_options(import_probe PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
endif()
