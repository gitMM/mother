-- CAR (body, RALLY): the avatar's car. A accelerates, B brakes and then reverses, left and right
-- steer; it has momentum and grip, and offroad tiles halve its top speed (core/drive.luau). Ramming:
-- its contact tier 1 lands only on the car that was hit (core/drive.luau), knocking it away, and
-- a hit spins it out: stun response, so it stops dead for 60 ticks. A racer; it starts facing the
-- racing line. Needs gravity none, a ground surface and avatar heading.
local axes = use("core/axes")
local units = use("core/units")
local drive = use("core/drive")

return {
    category = "body",
    layer = "ground",
    size = "M",
    w = 10,
    h = 10,
    kinematic = true, -- top-down, so gravity never applies (it requires gravity none anyway)
    damage = "stun",
    hp = 0,
    tier = 1,
    racer = true,
    requires = {
        gravity = { "none" },
        surface = { "ground" },
        scroll = axes.ANY,
        avatar = { "heading" },
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        speed = units.run, -- top speed
        sprite = 0, -- 0, 22.5, 45, 67.5 and 90 degrees
        sfx_ram = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 5 } },
    sounds = { "sfx_ram" },
    public = { "angle" },

    init = drive.init,

    update = function(e)
        local steer = (if btn("right") then 1 else 0) - (if btn("left") then 1 else 0)
        drive.step(e, btn("a"), btn("b"), steer)
    end,

    guard = drive.guard,

    on_contact = drive.rammed,

    draw = drive.draw,
}
