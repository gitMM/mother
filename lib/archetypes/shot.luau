-- SHOT (object): a straight projectile fired by other archetypes (the laser). It flies at its
-- (vx, vy), hurts the first damageable entity of another faction it touches with its tier, and
-- is spent on that or on anything solid; it also fades after `life` ticks. Not an export of
-- any cart: it exists so firing archetypes work everywhere.
local axes = use("core/axes")
local act = use("core/act")
local look = use("core/look")
local entity = use("core/entity")
local factions = use("core/factions")

return {
    category = "object",
    layer = "projectile",
    size = "S",
    w = 2,
    h = 4,
    kinematic = true, -- shots fly straight, gravity or not
    damage = "none",
    hp = 0,
    tier = 1,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = { vx = 0, vy = -4, tier = 1, life = 90, w = 2, h = 4, sprite = 0 },
    sprites = { sprite = { w = 1, h = 1, frames = 1 } },

    init = function(e)
        local p = e.params
        e.hw, e.hh, e.tier = p.w / 2, p.h / 2, p.tier
        e.mem.vx, e.mem.vy, e.mem.life = p.vx, p.vy, p.life
        e.vx, e.vy = p.vx, p.vy
    end,

    update = function(e)
        local m = e.mem
        -- spent on the step it hit something, after the contact that hurts
        m.life -= 1
        if m.spent or m.life <= 0 then
            entity.remove(e, "remove")
            return
        end
        act.drive(e)
    end,

    on_block = function(e)
        e.mem.spent = true
        e.mem.vx, e.mem.vy = 0, 0
        e.vx, e.vy = 0, 0
    end,

    on_contact = function(e, other)
        if other.damage ~= "none" and factions.hostile(e, other) then
            e.mem.spent = true
            e.mem.vx, e.mem.vy = 0, 0
            e.tier = 0 -- one hit only
        end
    end,

    draw = function(e)
        if not e.mem.spent then
            look.sprite(e, "sprite", 0)
        end
    end,
}
