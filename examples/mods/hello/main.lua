local updates = 0
function on_load()
    assert(MOD_API_VERSION == 1)
    log.info("Hello from Lua!")
end
function on_update(dt)
    assert(math.abs(dt - 1 / 50.007) < 0.000000001)
    updates = updates + 1
    if updates == 1 or updates % 250 == 0 then
        local status = game.get_status()
        log.info("PAL update " .. tostring(updates) .. ", frame " .. tostring(status.frame))
        local player = game.get_player()
        if player then log.info("Player: " .. tostring(player.x) .. ", " .. tostring(player.y)) end
    end
end
function on_unload()
    log.info("Mod unloaded after " .. tostring(updates) .. " updates")
end
