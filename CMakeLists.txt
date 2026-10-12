cmake_minimum_required(VERSION 3.20)
# Set before project() so every Apple language/dependency uses the same floor.
# The pinned Zig importer requires macOS 15; other platforms ignore this cache.
if(CMAKE_HOST_APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS deployment version" FORCE)
endif()
project(nes_native_lab VERSION 0.2.1 LANGUAGES C CXX)
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

add_library(user_game_view STATIC game/src/world_view.cpp game/src/actor_view.cpp game/src/god_mode.cpp
  game/src/gameplay_diagnostic.cpp)
target_include_directories(user_game_view PUBLIC game/include)
target_compile_definitions(user_game_view PUBLIC NES_USER_ROM=1)
target_link_libraries(user_game_view PUBLIC nes_runtime)

if(NES_ENABLE_DESKTOP)
  find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
  set(window_icon_header "${CMAKE_CURRENT_BINARY_DIR}/branding/application_icon_data.hpp")
  add_custom_command(OUTPUT "${window_icon_header}"
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/public/icon_assets.py"
      --png "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/nes-native-lab-64.png" --emit-header "${window_icon_header}"
    DEPENDS tools/public/icon_assets.py assets/icons/nes-native-lab-64.png
    COMMENT "Embedding official window icon (lossless RGBA, no image library)" VERBATIM)
  add_library(application_icon STATIC runtime/src/application_icon.cpp "${window_icon_header}")
  target_include_directories(application_icon PUBLIC runtime/include PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/branding")
  target_link_libraries(application_icon PUBLIC SDL2::SDL2-static)
  if(APPLE)
    target_sources(application_icon PRIVATE runtime/src/application_icon_mac.mm)
    target_link_libraries(application_icon PUBLIC "-framework Cocoa")
  endif()
  add_library(user_rom STATIC runtime/src/user_rom.cpp)
  target_link_libraries(user_rom PUBLIC user_game_view application_icon SDL2::SDL2-static)
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
  set(build_arch "${CMAKE_SYSTEM_PROCESSOR}")
  if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(build_arch "${CMAKE_OSX_ARCHITECTURES}")
  endif()
  add_custom_target(native_build_metadata
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/public/build_identity.py"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --version "${PROJECT_VERSION}"
      --system "${CMAKE_SYSTEM_NAME}" --arch "${build_arch}"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/branding/native_build_metadata.hpp"
      "${CMAKE_CURRENT_BINARY_DIR}/native-build-info.json"
    COMMENT "Recording source revision, version and ABI (including dirty state)" VERBATIM)
  add_dependencies(nes_native native_build_metadata)
  target_include_directories(nes_native PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/branding")
  set_target_properties(nes_native PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "NES Native Lab"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.nes-native-lab.preview"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  if(APPLE)
    target_sources(nes_native PRIVATE assets/icons/nes-native-lab.icns)
    set_source_files_properties(assets/icons/nes-native-lab.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(nes_native PROPERTIES MACOSX_BUNDLE_ICON_FILE "nes-native-lab.icns")
  endif()
  if(WIN32)
    enable_language(RC)
    set(NES_ICON_ICO "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/nes-native-lab.ico")
    configure_file(assets/icons/windows_icon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/branding/windows_icon.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/branding/windows_icon.rc"
      PROPERTIES OBJECT_DEPENDS "${NES_ICON_ICO}")
    target_sources(nes_native PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/branding/windows_icon.rc")
    # Both SDL and the C++ runtime are static in the MinGW portable build.
    target_link_options(nes_native PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
  add_test(NAME package_identity_contract COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/package_identity_checks.py")
  set_tests_properties(package_identity_contract PROPERTIES LABELS "packaging;identity;synthetic" TIMEOUT 30)
  add_test(NAME official_icon_assets COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/icon_asset_checks.py")
  set_tests_properties(official_icon_assets PROPERTIES LABELS "branding;packaging" TIMEOUT 30)
  if(NES_ENABLE_DESKTOP)
    add_executable(window_icon_checks tests/window_icon_checks.cpp)
    target_link_libraries(window_icon_checks PRIVATE application_icon)
    target_compile_definitions(window_icon_checks PRIVATE SDL_MAIN_HANDLED=1)
    if(WIN32)
      target_link_options(window_icon_checks PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME window_icon_contract COMMAND "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/window_icon_checks.py" "$<TARGET_FILE:window_icon_checks>")
    set_tests_properties(window_icon_contract PROPERTIES LABELS "branding;sdl" TIMEOUT 30 ENVIRONMENT "SDL_VIDEODRIVER=dummy")
  endif()
  add_test(NAME public_import_contract COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/public_import_checks.py")
  add_test(NAME diagnostic_harness_contract COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/known_divergence_checks.py")
  set_tests_properties(diagnostic_harness_contract PROPERTIES LABELS "harness;synthetic" TIMEOUT 30)
  add_test(NAME ctest_verification_contract COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_ctest_suite_checks.py")
  set_tests_properties(ctest_verification_contract PROPERTIES LABELS "harness;ci;synthetic" TIMEOUT 30)
  set(test_fixture_dir "${CMAKE_CURRENT_BINARY_DIR}/tests/generated")
  add_custom_command(
    OUTPUT "${test_fixture_dir}/opcode_fixtures.c" "${test_fixture_dir}/opcode_fixtures.hpp"
      "${test_fixture_dir}/timing_fixture.c" "${test_fixture_dir}/program_fixture.c"
      "${test_fixture_dir}/bus_timing_fixture.c"
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/emit_opcode_fixtures.py" "${test_fixture_dir}"
    DEPENDS tests/emit_opcode_fixtures.py tools/recompiler/emit_blocks.py tools/recompiler/opcodes_6502.py
    COMMENT "Generating synthetic native CPU test fixtures (no ROM)"
    VERBATIM)
  add_library(native_test_fixtures STATIC "${test_fixture_dir}/opcode_fixtures.c"
    "${test_fixture_dir}/timing_fixture.c" "${test_fixture_dir}/program_fixture.c"
      "${test_fixture_dir}/bus_timing_fixture.c")
  target_include_directories(native_test_fixtures PUBLIC "${test_fixture_dir}" include game/include)
  set_target_properties(native_test_fixtures PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
  add_executable(opcode_codegen_checks tests/opcode_codegen_checks.cpp)
  target_link_libraries(opcode_codegen_checks PRIVATE native_test_fixtures)
  add_test(NAME native_opcode_contract COMMAND opcode_codegen_checks)
  set_tests_properties(native_opcode_contract PROPERTIES LABELS "cpu;synthetic" TIMEOUT 30)
  add_executable(native_program_checks tests/native_program_checks.cpp)
  target_link_libraries(native_program_checks PRIVATE native_test_fixtures)
  add_test(NAME native_stack_programs COMMAND native_program_checks)
  set_tests_properties(native_stack_programs PROPERTIES LABELS "cpu;synthetic" TIMEOUT 30)
  # PAL program contracts and independent CPU-poll controls remain ordinary
  # tests, including the regressions that fail before the production fix.
  add_custom_command(
    OUTPUT "${test_fixture_dir}/irq_poll_fixtures.c" "${test_fixture_dir}/irq_poll_fixtures.hpp"
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/emit_irq_poll_fixtures.py" "${test_fixture_dir}"
    DEPENDS tests/emit_irq_poll_fixtures.py tools/recompiler/emit_blocks.py tools/recompiler/opcodes_6502.py
    COMMENT "Generating synthetic native IRQ-poll fixtures (no ROM)"
    VERBATIM)
  add_library(irq_poll_test_fixtures STATIC "${test_fixture_dir}/irq_poll_fixtures.c")
  target_include_directories(irq_poll_test_fixtures PUBLIC "${test_fixture_dir}" include game/include)
  set_target_properties(irq_poll_test_fixtures PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
  add_executable(cpu_irq_poll_checks tests/cpu_irq_poll_checks.cpp)
  target_link_libraries(cpu_irq_poll_checks PRIVATE irq_poll_test_fixtures nes_runtime)
  foreach(contract pal08_alignment0 pal08_alignment1 pal07 held_clear held_no_clear held_masked branches)
    add_test(NAME cpu_irq_poll_${contract} COMMAND cpu_irq_poll_checks "${contract}")
    set_tests_properties(cpu_irq_poll_${contract} PROPERTIES LABELS "cpu;irq-poll;synthetic" TIMEOUT 30)
  endforeach()
  set_tests_properties(cpu_irq_poll_pal08_alignment0 cpu_irq_poll_pal08_alignment1 cpu_irq_poll_pal07
    PROPERTIES LABELS "cpu;apu;irq-poll;synthetic;pal-published-contract")
  add_custom_command(
    OUTPUT "${test_fixture_dir}/irq_edge_fixtures.c" "${test_fixture_dir}/irq_edge_fixtures.hpp"
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/emit_irq_edge_fixtures.py" "${test_fixture_dir}"
    DEPENDS tests/emit_irq_edge_fixtures.py tests/emit_irq_poll_fixtures.py tools/recompiler/emit_blocks.py tools/recompiler/opcodes_6502.py
    COMMENT "Generating synthetic IRQ edge controls (no ROM)" VERBATIM)
  add_library(irq_edge_test_fixtures STATIC "${test_fixture_dir}/irq_edge_fixtures.c")
  target_include_directories(irq_edge_test_fixtures PUBLIC "${test_fixture_dir}" include game/include)
  set_target_properties(irq_edge_test_fixtures PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
  add_executable(cpu_irq_edge_checks tests/cpu_irq_edge_checks.cpp)
  target_link_libraries(cpu_irq_edge_checks PRIVATE irq_edge_test_fixtures user_game_view)
  foreach(contract masks edges priority_dma adapters)
    add_test(NAME cpu_irq_edge_${contract} COMMAND cpu_irq_edge_checks "${contract}")
    set_tests_properties(cpu_irq_edge_${contract} PROPERTIES LABELS "cpu;irq-poll;synthetic;runtime-poll-contract" TIMEOUT 30)
  endforeach()
  add_executable(ppustatus_timing_check tests/ppustatus_timing_check.cpp)
  target_link_libraries(ppustatus_timing_check PRIVATE native_test_fixtures nes_runtime)
  add_test(NAME ppustatus_read_timing COMMAND ppustatus_timing_check)
  set_tests_properties(ppustatus_read_timing PROPERTIES LABELS "ppu;synthetic;bus-timing" TIMEOUT 30)
  add_executable(bus_timing_checks tests/bus_timing_checks.cpp)
  target_link_libraries(bus_timing_checks PRIVATE native_test_fixtures nes_runtime)
  add_test(NAME native_bus_side_effects COMMAND bus_timing_checks)
  set_tests_properties(native_bus_side_effects PROPERTIES LABELS "cpu;ppu;synthetic;bus-timing" TIMEOUT 30)
  add_test(NAME import_cache_abi_contract COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/import_cache_checks.py")
  set_tests_properties(import_cache_abi_contract PROPERTIES LABELS "abi;synthetic" TIMEOUT 30)
  # The dynamic module loader uses SDL and belongs to the desktop build.
  # Keep its ABI regression mandatory whenever that loader is built.
  if(NES_ENABLE_DESKTOP)
    foreach(abi missing old v2 current)
      add_library(module_abi_${abi} MODULE tests/module_abi_fixture.c)
      target_include_directories(module_abi_${abi} PRIVATE include)
      set_target_properties(module_abi_${abi} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endforeach()
    target_compile_definitions(module_abi_old PRIVATE TEST_ABI_VERSION=1u)
    target_compile_definitions(module_abi_v2 PRIVATE TEST_ABI_VERSION=2u)
    target_compile_definitions(module_abi_current PRIVATE TEST_ABI_VERSION=NES_RECOMP_ABI_VERSION)
    add_executable(module_abi_checks tests/module_abi_checks.cpp)
    target_link_libraries(module_abi_checks PRIVATE user_rom)
    add_dependencies(module_abi_checks module_abi_missing module_abi_old module_abi_v2 module_abi_current)
    add_test(NAME module_abi_contract COMMAND module_abi_checks
      "$<TARGET_FILE:module_abi_missing>" "$<TARGET_FILE:module_abi_old>" "$<TARGET_FILE:module_abi_v2>" "$<TARGET_FILE:module_abi_current>")
    set_tests_properties(module_abi_contract PROPERTIES LABELS "abi;synthetic" TIMEOUT 30)
  endif()
  add_executable(device_checks tests/device_checks.cpp)
  target_link_libraries(device_checks PRIVATE nes_runtime)
  foreach(device ram mmc1 bus ppu)
    add_test(NAME synthetic_${device}_contract COMMAND device_checks "${device}")
    set_tests_properties(synthetic_${device}_contract PROPERTIES LABELS "device;synthetic;${device}" TIMEOUT 30)
  endforeach()
  add_executable(gameplay_diagnostic_checks tests/gameplay_diagnostic_checks.cpp)
  target_link_libraries(gameplay_diagnostic_checks PRIVATE user_game_view native_test_fixtures)
  add_test(NAME gameplay_diagnostic_contract COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/gameplay_diagnostic_checks.py" "$<TARGET_FILE:gameplay_diagnostic_checks>")
  set_tests_properties(gameplay_diagnostic_contract PROPERTIES LABELS "gameplay;diagnostic;synthetic" TIMEOUT 30)
  add_executable(apu_audio_checks tests/apu_audio_checks.cpp)
  target_link_libraries(apu_audio_checks PRIVATE nes_runtime native_test_fixtures)
  add_test(NAME apu_audio_contract COMMAND apu_audio_checks core)
  set_tests_properties(apu_audio_contract PROPERTIES LABELS "apu;audio;synthetic" TIMEOUT 30)
  # Audit-only reference discrepancies stay ordinary failing tests. No xfail,
  # WILL_FAIL or skip: an unfiltered CTest run must expose the production defect.
  foreach(reference status_open_bus status_irq_collision frame_mode_delay)
    add_test(NAME apu_${reference}_reference COMMAND apu_audio_checks "${reference}")
    set_tests_properties(apu_${reference}_reference PROPERTIES LABELS "apu;audio;synthetic;known-divergence" TIMEOUT 30)
  endforeach()
  set_tests_properties(apu_status_open_bus_reference PROPERTIES LABELS "apu;audio;synthetic")
  set_tests_properties(apu_frame_mode_delay_reference PROPERTIES LABELS "apu;audio;synthetic")
  add_test(NAME apu_frame_mode_edges_contract COMMAND apu_audio_checks frame_mode_edges)
  set_tests_properties(apu_frame_mode_edges_contract PROPERTIES LABELS "apu;audio;synthetic" TIMEOUT 30)
  if(NES_ENABLE_DESKTOP)
    add_executable(apu_sdl_queue_checks tests/apu_sdl_queue_checks.cpp)
    target_link_libraries(apu_sdl_queue_checks PRIVATE desktop_presenter)
    target_compile_definitions(apu_sdl_queue_checks PRIVATE SDL_MAIN_HANDLED=1)
    add_test(NAME apu_sdl_queue_contract COMMAND apu_sdl_queue_checks)
    set_tests_properties(apu_sdl_queue_contract PROPERTIES LABELS "audio;synthetic;sdl" TIMEOUT 30
      ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    if(WIN32)
      target_link_options(apu_sdl_queue_checks PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
  endif()
  if(NES_ENABLE_DESKTOP)
    add_executable(import_probe tests/import_probe.cpp)
    target_link_libraries(import_probe PRIVATE user_rom)
    if(WIN32)
      target_link_options(import_probe PRIVATE -static -static-libgcc -static-libstdc++)
      target_link_options(module_abi_checks PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
  endif()
  if(WIN32)
    foreach(test_target opcode_codegen_checks native_program_checks cpu_irq_poll_checks cpu_irq_edge_checks ppustatus_timing_check device_checks bus_timing_checks gameplay_diagnostic_checks apu_audio_checks)
      target_link_options(${test_target} PRIVATE -static -static-libgcc -static-libstdc++)
    endforeach()
  endif()
endif()
