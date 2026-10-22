-- Spawning (brief section 6): world:spawn, the [avatar] and spawners. A spawner releases
-- entities at the cart's [[spawn_points]], one every `every` steps (all at once when `every` is
-- 0) and cycling through its points, until `count` are out. Under [env] spawn = "camera" the
-- points are relative to the camera.
local S = use("core/state")
local archetypes = use("core/archetypes")
local entity = use("core/entity")
local events = use("core/events")
local bounds = use("core/bounds")

local spawn = {}

-- Imported solids decay this many steps after they appear (brief section 6).
spawn.DECAY = 1200

-- Creates an entity; the caller has checked every argument. `imported`: it came from another
-- cart (core/imports.luau), or from something that did.
function spawn.entity(name, params, x, y, faction, imported)
    local def = archetypes.get(name)
    local id = S.next_id
    S.next_id += 1
    local e = entity.new(id, name, def, params, x, y, faction, S.tick)
    e.imported = imported == true
    if e.imported and def.solid then
        e.decay = S.tick + spawn.DECAY
    end
    if def.init ~= nil then
        def.init(e)
    end
    entity.validate(e)
    e.sx, e.sy = e.x, e.y
    bounds.enter(e)
    table.insert(S.list, e)
    S.by_id[id] = e
    events.emit("spawn", id)
    return e
end

-- An entity spawned by another (a shot, a dropped pickup, a re-served ball). It must suit the
-- cart's [env] like any spawn, and it is imported when its parent is. The bounds' edges apply to
-- it from the start: it appears where its parent is, so even when that is just past an edge (a
-- shot from an avatar against a wall) it belongs inside.
function spawn.child(parent, name, params, x, y, faction)
    local def = archetypes.get(name)
    if def == nil then
        error(`{parent.name} spawns an unknown archetype {name}`, 0)
    end
    local why = archetypes.check(def, S.env)
    if why ~= nil then
        error(`{parent.name} spawns {name}, which {why}`, 0)
    end
    local e = spawn.entity(name, archetypes.params(name, def, params, 0), x, y, faction, parent.imported)
    e.entered = true
    return e
end

function spawn.point(i)
    local p = S.spawn_points[i]
    if S.env.spawn == "camera" then
        return S.cam_x + p.x, S.cam_y + p.y
    end
    return p.x, p.y
end

-- sp: { archetype, params (merged), faction, count, every, points (spawn point indices) }.
function spawn.add_spawner(sp)
    sp.id = S.next_spawner
    S.next_spawner += 1
    sp.left = sp.count
    sp.timer = 0
    sp.next = 1
    table.insert(S.spawners, sp)
    return sp.id
end

-- Once per step. Spawners with nothing left are dropped.
function spawn.run()
    local spawners = S.spawners
    for i = 1, #spawners do
        local sp = spawners[i]
        if sp.timer > 0 then
            sp.timer -= 1
        end
        while sp.left > 0 and sp.timer == 0 do
            local x, y = spawn.point(sp.points[sp.next])
            sp.next = sp.next % #sp.points + 1
            sp.left -= 1
            sp.timer = sp.every
            spawn.entity(sp.archetype, sp.params, x, y, sp.faction)
        end
    end
    local kept = {}
    for i = 1, #spawners do
        if spawners[i].left > 0 then
            table.insert(kept, spawners[i])
        end
    end
    S.spawners = kept
end

return table.freeze(spawn)
