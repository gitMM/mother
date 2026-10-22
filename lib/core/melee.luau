-- Melee and blasts: area damage for swords, skeleton swings and bombs.
local S = use("core/state")
local units = use("core/units")
local entity = use("core/entity")
local factions = use("core/factions")
local damage = use("core/damage")
local status = use("core/status")

local melee = {}

melee.KNOCKBACK = 6 -- ticks of knockback for whatever a swing or blast hits

local function candidate(src, o)
    return o ~= src and entity.live(o) and not o.ghost and o.damage ~= "none"
end

-- A swing from src: every damageable entity of another faction (or any, when src hurts its
-- owner) whose centre lies within `reach` px of src's edge plus its own half size, inside an
-- `arc`-degree cone around (fx, fy). Each one hit is knocked back. Returns how many were hit.
function melee.swing(src, fx, fy, reach, arc, tier)
    local cos_half = m.cos(arc * math.pi / 360)
    local list = S.list
    local hits = 0
    for i = 1, #list do
        local o = list[i]
        if candidate(src, o) and factions.hostile(src, o) then
            local dx, dy = o.x - src.x, o.y - src.y
            local d = math.sqrt(dx * dx + dy * dy)
            local r = reach + math.max(src.hw, src.hh) + math.max(o.hw, o.hh)
            if d <= r and (d == 0 or dx * fx + dy * fy >= d * cos_half) and damage.hit(o, tier, src) then
                status.apply(o, "knockback", melee.KNOCKBACK)
                hits += 1
            end
        end
    end
    return hits
end

-- A blast centred on src: every damageable entity (any faction) whose collider comes within
-- `radius` px takes the tier and is knocked back, and every layer-0 tile whose centre lies within
-- it takes a hit (damageable ones break). Returns how many entities were hit.
function melee.blast(src, radius, tier)
    local list = S.list
    local hits = 0
    for i = 1, #list do
        local o = list[i]
        if candidate(src, o) then
            local dx = math.clamp(src.x, o.x - o.hw, o.x + o.hw) - src.x
            local dy = math.clamp(src.y, o.y - o.hh, o.y + o.hh) - src.y
            if dx * dx + dy * dy <= radius * radius and damage.hit(o, tier, src) then
                status.apply(o, "knockback", melee.KNOCKBACK)
                hits += 1
            end
        end
    end
    local T = units.tile
    for ty = math.floor((src.y - radius) / T), math.floor((src.y + radius) / T) do
        for tx = math.floor((src.x - radius) / T), math.floor((src.x + radius) / T) do
            local dx, dy = tx * T + T / 2 - src.x, ty * T + T / 2 - src.y
            if dx * dx + dy * dy <= radius * radius then
                damage.tile(tx, ty)
            end
        end
    end
    return hits
end

return table.freeze(melee)
