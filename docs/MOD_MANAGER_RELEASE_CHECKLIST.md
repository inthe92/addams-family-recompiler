# Experimental Mod Manager — release and manual acceptance checklist

2026-10-14, Completion02. Not a publication authorization or stable release.

## Source and package gates

- [ ] Review cumulative public allowlist, SHA/provenance manifest, licences/pinned
      Lua/SDL/Python/Zig; no ROM/generated modules/receipts/caches/private captures.
- [ ] Exact authorized branch/SHA on all four native jobs, strict complete JUnit.
- [ ] A02 remains real FAIL; no other failures or omitted tests. Eligibility
      evidence permits diagnostics without relabelling the full suite.
- [ ] Archive integrity/hash/extraction outside build, including spaces; complete
      helper/importer/headers, Python/Zig, icons and experimental component marker.
- [ ] Native dependencies, ABI3 synthetic compile/load, safe-mode boot and manager
      package/config/profile transactions using bundled tools.
- [ ] macOS strict ad-hoc signature before/after extraction; not notarized.
      Windows Authenticode remains absent. Linux AppImage/FUSE and desktop launch.
- [ ] Default experimental store separate from older hosts; no automatic migration
      or deletion. Explicit --mods-dir can opt into a reviewed private test store.

## macOS and Steam Deck manual session

Use a separate extracted package/store and an owned supported ROM. Never attach
ROMs, modules, screenshots of commercial content or private traces to public CI.

1. Launch Main: keyboard, mouse, D-pad/stick; Play/Mods/Settings/Exit and all Back
   contexts. --mods-only closes on Back; --no-mods touches no managed store.
2. Import owned ROM with existing SHA validation; verify actual game launch,
   language/menu and normal movement without mods/GOD. Record which was tested.
3. Install the four ZIPs. They start Disabled; review metadata/permissions/errors.
4. Enable Super+timer+PNG together. Compare a fresh jump, distinguish real/sim/
   paused timer, own PNG's synthetic camera. Alternate Configurable Jump replaces
   Super's exclusive owner; attempting both must fail without partial activation.
5. Edit all four option types, boundaries/steps, Apply unchanged (no restart),
   cancel edits, Restore Defaults (Cancel first), actual next supported jump.
6. Save/rename profiles, confirm overwrite, Apply, Original; verify unmentioned
   disabled settings are retained. Delete confirms, Original forbidden, mods kept.
7. Export JSON, inspect contents, Import/cancel/confirm, verify inactive until Apply;
   test missing/wrong-version mod, duplicate name and file changed after review.
8. F2/Escape/F11, resize/fullscreen/4:3, focus loss; pause/audio silence/resume,
   neutral input barrier and no stuck controls. Confirm normal quit/Alt+F4/Cmd+Q.
9. Restart cleanly: persistence. In a disposable store, simulate interruption/Lua
   failure: conservative deactivation, errors visible, unrelated mods still usable.
10. Physical controller: hot plug/unplug, simultaneous stick+D-pad, A/B, shoulders,
    keyboard/mouse combination. Test on a real Deck; synthetic events are not this.

Record BUILT / PACKAGED / BOOT TESTED / GAMEPLAY TESTED / PHYSICAL CONTROLLER
TESTED separately. Linux and Deck full owned-ROM execution and historical high-
refresh pixel discrepancy need real evidence before claiming platform support.
No 2.5D/new physics/randomizer/CHR expansion is part of this checklist/delivery.

## Completion03 evidence

Only the experimental branch/Actions artifacts are authorized. No GitHub Release.
Use MOD_MANAGER_COMPLETION_03.md to distinguish newly executed clean-clone/CI
results from Completion02 history. Preserve the checklist's physical gameplay
and controller items as pending until actual hardware sessions are executed.
