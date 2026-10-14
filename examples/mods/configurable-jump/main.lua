function on_load()
 game.player.set_modifier("jump_extension",config.get("extension"))
end
function on_update()
 if config.get("show") then
  local y=config.get("position")=="top" and 45 or 205
  ui.rect("viewport",4,y,160,15,0,0,0,math.floor(config.get("opacity")*255))
  ui.text("viewport",8,y+4,"JUMP "..tostring(config.get("extension")))
 end
end
