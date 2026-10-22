-- KNIGHTFALL: three waves of skeletons (4, 6, then 8) pour in from the arena's edges. A swings the
-- sword, B dodges, R drops a bomb (3 per wave). Clear all three waves to win; HP 0 loses.

-- The same params as cart.toml's [[exports]], so native and imported ones look and sound alike.
local SKELETON = { sprite = 8, sfx_swing = 0, sfx_die = 2 }
local BOMB = { sprite = 18, sprite_blast = 20, sfx_blast = 3 }
local SWORD = { tier = 3, arc = 90, reach = 12, sprite = 14, sprite_v = 16, sfx_swing = 0, button = "a" }

local WAVES = { 4, 6, 8 }
local BOMBS_PER_WAVE = 3
local RESULT_TICKS = 240
local SFX = { hit = 1, wave = 5, won = 6, lost = 7, bomb = 8, hurt = 9 }
local HEART, BOMB_ICON = 136, 137

local knight = world:avatar()
local state = "title" -- title, play, won or lost
local wave, bombs, kills = 0, 0, 0
local banner = 0 -- ticks left showing "WAVE n"
local ending = 0 -- ticks left on the result screen

local function start_wave()
    wave += 1
    bombs = BOMBS_PER_WAVE
    banner = 90
    world:spawner({ archetype = "skeleton", faction = "host", count = WAVES[wave], every = 40, params = SKELETON })
    sfx(SFX.wave, 5)
end

local function finish(result)
    state, ending = result, RESULT_TICKS
    music(-1, 30)
    sfx(if result == "won" then SFX.won else SFX.lost, 5)
end

function cart.init()
    world:spawn("sword", SWORD, 0, 0, "player")
    world:on("objective", function()
        if state ~= "play" then
            return
        elseif wave < #WAVES then
            start_wave()
        else
            finish("won")
        end
    end)
    world:on("hit", function(id)
        sfx(if id == knight then SFX.hurt else SFX.hit, 6)
    end)
    world:on("death", function(id, view)
        if id == knight then
            if state == "play" then
                finish("lost")
            end
        elseif view.archetype == "skeleton" then
            kills += 1
        end
    end)
end

function cart.update()
    if state == "title" then
        if btnp("a") or btnp("start") then
            state = "play"
            music(0, 60)
            start_wave()
        end
    elseif state == "play" then
        if banner > 0 then
            banner -= 1
        end
        local k = world:get(knight)
        if k ~= nil and bombs > 0 and btnp("r") then
            world:spawn("bomb", BOMB, k.x, k.y, "player")
            bombs -= 1
            sfx(SFX.bomb, 6)
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
    local k = world:get(knight)
    local hp = if k then k.hp else 0
    spr(HEART, 4, 0)
    rectfill(14, 2, 60, 4, 2)
    rectfill(14, 2, hp * 2, 4, 4)
    print(`WAVE {wave}/{#WAVES}`, 132, 1, 37)
    for i = 1, bombs do
        spr(BOMB_ICON, 230 + i * 9, 0)
    end
    print(`KILLS {kills}`, 274, 1, 37)
end

function cart.draw()
    cls(0)
    map(0, 0, 0, 0, 40, 22)
    world:draw()
    if state == "title" then
        panel(48, 76)
        centred("K N I G H T F A L L", 58, 8)
        centred("THREE WAVES OF THE RESTLESS DEAD", 74, 37)
        centred("A SWORD   B DODGE   R BOMB", 90, 39)
        if (tick() // 30) % 2 == 0 then
            centred("PRESS A TO START", 108, 22)
        end
        return
    end
    hud()
    if state == "play" and banner > 0 then
        centred(`WAVE {wave}`, 60, 8)
    elseif state ~= "play" then
        panel(60, 44)
        centred(if state == "won" then "VICTORY" else "YOU HAVE FALLEN", 72, if state == "won" then 8 else 4)
        centred(`SKELETONS SLAIN: {kills}`, 86, 37)
    end
end

function cart.hash()
    local s = if state == "title" then 0 elseif state == "play" then 1 elseif state == "won" then 2 else 3
    return s + wave * 10 + bombs * 100 + kills * 1000 + banner * 100000 + ending * 1000000
end
