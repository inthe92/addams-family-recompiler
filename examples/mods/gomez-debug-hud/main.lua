assert(MOD_API_VERSION == 2)
local function draw()
    local status = game.get_status()
    local player = game.get_player()
    hud.text(4, 2, "GOMEZ DEBUG HUD")
    if player then
        hud.text(4, 12, "X " .. tostring(player.x) .. " Y " .. tostring(player.y))
    else
        hud.text(4, 12, "PLAYER UNAVAILABLE")
    end
    hud.text(4, 22, "STATE " .. status.state)
    hud.text(4, 32, "FRAME " .. tostring(status.frame) .. " ROOM " .. (status.room_id and tostring(status.room_id) or "?"))
end
function on_load()
    draw()
    log.info("Gomez Debug HUD enabled. Observation only; no physics changes.")
end
function on_update(dt)
    draw()
end
function on_unload()
    log.info("Gomez Debug HUD disabled.")
end
