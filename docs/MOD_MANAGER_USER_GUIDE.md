# Mod Manager — experimental Completion02 user guide

This is a local experimental development app, not an update to public beta0.9.0.
No commercial game, ROM or generated game module is distributed. Use your owned
compatible PAL ROM through the existing importer. Normal double-click opens:
**Play / Mods / Settings / Exit**. Play starts the existing ROM validation/import.
Settings selects fullscreen/mute for this launch only; F11 remains available.
Automated --frames/--unthrottled/import-only/capture launches bypass the menu.

## Controls and navigation

Arrows/D-pad navigate, left/right selects bottom actions; Enter/Space/gamepad A
confirms; Escape/gamepad B returns. Mouse clicks work too. F2/right-stick opens
Mods during play and preserves the existing pause/audio/input-neutral barrier.
From main-menu Mods, Back returns to the menu. During play it returns to the game.
`--mods-only` opens the same manager without a game: Back closes that diagnostic mode.

In Installed, D/gamepad X opens Details; C/Y opens Configure; P/left shoulder
opens Profiles. Click the labelled lower navigation strip for these pages.
Details shows full metadata, category, status, API, permissions, requirements,
conflicts and errors; Up/Down scrolls long descriptions. Actions: Enable/Disable,
Configure, Uninstall, Back. Uninstall requires a second activation.

## Install and manage

Choose Install ZIP on macOS/Windows; Linux currently installs by dropping a ZIP
onto the open Mods window. Packages start Disabled. Installation/inspection never
runs scripts. Select Enable explicitly; review permissions in Details first.
Dependency/conflict errors explain what must be installed/enabled/disabled.
A script error disables the offending mod and dependent mods; other mods continue.
A corrupted individual installed package appears as a disabled error row, removable
with Uninstall. Root/global-state corruption and stale locks require manual review.

Existing Super-Gomez-Gameplay-v1.0.zip, Speedrun-HUD-v1.0.zip and
Mansion-Parallax-Demo-v1.0.zip remain unchanged and can be active together.
Super jump is experimental and restricted to the previously approved first-room
states; no speed feature. Timer distinguishes SIM/REAL/PAUSED; the PNG camera is
synthetic decoration, not real scene depth. Ctrl+Alt+R/U/V retains its previous
reset timer/hide UI/hide layer behavior.

## Configuration

A mod with no declared settings says so; the manager does not invent options.
The additional Configurable-Jump-Example-v1.0.zip demonstrates real settings:
0..12 jump extension, boolean label visibility, 0..1 panel opacity and top/bottom
label position. Disable Super Gómez before enabling this alternate jump owner.
Configure: Up/Down selects, Decrease/Increase edits, Apply persists changed values
and restarts only a changed active mod at the paused UI boundary. Applying the
same values does not restart Lua. R/gamepad X opens Restore Defaults, with Cancel
selected initially. Defaults need explicit confirmation. Back discards unapplied edits.
A current trajectory is not rewound; compare the next valid jump or fresh session.

## Profiles

Original disables managed packages without deleting them or their settings.
It differs from forced `--no-mods`, which bypasses helpers/loading/storage and
also disables the legacy trusted script path. Original does not override explicit
legacy developer `--mod`/--god options; ordinary launches have those off.

Enable the desired mods and apply their settings, then Profiles > Save New.
Accept the generated name with A or type a custom name; Backspace deletes.
With gamepad: X deletes, Y appends A, Up/Down cycles the last character, A saves.
Rename uses the same editor; Original cannot be renamed. Saving a name already
present asks confirmation before updating that named profile with current selections/settings. Review the
name before saving. Profiles contain enabled mod IDs and per-mod settings, not
ROMs, gameplay saves or executable code. At most16 user profiles. Names are case-insensitive for uniqueness.

Select Apply twice to confirm after reviewing its package permissions in Details.
Missing/incompatible dependencies, duplicate jump owners, conflicting resources
or invalid saved settings reject the operation before persistence. Installed
packages are never downloaded automatically. Removed packages leave old saved
profiles requiring repair rather than silently dropping their requirements.
Script activation failures after commit are isolated and displayed; atomic state
validation does not mean an OS sandbox or guaranteed all-VM activation.

## Storage and safety

The experimental default directory is SDL_GetPrefPath("nes-native-lab",
"mods-completion02"), separate from older development hosts. An explicit
--mods-dir still takes precedence. Same storage structure: packages/, state.json, bounded private logs and
session marker. Optional `--mods-dir` isolates experiments. No second database.
Settings and profiles use schema1 under state.json's management section; existing
v1 enabled/errors state is read compatibly. Older pre-Completion helpers cannot
read the added management section: use the demo's separate --mods-dir and do not
point an older development host at an upgraded store. A previous unclean session disables
previously enabled packages conservatively. No caches/ROM/configuration outside
the manager-owned store are removed. Use reviewed mods; Lua isolation is in-process.

Native macOS ARM64 tested here. Windows/macOS Intel/Linux/Steam Deck need their
own execution and physical controls tests; neither target existence nor synthetic
controller events constitute that certification. A02 stays visibly failing.

## Profile operations — Completion02

In Profiles, O/gamepad right shoulder or the labelled strip opens Delete/Export/
Import/Back. Delete requires explicit confirmation and never removes packages or
other profiles. Original is immutable. Deleting the active profile removes its
saved name only; current mod selection remains until you choose another profile.

Export uses a JSON picker on macOS/Windows. Linux exports to exports/ under your
mod store and displays its path. Existing files are never overwritten: choose a
new filename. Export currently needs hard-link support in the destination file
system for atomic, no-overwrite publication; use local APFS/NTFS/ext4 if another
file system refuses the operation, then copy the JSON normally.

Import uses the JSON picker on macOS/Windows; on Linux drop a profile .json onto
Mods. Case-insensitive .JSON drops are accepted. Inspection checks schema1, game/
engine context, exact mod/API versions, dependencies, conflicts and typed settings.
Missing/incompatible packages cause an actionable error, with no state changes;
install compatible ZIPs first. Profiles never contain/download Lua, ROMs, private
modules or caches. Original is built-in, so a portable file named Original cannot
replace/import over it. A reviewed custom profile starts inactive.

Cancel is initially selected. Select Confirm explicitly to import. The file's
SHA256 must still match the review; changes require a new inspection. Import
saves the profile only. Select that profile and Apply explicitly to change mods.
Duplicate names are rejected; rename/delete the old profile first. Imported exact
version requirements persist and are rechecked on future Apply. Profiles are
currently pinned to the exporting numeric engine version and game profile.

A saved profile captures default values as well as configured values for enabled
mods. Applying it restores those values, while preserving current settings of
unmentioned disabled mods. Switching profiles is not a gameplay rewind. No Lua
runs during inspection/export/import validation. Activation happens afterward
at paused UI boundaries; failures are isolated and reported, not an atomic all-VM
success guarantee. Callback failure disables dependents conservatively.

Left-stick navigation uses a dead zone and neutral rearm, with at most one
navigation per event batch to prevent stick+D-pad double actions. It intentionally
does not repeat while held. Focus loss cancels staged confirmations. Blank controls
in Settings/name/confirmation pages are skipped by keyboard/D-pad navigation.
Physical-controller certification still needs real hardware.

## Local macOS Completion02 delivery

Use Start-Experimental.command or Open-Mod-Manager.command beside the verified
ZIP. They extract it in a fresh /private/tmp folder, strictly verify the ad-hoc
signature and launch; only that extraction is removed on exit. Persistent mods
remain in ExperimentalUserData beside the commands. A managed Documents/iCloud
folder may add FinderInfo metadata to an extracted app; that copy cannot be
assumed to retain strict signature validity. Original ZIP bytes are untouched.
No quarantine removal or re-signing is part of these commands.

## Experimental Actions packages (Completion03)

Download only mod-manager-beta artifacts from the exact verified Actions SHA
linked in MOD_MANAGER_COMPLETION_03.md. These are separate experimental builds;
retain public beta0.9.0 unchanged. Extract the complete package, use the supplied
Python/Zig/helper/resources and keep the separate experimental mod store.
For gameplay open the normal application/main menu and choose Play. Back in
--mods-only closes that diagnostic mode; during gameplay F2 opens Mods and Back
returns to play. Super/Speedrun/Parallax can coexist; Configurable Jump replaces
Super's exclusive owner. A02 remains a real failure, not all-suite PASS.
