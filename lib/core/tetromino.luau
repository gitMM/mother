-- Tetrominoes (brief section 8, BLOCKFALL) for falling_piece and piece_controller: the 7 shapes
-- 1-7 = I O T S Z J L, each with 4 clockwise rotations, as cells in a box of tiles. A piece is
-- placed by the box's top-left cell (col, row) on the map; its cells are map tiles of layer 0.
local S = use("core/state")
local units = use("core/units")
local collision = use("core/collision")

local T = units.tile
local tetromino = {}

tetromino.NAMES = table.freeze({ "I", "O", "T", "S", "Z", "J", "L" })

-- Rotation 0 of each shape and the size of the square box it turns in.
local BASE = {
    { n = 4, { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } },
    { n = 2, { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },
    { n = 3, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } },
    { n = 3, { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 } },
    { n = 3, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } },
    { n = 3, { 0, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } },
    { n = 3, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } },
}

-- SHAPES[shape][rot + 1] = { cells = { {x, y} x4 }, x0, y0, x1, y1 (the cells' extent) }
local SHAPES = {}
for s, base in BASE do
    local rots = {}
    local cells = {}
    for i = 1, 4 do
        cells[i] = { base[i][1], base[i][2] }
    end
    for r = 1, 4 do
        local x0, y0, x1, y1 = 9, 9, -1, -1
        for _, c in cells do
            x0, y0 = math.min(x0, c[1]), math.min(y0, c[2])
            x1, y1 = math.max(x1, c[1]), math.max(y1, c[2])
        end
        rots[r] = table.freeze({ cells = table.freeze(cells), x0 = x0, y0 = y0, x1 = x1, y1 = y1 })
        local turned = {}
        for i, c in cells do
            turned[i] = table.freeze({ base.n - 1 - c[2], c[1] }) -- clockwise
        end
        cells = turned
    end
    SHAPES[s] = table.freeze(rots)
end

function tetromino.valid(shape)
    return SHAPES[shape] ~= nil
end

-- The shape's cells in rotation rot (0-3), with their extent.
function tetromino.form(shape, rot)
    return SHAPES[shape][rot % 4 + 1]
end

-- The column a new piece of this shape starts in, for a well whose left column is `left` and
-- that is `w` columns wide: centred, rounding left.
function tetromino.spawn_col(shape, left, w)
    return left + (w - BASE[shape].n) // 2
end

-- A cell a piece can move into: inside the world rectangle's sides and above its bottom (above
-- it is open), not solid, and not critical (MOTHERLIB never overwrites those).
function tetromino.free(tx, ty)
    if tx < 0 or tx >= S.world_w // T or ty >= S.world_h // T or collision.solid_tile(tx, ty) then
        return false
    end
    local t = mget(tx, ty, 0)
    return t == 0 or not fget(t, units.flag_critical)
end

function tetromino.fits(shape, rot, col, row)
    for _, c in tetromino.form(shape, rot).cells do
        if not tetromino.free(col + c[1], row + c[2]) then
            return false
        end
    end
    return true
end

-- Puts piece entity e at (col, row) in rotation rot: its grid state in e.mem, its collider round
-- its cells.
function tetromino.place(e, rot, col, row)
    local m = e.mem
    local f = tetromino.form(m.shape, rot)
    m.rot, m.col, m.row = rot % 4, col, row
    e.hw, e.hh = (f.x1 - f.x0 + 1) * T / 2, (f.y1 - f.y0 + 1) * T / 2
    e.x, e.y = (col + f.x0) * T + e.hw, (row + f.y0) * T + e.hh
end

-- Draws a shape with tile `tile` (0: outlined boxes in colour c) with its box's top-left at (x, y).
function tetromino.draw(shape, rot, x, y, tile, c)
    for _, cell in tetromino.form(shape, rot).cells do
        local px, py = x + cell[1] * T, y + cell[2] * T
        if tile == 0 then
            rect(px, py, T, T, c)
        else
            spr(tile, px, py)
        end
    end
end

return table.freeze(tetromino)
