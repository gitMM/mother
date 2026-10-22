-- BREAKOUT: keeps the paddle under the lowest ball, off-centre by a slowly changing amount so the
-- ball leaves at different angles; taps A to start and to fire lasers. With `away` (set by the
-- test before this file) it does the opposite and keeps the paddle away from the ball.
return function(t)
    local tap = if t % 2 == 0 then "a" else ""
    local me = world:get(world:avatar())
    local ids = world:query(0, 0, 320, 180, { archetype = "ball" })
    if #ids == 0 then
        return 0, 0, tap
    end
    local b = world:get(ids[1])
    for i = 2, #ids do
        local v = world:get(ids[i])
        if v.y > b.y then
            b = v
        end
    end
    local aim = b.x + 10 * m.sin(t / 97)
    if away then
        aim = if b.x < 160 then 300 else 20
    end
    return math.clamp((aim - me.x) / 3, -1, 1), 0, tap
end
