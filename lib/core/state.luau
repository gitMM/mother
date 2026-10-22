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
    started = nil, -- the step count when the cart first spawned something (core/objective.luau)
    progress = 0, -- grows as the objective gets nearer (core/objective.luau), for the matrix test
    last_count = nil, -- kill_all and collect: entities (and spawns) left at the last step
    best_exit = math.huge, -- escape: the avatar's nearest approach to the exit so far

    -- The import system (core/imports.luau).
    imports = {}, -- one record per imported export
    import_instances = {}, -- what it spawned, for the instance cap
    import_deploy = 0, -- the deployable Y dropped last
    import_timer = 0, -- steps until the next creature release
    import_next = 0, -- creature releases so far
    out_most = 0, -- the most steps any entity has been wholly outside the bounds
    out_who = "", -- that entity, described

    -- Per step, from collision: solid entities, and pairs that blocked each other.
    solids = {},
    bumps = {},
}
