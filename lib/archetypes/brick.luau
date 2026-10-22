-- BRICK (creature, BREAKOUT): a static solid body, HP 1. When it dies it may drop a pickup: the
-- archetype named by `drop`, spawned with `drop_params` for the player's faction.
local axes = use("core/axes")
local act = use("core/act")
local look = use("core/look")
local spawn = use("core/spawn")

return {
    category = "creature",
    layer = "ground",
    size = "M",
    w = 16,
    h = 8,
    kinematic = true, -- static: it never moves, gravity or not
    static = true,
    solid = true,
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
    params = { hp = 1, drop = "", drop_params = {}, sprite = 0, sfx_break = -1 },
    sprites = { sprite = { w = 2, h = 1, frames = 1 } },
    sounds = { "sfx_break" },

    init = function(e)
        e.hp = e.params.hp
    end,

    on_leave = function(e, reason)
        if reason ~= "death" then
            return
        end
        act.sound(e, e.params.sfx_break)
        if e.params.drop ~= "" then
            spawn.child(e, e.params.drop, e.params.drop_params, e.x, e.y, "player")
        end
    end,

    draw = function(e)
        look.sprite(e, "sprite", 0)
    end,
}
