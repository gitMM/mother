-- RALLY: drives the racing line point to point (the cart's line, copied here), steering by the
-- car's heading and braking for sharp turns; fires the nitro on long straights and drops oil
-- when a rival is right behind.
local LINE = { { 324, 84 }, { 516, 84 }, { 708, 84 }, { 836, 116 }, { 884, 212 }, { 884, 372 }, { 868, 500 },
    { 788, 564 }, { 660, 548 }, { 596, 468 }, { 532, 388 }, { 452, 404 }, { 404, 484 }, { 324, 564 },
    { 196, 564 }, { 100, 500 }, { 84, 372 }, { 84, 212 }, { 132, 116 }, { 212, 84 } }
local REACH = 64

local function wrap(a)
    return a - 2 * math.pi * math.floor((a + math.pi) / (2 * math.pi))
end

local target = 1

return function(t)
    if t < 10 then
        return 0, 0, if t % 2 == 0 then "a" else ""
    end
    local car = world:get(world:avatar())
    local p = LINE[target]
    if (p[1] - car.x) ^ 2 + (p[2] - car.y) ^ 2 <= REACH * REACH then
        target = target % #LINE + 1
        p = LINE[target]
    end
    local dist = math.sqrt((p[1] - car.x) ^ 2 + (p[2] - car.y) ^ 2)
    local turn = wrap(m.atan2(p[2] - car.y, p[1] - car.x) - car.angle)
    local steer = if turn > 0.08 then 1 elseif turn < -0.08 then -1 else 0
    local speed = math.sqrt(car.vx * car.vx + car.vy * car.vy)
    local keys = if math.abs(turn) > 1.2 and speed > 1.2 then "b" else "a"
    if t % 2 == 0 then
        if math.abs(turn) < 0.15 and dist > 150 then
            keys ..= "l"
        end
        local behind = world:query(car.x - 40, car.y - 40, 80, 80, { faction = "host" })
        if #behind > 0 then
            keys ..= "r"
        end
    end
    return steer, 0, keys
end
