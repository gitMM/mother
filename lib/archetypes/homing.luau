-- HOMING (equipment, STARFIGHTER): on its button, launches a missile from the holder's front,
-- along its facing, that seeks the nearest damageable entity of another faction: tier 3 at fast
-- speed; 60 ticks between launches. As a loose pickup it drifts down until the avatar touches it.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local equip = use("core/equip")
local spawn = use("core/spawn")

return {
    category = "equipment",
    layer = "air",
    size = "M",
    w = 12,
    h = 8,
    kinematic = true, -- carried, or drifting at its own `drop` speed as a pickup
    damage = "none",
    hp = 0,
    tier = 0,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = equip.params({
        tier = 3,
        cooldown = 60,
        sprite = 0, -- the pickup
        missile_sprite = 0,
        sfx_fire = -1,
        sfx_take = -1,
    }),
    sprites = { sprite = { w = 2, h = 1, frames = 1 }, missile_sprite = { w = 1, h = 1, frames = 2 } },
    sounds = { "sfx_fire", "sfx_take" },

    init = function(e)
        e.mem.cool = 0
        equip.init(e)
    end,

    update = function(e)
        local p, mem = e.params, e.mem
        if mem.cool > 0 then
            mem.cool -= 1
        end
        local h = equip.update(e)
        if h == nil or mem.cool > 0 or not equip.pressed(e) then
            return
        end
        mem.cool = p.cooldown
        local front = math.abs(h.fx) * h.hw + math.abs(h.fy) * h.hh + 4
        spawn.child(e, "missile", {
            vx = h.fx * units.fast,
            vy = h.fy * units.fast,
            tier = p.tier,
            sprite = p.missile_sprite,
        }, h.x + h.fx * front, h.y + h.fy * front, e.faction)
        act.sound(e, p.sfx_fire)
    end,

    on_contact = function(e, other)
        if equip.take(e, other) then
            act.sound(e, e.params.sfx_take)
        end
    end,

    draw = function(e)
        if not equip.held(e) then
            look.sprite(e, "sprite", 0)
        end
    end,
}
