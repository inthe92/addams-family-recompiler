function on_load()
    log.info("RAM LOGGER ACTIVE")
    nes.memory.observe(0,16,function(changes)
        for _,c in ipairs(changes) do
            log.info("RAM "..c.address.." "..c.old_value.." TO "..c.new_value)
        end
    end)
end
function on_unload() log.info("RAM LOGGER STOPPED") end
