-- SKELETON (creature, KNIGHTFALL): chases the nearest damageable entity of another faction,
-- sliding round walls. In reach it winds up for 20 ticks (standing still), then swings a 90° arc:
-- tier 3, with knockback, then waits 60 ticks. HP 3. Top-down it walks in any direction; under
-- gravity it walks along the ground and falls. It needs a ground surface and spawn points.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local melee = use("core/melee")
local motion = use("core/motion")

return {
    category = "creature",
    layer = "ground",
    size = "M",
    w = 10,
    h = 14,
    kinematic = false, -- under gravity = "down" it falls like any walker
    damage = "hp",
    hp = 3,
    tier = 0,
    requires = {
        gravity = axes.ANY,
        surface = { "ground" },
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = { "world", "camera" },
        deploy = axes.ANY,
    },
    params = {
        speed = units.crawl,
        tier = 3,
        reach = 8,
        windup = 20,
        cooldown = 60,
        sprite = 0,
        sfx_swing = -1,
        sfx_die = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 3 } }, -- two steps, then the swing; facing right
    sounds = { "sfx_swing", "sfx_die" },

    init = function(e)
        e.mem.windup, e.mem.cool, e.mem.show = 0, 0, 0
    end,

    update = function(e)
        local p, m = e.params, e.mem
        if m.cool > 0 then
            m.cool -= 1
        end
        if m.show > 0 then
            m.show -= 1
        end
        if m.windup > 0 then
            act.halt(e)
            m.windup -= 1
            if m.windup == 0 then
                melee.swing(e, e.fx, e.fy, p.reach, 90, p.tier)
                act.sound(p.sfx_swing)
                m.cool, m.show = p.cooldown, 10
            end
            return
        end
        local t = act.target(e)
        if t == nil then
            act.halt(e)
            return
        end
        local dx, dy = t.x - e.x, t.y - e.y
        if motion.falls(e) then
            act.face(e, act.sign(dx), 0)
        else
            act.face(e, dx, dy)
        end
        local r = p.reach + math.max(e.hw, e.hh) + math.max(t.hw, t.hh)
        if m.cool == 0 and dx * dx + dy * dy <= r * r then
            m.windup = p.windup
            act.halt(e)
            return
        end
        act.chase(e, dx, dy, p.speed)
    end,

    on_leave = function(e, reason)
        if reason == "death" then
            act.sound(e.params.sfx_die)
        end
    end,

    draw = function(e)
        if look.blinking(e) then
            return
        end
        local m = e.mem
        local frame = if m.windup > 0 or m.show > 0 then 2 else (tick() // 10) % 2
        look.sprite(e, "sprite", frame, e.fx < 0)
    end,
}
