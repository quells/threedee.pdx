# ThreeDee.pdx

Messing around with graphics on Playdate.

The plan is to support rendering arbitrary 3D triangles in shades of grey,
dithered to 1-bit for the Playdate display.

So far 3D triangles are supported with a fixed camera:

![Spinning Triangle](https://github.com/quells/threedee.pdx/raw/main/Spinning%20Triangle.gif)

Real Device Performance

```
background 0.005
2d         0.001
triangles  0.001
dither     0.036

render 0.037
draw   0.008
```