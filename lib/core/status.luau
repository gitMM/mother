-- Statuses (brief section 6), applied here so every archetype honours them:
--   stun       no actions: update() doesn't run and the body stops (it still falls)
--   slip       no steering: update() runs, but the velocity it had is kept
--   knockback  an impulse: the body moves at run speed away from where its last hit came
--              from (or backwards along its facing), and doesn't act
-- Timers count steps; applying a status never shortens one already running. A static body never
-- moves at all.
local units = use("core/units")
local motion = use("core/motion")

local status = {}

status.FIELDS = table.freeze({ stun = "stun", slip = "slip", knockback = "knock" })

local function aim_knockback(e)
    local dx, dy = -e.fx, -e.fy
    if e.src_x ~= nil and (e.x ~= e.src_x or e.y ~= e.src_y) then
        dx, dy = e.x - e.src_x, e.y - e.src_y
    end
    local len = math.sqrt(dx * dx + dy * dy)
    if len == 0 then
        e.kx, e.ky = 0, 0
    else
        e.kx, e.ky = dx / len * units.run, dy / len * units.run
    end
end

function status.apply(e, kind, ticks)
    local field = status.FIELDS[kind]
    e[field] = math.max(e[field], ticks)
    if field == "knock" then
        aim_knockback(e)
    end
end

-- One step of e's own behaviour, under its statuses.
function status.behave(e)
    if e.knock > 0 then
        e.vx, e.vy = e.kx, e.ky
    elseif e.stun > 0 then
        e.vx = 0
        if not motion.falls(e) then
            e.vy = 0
        end
    else
        local vx, vy = e.vx, e.vy
        local update = e.arch.update
        if update ~= nil then
            update(e)
        end
        if e.slip > 0 then
            e.vx, e.vy = vx, vy
        end
    end
    if e.arch.static then
        e.vx, e.vy = 0, 0
    end
end

-- Counts every timer down by one step.
function status.tick(e)
    if e.stun > 0 then
        e.stun -= 1
    end
    if e.slip > 0 then
        e.slip -= 1
    end
    if e.knock > 0 then
        e.knock -= 1
    end
    if e.hurt > 0 then
        e.hurt -= 1
    end
end

return table.freeze(status)
