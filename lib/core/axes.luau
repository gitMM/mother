-- The [env] axes and their values (brief section 6). An archetype's `requires` names every axis:
-- either a list of the values it accepts, or axes.ANY.
local axes = {}

axes.ANY = table.freeze({})

axes.NAMES = table.freeze({ "gravity", "surface", "scroll", "avatar", "spawn", "deploy" })

axes.VALUES = table.freeze({
    gravity = table.freeze({ none = true, down = true }),
    surface = table.freeze({ none = true, ground = true }),
    scroll = table.freeze({ none = true, follow = true, auto = true }),
    avatar = table.freeze({ none = true, up = true, lr = true, ["8dir"] = true, heading = true }),
    spawn = table.freeze({ none = true, world = true, camera = true }),
    deploy = table.freeze({ [true] = true, [false] = true }),
})

return table.freeze(axes)
