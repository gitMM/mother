-- STARFIGHTER: the stars scroll by; formations of 4-8 divers swoop in from the top and dive at the
-- ship, and mines drift in with the scroll. A fires, B launches a homing missile. Survive 90
-- seconds to win; HP 0 loses.

-- The same params as cart.toml's [[exports]], so native and imported ones look and sound alike.
local DIVER = { sprite = 5, shot_sprite = 14, sfx_dive = 1, sfx_die = 2 }
local MINE = { sprite = 9, sprite_blast = 11, sfx_blast = 3 }
local HOMING = { sprite = 17, missile_sprite = 15, sfx_fire = 4, sfx_take = 9, button = "b" }

local SURVIVE = 5400
local RESULT_TICKS = 240
local POINTS = 5 -- spawn points along the top
local MAX_DIVERS = 10 -- no new formation while this many are about
local SFX = { hit = 5, start = 6, won = 7, lost = 8 }
local HEART = 19

local ship = world:avatar()
local state = "title" -- title, play, won or lost
local played, kills, ending = 0, 0, 0
local next_wave, next_mine = 0, 120

-- Stars at fixed places in three layers, from a small LCG (no rnd: they're only drawn).
local STARS = {}
do
    local seed = 7
    local function next_value(n)
        seed = (seed * 75 + 74) % 65537
        return seed % n
    end
    for i = 1, 70 do
        local layer = i % 3 + 1
        STARS[i] = { x = next_value(320), y = next_value(180), speed = layer * 0.25, c = ({ 41, 39, 22 })[layer] }
    end
end

local function finish(result)
    state, ending = result, RESULT_TICKS
    music(-1, 30)
    sfx(if result == "won" then SFX.won else SFX.lost, 5)
end

local function divers_about()
    local cx, cy = world:camera()
    return #world:query(cx - 320, cy - 180, 960, 540, { archetype = "diver" })
end

-- Formations come every 5 s at first, every 2.5 s by the end; a mine every 1.5-3 s.
local function call_in()
    if played >= next_wave then
        next_wave = played + math.max(150, 300 - played // 36)
        if divers_about() < MAX_DIVERS then
            world:spawner({ archetype = "diver", faction = "host", params = DIVER, count = rndi(4, 8), every = 10,
                points = { rndi(1, POINTS) } })
        end
    end
    if played >= next_mine then
        next_mine = played + rndi(90, 180)
        world:spawner({ archetype = "mine", faction = "host", params = MINE, points = { rndi(1, POINTS) } })
    end
end

function cart.init()
    world:on("objective", function()
        if state == "play" then
            finish("won")
        end
    end)
    world:on("hit", function(id)
        if id == ship and state == "play" then
            sfx(SFX.hit, 6)
        end
    end)
    world:on("death", function(id, view)
        if id == ship then
            if state == "play" then
                finish("lost")
            end
        elseif view.archetype == "diver" and state == "play" then
            kills += 1
        end
    end)
end

function cart.update()
    if state == "title" then
        if btnp("a") or btnp("start") then
            state = "play"
            world:spawn("homing", HOMING, 0, 0, "player")
            sfx(SFX.start, 5)
            music(0, 60)
        end
    elseif state == "play" then
        played += 1
        if world:avatar() == nil then
            finish("lost") -- pushed off the screen
        else
            call_in()
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
    local s = world:get(ship)
    for i = 1, (if s then s.hp else 0) do
        spr(HEART, 2 + (i - 1) * 9, 2)
    end
    local secs = (math.max(SURVIVE - played, 0) + 59) // 60
    centred(string.format("%d:%02d", secs // 60, secs % 60), 3, if secs <= 10 then 8 else 22)
    local text = `KILLS {kills}`
    print(text, 318 - #text * 4, 3, 37)
end

function cart.draw()
    cls(0)
    local t = tick()
    for _, star in STARS do
        pset(star.x, (star.y + t * star.speed) % 180, star.c)
    end
    camera(world:camera())
    world:draw()
    camera(0, 0)
    if state == "title" then
        panel(48, 76)
        centred("S T A R F I G H T E R", 58, 19)
        centred("HOLD THE LINE FOR NINETY SECONDS", 74, 37)
        centred("A FIRE   B HOMING MISSILE", 90, 39)
        if (t // 30) % 2 == 0 then
            centred("PRESS A TO START", 108, 22)
        end
        return
    end
    hud()
    if state ~= "play" then
        panel(60, 44)
        centred(if state == "won" then "SECTOR CLEAR" else "SHOT DOWN", 72, if state == "won" then 19 else 4)
        centred(`DIVERS DOWNED: {kills}`, 86, 37)
    end
end

function cart.hash()
    local s = if state == "title" then 0 elseif state == "play" then 1 elseif state == "won" then 2 else 3
    return s + kills * 10 + played * 10000 + ending * 100000000 + next_wave * 1e12 + next_mine * 1e16
end
