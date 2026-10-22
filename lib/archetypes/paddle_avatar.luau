-- PADDLE_AVATAR (body, BREAKOUT): the avatar. Moves along one axis, left and right with the
-- stick, at fast speed, holding its row (after a knockback it slides back). Faces up and reflects
-- objects and projectiles that hit it: the further from its centre, the steeper the angle.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local reflect = use("core/reflect")

return {
    category = "body",
    layer = "ground",
    size = "L",
    w = 32,
    h = 8,
    kinematic = true, -- it holds its row, gravity or not (it requires the up avatar anyway)
    reflector = true,
    damage = "stun",
    hp = 0,
    tier = 0,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = { "up" },
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = { speed = units.fast, sprite = 0, sfx_reflect = -1 },
    sprites = { sprite = { w = 4, h = 1, frames = 1 } },
    sounds = { "sfx_reflect" },

    init = function(e)
        e.fx, e.fy = 0, -1
        e.mem.row = e.y
    end,

    update = function(e)
        local speed = e.params.speed
        local sx = stick()
        e.vx = sx * speed
        e.vy = math.clamp(e.mem.row - e.y, -speed, speed)
    end,

    on_contact = function(e, other)
        if reflect.reflectable(other) and reflect.off(e, other) then
            act.sound(e.params.sfx_reflect)
        end
    end,

    draw = function(e)
        if not look.blinking(e) then
            look.sprite(e, "sprite", 0)
        end
    end,
}
