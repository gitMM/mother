-- BREAKOUT: 6 rows of 18 bricks. Keep the ball in play with the paddle: where it hits the paddle
-- sets the angle it leaves at. One brick in ten drops a laser (15 s, fires on A). Clear every
-- brick to win; lose 3 balls through the bottom and the game is over.

-- The same params as cart.toml's [[exports]], so native and imported ones look and sound alike.
local BALL = { sprite = 5, sfx_bounce = 2 }
local LASER = { sprite = 44, shot_sprite = 46, sfx_fire = 4, sfx_take = 5, loose = true, ticks = 900, button = "a" }

local ROWS, COLS = 6, 18
local LIVES = 3
local DROP_CHANCE = 0.1
local RESULT_TICKS = 240
local SFX = { lost = 3, won = 6, over = 7, start = 8 }

local state = "title" -- title, play, won or lost
local lives, score, ending = LIVES, 0, 0
local balls = {} -- ids of every ball, waiting ones included

local function lay_bricks()
    for r = 0, ROWS - 1 do
        for c = 0, COLS - 1 do
            local drops = rnd() < DROP_CHANCE
            world:spawn("brick", {
                sprite = 32 + 2 * r,
                sfx_break = 1,
                drop = if drops then "laser" else "",
                drop_params = if drops then LASER else {},
            }, 24 + 16 * c, 44 + 8 * r, "host")
        end
    end
end

local function finish(result)
    state, ending = result, RESULT_TICKS
    for _, id in balls do
        world:remove(id)
    end
    music(-1, 30)
    sfx(if result == "won" then SFX.won else SFX.over, 5)
end

function cart.init()
    world:on("spawn", function(id)
        local e = world:get(id)
        if e ~= nil and e.archetype == "ball" then
            table.insert(balls, id) -- re-served balls too, so the result screen can clear them
        end
    end)
    world:on("death", function(id, view)
        if view.archetype == "brick" then
            score += 10 * (ROWS - math.floor((view.y - 44) / 8))
        elseif view.archetype == "ball" and state == "play" then
            lives -= 1
            sfx(SFX.lost, 5)
            if lives == 0 then
                finish("lost")
            end
        end
    end)
    world:on("objective", function()
        if state == "play" then
            finish("won")
        end
    end)
end

function cart.update()
    if state == "title" then
        if btnp("a") or btnp("start") then
            state = "play"
            lay_bricks()
            world:spawn("ball", { sprite = BALL.sprite, sfx_bounce = BALL.sfx_bounce, wait = 60 }, 160, 150, "player")
            music(0, 60)
            sfx(SFX.start, 5)
        end
    elseif state ~= "play" then
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
    rectfill(72, y, 176, h, 17)
    rect(72, y, 176, h, 19)
end

function cart.draw()
    cls(0)
    map(0, 0, 0, 0, 40, 22)
    world:draw()
    print(`SCORE {score}`, 12, 5, 22)
    print("BALLS", 250, 5, 39)
    for i = 1, lives do
        spr(BALL.sprite, 268 + i * 9, 3)
    end
    if state == "title" then
        panel(72, 64)
        centred("B R E A K O U T", 82, 19)
        centred("CLEAR EVERY BRICK", 98, 37)
        centred("ONE IN TEN HIDES A LASER", 108, 37)
        if (tick() // 30) % 2 == 0 then
            centred("PRESS A TO START", 122, 22)
        end
    elseif state ~= "play" then
        panel(100, 40)
        centred(if state == "won" then "ALL CLEAR!" else "GAME OVER", 110, if state == "won" then 9 else 4)
        centred(`SCORE {score}`, 124, 37)
    end
end

function cart.hash()
    local s = if state == "title" then 0 elseif state == "play" then 1 elseif state == "won" then 2 else 3
    return s + lives * 10 + score * 100 + ending * 10000000
end
