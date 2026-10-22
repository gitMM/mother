-- KNIGHT (body, KNIGHTFALL): the avatar. Walks in 8 directions; B dodges: 12 ticks at fast speed
-- along its facing, unhurtable, then a 30-tick cooldown. Its sword is separate equipment
-- (archetypes/sword.luau) that the cart binds to A. Top-down only: gravity none, 8dir avatar.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")

local DIAGONAL = math.sqrt(0.5)

return {
    category = "body",
    layer = "ground",
    size = "M",
    w = 10,
    h = 12,
    kinematic = true, -- top-down, so gravity never applies (it requires gravity none anyway)
    damage = "hp",
    hp = 30,
    tier = 0,
    requires = {
        gravity = { "none" },
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = { "8dir" },
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        speed = units.walk,
        dodge_ticks = 12,
        dodge_cooldown = 30,
        sprite = 0,
        sfx_dodge = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 3 } }, -- standing, stepping, dodging; facing right
    sounds = { "sfx_dodge" },

    init = function(e)
        e.fx, e.fy = 0, 1
        e.mem.dodge, e.mem.cool, e.mem.side = 0, 0, 1
    end,

    update = function(e)
        local p, m = e.params, e.mem
        if m.cool > 0 then
            m.cool -= 1
        end
        if m.dodge > 0 then
            m.dodge -= 1
            local speed = act.speed(e, units.fast)
            e.vx, e.vy = e.fx * speed, e.fy * speed
            return
        end
        local x, y = act.dir8()
        if x ~= 0 or y ~= 0 then
            act.face(e, x, y)
            if x ~= 0 then
                m.side = x
            end
        end
        local k = if x ~= 0 and y ~= 0 then DIAGONAL else 1
        local speed = act.speed(e, p.speed)
        e.vx, e.vy = x * speed * k, y * speed * k
        if m.cool == 0 and btnp("b") then
            m.dodge, m.cool = p.dodge_ticks, p.dodge_cooldown
            e.hurt = math.max(e.hurt, p.dodge_ticks)
            act.sound(e, p.sfx_dodge)
        end
    end,

    draw = function(e)
        if look.blinking(e) and e.mem.dodge == 0 then
            return
        end
        local moving = e.vx ~= 0 or e.vy ~= 0
        local frame = if e.mem.dodge > 0 then 2 elseif moving and (tick() // 8) % 2 == 1 then 1 else 0
        look.sprite(e, "sprite", frame, e.mem.side < 0)
    end,
}
