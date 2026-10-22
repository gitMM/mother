-- DUMMY: a plain configurable body for MOTHERLIB's own tests and the archetype axis tests
-- ("a few other-faction dummies", brief section 9). Its params set its collider, layer, damage
-- response and contact tier; it steers at a constant (vx, vy) and, when it falls, leaves vy to
-- gravity. It accepts every [env] value.
local axes = use("core/axes")
local motion = use("core/motion")
local objective = use("core/objective")

return {
    category = "creature",
    layer = "ground",
    size = "S",
    w = 8,
    h = 8,
    kinematic = true,
    damage = "hp",
    hp = 1,
    tier = 0,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        w = 8,
        h = 8,
        layer = "ground",
        kinematic = true,
        solid = false,
        damage = "hp",
        hp = 1,
        tier = 0,
        hurts_owner = false,
        vx = 0,
        vy = 0,
        bounce = false, -- reverse along an axis when blocked, instead of stopping
        colour = 8,
        racer = false,
        lap_every = 0, -- with racer: one lap every this many steps
    },

    init = function(e)
        local p = e.params
        e.hw, e.hh = p.w / 2, p.h / 2
        e.layer, e.kinematic, e.solid = p.layer, p.kinematic, p.solid
        e.damage, e.hp, e.tier, e.hurts_owner = p.damage, p.hp, p.tier, p.hurts_owner
        e.racer = p.racer
        e.vx, e.vy = p.vx, p.vy
        e.mem.vx, e.mem.vy = p.vx, p.vy
    end,

    update = function(e)
        e.vx = e.mem.vx
        if not motion.falls(e) then
            e.vy = e.mem.vy
        end
        local every = e.params.lap_every
        if e.racer and every > 0 and (e.mem.steps or 0) % every == every - 1 then
            objective.lap(e)
        end
        e.mem.steps = (e.mem.steps or 0) + 1
    end,

    on_block = function(e, nx, ny)
        local bounce = e.params.bounce
        if nx ~= 0 then
            e.mem.vx = if bounce then -e.mem.vx else 0
            e.vx = e.mem.vx
        else
            e.mem.vy = if bounce then -e.mem.vy else 0
            e.vy = if motion.falls(e) then 0 else e.mem.vy
        end
    end,

    draw = function(e)
        rectfill(e.x - e.hw, e.y - e.hh, e.hw * 2, e.hh * 2, e.params.colour)
    end,
}
