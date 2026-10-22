-- Entity records: everything that moves, collides or takes damage. Archetype code works on the
-- records; carts only ever see frozen snapshots (views). Positions are the collider's centre.
local archetypes = use("core/archetypes")
local check = use("core/check")

local entity = {}

entity.FACTIONS = table.freeze({ player = true, host = true, wild = true })

function entity.new(id, name, def, params, x, y, faction, tick)
    return {
        id = id,
        name = name, -- archetype name
        arch = def,
        params = params,
        faction = faction, -- for objects, the owner's faction
        imported = false,
        born = tick,

        x = x,
        y = y,
        hw = def.w / 2, -- collider half sizes
        hh = def.h / 2,
        vx = 0,
        vy = 0,
        fx = 1, -- facing, for equipment and knockback
        fy = 0,
        sx = x, -- position before this step's move
        sy = y,
        bx0 = 0, -- the box swept this step, for the contact broadphase
        by0 = 0,
        bx1 = 0,
        by1 = 0,

        layer = def.layer,
        kinematic = def.kinematic,
        solid = def.solid == true,
        damage = def.damage,
        hp = def.hp,
        tier = def.tier,
        hurts_owner = def.hurts_owner == true,
        racer = def.racer == true,

        -- status timers, in ticks; knockback moves the body at (kx, ky)
        stun = 0,
        slip = 0,
        knock = 0,
        kx = 0,
        ky = 0,
        hurt = 0, -- no damage while above 0
        boost = 0, -- ticks left at double speed (nitro); see act.speed
        src_x = nil, -- where the last hit came from
        src_y = nil,

        -- what stopped its movement this step, by side
        left = false,
        right = false,
        up = false,
        down = false,

        entered = false, -- has overlapped the bounds, so the edges apply to it
        out = 0, -- steps wholly outside the bounds (core/imports.luau's audit)
        decay = 0, -- an imported solid: the step it decays at (0: never)
        ghost = false, -- out of play for now: no contacts, blocking, queries or bounds
        dead = false,
        gone = false, -- removed without dying
        laps = 0,
        wp = 1, -- the racing line point it heads for next (core/race.luau)
        started = false, -- has passed the line's first point
        mem = {}, -- the archetype's own state
    }
end

function entity.live(e)
    return e ~= nil and not e.dead and not e.gone
end

-- Takes a live entity out of the world without killing it; its archetype's on_leave(e, reason)
-- hook runs.
function entity.remove(e, reason)
    if entity.live(e) then
        e.gone = true
        local hook = e.arch.on_leave
        if hook ~= nil then
            hook(e, reason)
        end
    end
end

-- After an archetype's init(), which may change these fields.
function entity.validate(e)
    local where = `{e.name} (entity {e.id})`
    if not archetypes.collider(e.hw * 2) or not archetypes.collider(e.hh * 2) then
        error(`{where}: the collider must be even, 2-32 px a side`, 0)
    end
    check.one_of(e.layer, archetypes.LAYERS, `{where} layer`, 0)
    check.one_of(e.damage, archetypes.DAMAGE, `{where} damage`, 0)
    check.one_of(e.tier, archetypes.TIERS, `{where} tier`, 0)
    check.integer(e.hp, `{where} hp`, 0, 0)
end

local function freeze_status(e)
    return table.freeze({ stun = e.stun, slip = e.slip, knockback = e.knock })
end

function entity.view(e)
    local v = {
        id = e.id,
        archetype = e.name,
        faction = e.faction,
        layer = e.layer,
        imported = e.imported,
        x = e.x,
        y = e.y,
        vx = e.vx,
        vy = e.vy,
        w = e.hw * 2,
        h = e.hh * 2,
        hp = e.hp,
        laps = e.laps,
        status = freeze_status(e),
    }
    -- an archetype's public state: the mem fields it lists in `public`
    local public = e.arch.public
    if public ~= nil then
        for i = 1, #public do
            v[public[i]] = e.mem[public[i]]
        end
    end
    return table.freeze(v)
end

-- Filters for world:query and world:nearest. Every key is optional:
--   faction = f      only faction f          enemy_of = f   every faction but f
--   layer = l        only layer l            archetype = a  only archetype a
--   damageable = b   damage response other than "none" (true) or "none" (false)
--   exclude = id     never this entity
local FILTER = {
    faction = true, enemy_of = true, layer = true, archetype = true, damageable = true, exclude = true,
}

function entity.check_filter(f, level)
    if f ~= nil then
        check.table(f, "filter", level + 1)
        check.keys(f, FILTER, "filter", level + 1)
    end
end

function entity.matches(e, f)
    if f == nil then
        return true
    end
    if f.exclude ~= nil and e.id == f.exclude then
        return false
    end
    if f.faction ~= nil and e.faction ~= f.faction then
        return false
    end
    if f.enemy_of ~= nil and e.faction == f.enemy_of then
        return false
    end
    if f.layer ~= nil and e.layer ~= f.layer then
        return false
    end
    if f.archetype ~= nil and e.name ~= f.archetype then
        return false
    end
    if f.damageable ~= nil and (e.damage ~= "none") ~= f.damageable then
        return false
    end
    return true
end

-- Strict overlap of e's collider with the rectangle x0..x1, y0..y1: touching edges don't count.
function entity.overlaps(e, x0, y0, x1, y1)
    return e.x - e.hw < x1 and x0 < e.x + e.hw and e.y - e.hh < y1 and y0 < e.y + e.hh
end

return table.freeze(entity)
