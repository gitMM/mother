-- Car handling (brief section 7) for car and rival_ai: momentum along a heading, with grip. Each
-- step a car turns (steer -1..1, at a rate that grows with its speed up to walk speed, and
-- reverses when it backs up), then splits its velocity along and across its new heading. Along
-- it, throttle accelerates, brake slows it and then backs it up at walk speed, and coasting drags
-- it slowly to a stop; above top speed it slows down. Across it, grip removes a share of the
-- slide. Top speed is the car's `speed` param, halved on an offroad tile (flag bit 3, under its
-- centre); a nitro boost doubles top speed and acceleration (act.speed).
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local race = use("core/race")
local status = use("core/status")

local drive = {}

local ACCEL = 0.05 -- px/tick²
local BRAKE = 0.1
local DRAG = 0.01
local TURN = 0.06 -- rad/tick at walk speed and above
local GRIP = 0.2 -- the share of the sideways slide lost each step
local SPIN = 0.5 -- rad/tick, shown while spun out
local RAM_KNOCKBACK = 8 -- ticks

-- Faces the racing line's next point (or right) when there is one.
function drive.init(e)
    local a = 0
    local px, py = race.next(e)
    if px ~= nil then
        a = m.atan2(py - e.y, px - e.x)
    end
    e.mem.angle = a
    e.fx, e.fy = m.cos(a), m.sin(a)
end

function drive.offroad(e)
    local t = mget(math.floor(e.x / units.tile), math.floor(e.y / units.tile), 0)
    return t ~= 0 and fget(t, units.flag_offroad)
end

-- Its speed along its heading (negative backing up).
function drive.forward(e)
    return e.vx * e.fx + e.vy * e.fy
end

function drive.step(e, throttle, brake, steer)
    local mem = e.mem
    local k = math.clamp(drive.forward(e) / units.walk, -1, 1)
    local a = act.wrap(mem.angle + math.clamp(steer, -1, 1) * TURN * k)
    local fx, fy = m.cos(a), m.sin(a)
    local fwd = e.vx * fx + e.vy * fy
    local side = e.vy * fx - e.vx * fy
    local top = act.speed(e, e.params.speed)
    if drive.offroad(e) then
        top /= 2
    end
    local accel = act.speed(e, ACCEL)
    if throttle then
        fwd += accel
    elseif brake then
        fwd = if fwd > 0 then math.max(fwd - BRAKE, 0) else math.max(fwd - accel, -units.walk)
    else
        fwd -= act.sign(fwd) * math.min(math.abs(fwd), DRAG)
    end
    if fwd > top then
        fwd = math.max(fwd - BRAKE, top)
    end
    side *= 1 - GRIP
    e.vx, e.vy = fx * fwd - fy * side, fy * fwd + fx * side
    mem.angle, e.fx, e.fy = a, fx, fy
end

-- Ramming. In a contact a car is hurt only when the other is closing in on it at least as fast as
-- it is closing in on the other (so a rear-ender hurts the car in front, a head-on both, and two
-- cars at rest neither); the guard hook says when it isn't.
function drive.guard(e, other)
    local dx, dy = other.x - e.x, other.y - e.y
    local len = math.sqrt(dx * dx + dy * dy)
    if len == 0 then
        return true
    end
    local mine = (e.vx * dx + e.vy * dy) / len
    local theirs = -(other.vx * dx + other.vy * dy) / len
    return not (theirs > 0 and theirs >= mine)
end

-- After a contact: when e's ram landed on `other` this step, other is knocked away (so the two
-- part) and e plays its sound.
function drive.rammed(e, other)
    if other.hurt == units.hurt_ticks and other.src_x == e.x and other.src_y == e.y then
        status.apply(other, "knockback", RAM_KNOCKBACK)
        act.sound(e, e.params.sfx_ram)
    end
end

-- Draws its `sprite` param: 5 frames for 0, 22.5, 45, 67.5 and 90 degrees (right to down),
-- mirrored for the other 11 of 16 headings. A spun-out car (stunned) spins.
function drive.draw(e)
    local a = e.mem.angle
    if e.stun > 0 then
        a += tick() * SPIN
    end
    local k = math.floor(act.wrap(a) / (math.pi / 8) + 0.5) % 16
    if k <= 4 then
        look.sprite(e, "sprite", k)
    elseif k <= 8 then
        look.sprite(e, "sprite", 8 - k, true, false)
    elseif k <= 12 then
        look.sprite(e, "sprite", k - 8, true, true)
    else
        look.sprite(e, "sprite", 16 - k, false, true)
    end
end

return table.freeze(drive)
