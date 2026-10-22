-- Damage (brief section 6). Tiers are 1, 3, 10 and 30. An "hp" body loses the tier from its HP
-- and dies at 0; a "stun" body is stunned for 60 ticks per hit and never dies; a "none" body
-- is immune. After a hit that lands, a body takes no damage for units.hurt_ticks.
local units = use("core/units")
local entity = use("core/entity")
local events = use("core/events")
local objective = use("core/objective")

local damage = {}

damage.TIERS = table.freeze({ [1] = true, [3] = true, [10] = true, [30] = true })

-- One hit on e from src (an entity or nil). Returns true when it landed.
function damage.hit(e, tier, src)
    if not entity.live(e) or e.damage == "none" or e.hurt > 0 then
        return false
    end
    if src ~= nil and src.imported and objective.counts(e) then
        return false -- objective entities are safe from non-native ones
    end
    e.hurt = units.hurt_ticks
    if src ~= nil then
        e.src_x, e.src_y = src.x, src.y
    end
    if e.damage == "hp" then
        e.hp = math.max(e.hp - tier, 0)
    else
        e.stun = math.max(e.stun, units.stun_ticks)
    end
    events.emit("hit", e.id, tier, src and src.id)
    if e.damage == "hp" and e.hp == 0 then
        damage.kill(e, src)
    end
    return true
end

-- Death: from HP running out or a kill edge. The entity leaves the world at the end of the step.
function damage.kill(e, src)
    if not entity.live(e) then
        return
    end
    e.dead = true
    events.emit("death", e.id, entity.view(e), src and src.id)
end

-- Tile (tx, ty) on layer 0 takes a hit: a damageable tile (flag bit 1, HP 1) breaks, leaving an
-- empty cell, unless it is critical (bit 2). Returns true when it broke.
function damage.tile(tx, ty)
    local t = mget(tx, ty, 0)
    if t == 0 or not fget(t, units.flag_damageable) or fget(t, units.flag_critical) then
        return false
    end
    mset(tx, ty, 0, 0)
    return true
end

return table.freeze(damage)
