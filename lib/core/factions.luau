-- Factions and ownership (brief section 6). Every entity has a faction: player, host or wild.
-- For objects it is the owner's faction, and a reflector takes ownership of what it reflects.
local factions = {}

-- Contact damage from src to dst: only across factions, unless src also hurts its owner's.
function factions.hostile(src, dst)
    return src.faction ~= dst.faction or src.hurts_owner
end

function factions.transfer(object, new_owner)
    object.faction = new_owner.faction
end

return table.freeze(factions)
