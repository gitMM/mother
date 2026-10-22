-- SHIP (body, STARFIGHTER): the avatar. Flies in 8 directions at run speed and never leaves the
-- screen: it rides along with an auto-scrolling camera and stops at the view's edges. A fires a
-- shot straight ahead (it faces up) at projectile speed, tier 1, with at most 2 of its shots on
-- screen at once. Needs avatar up.
local axes = use("core/axes")
local units = use("core/units")
local S = use("core/state")
local act = use("core/act")
local look = use("core/look")
local entity = use("core/entity")
local spawn = use("core/spawn")

local DIAGONAL = math.sqrt(0.5)

-- Live shots of this ship, dropping the spent ones.
local function shots_out(m)
    local live = {}
    for _, s in m.shots do
        if entity.live(s) then
            table.insert(live, s)
        end
    end
    m.shots = live
    return #live
end

return {
    category = "body",
    layer = "air",
    size = "M",
    w = 12,
    h = 12,
    kinematic = true, -- it flies; gravity never pulls it
    damage = "hp",
    hp = 3,
    tier = 0,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = { "up" },
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        speed = units.run,
        shots = 2,
        tier = 1,
        sprite = 0, -- level, then thrusting
        shot_sprite = 0,
        sfx_fire = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 2 }, shot_sprite = { w = 1, h = 1, frames = 1 } },
    sounds = { "sfx_fire" },

    init = function(e)
        e.fx, e.fy = 0, -1
        e.mem.shots = {}
    end,

    update = function(e)
        local p, m = e.params, e.mem
        local x, y = act.dir8()
        local k = if x ~= 0 and y ~= 0 then DIAGONAL else 1
        local speed = act.speed(e, p.speed)
        local dx, dy = act.drift()
        -- the next position, kept inside the view where the camera will be after this step
        local cx, cy = S.cam_x + dx, S.cam_y + dy
        local nx = math.clamp(e.x + x * speed * k + dx, cx + e.hw, cx + units.screen_w - e.hw)
        local ny = math.clamp(e.y + y * speed * k + dy, cy + e.hh, cy + units.screen_h - e.hh)
        e.vx, e.vy = nx - e.x, ny - e.y
        if btnp("a") and shots_out(m) < p.shots then
            local shot = spawn.child(e, "shot", {
                vx = 0,
                vy = -units.projectile,
                tier = p.tier,
                life = 60,
                w = 2,
                h = 6,
                sprite = p.shot_sprite,
            }, e.x, e.y - e.hh - 3, e.faction)
            table.insert(m.shots, shot)
            act.sound(p.sfx_fire)
        end
    end,

    draw = function(e)
        if not look.blinking(e) then
            look.sprite(e, "sprite", (tick() // 4) % 2) -- the thruster flickers
        end
    end,
}
