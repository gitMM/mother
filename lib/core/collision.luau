-- Swept AABB collision (brief section 6). A body moves one axis at a time, swept against solid
-- tiles and solid entities, so it can't pass through either at 4 px/tick. Then every pair
-- whose paths overlapped during the step, or that stopped one another, is in contact: contact
-- damage, then each side's on_contact.
local S = use("core/state")
local units = use("core/units")
local entity = use("core/entity")
local factions = use("core/factions")
local damage = use("core/damage")
local objective = use("core/objective")

local T = units.tile
local collision = {}

-- A cell blocks movement when its layer-0 tile isn't 0 and has flag bit 0. Outside the map
-- nothing is solid (the bounds take over there).
function collision.solid_tile(tx, ty)
    local t = mget(tx, ty, 0)
    return t ~= 0 and fget(t, units.flag_solid)
end

-- Air bodies pass over everything. Non-native solids never block objective entities.
local function blocks(o, e)
    return o ~= e and entity.live(o) and not (o.imported and objective.counts(e))
end

-- Is any cell solid in line c along the axis, cells r0..r1 across it?
local function line_solid(xaxis, c, r0, r1)
    for r = r0, r1 do
        if (xaxis and collision.solid_tile(c, r)) or (not xaxis and collision.solid_tile(r, c)) then
            return true
        end
    end
    return false
end

-- How far e can move by d (not 0) along one axis, and what stops it: true for a tile, an
-- entity, or nil. Cells the box already overlaps are skipped, so a body inside a wall can leave.
local function sweep(e, xaxis, d)
    if e.layer == "air" then
        return d, nil
    end
    local lo, hi, clo, chi
    if xaxis then
        lo, hi, clo, chi = e.x - e.hw, e.x + e.hw, e.y - e.hh, e.y + e.hh
    else
        lo, hi, clo, chi = e.y - e.hh, e.y + e.hh, e.x - e.hw, e.x + e.hw
    end
    local forward = d > 0
    local hit = nil
    local r0, r1 = math.floor(clo / T), math.ceil(chi / T) - 1
    if forward then
        for c = math.ceil(hi / T), math.ceil((hi + d) / T) - 1 do
            if line_solid(xaxis, c, r0, r1) then
                d, hit = c * T - hi, true
                break
            end
        end
    else
        for c = math.floor(lo / T) - 1, math.floor((lo + d) / T), -1 do
            if line_solid(xaxis, c, r0, r1) then
                d, hit = (c + 1) * T - lo, true
                break
            end
        end
    end
    local solids = S.solids
    for i = 1, #solids do
        local o = solids[i]
        if blocks(o, e) then
            local olo, ohi, oclo, ochi
            if xaxis then
                olo, ohi, oclo, ochi = o.x - o.hw, o.x + o.hw, o.y - o.hh, o.y + o.hh
            else
                olo, ohi, oclo, ochi = o.y - o.hh, o.y + o.hh, o.x - o.hw, o.x + o.hw
            end
            if clo < ochi and oclo < chi then
                if forward then
                    if olo >= hi and olo - hi < d then
                        d, hit = olo - hi, o
                    end
                elseif ohi <= lo and ohi - lo > d then
                    d, hit = ohi - lo, o
                end
            end
        end
    end
    return d, hit
end

-- Start of the motion phase: the step's solid entities, and no bumps yet.
function collision.begin()
    local solids = {}
    local list = S.list
    for i = 1, #list do
        local e = list[i]
        if e.solid and entity.live(e) and not e.ghost then
            table.insert(solids, e)
        end
    end
    S.solids = solids
    table.clear(S.bumps)
end

-- Something stopped e; (nx, ny) points away from it. The archetype's on_block decides the
-- response, by default stopping along that axis. `hit` is the entity that stopped it, if any.
function collision.block(e, nx, ny, hit)
    if type(hit) == "table" then
        table.insert(S.bumps, { e, hit })
    end
    local on_block = e.arch.on_block
    if on_block ~= nil then
        on_block(e, nx, ny, if type(hit) == "table" then hit else nil)
    elseif nx ~= 0 then
        e.vx = 0
    else
        e.vy = 0
    end
end

-- Moves e by its velocity, x first, then y.
function collision.move(e)
    e.left, e.right, e.up, e.down = false, false, false, false
    local dx, dy = e.vx, e.vy
    if dx ~= 0 then
        local d, hit = sweep(e, true, dx)
        e.x += d
        if hit ~= nil then
            if dx > 0 then
                e.right = true
            else
                e.left = true
            end
            collision.block(e, if dx > 0 then -1 else 1, 0, hit)
        end
    end
    if dy ~= 0 then
        local d, hit = sweep(e, false, dy)
        e.y += d
        if hit ~= nil then
            if dy > 0 then
                e.down = true
            else
                e.up = true
            end
            collision.block(e, 0, if dy > 0 then -1 else 1, hit)
        end
    end
end

-- Did a and b overlap at some moment of the step, each moving in a straight line from (sx, sy)
-- to (x, y)? Touching edges don't count.
local function swept(a, b)
    local px, py = a.sx - b.sx, a.sy - b.sy
    local dx = (a.x - a.sx) - (b.x - b.sx)
    local dy = (a.y - a.sy) - (b.y - b.sy)
    local wx, wy = a.hw + b.hw, a.hh + b.hh
    local t0, t1 = 0, 1
    if dx == 0 then
        if px <= -wx or px >= wx then
            return false
        end
    else
        local ta, tb = (-wx - px) / dx, (wx - px) / dx
        if ta > tb then
            ta, tb = tb, ta
        end
        t0, t1 = math.max(t0, ta), math.min(t1, tb)
    end
    if dy == 0 then
        if py <= -wy or py >= wy then
            return false
        end
    else
        local ta, tb = (-wy - py) / dy, (wy - py) / dy
        if ta > tb then
            ta, tb = tb, ta
        end
        t0, t1 = math.max(t0, ta), math.min(t1, tb)
    end
    return t0 < t1
end

local function key(a, b)
    if a.id < b.id then
        return a.id * 67108864 + b.id
    end
    return b.id * 67108864 + a.id
end

local function resolve(a, b)
    if a.id > b.id then
        a, b = b, a
    end
    if not (entity.live(a) and entity.live(b)) or a.ghost or b.ghost then
        return
    end
    -- Both sides hit at once, even if the first hit kills.
    if a.tier > 0 and factions.hostile(a, b) then
        damage.hit(b, a.tier, a)
    end
    if b.tier > 0 and factions.hostile(b, a) then
        damage.hit(a, b.tier, b)
    end
    local on_a, on_b = a.arch.on_contact, b.arch.on_contact
    if on_a ~= nil and entity.live(a) then
        on_a(a, b)
    end
    if on_b ~= nil and entity.live(b) then
        on_b(b, a)
    end
end

local function by_left(a, b)
    if a.bx0 ~= b.bx0 then
        return a.bx0 < b.bx0
    end
    return a.id < b.id
end

-- Contacts among the first n entities (those that existed when the step began): bumps first,
-- in the order they happened, then swept overlaps found by sweep-and-prune on x.
function collision.contacts(n)
    local seen = {} -- pair key -> true; looked up, never iterated
    local found = {}
    local bumps = S.bumps
    for i = 1, #bumps do
        local k = key(bumps[i][1], bumps[i][2])
        if not seen[k] then
            seen[k] = true
            table.insert(found, bumps[i])
        end
    end
    local list = S.list
    local order = {}
    for i = 1, n do
        local e = list[i]
        if entity.live(e) and not e.ghost then
            e.bx0, e.bx1 = math.min(e.sx, e.x) - e.hw, math.max(e.sx, e.x) + e.hw
            e.by0, e.by1 = math.min(e.sy, e.y) - e.hh, math.max(e.sy, e.y) + e.hh
            table.insert(order, e)
        end
    end
    table.sort(order, by_left)
    for i = 1, #order do
        local a = order[i]
        for j = i + 1, #order do
            local b = order[j]
            if b.bx0 >= a.bx1 then
                break
            end
            if b.by0 < a.by1 and a.by0 < b.by1 and swept(a, b) then
                local k = key(a, b)
                if not seen[k] then
                    seen[k] = true
                    table.insert(found, { a, b })
                end
            end
        end
    end
    for i = 1, #found do
        resolve(found[i][1], found[i][2])
    end
end

return table.freeze(collision)
