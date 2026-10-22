-- KNIGHTFALL: walks to the nearest skeleton and swings when close; taps A to start.
local function sgn(v)
    return if v > 2 then 1 elseif v < -2 then -1 else 0
end

return function(t)
    local tap = if t % 2 == 0 then "a" else ""
    local me = world:get(world:avatar())
    if me == nil then
        return 0, 0, ""
    end
    local id = world:nearest(me.x, me.y, { faction = "host", damageable = true })
    if id == nil then
        return 0, 0, tap
    end
    local s = world:get(id)
    local dx, dy = s.x - me.x, s.y - me.y
    return sgn(dx), sgn(dy), if dx * dx + dy * dy > 16 * 16 then "" else tap
end
