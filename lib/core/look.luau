-- Drawing for archetypes. Sprites come from params named in the archetype's `sprites` (see
-- core/archetypes.luau): the first tile of a block, with frames side by side. A sprite param of 0
-- draws a box in the faction's colour instead, so every entity shows up.
local look = {}

local BOX = { player = 12, host = 8, wild = 27 }

-- Draws frame `frame` of the block in param `name`, centred on (x, y).
function look.sprite_at(e, name, frame, x, y, flip_x, flip_y)
    local n = e.params[name]
    if n == 0 then
        rect(e.x - e.hw, e.y - e.hh, e.hw * 2, e.hh * 2, BOX[e.faction])
        return
    end
    local block = e.arch.sprites[name]
    spr(n + frame * block.w, x - block.w * 4, y - block.h * 4, block.w, block.h, flip_x, flip_y)
end

-- Draws it centred on the entity.
function look.sprite(e, name, frame, flip_x, flip_y)
    look.sprite_at(e, name, frame, e.x, e.y, flip_x, flip_y)
end

-- A body that has just been hurt blinks while it can't be hurt again.
function look.blinking(e)
    return e.hurt > 0 and (tick() // 4) % 2 == 1
end

return table.freeze(look)
