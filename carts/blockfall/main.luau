-- BLOCKFALL: tetrominoes fall into a 10x20 well. Left/right move, down soft-drops, up hard-drops,
-- A and B rotate. Full rows clear and every 10 lines the pieces fall faster. Survive two minutes
-- to win; a piece with no room to appear loses.

-- The well and the piece tiles, as in the map; sounds as in audio.trk.
local CONTROLLER = { well_x = 15, well_y = 1, well_w = 10, well_h = 20, tiles = 1,
    sfx_move = 0, sfx_rotate = 1, sfx_drop = 2, sfx_clear = 3 }
local PREVIEW_X, PREVIEW_Y = 240, 48 -- the next-piece box's centre
local SURVIVE = 7200
local RESULT_TICKS = 240
local SFX = { start = 4, won = 5, lost = 6 }

local state = "title" -- title, play, won or lost
local controller = nil
local played = 0 -- ticks since the first piece
local lines = 0
local ending = 0

local function finish(result)
    state, ending = result, RESULT_TICKS
    music(-1, 30)
    sfx(if result == "won" then SFX.won else SFX.lost, 5)
    local c = world:get(controller)
    if c ~= nil then
        world:remove(c.piece)
        world:remove(controller)
    end
end

function cart.init()
    world:on("objective", function()
        if state == "play" then
            finish("won")
        end
    end)
    world:on("death", function(id)
        if id == controller and state == "play" then
            finish("lost")
        end
    end)
end

function cart.update()
    if state == "title" then
        if btnp("a") or btnp("start") then
            state = "play"
            controller = world:spawn("piece_controller", CONTROLLER, PREVIEW_X, PREVIEW_Y, "player")
            sfx(SFX.start, 5)
            music(0, 60)
        end
    elseif state == "play" then
        played += 1
        local c = world:get(controller)
        if c ~= nil then
            lines = c.lines
        end
    else
        ending -= 1
        if ending == 0 then
            mother.finish(if state == "won" then "win" else "lose")
        end
    end
end

local function centred(text, y, c)
    print(text, 160 - #text * 2, y, c)
end

local function panel(y, h)
    rectfill(64, y, 192, h, 1)
    rect(64, y, 192, h, 39)
end

local function hud()
    print("NEXT", 232, 8, 37)
    local left = math.max(SURVIVE - played, 0)
    local secs = (left + 59) // 60
    print("TIME", 216, 92, 37)
    print(string.format("%d:%02d", secs // 60, secs % 60), 216, 100, if secs <= 10 then 8 else 22)
    print("LINES", 216, 116, 37)
    print(tostring(lines), 216, 124, 22)
    print("LEVEL", 216, 140, 37)
    print(tostring(lines // 10 + 1), 216, 148, 22)
    print("A/B ROTATE", 24, 120, 40)
    print("UP DROP", 24, 130, 40)
    print("DOWN FAST", 24, 140, 40)
end

function cart.draw()
    cls(0)
    map(0, 0, 0, 0, 40, 22, 1)
    map(0, 0, 0, 0, 40, 22, 0)
    world:draw()
    if state == "title" then
        panel(48, 76)
        centred("B L O C K F A L L", 58, 20)
        centred("STACK THEM FOR TWO MINUTES", 74, 37)
        centred("A/B ROTATE  UP DROP  DOWN FAST", 90, 39)
        if (tick() // 30) % 2 == 0 then
            centred("PRESS A TO START", 108, 22)
        end
        return
    end
    hud()
    if state ~= "play" then
        panel(60, 44)
        centred(if state == "won" then "YOU SURVIVED" else "TOPPED OUT", 72, if state == "won" then 20 else 4)
        centred(`LINES CLEARED: {lines}`, 86, 37)
    end
end

function cart.hash()
    local s = if state == "title" then 0 elseif state == "play" then 1 elseif state == "won" then 2 else 3
    return s + lines * 10 + played * 10000 + ending * 100000000
end
