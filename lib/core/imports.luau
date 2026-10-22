-- The import system (brief sections 6 and 9): exports of other carts, running in this one. The
-- host hands each import over once the world exists (imports.add, before main.luau runs). Its
-- art and sounds are copied in first (host.import_tiles into sheet 3, host.import_sfx into the
-- reserved sfx slots); then, by category:
--   equipment   held by the avatar from the start and used with X
--   deployable  dropped behind the avatar on Y (several deployables take turns)
--   creature    released at the cart's spawn points: one at once, then one every 300 ticks
--   object      an auto-object: one at the start, at the top of the view over the avatar
-- Creatures and auto-objects are faction wild (creatures and hazards); equipment and deployables
-- are the player's. At most 3 exports, and at most 8 instances of them alive (what this system
-- spawns; their shots and drops are imported too, but don't count). Imported solids decay after
-- 1,200 ticks. Every step also notes how long each entity has been wholly outside the bounds,
-- for the matrix test's audit.
local S = use("core/state")
local units = use("core/units")
local archetypes = use("core/archetypes")
local entity = use("core/entity")
local spawn = use("core/spawn")
local bounds = use("core/bounds")

local imports = {}

imports.MAX_EXPORTS = 3
imports.MAX_INSTANCES = 8
imports.RELEASE_EVERY = 300

local W = units.screen_w

local function sorted_keys(t)
    local keys = {}
    for k in t do
        table.insert(keys, k)
    end
    table.sort(keys)
    return keys
end

-- Points the export's art and sounds at the copies the host made of them in this cart.
local function copy_assets(def, p)
    if def.sprites ~= nil then
        for _, name in sorted_keys(def.sprites) do
            local block = def.sprites[name]
            if p[name] > 0 then
                p[name] = host.import_tiles(p[name], block.w * block.frames, block.h)
            end
        end
    end
    for _, name in def.sounds or {} do
        if p[name] >= 0 then
            p[name] = host.import_sfx(p[name])
        end
    end
end

local function live_instances()
    local kept = {}
    for _, e in S.import_instances do
        if entity.live(e) then
            table.insert(kept, e)
        end
    end
    S.import_instances = kept
    return #kept
end

-- params: imp.params unless given.
local function release(imp, x, y, params)
    local e = spawn.entity(imp.name, params or imp.params, x, y, imp.faction, true)
    table.insert(S.import_instances, e)
    return e
end

-- spec: { archetype, params (a table or nil), source, export }. Returns nil once it is in, or
-- why this cart can't take it ("needs surface=ground, host has none").
function imports.add(spec)
    local label = `{spec.source}:{spec.export}`
    if #S.imports >= imports.MAX_EXPORTS then
        error(`import {label}: at most {imports.MAX_EXPORTS} exports can be imported`, 0)
    end
    local def = archetypes.get(spec.archetype)
    if def == nil then
        error(`import {label}: there is no archetype {spec.archetype}`, 0)
    end
    local why = archetypes.check(def, S.env)
    if why ~= nil then
        return why
    end
    if def.category == "body" then
        return "is a body (an avatar), which never leaves its cart"
    end
    local p = table.clone(archetypes.params(spec.archetype, def, spec.params, 0))
    copy_assets(def, p)
    local imp = { name = spec.archetype, def = def, label = label }
    if def.category == "equipment" then
        imp.mode, imp.faction = "equip", "player"
        p.button, p.loose, p.ticks = "x", false, 0
    elseif def.deployable then
        imp.mode, imp.faction = "deploy", "player"
    elseif def.category == "creature" then
        imp.mode, imp.faction = "creature", "wild"
    else
        imp.mode, imp.faction = "auto", "wild"
    end
    imp.params = table.freeze(p)
    table.insert(S.imports, imp)
    local a = S.avatar
    if imp.mode == "equip" then
        release(imp, a.x, a.y)
    elseif imp.mode == "auto" then
        release(imp, if entity.live(a) then a.x else S.cam_x + W / 2, S.cam_y + 16)
    end
    return nil
end

-- Y: the next deployable, dropped just behind the avatar; a `dir` param points away from it.
local function deploy()
    local a = S.avatar
    local list = {}
    for _, imp in S.imports do
        if imp.mode == "deploy" then
            table.insert(list, imp)
        end
    end
    if #list == 0 or not entity.live(a) or live_instances() >= imports.MAX_INSTANCES then
        return
    end
    S.import_deploy = S.import_deploy % #list + 1
    local imp = list[S.import_deploy]
    local d = imp.def
    local x = a.x - a.fx * (a.hw + d.w / 2 + 2)
    local y = a.y - a.fy * (a.hh + d.h / 2 + 2)
    local p = imp.params
    if p.dir ~= nil then
        p = table.clone(p)
        p.dir = if a.fx > 0 then -1 else 1
        table.freeze(p)
    end
    release(imp, x, y, p).entered = true -- like a shot: it belongs inside, even off an edge
end

-- Creatures: one at the next spawn point every RELEASE_EVERY steps (taking turns), from the first.
local function release_creatures()
    if S.import_timer > 0 then
        S.import_timer -= 1
        return
    end
    local list = {}
    for _, imp in S.imports do
        if imp.mode == "creature" then
            table.insert(list, imp)
        end
    end
    if #list == 0 then
        return
    end
    S.import_timer = imports.RELEASE_EVERY - 1
    if live_instances() >= imports.MAX_INSTANCES then
        return
    end
    S.import_next += 1
    local imp = list[(S.import_next - 1) % #list + 1]
    release(imp, spawn.point((S.import_next - 1) % #S.spawn_points + 1))
end

-- Once per step, after the spawners.
function imports.step()
    if #S.imports > 0 then
        if host.btnp("y") then
            deploy()
        end
        release_creatures()
    end
    local x0, y0, x1, y1 = bounds.rect()
    local most = 0
    for _, e in S.list do
        if entity.live(e) then
            if e.imported and e.solid and e.decay > 0 and S.tick >= e.decay then
                entity.remove(e, "remove")
            elseif not e.ghost and not entity.overlaps(e, x0, y0, x1, y1) then
                e.out += 1
                if e.out > most then
                    most = e.out
                    S.out_who = `{e.name} {e.id} at {math.floor(e.x)},{math.floor(e.y)}`
                end
            else
                e.out = 0
            end
        end
    end
    S.out_most = most
end

-- For the matrix test, after a step: the most steps any entity has been wholly outside the
-- bounds (and which entity that is), whether an imported solid is in the world, and the
-- objective's progress (a count that grows as the objective gets nearer; core/objective.luau).
function imports.audit()
    local solids = false
    for _, e in S.list do
        solids = solids or (e.imported and e.solid and entity.live(e))
    end
    return S.out_most, solids, S.progress, S.out_who
end

-- Imported entities on screen get a Mother Gold outline from the host (equipment in hand and
-- entities out of play don't).
function imports.outline(e)
    if e.imported and not e.ghost and e.mem.holder == nil then
        host.outline(e.x - e.hw - 1, e.y - e.hh - 1, e.hw * 2 + 2, e.hh * 2 + 2)
    end
end

return table.freeze(imports)
