-- STARFIGHTER: each tick, tries the 9 stick directions, flying each one 40 ticks ahead against
-- every host entity's straight-line path, and takes the one that stays furthest from them all
-- (ties: under the nearest diver, low on the screen). Fires A and B whenever it can.
local HORIZON = 40
local SAFE = 26 -- gaps beyond this all count the same

local function ahead(me, cx, cy, sx, sy, k)
    local speed = if sx ~= 0 and sy ~= 0 then 2 * math.sqrt(0.5) else 2
    local cam_y = cy - 0.5 * k
    local x = math.clamp(me.x + sx * speed * k, cx + 6, cx + 314)
    local y = math.clamp(me.y + (sy * speed - 0.5) * k, cam_y + 6, cam_y + 174)
    return x, y
end

return function(t)
    local tap = if t % 2 == 0 then "ab" else ""
    local id = world:avatar()
    local me = id and world:get(id)
    if me == nil then
        return 0, 0, tap
    end
    local cx, cy = world:camera()
    local threats = {}
    for _, oid in world:query(cx - 60, cy - 60, 440, 300, { faction = "host" }) do
        table.insert(threats, world:get(oid))
    end
    local aim = world:nearest(me.x, me.y - 60, { archetype = "diver" })
    local aim_x = if aim then world:get(aim).x else cx + 160
    local best, best_x, best_y = -math.huge, 0, 0
    for sy = -1, 1 do
        for sx = -1, 1 do
            local gap = SAFE
            for k = 1, HORIZON, 3 do
                local x, y = ahead(me, cx, cy, sx, sy, k)
                for _, o in threats do
                    local ox, oy = o.x + o.vx * k, o.y + o.vy * k
                    local g = math.max(math.abs(ox - x) - (o.w + 12) / 2, math.abs(oy - y) - (o.h + 12) / 2)
                    gap = math.min(gap, g + k * 0.2) -- nearer dangers count more
                end
            end
            local x, y = ahead(me, cx, cy, sx, sy, 10)
            local score = gap * 10 - math.abs(x - aim_x) / 8 - math.abs(y - (cy + 150)) / 6
            if score > best then
                best, best_x, best_y = score, sx, sy
            end
        end
    end
    return best_x, best_y, tap
end
