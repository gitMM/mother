-- MISSILE (object): the homing projectile that homing launches. It flies at `speed`, turning at
-- most `turn` radians a tick towards the nearest damageable entity of another faction; it hurts
-- the first such entity it touches with its tier and is spent on that or on anything solid; it
-- fades after `life` ticks. Not an export of any cart, like shot.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local entity = use("core/entity")
local factions = use("core/factions")

local function spend(e)
    e.mem.spent = true
    e.mem.vx, e.mem.vy = 0, 0
    e.vx, e.vy = 0, 0
    e.tier = 0
end

return {
    category = "object",
    layer = "projectile",
    size = "S",
    w = 4,
    h = 4,
    kinematic = true, -- it flies under its own power, gravity or not
    damage = "none",
    hp = 0,
    tier = 3,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = { vx = 0, vy = -3, speed = units.fast, turn = 0.1, tier = 3, life = 120, sprite = 0 },
    sprites = { sprite = { w = 1, h = 1, frames = 2 } }, -- two flame flickers

    init = function(e)
        local p, mem = e.params, e.mem
        e.tier = p.tier
        mem.vx, mem.vy, mem.life = p.vx, p.vy, p.life
        e.vx, e.vy = p.vx, p.vy
    end,

    update = function(e)
        local p, mem = e.params, e.mem
        mem.life -= 1
        if mem.spent or mem.life <= 0 then
            entity.remove(e, "remove")
            return
        end
        local heading = m.atan2(mem.vy, mem.vx)
        local t = act.target(e)
        if t ~= nil then
            local turn = act.wrap(m.atan2(t.y - e.y, t.x - e.x) - heading)
            heading += math.clamp(turn, -p.turn, p.turn)
        end
        mem.vx, mem.vy = m.cos(heading) * p.speed, m.sin(heading) * p.speed
        act.drive(e)
    end,

    on_block = function(e)
        spend(e)
    end,

    on_contact = function(e, other)
        if other.damage ~= "none" and factions.hostile(e, other) then
            spend(e)
        end
    end,

    draw = function(e)
        if not e.mem.spent then
            look.sprite(e, "sprite", (tick() // 3) % 2)
        end
    end,
}
