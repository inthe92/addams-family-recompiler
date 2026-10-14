local changed_bytes=0
local notifications=0
function on_load()
    nes.memory.observe(256,16,function(changes)
        changed_bytes=changed_bytes+#changes
        notifications=notifications+1
        log.info("RAM COUNTER "..changed_bytes)
    end)
end
function on_update(dt)
    hud.text(4,2,"RAM CHANGE COUNTER")
    hud.text(4,12,"BYTES "..changed_bytes)
    hud.text(4,22,"NOTIFICATIONS "..notifications)
end
function on_unload() log.info("RAM COUNTER STOPPED") end
