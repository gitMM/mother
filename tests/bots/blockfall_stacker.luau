-- BLOCKFALL: for each new piece, tries every rotation and column from the top of the well, drops it,
-- and scores the stack it leaves (lines cleared, total height, holes, bumpiness; the usual weights);
-- then turns and steps the piece there and hard-drops it. Taps alternate with releases so each
-- press counts once.
local LEFT, TOP, W, H = 15, 1, 10, 20

local BASE = {
    { n = 4, { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } },
    { n = 2, { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },
    { n = 3, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } },
    { n = 3, { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 } },
    { n = 3, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } },
    { n = 3, { 0, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } },
    { n = 3, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } },
}
local FORMS = {} -- FORMS[shape][rot + 1] = cells
for s, base in BASE do
    FORMS[s] = {}
    local cells = { base[1], base[2], base[3], base[4] }
    for r = 1, 4 do
        FORMS[s][r] = cells
        local turned = {}
        for i, c in cells do
            turned[i] = { base.n - 1 - c[2], c[1] }
        end
        cells = turned
    end
end

local function filled(grid, x, y)
    return x < 1 or x > W or y > H or (y >= 1 and grid[y][x])
end

local function fits(grid, cells, col, row)
    for _, c in cells do
        if filled(grid, col + c[1], row + c[2]) then
            return false
        end
    end
    return true
end

local function score(grid)
    local lines = 0
    for y = 1, H do
        local full = true
        for x = 1, W do
            full = full and grid[y][x]
        end
        if full then
            lines += 1
        end
    end
    local heights, holes, total, bumps = {}, 0, 0, 0
    for x = 1, W do
        local h, seen = 0, false
        for y = 1, H do
            if grid[y][x] then
                if not seen then
                    h, seen = H - y + 1, true
                end
            elseif seen then
                holes += 1
            end
        end
        heights[x] = h
        total += h
        if x > 1 then
            bumps += math.abs(h - heights[x - 1])
        end
    end
    return -0.510066 * total + 0.760666 * lines - 0.35663 * holes - 0.184483 * bumps
end

-- The best (rot, col) for shape on grid, in well coordinates (col 1 = the well's left column).
local function plan(grid, shape)
    local best, best_rot, best_col = -math.huge, 0, 4
    for rot = 0, 3 do
        local cells = FORMS[shape][rot + 1]
        for col = -2, W do
            if fits(grid, cells, col, 1) then
                local row = 1
                while fits(grid, cells, col, row + 1) do
                    row += 1
                end
                for _, c in cells do
                    grid[row + c[2]][col + c[1]] = true
                end
                local s = score(grid)
                for _, c in cells do
                    grid[row + c[2]][col + c[1]] = false
                end
                if s > best then
                    best, best_rot, best_col = s, rot, col
                end
            end
        end
    end
    return best_rot, best_col
end

local piece_id, target_rot, target_col, since = nil, 0, 0, 0

return function(t)
    local tap = t % 2 == 0
    local ids = world:query(0, 0, 320, 180, { archetype = "piece_controller" })
    if #ids == 0 then
        return 0, 0, if t % 2 == 0 then "a" else ""
    end
    local piece = world:get(world:get(ids[1]).piece)
    if piece == nil then
        return 0, 0, ""
    end
    if piece.id ~= piece_id then
        local grid = {}
        for y = 1, H do
            grid[y] = {}
            for x = 1, W do
                grid[y][x] = mget(LEFT + x - 1, TOP + y - 1, 0) ~= 0
            end
        end
        piece_id, since = piece.id, 0
        target_rot, target_col = plan(grid, piece.shape)
        target_col += LEFT - 1
    end
    since += 1
    if not tap then
        return 0, 0, ""
    elseif piece.rot ~= target_rot and since < 60 then
        return 0, 0, "a"
    elseif piece.col < target_col and since < 60 then
        return 1, 0, ""
    elseif piece.col > target_col and since < 60 then
        return -1, 0, ""
    end
    return 0, -1, ""
end
