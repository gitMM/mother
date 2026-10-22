-- NITRO (equipment, RALLY): on its button, doubles the holder's speed for 60 ticks (a boost: every
-- avatar archetype's speeds go through act.speed), then needs 300 ticks to recharge. With `charges`
-- above 0 it is used up after that many boosts (RALLY: 3). As a loose pickup it drifts down until
-- the avatar touches it.
local axes = use("core/axes")
local act = use("core/act")
local look = use("core/look")
local equip = use("core/equip")
local entity = use("core/entity")

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
        boost = 60,
        cooldown = 300,
        charges = 0, -- 0: never runs out
        sprite = 0, -- the pickup
        sfx_boost = -1,
        sfx_take = -1,
    }),
    sprites = { sprite = { w = 2, h = 1, frames = 1 } },
    sounds = { "sfx_boost", "sfx_take" },
    public = { "charges" },

    init = function(e)
        e.mem.cool, e.mem.charges = 0, e.params.charges
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
        h.boost = math.max(h.boost, p.boost)
        mem.cool = p.cooldown
        act.sound(p.sfx_boost)
        if p.charges > 0 then
            mem.charges -= 1
            if mem.charges == 0 then
                entity.remove(e, "remove")
            end
        end
    end,

    on_contact = function(e, other)
        if equip.take(e, other) then
            act.sound(e.params.sfx_take)
        end
    end,

    draw = function(e)
        if not equip.held(e) then
            look.sprite(e, "sprite", 0)
        end
    end,
}
