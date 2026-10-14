# Mod Manager Completion — delivery roadmap

2026-10-14. Local feature/mod-manager-completion, historical HEAD95c194a plus
recovered dirty SDK work. No commit/remote action or beta/main modification.

## Delivery01 (implemented and locally verified)

A. Recover the full functional Phase5 tree:136 SHA256 matches, original tree intact.
B. Main menu Play/Mods/Settings/Exit and context-specific Back; preserve mods-only.
C. Installed list/details/actions and readable per-package errors.
D. Optional bounded API2 typed options, same state store, actual Lua config getter.
E. Named profiles with Original, create/save/rename/apply and validation before commit.
F. Three unchanged Phase5 ZIPs together, original fallback, safe mode and regression suites.

## Next bounded delivery

1. Native Windows/Intel/Linux CI after authorization; physical Deck controls and
   local file installation. Preserve historical Linux high-refresh GL issue.
2. Longer manual Mac/Deck game session exercising F2/pause/audio/neutral input,
   profile/config changes, restart/recovery. No full-game certification here.
3. Profile update/delete/export with explicit confirmation and versioned schema,
   user-friendly dependency diagnostics and adversarial storage fuzzing.
4. UI accessibility and gamepad text entry polish. Existing D-pad/X/Y name editor
   works without mouse; analog navigation and system onscreen keyboard are future.
5. Consider package updates/explicit permissions review staging only after these
   contracts are validated. No Internet updater/channel or new Scene API in01.

Hardware/core/ABI/PAL behavior, new Super Gómez physics and 2.5D are outside this
completion delivery. A02 remains ordinary FAIL; progress is never an all-suite PASS.

## Delivery02 local continuation

Implemented profile deletion/export/inspection/import, explicit Cancel-default
confirmations, typed-default restoration and no-op VM preservation. Analog
neutral-rearmed navigation and native profile pickers are included. Existing
mod versions, original gameplay/hardware/ABI/PAL and public beta are preserved.
See MOD_MANAGER_COMPLETION_02.md for actual results and remaining native-platform,
physical-controller and longer-gameplay checks. No Internet updater or future
Scene API is introduced. The older future-work list above is historical.
