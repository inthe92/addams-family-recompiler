local origin=nil
local visible=true
function on_update()
 local c=game.get_clocks()
 local input=game.get_input()
 if not origin or input.reset_timer then origin=c end
 if input.toggle_ui then visible=not visible end
 if visible then
  ui.rect("viewport",4,4,198,39,0,0,0,190)
  ui.text("viewport",8,8,"SIM "..tostring(math.floor((c.simulation_seconds-origin.simulation_seconds)*10)/10))
  ui.text("viewport",8,18,"REAL "..tostring(math.floor((c.real_seconds-origin.real_seconds)*10)/10))
  ui.text("viewport",8,28,"PAUSED "..tostring(math.floor((c.paused_seconds-origin.paused_seconds)*10)/10))
 end
end
