-- SHELL (object, JUMPER): slides along its facing (dir) at fast speed, turns round at solids and
-- falls under gravity. Harmless for its first 30 ticks, then tier 3 on contact to every faction,
-- its own included. Immune to damage. A deployable.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local motion = use("core/motion")

return {
    category = "object",
    layer = "object",
    size = "M",
    w = 12,
    h = 10,
    kinematic = false, -- it falls under gravity
    damage = "none",
    hp = 0,
    tier = 0, -- 3 once armed
    hurts_owner = true,
    deployable = true,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        dir = 1, -- 1 right, -1 left
        speed = units.fast,
        arm = 30, -- ticks before it hurts
        tier = 3,
        sprite = 0, -- two spin frames
        sfx_bump = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 2 } },
    sounds = { "sfx_bump" },

    init = function(e)
        e.fx = if e.params.dir < 0 then -1 else 1
        e.mem.age = 0
    end,

    update = function(e)
        local mem = e.mem
        mem.age += 1
        if mem.age == e.params.arm then
            e.tier = e.params.tier
        end
        e.vx = e.fx * e.params.speed
        if not motion.falls(e) then
            e.vy = 0
        end
    end,

    on_block = function(e, nx, ny)
        if nx ~= 0 then
            e.fx, e.vx = nx, 0
            act.sound(e, e.params.sfx_bump)
        else
            e.vy = 0
        end
    end,

    draw = function(e)
        look.sprite(e, "sprite", (tick() // 4) % 2)
    end,
}
