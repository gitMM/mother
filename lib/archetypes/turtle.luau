-- TURTLE (creature, JUMPER): walks like a walker (archetypes/walker.luau): crawl speed, turning at
-- solids, contact tier 1, HP 1. Killed by a hit from above (a stomp), it leaves a shell sliding
-- away from whatever hit it. Needs gravity down, a ground surface and spawn points; its shell
-- needs deploy.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local spawn = use("core/spawn")
local walker = use("archetypes/walker")

return {
    category = "creature",
    layer = "ground",
    size = "M",
    w = 12,
    h = 14,
    kinematic = false, -- it falls
    damage = "hp",
    hp = 1,
    tier = 1,
    requires = {
        gravity = { "down" },
        surface = { "ground" },
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = { "world", "camera" },
        deploy = axes.ANY,
    },
    params = {
        speed = units.crawl,
        sprite = 0, -- two steps, facing left
        shell_sprite = 0, -- passed to its shell
        sfx_die = -1,
        sfx_kick = -1, -- passed to its shell, for its bumps
    },
    sprites = { sprite = { w = 2, h = 2, frames = 2 }, shell_sprite = { w = 2, h = 2, frames = 2 } },
    sounds = { "sfx_die", "sfx_kick" },

    init = walker.init,
    update = walker.update,
    on_block = walker.on_block,
    draw = walker.draw,

    on_leave = function(e, reason)
        if reason ~= "death" then
            return
        end
        act.sound(e.params.sfx_die)
        if e.src_y ~= nil and e.src_y < e.y - e.hh then
            local dir = if e.src_x > e.x then -1 elseif e.src_x < e.x then 1 else -e.fx
            spawn.child(e, "shell", { dir = dir, sprite = e.params.shell_sprite, sfx_bump = e.params.sfx_kick },
                e.x, e.y + e.hh - 5, e.faction)
        end
    end,
}
