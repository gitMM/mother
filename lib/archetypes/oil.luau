-- OIL (object, RALLY): a slick. Ground-layer bodies crossing it slip for 60 ticks (no steering:
-- they keep the velocity they had). At most 3 lie about at once; a 4th clears the oldest. Under
-- gravity it drops to the floor. Immune. A deployable that needs a ground surface.
local axes = use("core/axes")
local S = use("core/state")
local act = use("core/act")
local look = use("core/look")
local entity = use("core/entity")
local status = use("core/status")

return {
    category = "object",
    layer = "object",
    size = "M",
    w = 12,
    h = 12,
    kinematic = false, -- under gravity it falls to the floor
    damage = "none",
    hp = 0,
    tier = 0,
    deployable = true,
    requires = {
        gravity = axes.ANY,
        surface = { "ground" },
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = { ticks = 60, max = 3, sprite = 0, sfx_slip = -1 },
    sprites = { sprite = { w = 2, h = 2, frames = 1 } },
    sounds = { "sfx_slip" },

    init = function(e)
        local slicks = {}
        for _, o in S.list do
            if o.name == "oil" and entity.live(o) then
                table.insert(slicks, o)
            end
        end
        for i = 1, #slicks - e.params.max + 1 do
            entity.remove(slicks[i], "remove") -- the oldest first: the list is in id order
        end
    end,

    update = function(e)
        act.halt(e)
    end,

    on_contact = function(e, other)
        if other.layer == "ground" then
            if other.slip == 0 then
                act.sound(e, e.params.sfx_slip)
            end
            status.apply(other, "slip", e.params.ticks)
        end
    end,

    draw = function(e)
        look.sprite(e, "sprite", 0)
    end,
}
