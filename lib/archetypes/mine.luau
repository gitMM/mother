-- MINE (creature, hazard, STARFIGHTER): sits still in world space, so an auto-scrolling camera
-- brings it into view. Any contact or hit sets it off: a blast of radius 1 tile, tier 10 to every
-- damageable entity in it, any faction, with knockback (core/melee.luau). It takes hits without
-- dying (stun response) because the blast is its end; it then shows the blast for 12 ticks, out
-- of play. Needs spawn points.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local melee = use("core/melee")
local entity = use("core/entity")

local BLAST_TICKS = 12

local function detonate(e)
    local p, m = e.params, e.mem
    if m.blast > 0 then
        return
    end
    m.blast = BLAST_TICKS
    e.ghost = true
    e.stun = 0 -- out of play: no status holds the blast back
    melee.blast(e, p.radius, p.tier)
    act.sound(e, p.sfx_blast)
end

return {
    category = "creature",
    layer = "ground",
    size = "S",
    w = 8,
    h = 8,
    kinematic = true, -- static in world space, gravity or not
    static = true,
    damage = "stun",
    hp = 0,
    tier = 0,
    requires = {
        gravity = axes.ANY,
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = { "world", "camera" },
        deploy = axes.ANY,
    },
    params = {
        radius = units.tile,
        tier = 10,
        sprite = 0, -- two frames: dark, blinking
        sprite_blast = 0,
        sfx_blast = -1,
    },
    sprites = { sprite = { w = 1, h = 1, frames = 2 }, sprite_blast = { w = 2, h = 2, frames = 1 } },
    sounds = { "sfx_blast" },

    init = function(e)
        e.mem.blast = 0
    end,

    update = function(e)
        local m = e.mem
        if m.blast > 0 then
            m.blast -= 1
            if m.blast == 0 then
                entity.remove(e, "remove")
            end
        elseif e.hurt > 0 then
            detonate(e) -- hit by something that never touched it (a blast)
        end
    end,

    on_contact = function(e)
        detonate(e)
    end,

    draw = function(e)
        if e.mem.blast > 0 then
            look.sprite(e, "sprite_blast", 0)
        else
            look.sprite(e, "sprite", (tick() // 20) % 2)
        end
    end,
}
