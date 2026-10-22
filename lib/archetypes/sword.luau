-- SWORD (equipment, KNIGHTFALL): on its button, swings a 90° arc along the holder's facing,
-- reaching 12 px past the holder: tier 3 to every damageable entity of another faction, each
-- knocked back. 20 ticks between swings. Works for any avatar, facing any way.
local axes = use("core/axes")
local act = use("core/act")
local look = use("core/look")
local melee = use("core/melee")
local equip = use("core/equip")

local SHOW = 8 -- ticks the swing stays on screen

return {
    category = "equipment",
    layer = "air",
    size = "M",
    w = 4,
    h = 4,
    kinematic = true, -- it is carried, never falls
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
        arc = 90,
        reach = 12,
        cooldown = 20,
        sprite = 0, -- the swing, to the right; also the loose pickup
        sprite_v = 0, -- the swing, downwards
        sfx_swing = -1,
    }),
    sprites = { sprite = { w = 2, h = 2, frames = 1 }, sprite_v = { w = 2, h = 2, frames = 1 } },
    sounds = { "sfx_swing" },

    init = function(e)
        e.mem.cool, e.mem.show = 0, 0
        equip.init(e)
    end,

    update = function(e)
        local m, p = e.mem, e.params
        if m.cool > 0 then
            m.cool -= 1
        end
        if m.show > 0 then
            m.show -= 1
        end
        local h = equip.update(e)
        if h ~= nil and m.cool == 0 and equip.pressed(e) then
            m.cool, m.show = p.cooldown, SHOW
            m.fx, m.fy = h.fx, h.fy
            melee.swing(h, h.fx, h.fy, p.reach, p.arc, p.tier)
            act.sound(p.sfx_swing)
        end
    end,

    on_contact = function(e, other)
        equip.take(e, other)
    end,

    draw = function(e)
        local m = e.mem
        if not equip.held(e) then
            look.sprite(e, "sprite", 0)
            return
        end
        if m.show == 0 then
            return
        end
        local h = m.holder
        local reach = math.max(h.hw, h.hh) + 6
        local x, y = h.x + m.fx * reach, h.y + m.fy * reach
        if math.abs(m.fx) >= math.abs(m.fy) then
            look.sprite_at(e, "sprite", 0, x, y, m.fx < 0, false)
        else
            look.sprite_at(e, "sprite_v", 0, x, y, false, m.fy < 0)
        end
    end,
}
