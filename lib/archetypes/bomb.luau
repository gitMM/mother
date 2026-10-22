-- BOMB (object, KNIGHTFALL): a deployable. After a 120-tick fuse it blasts everything within 2
-- tiles: tier 10 to every faction, its own included, with knockback, and it breaks damageable
-- tiles. It falls under gravity; a stun holds the fuse.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local melee = use("core/melee")
local entity = use("core/entity")

local BLAST_TICKS = 12 -- the blast stays on screen this long

return {
    category = "object",
    layer = "object",
    size = "S",
    w = 6,
    h = 6,
    kinematic = false, -- under gravity = "down" it drops to the ground
    damage = "none",
    hp = 0,
    tier = 0,
    deployable = true,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        fuse = 120,
        radius = 2 * units.tile,
        tier = 10,
        sprite = 0, -- two frames: lit, flashing
        sprite_blast = 0,
        sfx_blast = -1,
    },
    sprites = { sprite = { w = 1, h = 1, frames = 2 }, sprite_blast = { w = 4, h = 4, frames = 1 } },
    sounds = { "sfx_blast" },

    init = function(e)
        e.mem.fuse, e.mem.blast = e.params.fuse, 0
    end,

    update = function(e)
        local m, p = e.mem, e.params
        act.halt(e)
        if m.blast > 0 then
            m.blast -= 1
            if m.blast == 0 then
                entity.remove(e, "remove")
            end
            return
        end
        m.fuse -= 1
        if m.fuse <= 0 then
            m.blast = BLAST_TICKS
            melee.blast(e, p.radius, p.tier)
            act.sound(p.sfx_blast)
        end
    end,

    draw = function(e)
        local m = e.mem
        if m.blast > 0 then
            look.sprite(e, "sprite_blast", 0)
        else
            local rate = if m.fuse < 40 then 4 else 12
            look.sprite(e, "sprite", (tick() // rate) % 2)
        end
    end,
}
