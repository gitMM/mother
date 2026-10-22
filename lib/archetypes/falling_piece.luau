-- FALLING_PIECE (object, BLOCKFALL): a tetromino that steps down the tile grid one cell every
-- `fall` ticks. Once it has rested on something for `lock` ticks (or at once after a hard drop)
-- it lands: its cells become layer-0 map tiles, from `tiles` (the first of 7 tiles, one per
-- shape, which the cart flags solid and damageable), and the piece is gone. It moves itself along
-- the grid, so it is static: statuses never push it off the grid, and stun or knockback pause it.
-- A piece_controller steers it; on its own it just falls. Needs gravity down.
-- An imported piece's tiles are its art in sheet 3 (MOTHERLIB flags them solid and damageable),
-- and they decay: the piece stays, out of play, as an imported solid, and when it decays
-- (core/imports.luau) it puts back what its cells held before, unless something broke them.
local axes = use("core/axes")
local S = use("core/state")
local units = use("core/units")
local entity = use("core/entity")
local spawn = use("core/spawn")
local tetromino = use("core/tetromino")

local T = 8

-- Lands e: its cells become tiles, except where a critical tile is.
local function land(e)
    local m = e.mem
    local tile = if e.params.tiles == 0 then 0 else e.params.tiles + m.shape - 1
    if e.imported and tile >= 3072 then
        fset(tile, units.flag_solid, true)
        fset(tile, units.flag_damageable, true)
    end
    m.cells = {}
    for _, c in tetromino.form(m.shape, m.rot).cells do
        local tx, ty = m.col + c[1], m.row + c[2]
        if tetromino.free(tx, ty) then
            table.insert(m.cells, { tx, ty, mget(tx, ty, 0) })
            mset(tx, ty, tile, 0)
        end
    end
    if e.imported then
        m.landed, m.tile = true, tile
        e.ghost, e.solid, e.decay = true, true, S.tick + spawn.DECAY
    else
        entity.remove(e, "remove")
    end
end

return {
    category = "object",
    layer = "object",
    size = "L",
    w = 24,
    h = 16,
    kinematic = true, -- it steps down the grid itself; world gravity never moves it
    static = true,
    damage = "none",
    hp = 0,
    tier = 0,
    requires = {
        gravity = { "down" },
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = axes.ANY,
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        shape = 1, -- 1-7: I O T S Z J L
        rot = 0,
        fall = 48, -- ticks per cell
        lock = 0, -- ticks resting before it lands
        tiles = 0, -- 0: drawn as boxes, and lands as nothing
    },
    sprites = { tiles = { w = 1, h = 1, frames = 7 } },
    public = { "shape", "rot", "col", "row" },

    -- (x, y) is anywhere in the box's top-left cell.
    init = function(e)
        local p, m = e.params, e.mem
        if not tetromino.valid(p.shape) then
            error(`falling_piece: shape must be 1-7, got {p.shape}`, 0)
        end
        m.shape, m.timer, m.rest, m.drop = p.shape, 0, 0, false
        tetromino.place(e, p.rot, math.floor(e.x / T), math.floor(e.y / T))
    end,

    update = function(e)
        local m, p = e.mem, e.params
        if m.landed then
            return
        end
        m.timer += 1
        if m.drop then
            land(e)
        elseif tetromino.fits(m.shape, m.rot, m.col, m.row + 1) then
            m.rest = 0
            if m.timer >= p.fall then
                m.timer = 0
                tetromino.place(e, m.rot, m.col, m.row + 1)
            end
        else
            m.rest += 1
            if m.rest >= p.lock then
                land(e)
            end
        end
    end,

    on_leave = function(e)
        local m = e.mem
        if m.landed then
            for _, c in m.cells do
                if mget(c[1], c[2], 0) == m.tile then
                    mset(c[1], c[2], c[3], 0)
                end
            end
        end
    end,

    draw = function(e)
        local m = e.mem
        if m.landed then
            return
        end
        local tile = if e.params.tiles == 0 then 0 else e.params.tiles + m.shape - 1
        tetromino.draw(m.shape, m.rot, m.col * T, m.row * T, tile, 12)
    end,
}
