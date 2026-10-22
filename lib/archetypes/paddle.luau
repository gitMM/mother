-- PADDLE (equipment, BREAKOUT): a 24 px reflector held just in front of the avatar, across its
-- facing. Objects and projectiles that hit it go back out along the facing, angled by where they
-- hit, and its faction takes ownership of them. Always on: it has no use for its button.
local axes = use("core/axes")
local act = use("core/act")
local look = use("core/look")
local equip = use("core/equip")
local reflect = use("core/reflect")

local THICK = 4

-- Places the paddle in front of its holder, across the holder's main facing axis.
local function place(e, h)
    local p = e.params
    if math.abs(h.fy) >= math.abs(h.fx) then
        local s = if h.fy < 0 then -1 else 1
        e.fx, e.fy = 0, s
        e.hw, e.hh = p.width / 2, THICK / 2
        e.x, e.y = h.x, h.y + s * (h.hh + THICK)
    else
        local s = if h.fx < 0 then -1 else 1
        e.fx, e.fy = s, 0
        e.hw, e.hh = THICK / 2, p.width / 2
        e.x, e.y = h.x + s * (h.hw + THICK), h.y
    end
end

return {
    category = "equipment",
    layer = "air",
    size = "L",
    w = 24,
    h = 4,
    kinematic = true, -- carried, or drifting at its own `drop` speed as a pickup
    reflector = true,
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
        width = 24,
        sprite = 0, -- across, 24x8 with the bar in the top rows
        sprite_v = 0, -- upright, 8x24 with the bar in the left columns
        sfx_reflect = -1,
    }),
    sprites = { sprite = { w = 3, h = 1, frames = 1 }, sprite_v = { w = 1, h = 3, frames = 1 } },
    sounds = { "sfx_reflect" },

    init = function(e)
        equip.init(e)
        if equip.held(e) then
            place(e, e.mem.holder)
        end
    end,

    update = function(e)
        equip.update(e)
        if equip.held(e) and e.mem.holder ~= nil then
            place(e, e.mem.holder)
        end
    end,

    on_contact = function(e, other)
        if not equip.held(e) then
            equip.take(e, other)
        elseif other ~= e.mem.holder and reflect.reflectable(other) and reflect.off(e, other) then
            act.sound(e.params.sfx_reflect)
        end
    end,

    draw = function(e)
        -- the bar fills the sprite's top rows (left columns upright), so shift it onto the collider
        if e.fx == 0 then
            look.sprite_at(e, "sprite", 0, e.x, e.y + 2)
        else
            look.sprite_at(e, "sprite_v", 0, e.x + 2, e.y)
        end
    end,
}
