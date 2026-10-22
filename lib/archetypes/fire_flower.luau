-- FIRE_FLOWER (equipment, JUMPER): on its button throws a fireball from the holder's front along
-- its facing: tier 3, at most 2 out at once. Under gravity the fireball sets off level and bounces
-- along floors; without gravity it flies along the facing and ricochets (archetypes/fireball).
-- Taking it (or being given it) raises the holder's HP to `hp` if that is higher (JUMPER: 2). As a
-- loose pickup it drifts down until the avatar touches it.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local equip = use("core/equip")
local spawn = use("core/spawn")
local entity = use("core/entity")
local motion = use("core/motion")

local function power_up(e)
    local h = e.mem.holder
    if h ~= nil and h.damage == "hp" then
        h.hp = math.max(h.hp, e.params.hp)
    end
end

local function out(mem)
    local live = {}
    for _, b in mem.balls do
        if entity.live(b) then
            table.insert(live, b)
        end
    end
    mem.balls = live
    return #live
end

return {
    category = "equipment",
    layer = "air",
    size = "M",
    w = 10,
    h = 12,
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
        max = 2,
        hp = 0, -- the holder's HP once taken, at least
        sprite = 0, -- the pickup
        fireball_sprite = 0,
        sfx_fire = -1,
        sfx_take = -1,
    }),
    sprites = { sprite = { w = 2, h = 2, frames = 1 }, fireball_sprite = { w = 1, h = 1, frames = 2 } },
    sounds = { "sfx_fire", "sfx_take" },

    init = function(e)
        e.mem.balls = {}
        equip.init(e)
        power_up(e)
    end,

    update = function(e)
        local p, mem = e.params, e.mem
        local h = equip.update(e)
        if h == nil or not equip.pressed(e) or out(mem) >= p.max then
            return
        end
        local fx, fy, vy = h.fx, h.fy, 0
        if motion.falls(h) then
            fx, fy, vy = if h.fx < 0 then -1 else 1, 0, units.walk
        end
        local front = math.abs(fx) * h.hw + math.abs(fy) * h.hh + 3
        local ball = spawn.child(e, "fireball", {
            vx = fx * units.fast,
            vy = fy * units.fast + vy,
            tier = p.tier,
            sprite = p.fireball_sprite,
        }, h.x + fx * front, h.y + fy * front, e.faction)
        table.insert(mem.balls, ball)
        act.sound(e, p.sfx_fire)
    end,

    on_contact = function(e, other)
        -- taken, or it renewed the flower the avatar holds: the avatar is powered up either way
        if equip.take(e, other) then
            if other.damage == "hp" then
                other.hp = math.max(other.hp, e.params.hp)
            end
            act.sound(e, e.params.sfx_take)
        end
    end,

    draw = function(e)
        if not equip.held(e) then
            look.sprite(e, "sprite", 0)
        end
    end,
}
