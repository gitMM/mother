-- The MOTHERLIB world (brief section 6): the public `world` table, its creation from cart.toml
-- and world:step(). The host runs this module; it returns the hooks the host calls:
--   create(manifest)   once, before main.luau runs
--   import(spec)       after create, once per imported export (core/imports.luau)
--   step()             every tick, after cart.update()
--   hash()             every tick after step: the buffer and byte count to hash
--   audit()            the matrix test's checks after a step (core/imports.luau)
--   describe()         every archetype's declarations (core/archetypes.luau), for the host
--   check(name, env)   why a cart with this [env] can't import archetype `name`, nil if it can
local S = use("core/state")
local check = use("core/check")
local archetypes = use("core/archetypes")
local entity = use("core/entity")
local events = use("core/events")
local damage = use("core/damage")
local status = use("core/status")
local motion = use("core/motion")
local collision = use("core/collision")
local bounds = use("core/bounds")
local spawn = use("core/spawn")
local objective = use("core/objective")
local race = use("core/race")
local imports = use("core/imports")
local hash = use("core/hash")

local live = entity.live
local world = {}

-- Every method is called as world:name(...); simulation changes can't happen in cart.draw().
local function method(self, name, changes)
    if self ~= world then
        error(`call world:{name}(...) with a colon`, 3)
    end
    if changes and host.in_draw() then
        error(`world:{name}() changes the simulation, so it can't be called from cart.draw()`, 3)
    end
end

local function find(id)
    local e = S.by_id[id]
    return if live(e) then e else nil
end

-- A damage source: an entity id, or nil. An id that has left the world counts as no source.
local function source_entity(source)
    if source == nil then
        return nil
    end
    if type(source) ~= "number" then
        error(`world:damage: the source must be an entity id, got {type(source)}`, 3)
    end
    return S.by_id[source]
end

function world.spawn(self, archetype, params, x, y, faction)
    method(self, "spawn", true)
    local def = archetypes.get(archetype)
    if def == nil then
        error(`world:spawn: unknown archetype {archetype}`, 2)
    end
    local why = archetypes.check(def, S.env)
    if why ~= nil then
        error(`world:spawn: {archetype} {why}`, 2)
    end
    local p = archetypes.params(archetype, def, params, 3)
    check.number(x, "world:spawn x", 3)
    check.number(y, "world:spawn y", 3)
    check.one_of(faction, entity.FACTIONS, "world:spawn faction", 3)
    objective.start()
    return spawn.entity(archetype, p, x, y, faction).id
end

-- Removes an entity silently (no death event). Unknown or gone ids are ignored.
function world.remove(self, id)
    method(self, "remove", true)
    entity.remove(find(id), "remove")
end

function world.get(self, id)
    method(self, "get", false)
    local e = find(id)
    return if e ~= nil then entity.view(e) else nil
end

-- Ids of the entities whose colliders overlap the rectangle, in id order.
function world.query(self, x, y, w, h, filter)
    method(self, "query", false)
    check.number(x, "world:query x", 3)
    check.number(y, "world:query y", 3)
    check.number(w, "world:query w", 3)
    check.number(h, "world:query h", 3)
    entity.check_filter(filter, 3)
    local ids = {}
    local list = S.list
    for i = 1, #list do
        local e = list[i]
        if live(e) and not e.ghost and entity.overlaps(e, x, y, x + w, y + h) and entity.matches(e, filter) then
            table.insert(ids, e.id)
        end
    end
    return ids
end

-- The id of the entity whose centre is nearest (x, y); the lower id wins a tie.
function world.nearest(self, x, y, filter)
    method(self, "nearest", false)
    check.number(x, "world:nearest x", 3)
    check.number(y, "world:nearest y", 3)
    entity.check_filter(filter, 3)
    local best, best_d = nil, math.huge
    local list = S.list
    for i = 1, #list do
        local e = list[i]
        if live(e) and not e.ghost and entity.matches(e, filter) then
            local dx, dy = e.x - x, e.y - y
            local d = dx * dx + dy * dy
            if d < best_d then
                best, best_d = e.id, d
            end
        end
    end
    return best
end

-- Returns true when the hit landed (see core/damage.luau).
function world.damage(self, id, tier, source)
    method(self, "damage", true)
    if not damage.TIERS[tier] then
        error(`world:damage: the tier must be 1, 3, 10 or 30, got {tier}`, 2)
    end
    local src = source_entity(source)
    local e = find(id)
    return e ~= nil and damage.hit(e, tier, src)
end

function world.status(self, id, kind, ticks)
    method(self, "status", true)
    check.one_of(kind, status.FIELDS, "world:status kind", 3)
    check.integer(ticks, "world:status ticks", 0, 3)
    local e = find(id)
    if e ~= nil then
        status.apply(e, kind, ticks)
    end
end

function world.on(self, event, fn)
    method(self, "on", true)
    check.one_of(event, events.names, "world:on event", 3)
    if type(fn) ~= "function" then
        error("world:on: the handler must be a function", 2)
    end
    events.on(event, fn)
end

function world.avatar(self)
    method(self, "avatar", false)
    return if live(S.avatar) then S.avatar.id else nil
end

function world.camera(self)
    method(self, "camera", false)
    return S.cam_x, S.cam_y
end

local DRAW_ORDER = { "ground", "object", "projectile", "air" }

-- Draws every entity with a draw hook, layer by layer (air on top), in id order; imported ones
-- also get their outline queued for the host.
function world.draw(self)
    method(self, "draw", false)
    if not host.in_draw() then
        error("world:draw() can only be called from cart.draw()", 2)
    end
    local list = S.list
    for l = 1, #DRAW_ORDER do
        local layer = DRAW_ORDER[l]
        for i = 1, #list do
            local e = list[i]
            if live(e) and e.layer == layer and e.arch.draw ~= nil then
                e.arch.draw(e)
                imports.outline(e)
            end
        end
    end
end

local SPAWNER = { archetype = true, params = true, faction = true, count = true, every = true, points = true }

-- spec: { archetype, faction, params = nil, count = 1, every = 0, points = every spawn point }.
function world.spawner(self, spec)
    method(self, "spawner", true)
    check.table(spec, "world:spawner spec", 3)
    check.keys(spec, SPAWNER, "world:spawner spec", 3)
    if S.env.spawn == "none" then
        error("world:spawner: this cart has [env] spawn = none, so no spawn points", 2)
    end
    local name = spec.archetype
    local def = archetypes.get(name)
    if def == nil then
        error(`world:spawner: unknown archetype {name}`, 2)
    end
    local why = archetypes.check(def, S.env)
    if why ~= nil then
        error(`world:spawner: {name} {why}`, 2)
    end
    local points = {}
    if spec.points == nil then
        for i = 1, #S.spawn_points do
            points[i] = i
        end
    else
        check.table(spec.points, "world:spawner points", 3)
        for i = 1, #spec.points do
            points[i] = check.integer(spec.points[i], "world:spawner points", 1, 3)
            if points[i] > #S.spawn_points then
                error(`world:spawner: there is no spawn point {points[i]}`, 2)
            end
        end
        if #points == 0 then
            error("world:spawner: points is empty", 2)
        end
    end
    objective.start()
    return spawn.add_spawner({
        archetype = name,
        params = archetypes.params(name, def, spec.params, 3),
        faction = check.one_of(spec.faction, entity.FACTIONS, "world:spawner faction", 3),
        count = check.integer(if spec.count == nil then 1 else spec.count, "world:spawner count", 1, 3),
        every = check.integer(if spec.every == nil then 0 else spec.every, "world:spawner every", 0, 3),
        points = points,
    })
end

table.freeze(world)

local function create(m)
    archetypes.load()
    S.env = table.freeze(m.env)
    S.objective = table.freeze(m.objective)
    S.spawn_points = table.freeze(m.spawn_points or {})
    S.exports = table.freeze(m.exports or {})
    bounds.setup()
    local a = m.avatar
    if a ~= nil then
        local def = archetypes.get(a.archetype)
        if def == nil then
            error(`cart.toml: [avatar] archetype {a.archetype} doesn't exist`, 0)
        end
        local why = archetypes.check(def, S.env)
        if why ~= nil then
            error(`cart.toml: [avatar] {a.archetype} {why}`, 0)
        end
        local e = spawn.entity(a.archetype, archetypes.params(a.archetype, def, a.params, 0), a.x, a.y, "player")
        e.damage, e.hp = a.damage, a.hp
        S.avatar = e
    end
    bounds.update_camera()
end

local function purge()
    local kept = {}
    local list = S.list
    for i = 1, #list do
        local e = list[i]
        if live(e) then
            table.insert(kept, e)
        else
            S.by_id[e.id] = nil
        end
    end
    S.list = kept
end

-- One tick of simulation (brief section 4, step 3). Entities spawned during a step first act in
-- the next one. Racers move along the racing line after contacts, before the objective.
local function step()
    S.tick += 1
    local list = S.list
    local n = #list
    for i = 1, n do
        if live(list[i]) then
            status.behave(list[i])
        end
    end
    collision.begin()
    for i = 1, n do
        local e = list[i]
        if live(e) then
            motion.integrate(e)
            e.sx, e.sy = e.x, e.y
            collision.move(e)
        end
    end
    for i = 1, n do
        status.tick(list[i])
    end
    bounds.update_camera()
    for i = 1, n do
        if live(list[i]) then
            bounds.apply(list[i])
        end
    end
    collision.contacts(n)
    race.step()
    spawn.run()
    imports.step()
    objective.check()
    events.flush()
    purge()
end

return table.freeze({
    world = world,
    create = create,
    import = imports.add,
    step = step,
    hash = hash.write,
    audit = imports.audit,
    describe = archetypes.describe,
    check = imports.check,
})
