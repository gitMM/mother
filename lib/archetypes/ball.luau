-- BALL (object, BREAKOUT): an auto-object at fast speed. It reflects off solids (tiles, solid
-- entities, solid edges) and bounces off player-faction bodies; reflectors (paddles) send it back
-- themselves. Tier 1 on contact; immune to damage. When it is lost (killed by an edge, or
-- despawned) a new ball is served 180 ticks later: from just above the avatar, or from where
-- the first one started when there is no avatar, heading up within 30° of straight.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local reflect = use("core/reflect")
local spawn = use("core/spawn")
local S = use("core/state")
local entity = use("core/entity")

local SPREAD = math.pi / 6

local function serve(e)
    local a = S.avatar
    if entity.live(a) then
        e.x, e.y = a.x, a.y - a.hh - e.hh - 2
    else
        e.x, e.y = e.mem.home_x, e.mem.home_y
    end
    e.ghost = false
    local angle = (rnd(2) - 1) * SPREAD
    local speed = e.params.speed
    act.push(e, speed * m.sin(angle), -speed * m.cos(angle))
end

return {
    category = "object",
    layer = "object",
    size = "S",
    w = 6,
    h = 6,
    kinematic = true, -- it ignores gravity: a ball keeps its speed and only bounces
    damage = "none",
    hp = 0,
    tier = 1,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        speed = units.fast,
        wait = 0, -- ticks before the first serve
        serve_wait = 180,
        home_x = -1, -- where to serve from without an avatar; -1: where it was spawned
        home_y = -1,
        sprite = 0,
        sfx_bounce = -1,
    },
    sprites = { sprite = { w = 1, h = 1, frames = 1 } },
    sounds = { "sfx_bounce" },

    init = function(e)
        local m, p = e.mem, e.params
        m.home_x = if p.home_x >= 0 then p.home_x else e.x
        m.home_y = if p.home_y >= 0 then p.home_y else e.y
        m.vx, m.vy = 0, 0
        m.wait = p.wait
        m.faction = e.faction
        if m.wait > 0 then
            e.ghost = true
        else
            serve(e)
        end
    end,

    update = function(e)
        local m = e.mem
        if m.wait > 0 then
            m.wait -= 1
            if m.wait == 0 then
                serve(e)
            end
            return
        end
        act.drive(e)
    end,

    on_block = function(e, nx, ny)
        local m = e.mem
        if nx ~= 0 then
            m.vx = math.abs(m.vx) * nx
        else
            m.vy = math.abs(m.vy) * ny
        end
        e.vx, e.vy = m.vx, m.vy
        act.sound(e.params.sfx_bounce)
    end,

    on_contact = function(e, other)
        local body = other.arch.category == "body" or other.arch.category == "creature"
        if body and other.faction == "player" and not other.arch.reflector and reflect.bounce(e, other) then
            act.sound(e.params.sfx_bounce)
        end
    end,

    on_leave = function(e, reason)
        if reason == "remove" then
            return
        end
        local p = table.clone(e.params)
        p.wait, p.home_x, p.home_y = p.serve_wait, e.mem.home_x, e.mem.home_y
        spawn.child(e, "ball", p, e.mem.home_x, e.mem.home_y, e.mem.faction)
    end,

    draw = function(e)
        if not e.ghost then
            look.sprite(e, "sprite", 0)
        end
    end,
}
