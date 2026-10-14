local visible=true
function on_update()
 if game.get_input().toggle_layers then visible=not visible end
 if visible then
  local t=game.get_clocks().simulation_seconds
  graphics.layer("viewport","mansion",210,180,100,50,-4,180,math.sin(t)*45,math.cos(t)*8,0.4,0.15)
  ui.text("viewport",8,230,"PARALLAX: SYNTHETIC CAMERA")
 end
end
