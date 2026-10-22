-- DIVER (creature, STARFIGHTER): flies in from its spawn point on a swooping curve (sideways out
-- towards the middle of the screen and back) for `swoop` ticks, then dives straight at the
-- nearest damageable entity of another faction at fast speed, firing 1 bullet at it. Once it has
-- left the screen, or met a solid edge, it comes back in where it first appeared and swoops
-- again. Air layer: tiles never stop it. HP 1, contact tier 1. All its moves are relative to the
-- camera, so it keeps formation under auto scroll. Needs spawn points.
local axes = use("core/axes")
local units = use("core/units")
local S = use("core/state")
local act = use("core/act")
local look = use("core/look")
local spawn = use("core/spawn")

local W, H = units.screen_w, units.screen_h

local function on_screen(e)
    return e.x >= S.cam_x and e.x <= S.cam_x + W and e.y >= S.cam_y and e.y <= S.cam_y + H
end

-- Back to where it first appeared (relative to the camera), outside the edges' reach until it is
-- on screen again.
local function reenter(e)
    local m = e.mem
    e.x, e.y = S.cam_x + m.hx, S.cam_y + m.hy
    e.entered = false
    m.state, m.t, m.seen = "swoop", 0, false
end

local function dive(e)
    local p, m = e.params, e.mem
    local t = act.target(e)
    local dx, dy = 0, 1
    if t ~= nil then
        local len = math.sqrt((t.x - e.x) ^ 2 + (t.y - e.y) ^ 2)
        if len > 0 then
            dx, dy = (t.x - e.x) / len, (t.y - e.y) / len
        end
    end
    m.state, m.vx, m.vy = "dive", dx * p.speed, dy * p.speed
    if t ~= nil then
        spawn.child(e, "shot", {
            vx = dx * p.bullet_speed,
            vy = dy * p.bullet_speed,
            tier = p.bullet_tier,
            life = 180,
            w = 4,
            h = 4,
            sprite = p.shot_sprite,
        }, e.x + dx * (e.hh + 3), e.y + dy * (e.hh + 3), e.faction)
    end
    act.sound(p.sfx_dive)
end

return {
    category = "creature",
    layer = "air",
    size = "M",
    w = 10,
    h = 10,
    kinematic = true, -- it flies; gravity never pulls it
    damage = "hp",
    hp = 1,
    tier = 1,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = { "world", "camera" },
        deploy = axes.ANY,
    },
    params = {
        speed = units.fast, -- diving
        swoop = 90, -- ticks
        swing = 1.5, -- the swoop's sideways speed at its widest, px/tick
        bullet_speed = units.run,
        bullet_tier = 1,
        sprite = 0, -- two wing beats, facing down
        shot_sprite = 0,
        sfx_dive = -1,
        sfx_die = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 2 }, shot_sprite = { w = 1, h = 1, frames = 1 } },
    sounds = { "sfx_dive", "sfx_die" },

    init = function(e)
        local m = e.mem
        m.hx, m.hy = e.x - S.cam_x, e.y - S.cam_y
        m.side = if m.hx < W / 2 then 1 else -1
        m.state, m.t, m.seen, m.vx, m.vy = "swoop", 0, false, 0, 0
    end,

    update = function(e)
        local p, mem = e.params, e.mem -- (m is the maths API)
        if on_screen(e) then
            mem.seen = true
        elseif mem.seen then
            reenter(e)
        end
        if mem.state == "swoop" then
            mem.t += 1
            mem.vx = mem.side * p.swing * m.cos(mem.t / p.swoop * math.pi)
            mem.vy = units.walk
            if mem.t >= p.swoop then
                dive(e)
            end
        end
        local dx, dy = act.drift()
        e.vx, e.vy = mem.vx + dx, mem.vy + dy
    end,

    on_block = function(e)
        reenter(e)
    end,

    on_leave = function(e, reason)
        if reason == "death" then
            act.sound(e.params.sfx_die)
        end
    end,

    draw = function(e)
        if not look.blinking(e) then
            look.sprite(e, "sprite", (tick() // 8) % 2)
        end
    end,
}
