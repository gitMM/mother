-- PIECE_CONTROLLER (body, BLOCKFALL): the player's hands on the well. It deals tetrominoes from a
-- 7-bag (shuffled with rndi) as falling_pieces at the top of its well and steers the current one:
-- left/right move (held: again after 12 ticks, then every 4), down soft-drops (a cell every 2
-- ticks), up hard-drops, A rotates clockwise and B anticlockwise, with a 1-cell wall kick either
-- way. Pieces lock 30 ticks after coming to rest. When a piece has landed it clears the well's
-- full rows (only full rows collapse; holes stay), then deals the next. Pieces fall a cell per
-- 48 ticks, 4 ticks faster every 10 lines, down to 8. When a new piece can't fit, it dies (the
-- death event is the cart's top-out). It draws the next piece at its own position. It stays put:
-- gravity down for its pieces, and no avatar of the cart's (it reads the pad itself).
local axes = use("core/axes")
local S = use("core/state")
local units = use("core/units")
local entity = use("core/entity")
local act = use("core/act")
local damage = use("core/damage")
local spawn = use("core/spawn")
local collision = use("core/collision")
local tetromino = use("core/tetromino")

local T = 8
local REPEAT_AFTER, REPEAT_EVERY, SOFT_EVERY = 12, 4, 2

local function deal(m)
    if #m.bag == 0 then
        local bag = { 1, 2, 3, 4, 5, 6, 7 }
        for i = 7, 2, -1 do
            local j = rndi(1, i)
            bag[i], bag[j] = bag[j], bag[i]
        end
        m.bag = bag
    end
    return table.remove(m.bag)
end

local function fall_ticks(p, lines)
    return math.max(p.min_fall, p.fall - p.speedup * (lines // p.every))
end

local function critical(x, y)
    local t = mget(x, y, 0)
    return t ~= 0 and fget(t, units.flag_critical)
end

-- Clears the well's full rows, bottom up, moving everything above down a row. Returns how many.
-- Critical tiles are never moved or overwritten, so a row with one is never full.
local function clear_lines(p)
    local cleared = 0
    local x0, x1, top = p.well_x, p.well_x + p.well_w - 1, p.well_y
    local row = p.well_y + p.well_h - 1
    while row >= top do
        local full = true
        for x = x0, x1 do
            full = full and collision.solid_tile(x, row) and not critical(x, row)
        end
        if full then
            cleared += 1
            for r = row, top, -1 do
                for x = x0, x1 do
                    if not critical(x, r) then
                        local above = if r > top and not critical(x, r - 1) then mget(x, r - 1, 0) else 0
                        mset(x, r, above, 0)
                    end
                end
            end
        else
            row -= 1
        end
    end
    return cleared
end

-- Deals the next piece, or tops out when it can't fit.
local function next_piece(e)
    local p, m = e.params, e.mem
    local shape = m.next
    m.next = deal(m)
    local col, row = tetromino.spawn_col(shape, p.well_x, p.well_w), p.well_y
    if not tetromino.fits(shape, 0, col, row) then
        m.piece = 0
        damage.kill(e, nil)
        return
    end
    local params = { shape = shape, fall = fall_ticks(p, m.lines), lock = p.lock, tiles = p.tiles }
    m.piece = spawn.child(e, "falling_piece", params, col * T, row * T, e.faction).id
end

local function try(piece, rot, col, row)
    local pm = piece.mem
    if tetromino.fits(pm.shape, rot, col, row) then
        tetromino.place(piece, rot, col, row)
        return true
    end
    return false
end

local function rotate(e, piece, turn)
    local pm = piece.mem
    local rot = (pm.rot + turn) % 4
    for _, kick in { 0, -1, 1 } do
        if try(piece, rot, pm.col + kick, pm.row) then
            act.sound(e, e.params.sfx_rotate)
            return
        end
    end
end

-- Left/right, with auto-repeat while held.
local function shift(e, piece)
    local m = e.mem
    local dx = (if btn("right") then 1 else 0) - (if btn("left") then 1 else 0)
    if dx == 0 or dx ~= m.held then
        m.held, m.repeat_t = dx, 0
        if dx == 0 then
            return
        end
    else
        m.repeat_t += 1
        if m.repeat_t < REPEAT_AFTER or (m.repeat_t - REPEAT_AFTER) % REPEAT_EVERY ~= 0 then
            return
        end
    end
    if try(piece, piece.mem.rot, piece.mem.col + dx, piece.mem.row) then
        act.sound(e, e.params.sfx_move)
    end
end

return {
    category = "body",
    layer = "air",
    size = "L",
    w = 32,
    h = 32,
    kinematic = true, -- it never moves; its pieces fall
    static = true,
    damage = "none",
    hp = 0,
    tier = 0,
    requires = {
        gravity = { "down" },
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = { "none" },
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        well_x = 15, -- the well, in tiles
        well_y = 1,
        well_w = 10,
        well_h = 20,
        fall = 48,
        speedup = 4,
        every = 10,
        min_fall = 8,
        lock = 30,
        tiles = 0, -- the pieces' tiles (see falling_piece)
        sfx_move = -1,
        sfx_rotate = -1,
        sfx_drop = -1,
        sfx_clear = -1,
    },
    sprites = { tiles = { w = 1, h = 1, frames = 7 } },
    sounds = { "sfx_move", "sfx_rotate", "sfx_drop", "sfx_clear" },
    public = { "lines", "piece" },

    init = function(e)
        local m = e.mem
        m.bag, m.lines, m.piece, m.held, m.repeat_t, m.soft = {}, 0, 0, 0, 0, 0
        m.next = deal(m)
    end,

    update = function(e)
        local m, p = e.mem, e.params
        local piece = S.by_id[m.piece]
        if not entity.live(piece) then
            if m.piece ~= 0 then
                local n = clear_lines(p)
                m.lines += n
                act.sound(e, if n > 0 then p.sfx_clear else p.sfx_drop)
            end
            next_piece(e)
            return
        end
        local pm = piece.mem
        if btnp("up") then
            while try(piece, pm.rot, pm.col, pm.row + 1) do
            end
            pm.drop = true
            return
        end
        if btnp("a") then
            rotate(e, piece, 1)
        elseif btnp("b") then
            rotate(e, piece, 3)
        end
        shift(e, piece)
        if btn("down") then
            m.soft += 1
            if m.soft % SOFT_EVERY == 0 and try(piece, pm.rot, pm.col, pm.row + 1) then
                pm.timer = 0
            end
        else
            m.soft = 0
        end
    end,

    -- The next piece, centred on the controller.
    draw = function(e)
        local m = e.mem
        local f = tetromino.form(m.next, 0)
        local tile = if e.params.tiles == 0 then 0 else e.params.tiles + m.next - 1
        local x = e.x - (f.x0 + f.x1 + 1) * T / 2
        local y = e.y - (f.y0 + f.y1 + 1) * T / 2
        tetromino.draw(m.next, 0, x, y, tile, 12)
    end,
}
