local gfx = playdate.graphics

local triangles = threelib.triangles:init()

triangles:setBackground(63)

local x1 = 0
local y1 = 0
local x2 = 200
local y2 = 240
local x3 = 400
local y3 = 120
local color = 0
triangles:add(
	x1, y1,
	x2, y2,
	x3, y3,
	color
)

playdate.display.setRefreshRate(50)

local r = 100
local t = 0
function playdate.update()
	-- r = (math.cos(2*t) + 2) * 50
	x1 = math.floor(200 + r * math.cos(t))
	y1 = math.floor(120 + r * math.sin(t))
	x2 = math.floor(200 + r * math.cos(t + 2.09))
	y2 = math.floor(120 + r * math.sin(t + 2.09))
	x3 = math.floor(200 + r * math.cos(t - 2.09))
	y3 = math.floor(120 + r * math.sin(t - 2.09))
	color = math.floor((math.sin(t) + 1) * 63)
	triangles:update(
		1,
		x1, y1,
		x2, y2,
		x3, y3,
		color
	)
	triangles:draw()

	playdate.drawFPS(0, 0)
	
	t += 0.01
	if t > 6.28 then
		t = 0
	end
end
