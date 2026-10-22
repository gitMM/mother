-- FIREBALL (object): the fire flower's shot. Under gravity it bounces along floors (`bounce`
-- px/tick back up) and is spent when it hits a wall; without gravity it flies straight and
-- ricochets off solids `ricochets` times, then is spent. It hurts the first damageable entity of
-- another faction it touches with its tier and is spent; it fades after `life` ticks. Not an
-- export of any cart, like shot.
local axes = use("core/axes")
local act = use("core/act")
local look = use("core/look")
local entity = use("core/entity")
local factions = use("core/factions")
local motion = use("core/motion")

local function spend(e)
    e.mem.spent = true
    e.mem.vx, e.mem.vy = 0, 0
    e.vx, e.vy = 0, 0
    e.tier = 0
end

return {
    category = "object",
    layer = "projectile",
    size = "S",
    w = 6,
    h = 6,
    kinematic = false, -- under gravity it drops and bounces
    damage = "none",
    hp = 0,
    tier = 3,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = { vx = 3, vy = 0, tier = 3, bounce = 2, ricochets = 3, life = 150, sprite = 0 },
    sprites = { sprite = { w = 1, h = 1, frames = 2 } },

    init = function(e)
        local p, mem = e.params, e.mem
        e.tier = p.tier
        mem.vx, mem.vy, mem.life, mem.left = p.vx, p.vy, p.life, p.ricochets
        e.vx, e.vy = p.vx, p.vy
    end,

    update = function(e)
        local mem = e.mem
        mem.life -= 1
        if mem.spent or mem.life <= 0 then
            entity.remove(e, "remove")
        elseif motion.falls(e) then
            e.vx = mem.vx -- gravity owns vy
        else
            act.drive(e)
        end
    end,

    on_block = function(e, nx, ny)
        local mem = e.mem
        if motion.falls(e) then
            if ny < 0 then
                e.vy = -e.params.bounce
            elseif nx ~= 0 then
                spend(e)
            else
                e.vy = 0
            end
            return
        end
        mem.left -= 1
        if mem.left < 0 then
            spend(e)
        elseif nx ~= 0 then
            mem.vx = math.abs(mem.vx) * nx
        else
            mem.vy = math.abs(mem.vy) * ny
        end
        e.vx, e.vy = mem.vx, mem.vy
    end,

    on_contact = function(e, other)
        if other.damage ~= "none" and factions.hostile(e, other) then
            spend(e)
        end
    end,

    draw = function(e)
        if not e.mem.spent then
            look.sprite(e, "sprite", (tick() // 4) % 2)
        end
    end,
}
