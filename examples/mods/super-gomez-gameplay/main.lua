function on_load() game.player.set_modifier("jump_extension",12) end
function on_update()
 ui.rect("viewport",4,205,150,15,0,0,0,170)
 ui.text("viewport",8,209,"SUPER GOMEZ: JUMP ON")
end
-- Host unconditionally revokes the request on unload/error.
