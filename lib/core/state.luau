-- The world's mutable state, shared by the core modules. Carts never reach it: they get the
-- frozen `world` table and frozen entity views.
return {
    -- From cart.toml, set once by world.create().
    env = {},
    objective = { type = "kill_all" },
    spawn_points = {},
    exports = {},

    tick = 0, -- steps run
    next_id = 1, -- entity ids are monotonic and never reused
    list = {}, -- entities in id order; dead and removed ones leave at the end of each step
    by_id = {}, -- id -> entity, for lookups only (never iterated)
    avatar = nil, -- the [avatar] entity

    world_w = 320, -- the world rectangle: layer 0 of the map, or the screen without one
    world_h = 180,
    cam_x = 0,
    cam_y = 0,

    spawners = {},
    next_spawner = 1,
    objective_met = false,

    -- Per step, from collision: solid entities, and pairs that blocked each other.
    solids = {},
    bumps = {},
}
