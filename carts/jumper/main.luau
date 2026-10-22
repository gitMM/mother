-- JUMPER: run right through one long level to the flagpole. A jumps (hold it to go higher), stomp
-- walkers and turtles from above; a stomped turtle leaves a shell that slides away and, after a
-- moment, hurts everyone. ?-blocks hold fire flowers: HP 2, and B throws fireballs. Falling in a
-- pit, a hit at HP 1, or the 300-second clock running out loses.

-- The same params as cart.toml's [[exports]], so native and imported ones look and sound alike.
local WALKER = { sprite = 9, sfx_die = 10 }
local TURTLE = { sprite = 13, shell_sprite = 17, sfx_die = 10, sfx_kick = 9 }
local FLOWER = { hp = 2, sprite = 21, fireball_sprite = 23, sfx_fire = 3, sfx_take = 4, button = "b", loose = true }

-- The spawn points, left to right: who stands there, and where (cart.toml).
local ENEMIES = { "walker", "turtle", "walker", "walker", "walker", "walker", "turtle", "walker", "walker",
    "turtle", "walker", "walker", "turtle", "walker", "turtle", "walker", "turtle", "walker" }
local POINT_X = { 212, 308, 324, 348, 388, 508, 564, 612, 660, 740, 764, 924, 1028, 1124, 1220, 1268, 1324, 1388 }
local WAKE = 40 -- px beyond the screen's right edge at which an enemy appears

local QBLOCK, USED = 131, 132
local LEVEL_W, LEVEL_H = 200, 22
local TIME_LIMIT = 300 * 60
local RESULT_TICKS = 240
local SFX = { flower = 11, hurt = 5, start = 6, won = 7, lost = 8 }
local HEART, FLOWER_ICON = 25, 26
local SKY = 51

local runner = world:avatar()
local state = "title" -- title, play, won or lost
local left = TIME_LIMIT
local released = 0 -- spawn points released so far, left to right
local ending = 0
local blocks = {} -- the ?-blocks still unopened, as { x, y } tiles

local function finish(result)
    state, ending = result, RESULT_TICKS
    music(-1, 30)
    sfx(if result == "won" then SFX.won else SFX.lost, 5)
end

-- Enemies appear as the camera reaches them.
local function wake_enemies()
    local cx = world:camera()
    while released < #POINT_X and POINT_X[released + 1] < cx + 320 + WAKE do
        released += 1
        local kind = ENEMIES[released]
        world:spawner({ archetype = kind, faction = "host", points = { released },
            params = if kind == "walker" then WALKER else TURTLE })
    end
end

-- A ?-block that was hit has broken (MOTHERLIB empties damageable tiles): it becomes a used block,
-- and a fire flower drops out of it onto whoever hit it (a jump can't reach the block's top).
local function open_blocks()
    local kept = {}
    for _, b in blocks do
        if mget(b[1], b[2], 0) == 0 then
            mset(b[1], b[2], USED, 0)
            world:spawn("fire_flower", FLOWER, b[1] * 8 + 4, b[2] * 8 + 15, "player")
            sfx(SFX.flower, 6)
        else
            table.insert(kept, b)
        end
    end
    blocks = kept
end

function cart.init()
    for y = 0, LEVEL_H - 1 do
        for x = 0, LEVEL_W - 1 do
            if mget(x, y, 0) == QBLOCK then
                table.insert(blocks, { x, y })
            end
        end
    end
    world:on("objective", function()
        if state == "play" then
            finish("won")
        end
    end)
    world:on("hit", function(id)
        if id == runner and state == "play" then
            sfx(SFX.hurt, 6)
        end
    end)
    world:on("death", function(id)
        if id == runner and state == "play" then
            finish("lost")
        end
    end)
end

function cart.update()
    if state == "title" then
        world:status(runner, "stun", 2) -- holds still behind the title
        if btnp("a") or btnp("start") then
            state = "play"
            sfx(SFX.start, 5)
            music(0, 30)
        end
    elseif state == "play" then
        left -= 1
        if left == 0 then
            finish("lost")
            return
        end
        wake_enemies()
        open_blocks()
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
    local r = world:get(runner)
    local hp = if r then r.hp else 0
    for i = 1, hp do
        spr(HEART, 2 + (i - 1) * 9, 2)
    end
    if hp >= 2 then
        spr(FLOWER_ICON, 22, 2)
    end
    local secs = (left + 59) // 60
    local text = `TIME {secs}`
    print(text, 318 - #text * 4, 3, if secs <= 30 then 4 else 22)
end

function cart.draw()
    cls(SKY)
    local cx, cy = world:camera()
    camera(cx, cy)
    map(0, 0, 0, 0, LEVEL_W, LEVEL_H, 1)
    map(0, 0, 0, 0, LEVEL_W, LEVEL_H, 0)
    world:draw()
    camera(0, 0)
    if state == "title" then
        panel(48, 76)
        centred("J U M P E R", 58, 7)
        centred("RUN FOR THE FLAG BEFORE TIME RUNS OUT", 74, 37)
        centred("A JUMP   B FIREBALL (WITH A FLOWER)", 90, 39)
        if (tick() // 30) % 2 == 0 then
            centred("PRESS A TO START", 108, 22)
        end
        return
    end
    hud()
    if state ~= "play" then
        panel(60, 44)
        centred(if state == "won" then "COURSE CLEAR" else "GAME OVER", 72, if state == "won" then 7 else 4)
        centred(`TIME LEFT: {(left + 59) // 60}`, 86, 37)
    end
end

function cart.hash()
    local s = if state == "title" then 0 elseif state == "play" then 1 elseif state == "won" then 2 else 3
    return s + released * 10 + #blocks * 1000 + left * 100000 + ending * 1e10
end
