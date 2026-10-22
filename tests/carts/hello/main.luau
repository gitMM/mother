-- HELLO: the M1 test cart. Draws a few shapes, moves a box with the stick or arrow keys,
-- flashes the box on A and draws a ring under the pointer.

local box = import("box")

function cart.init()
    box.reset()
end

function cart.update()
    local sx, sy = stick()
    box.move(sx * 2, sy * 2)
    if btnp("a") then
        box.flash = 10
    elseif box.flash > 0 then
        box.flash -= 1
    end
end

function cart.draw()
    cls(17)
    rect(4, 4, 312, 172, 40)
    line(4, 4, 315, 175, 18)
    circfill(40, 140, 20, 12)
    circ(280, 40, 24, 9)
    rectfill(box.x, box.y, box.size, box.size, if box.flash > 0 then 9 else 22)

    local px, py, down = pointer()
    if px then
        circ(px, py, 3, if down then 9 else 22)
    end
end

function cart.hash()
    return math.floor(box.x) * 1000 + math.floor(box.y)
end
