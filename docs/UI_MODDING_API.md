# UI modding — implemented API2

Requires permission `ui.draw` and the current experimental host. Existing
`overlay.text` / `hud.text(x,y,text)` remain compatible.

```lua
function on_update()
    local clocks = game.get_clocks()
    ui.rect("viewport", 4, 4, 180, 16, 0, 0, 0, 180)
    ui.text("viewport", 8, 8, "SIM " .. tostring(math.floor(clocks.simulation_seconds)))
end
```

`ui.rect(space,x,y,w,h,r,g,b,a)` requires nine arguments: positive dimensions,
integer RGBA0..255, entirely within the bounded320×240 canvas.
`ui.text(space,x,y,text)` requires four arguments: integer coordinates, at most
48 printable supported characters, entire five-by-seven font line fitting the
canvas. Font supports Latin A–Z (case folded), digits, spaces and
`: . / - ? ( ) , >`; Unicode fonts, colored text and text opacity are not
implemented. Text/rectangles have z0, submission order resolves ties. Bars and
indicators can be composed from rectangles, without another API/framework.

Both methods are callable only in load/update callbacks. Captures replace the
previous capture; submit no commands to hide it. Up to16 commands and100,000
candidate pixels per mod/capture. Errors unload only that owner. No framebuffer,
PPU, CHR or nametable changes occur.

## Coordinates and clocks

`viewport`:320×240 logical coordinates mapped to the centered original4:3
viewport. `window`: the same uniformly scaled4:3 canvas anchored at the top-left
of the drawable, including possible letterbox margins. This window canvas does
not cover an arbitrary ultrawide screen. Rectangular clipping is fixed; generalized
anchors, full-window coordinate querying and configurable scales are future work.
Nearest filtering, resize and fullscreen preserve aspect. Built-in confirmation
and Mods UI remain above mod planes.

These clock/input functions are API2 only; v1 retains its original surface.
`game.get_clocks()` returns sampled `real_seconds`, `simulation_seconds`,
`paused_seconds`. Real is monotonic wall time since the game-loop session began,
including host pauses. Paused sums periods recognized by the existing frontend
pause/focus/modal state. Simulation is completed original NES units/50.007,
consistent with the existing callback contract, not a new hardware clock or
certified speedrun duration. No update callback executes while paused or for
repeated presentations; timer display freezes while paused and catches up after
resume. OS suspension without a recognized pause is not certified separately.

`game.get_input()` returns the sampled NES buttons and one-unit host gestures:
`reset_timer`, `toggle_ui`, `toggle_layers`. Ctrl+Alt+R/U/V generate these gestures
respectively; repeats are ignored and pending gestures clear on pause/focus loss.
They do not change NES controller mapping. There is no synthetic input setter.

## Speedrun HUD example

`examples/mods/speedrun-hud` uses only this API. Its origin is the first completed
update after activation, reset by Ctrl+Alt+R. Ctrl+Alt+U hides/shows the capture.
SIM, REAL and PAUSED are distinct labels. Enable/Disable/Uninstall use the normal
manager. This is a session timer; official run start/end, splits, death counting,
records and export are NOT IMPLEMENTED because their game events are unverified.
The unchanged host can run other scripts using these primitives without rebuilding.

Gameplay authors should use simulation state/time and sampled input for replayable
decisions. REAL is intentionally nondeterministic host timing, appropriate for UI;
the SDK does not promise that arbitrary scripts using it for gameplay are replayable.
