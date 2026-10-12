#pragma once
#include "runtime/app_version.hpp"
namespace nes::ui::text {
inline constexpr const char* name="NES Native Lab";
inline constexpr const char* select_rom="Select your NES ROM";
inline constexpr const wchar_t* select_rom_w=L"Select your NES ROM";
inline constexpr const char* rom_instruction="Select or drag and drop your compatible European PAL NES ROM to start playing. This application does not include the game.";
inline constexpr const char* drop_title="Drop your compatible NES ROM here";
inline constexpr const char* drop_help="Drag and drop your compatible .nes file into this window.\nYou can also start with --rom <path>.\nPython and the compiler are included in the package.";
inline constexpr const char* preparing="Preparing your game...";
inline constexpr const char* compiling="Compiling game code. This may take a few minutes...";
inline constexpr const char* preparing_help="Checking your ROM and preparing the native game.\nThe first compilation may take a few minutes.";
inline constexpr const char* load_error="Unable to load the game. Check the diagnostic log for details.";
inline constexpr const char* experimental="Experimental beta. Some game situations still need verification.";
inline constexpr const char* quit_title="Exit NES Native Lab?";
inline constexpr const char* quit_question="Are you sure you want to quit the game?";
inline constexpr const char* cancel="Cancel";
inline constexpr const char* exit_game="Exit Game";
inline constexpr const char* controls_title="Controls";
inline constexpr const char* controls[]={
    "ARROWS: MOVE   Z: JUMP   X: ACTION",
    "ENTER: START / INVENTORY   RSHIFT: SELECT",
    "GAMEPAD: D-PAD/STICK, A/B, START/BACK",
    "F11: FULLSCREEN  ESC/GUIDE: EXIT MENU  F1/Y: HIDE"};
}
