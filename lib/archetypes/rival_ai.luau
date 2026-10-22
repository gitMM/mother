-- RIVAL_AI (creature, RALLY): a car that drives itself, with the car's handling, ramming and
-- spin-outs (core/drive.luau). It follows the racing line point to point, braking for sharp turns;
-- it swerves to ram any damageable entity of another faction within 4 tiles ahead of it; with no
-- racing line it only rams, chasing the nearest one. Stuck against something for a second, it
-- backs off for half a second. A racer. Needs gravity none, a ground surface and spawn points.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local race = use("core/race")
local drive = use("core/drive")

local RAM_RANGE = 4 * units.tile
local STUCK, BACK_UP = 60, 30

-- Where it steers: a target to ram, else the racing line, else any target; nil for none.
local function aim(e)
    local t = act.target(e)
    if t ~= nil then
        local dx, dy = t.x - e.x, t.y - e.y
        if dx * dx + dy * dy <= RAM_RANGE * RAM_RANGE and dx * e.fx + dy * e.fy > 0 then
            return t.x, t.y
        end
    end
    local px, py = race.next(e)
    if px ~= nil then
        return px, py
    end
    if t ~= nil then
        return t.x, t.y
    end
    return nil
end

return {
    category = "creature",
    layer = "ground",
    size = "M",
    w = 10,
    h = 10,
    kinematic = true, -- top-down, so gravity never applies (it requires gravity none anyway)
    damage = "stun",
    hp = 0,
    tier = 1,
    racer = true,
    requires = {
        gravity = { "none" },
        surface = { "ground" },
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = { "world", "camera" },
        deploy = axes.ANY,
    },
    params = {
        speed = units.run, -- top speed
        sprite = 0, -- 0, 22.5, 45, 67.5 and 90 degrees
        sfx_ram = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 5 } },
    sounds = { "sfx_ram" },

    init = function(e)
        drive.init(e)
        e.mem.stuck, e.mem.back = 0, 0
    end,

    update = function(e)
        local mem = e.mem
        local tx, ty = aim(e)
        if tx == nil then
            drive.step(e, false, true, 0)
            return
        end
        local turn = act.wrap(m.atan2(ty - e.y, tx - e.x) - mem.angle)
        local steer = math.clamp(turn / 0.15, -1, 1)
        if mem.back > 0 then
            mem.back -= 1
            drive.step(e, false, true, -steer)
            return
        end
        local fwd = drive.forward(e)
        local throttle = math.abs(turn) < 1 or fwd < 0.8
        local brake = math.abs(turn) > 1.2 and fwd > 1.2
        mem.stuck = if throttle and math.abs(fwd) < 0.2 then mem.stuck + 1 else 0
        if mem.stuck > STUCK then
            mem.stuck, mem.back = 0, BACK_UP
        end
        drive.step(e, throttle and not brake, brake, steer)
    end,

    guard = drive.guard,

    on_contact = drive.rammed,

    draw = drive.draw,
}
