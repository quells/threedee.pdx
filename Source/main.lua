local gfx = playdate.graphics

local triangles = threelib.triangles:init()

-- playdate.display.setRefreshRate(10)

function playdate.update()
	triangles:draw()
	playdate.drawFPS(0, 0)
end
