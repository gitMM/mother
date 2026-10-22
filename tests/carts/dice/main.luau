-- DICE: the M4 determinism cart. Everything that feeds the state hash changes all the time:
-- the RNG (rnd, rndi), the fixed maths (m.sin, m.cos, m.atan2, m.exp, m.sqrt), the tilemap
-- (mset, fset) and a float total reported by cart.hash(). The stick moves a ring of sparks;
-- A rolls dice onto the bottom row of the map.

local sparks = {}
local x, y = 160, 90
local total = 0
local rolls = 0

function cart.init()
    for _ = 1, 24 do
        table.insert(sparks, { a = rnd(6.283), r = 20 + rndi(0, 60), s = (rnd(2) - 1) * 0.05 })
    end
end

function cart.update()
    local sx, sy = stick()
    x = m.clamp(x + sx * 2, 0, 319)
    y = m.clamp(y + sy * 2, 0, 179)
    for _, p in sparks do
        p.a += p.s
        local px, py = x + m.cos(p.a) * p.r, y + m.sin(p.a) * p.r
        total += m.atan2(py - y, px - x) + m.sqrt(p.r) + m.exp(-p.r / 40)
    end
    if btnp("a") then
        rolls += 1
        mset(rndi(0, 39), 0, rndi(1, 3))
        fset(rndi(1, 3), rndi(0, 7), rnd() < 0.5)
    end
    if tick() % 30 == 0 then
        sparks[rndi(1, #sparks)].s = (rnd(2) - 1) * 0.05
    end
end

function cart.draw()
    cls(0)
    for i = 0, 39 do
        local die = mget(i, 0)
        if die > 0 then
            rectfill(i * 8, 172, 7, 7, 8 + die)
            pset(i * 8 + 3, 175, 0)
        end
    end
    for i, p in sparks do
        pset(x + m.cos(p.a) * p.r, y + m.sin(p.a) * p.r, 9 + i % 4)
    end
    circ(x, y, 3, 22)
    print("ROLLS " .. rolls, 4, 4, 22)
end

function cart.hash()
    return total + rolls * 1000 + x * 7 + y * 13
end
