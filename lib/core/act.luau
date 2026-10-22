-- Shared behaviour for archetypes: facing, targets, steering, input and sounds.
-- A body that drives itself (a ball, a shot, a loose pickup) keeps the velocity it wants in
-- e.mem.vx, e.mem.vy and copies it into e.vx, e.vy each update (act.drive), so the velocity
-- survives a stun or a knockback; act.push changes it from outside (a reflector, a bounce).
local S = use("core/state")
local entity = use("core/entity")
local motion = use("core/motion")

local act = {}

-- Faces along (dx, dy) as a unit vector; (0, 0) leaves the facing as it was.
function act.face(e, dx, dy)
    local len = math.sqrt(dx * dx + dy * dy)
    if len > 0 then
        e.fx, e.fy = dx / len, dy / len
    end
end

-- What every AI targets: the nearest live, damageable entity of another faction (centre to
-- centre; the lower id on a tie), or nil.
function act.target(e)
    local best, best_d = nil, math.huge
    local list = S.list
    for i = 1, #list do
        local o = list[i]
        if o ~= e and entity.live(o) and not o.ghost and o.damage ~= "none" and o.faction ~= e.faction then
            local dx, dy = o.x - e.x, o.y - e.y
            local d = dx * dx + dy * dy
            if d < best_d then
                best, best_d = o, d
            end
        end
    end
    return best
end

local function sign(v)
    return if v > 0 then 1 elseif v < 0 then -1 else 0
end
act.sign = sign

-- Steers e at `speed` towards a point (dx, dy) away: in any direction, or only along x for a
-- body that falls (gravity owns its vertical speed).
function act.toward(e, dx, dy, speed)
    if motion.falls(e) then
        e.vx = if math.abs(dx) < speed then dx else sign(dx) * speed
        return
    end
    local len = math.sqrt(dx * dx + dy * dy)
    if len > speed then
        e.vx, e.vy = dx / len * speed, dy / len * speed
    else
        e.vx, e.vy = dx, dy
    end
end

-- Steers towards a point like act.toward, but slides round obstacles: when its last move was
-- blocked on one axis, it keeps pressing on that axis while moving along the other (towards the
-- target's side, else a side picked by its id) until the way is clear; stuck in a corner, it
-- turns to the other side. A body that falls just walks.
function act.chase(e, dx, dy, speed)
    if motion.falls(e) then
        act.toward(e, dx, dy, speed)
        return
    end
    local m = e.mem
    local bx, by = e.left or e.right, e.up or e.down
    local pick = if e.id % 2 == 0 then 1 else -1
    if bx and by and m.slide ~= nil then
        m.slide = -m.slide
    end
    if bx and math.abs(dx) > speed then
        m.slide = m.slide or (if dy ~= 0 then sign(dy) else pick)
        e.vx, e.vy = sign(dx) * speed, m.slide * speed
    elseif by and math.abs(dy) > speed then
        m.slide = m.slide or (if dx ~= 0 then sign(dx) else pick)
        e.vx, e.vy = m.slide * speed, sign(dy) * speed
    else
        m.slide = nil
        act.toward(e, dx, dy, speed)
    end
end

-- Stops e's own movement; a falling body keeps falling.
function act.halt(e)
    e.vx = 0
    if not motion.falls(e) then
        e.vy = 0
    end
end

function act.drive(e)
    e.vx, e.vy = e.mem.vx, e.mem.vy
end

function act.velocity(e)
    if e.mem.vx ~= nil then
        return e.mem.vx, e.mem.vy
    end
    return e.vx, e.vy
end

function act.push(e, vx, vy)
    e.vx, e.vy = vx, vy
    if e.mem.vx ~= nil then
        e.mem.vx, e.mem.vy = vx, vy
    end
end

-- Stunned or knocked back: it can't act, or use what it holds.
function act.busy(e)
    return e.stun > 0 or e.knock > 0
end

-- The pad's direction as -1, 0 or 1 on each axis (8 ways).
function act.dir8()
    local x = (if btn("right") then 1 else 0) - (if btn("left") then 1 else 0)
    local y = (if btn("down") then 1 else 0) - (if btn("up") then 1 else 0)
    return x, y
end

-- Plays sfx n, -1 meaning the archetype has no sound for this. Archetype sounds take turns on
-- voices 5 and 6, so a cart's music on channels 1-4 plays on undisturbed.
local voice = 5

function act.sound(n)
    if n >= 0 then
        sfx(n, voice)
        voice = if voice == 5 then 6 else 5
    end
end

return table.freeze(act)
