-- WALKER (creature, JUMPER): walks along x at crawl speed, setting off towards the nearest
-- damageable entity of another faction, and turns round whenever a solid stops it. Contact tier
-- 1; HP 1. Under gravity it falls (off ledges too); top-down it just paces. Needs a ground
-- surface and spawn points. TURTLE walks the same way (archetypes/turtle.luau).
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local motion = use("core/motion")

return {
    category = "creature",
    layer = "ground",
    size = "M",
    w = 12,
    h = 12,
    kinematic = false, -- under gravity = "down" it falls
    damage = "hp",
    hp = 1,
    tier = 1,
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
        sprite = 0, -- two steps, facing left
        sfx_die = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 2 } },
    sounds = { "sfx_die" },

    init = function(e)
        local t = act.target(e)
        e.fx = if t ~= nil and t.x > e.x then 1 else -1
    end,

    update = function(e)
        e.vx = e.fx * e.params.speed
        if not motion.falls(e) then
            e.vy = 0
        end
    end,

    on_block = function(e, nx, ny)
        if nx ~= 0 then
            e.fx, e.vx = nx, 0
        else
            e.vy = 0
        end
    end,

    on_leave = function(e, reason)
        if reason == "death" then
            act.sound(e.params.sfx_die)
        end
    end,

    draw = function(e)
        if not look.blinking(e) then
            look.sprite(e, "sprite", (tick() // 12) % 2, e.fx > 0)
        end
    end,
}
