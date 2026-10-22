-- LASER (equipment, BREAKOUT): on its button, fires 2 shots side by side along the holder's
-- facing, tier 1, at projectile speed; 30 ticks between pairs. As a loose pickup it drifts down
-- until the avatar touches it.
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
    w = 14,
    h = 6,
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
        tier = 1,
        cooldown = 30,
        sprite = 0, -- the pickup
        shot_sprite = 0,
        sfx_fire = -1,
        sfx_take = -1,
    }),
    sprites = { sprite = { w = 2, h = 1, frames = 1 }, shot_sprite = { w = 1, h = 1, frames = 1 } },
    sounds = { "sfx_fire", "sfx_take" },

    init = function(e)
        e.mem.cool = 0
        equip.init(e)
    end,

    update = function(e)
        local m, p = e.mem, e.params
        if m.cool > 0 then
            m.cool -= 1
        end
        local h = equip.update(e)
        if h == nil or m.cool > 0 or not equip.pressed(e) then
            return
        end
        m.cool = p.cooldown
        -- two shots from either side of the holder's front edge
        local fx, fy = h.fx, h.fy
        local tx, ty = -fy, fx
        local front = math.abs(fx) * h.hw + math.abs(fy) * h.hh + 3
        local side = math.max(math.abs(tx) * h.hw + math.abs(ty) * h.hh - 3, 1)
        local speed = units.projectile
        for _, s in { -side, side } do
            spawn.child(e, "shot", {
                vx = fx * speed,
                vy = fy * speed,
                tier = p.tier,
                w = if math.abs(fx) > math.abs(fy) then 4 else 2,
                h = if math.abs(fx) > math.abs(fy) then 2 else 4,
                sprite = p.shot_sprite,
            }, h.x + fx * front + tx * s, h.y + fy * front + ty * s, e.faction)
        end
        act.sound(p.sfx_fire)
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
