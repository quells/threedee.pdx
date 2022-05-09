local gfx = playdate.graphics

local triangles = threelib.triangles:init()

triangles:load_model("cube.3d")

triangles:setBackground(63)

local x1 = 0
local y1 = 0
local x2 = 200
local y2 = 240
local x3 = 400
local y3 = 120
local z0 = 550
local color = 0
triangles:add(
	x1, y1, z0,
	x2, y2, z0,
	x3, y3, z0,
	color
)

triangles:add(
	30, 15, z0,
	20, 35, z0,
	40, 35, z0,
	0
)

triangles:add(
	370, 15, z0,
	380, 35, z0,
	360, 35, z0,
	127
)

triangles:add(
	30, 225, z0,
	20, 205, z0,
	40, 205, z0,
	96
)

triangles:add(
	370, 225, z0,
	380, 205, z0,
	360, 205, z0,
	32
)

playdate.display.setRefreshRate(50)

local r = 100
local t = 0
function playdate.update()
	x1 = 200 + r * math.cos(7*t)
	y1 = 120
	z1 = z0 + r * math.sin(7*t)
	x2 = 200 + r * math.cos(7*t + 2.09)
	y2 = 120 + r * math.sin(2.09)
	z2 = z0 + r * math.sin(7*t + 2.09)
	x3 = 200 + r * math.cos(7*t - 2.09)
	y3 = 120 + r * math.sin(-2.09)
	z3 = z0 + r * math.sin(7*t - 2.09)

	color = math.floor((math.sin(3*t) + 1) * 63)
	triangles:update(
		1,
		x1, y1, z1,
		x2, y2, z2,
		x3, y3, z3,
		color
	)
	triangles:draw()

	playdate.drawFPS(0, 0)
	
	t += 0.01
	if t > 6.28 then
		t = 0
	end
end
