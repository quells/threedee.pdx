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
	x1, y1, 1000,
	x2, y2, 1000,
	x3, y3, 1000,
	color
)

triangles:add(
	30, 15, 1000,
	20, 35, 1000,
	40, 35, 1000,
	0
)

triangles:add(
	370, 15, 1000,
	380, 35, 1000,
	360, 35, 1000,
	127
)

triangles:add(
	30, 225, 1000,
	20, 205, 1000,
	40, 205, 1000,
	96
)

triangles:add(
	370, 225, 1000,
	380, 205, 1000,
	360, 205, 1000,
	32
)

playdate.display.setRefreshRate(50)

local r = 100
local z = 1000
local t = 0
function playdate.update()
	x1 = 200 + r * math.cos(0)
	y1 = 120 + r * math.sin(0)
	x2 = 200 + r * math.cos(0 + 2.09)
	y2 = 120 + r * math.sin(0 + 2.09)
	x3 = 200 + r * math.cos(0 - 2.09)
	y3 = 120 + r * math.sin(0 - 2.09)
	z = 1000 + 200 * math.cos(5*t)
	color = math.floor((math.sin(3*t) + 1) * 63)
	triangles:update(
		1,
		x1, y1, z,
		x2, y2, z,
		x3, y3, z,
		color
	)
	triangles:draw()

	playdate.drawFPS(0, 0)
	
	t += 0.01
	if t > 6.28 then
		t = 0
	end
end
