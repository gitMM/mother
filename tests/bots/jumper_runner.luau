-- JUMPER: holds right. Without a flower it jumps under the next ?-block to knock one out. From the
-- ground it makes a full jump (A held 18 ticks) at a wall or a pit edge just ahead, or at an enemy
-- just ahead when no pit lies where it would come down; once it has a flower it leaves enemies to
-- fireballs (B, whenever one is ahead) and only jumps shells.
local QBLOCK = 131

local function solid(x, y)
    local t = mget(x // 8, y // 8, 0)
    return t ~= 0 and fget(t, 0)
end

local hold, rest = 0, 0

-- The column of a ?-block within jumping reach, from the runner's column to `ahead` columns on.
local function block_ahead(me, ahead)
    local col, head = me.x // 8, (me.y - me.h / 2) // 8
    for c = col, col + ahead do
        for row = head - 3, head - 1 do
            if mget(c, row, 0) == QBLOCK then
                return c
            end
        end
    end
    return nil
end

return function(t)
    if t < 10 then
        return 0, 0, if t % 2 == 0 then "a" else ""
    end
    local id = world:avatar()
    local me = id and world:get(id)
    if me == nil then
        return 0, 0, ""
    end
    local feet = me.y + me.h / 2
    local front = me.x + me.w / 2
    local grounded = me.vy == 0 and (solid(me.x - 4, feet + 1) or solid(me.x + 4, feet + 1))
    local danger = false
    for _, dy in { -6, 0, 6 } do
        danger = danger or solid(front + 6, me.y + dy)
    end
    local function ground_at(x)
        for y = feet + 1, 176, 8 do
            if solid(x, y) then
                return true
            end
        end
        return false
    end
    danger = danger or not ground_at(front + 10)
    local landing = true -- no pit where an early jump would come down
    for x = front + 12, front + 80, 4 do
        landing = landing and ground_at(x)
    end
    local powered = me.hp >= 2
    local enemy = false -- fireballs see to most of them once powered
    for _, oid in world:query(me.x, me.y - 40, 40, 56, { faction = "host" }) do
        enemy = enemy or not powered or world:get(oid).archetype == "shell"
    end
    local block = if powered then nil else block_ahead(me, 4)
    if block ~= nil then
        danger = me.x // 8 == block -- the flower first
    else
        danger = danger or (enemy and landing)
    end
    local keys = ""
    if hold > 0 then
        hold -= 1
        keys = "a"
        if hold == 0 then
            rest = 1
        end
    elseif rest > 0 then
        rest -= 1
    elseif danger and grounded then
        hold = 18
        keys = "a"
    end
    if powered and t % 2 == 0 and #world:query(me.x, me.y - 30, 110, 40, { faction = "host" }) > 0 then
        keys ..= "b"
    end
    return 1, 0, keys
end
