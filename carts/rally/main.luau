-- RALLY: three laps against three rival cars, who ram anyone near them. A accelerates, B brakes,
-- left/right steer; L fires the nitro (3 charges), R drops oil behind you (3 charges). Finish first
-- to win; a rival finishing first loses.

-- The same params as cart.toml's [[exports]] (each rival in its own colour).
local RIVALS = { { sprite = 11, sfx_ram = 0 }, { sprite = 21, sfx_ram = 0 }, { sprite = 65, sfx_ram = 0 } }
local OIL = { sprite = 75, sfx_slip = 3 }
local NITRO = { sprite = 77, sfx_boost = 4, button = "l", charges = 3 }
local OILS = 3
local LAPS = 3
local COUNTDOWN = 180
local RESULT_TICKS = 240
local SFX = { spin = 1, oil = 2, lap = 5, count = 6, go = 7, won = 8, lost = 9 }
local NITRO_ICON, OIL_ICON, FLAG_ICON = 79, 80, 81
local MAP_W, MAP_H = 120, 80

local car = world:avatar()
local state = "title" -- title, countdown, race, won or lost
local count = 0 -- countdown ticks left
local nitro = nil
local oils = OILS
local laps = 0
local ending = 0
local raced = 0 -- ticks since the start

local function finish(result)
    state, ending = result, RESULT_TICKS
    music(-1, 30)
    sfx(if result == "won" then SFX.won else SFX.lost, 5)
end

local function go()
    state = "race"
    for i, params in RIVALS do
        world:spawner({ archetype = "rival_ai", faction = "host", params = params, points = { i } })
    end
    nitro = world:spawn("nitro", NITRO, 0, 0, "player")
    sfx(SFX.go, 5)
    music(0, 30)
end

function cart.init()
    world:on("objective", function(_, winner)
        if state == "race" then
            finish(if winner == car then "won" else "lost")
        end
    end)
    world:on("hit", function(id)
        if id == car then
            sfx(SFX.spin, 6)
        end
    end)
end

function cart.update()
    if state == "title" or state == "countdown" then
        world:status(car, "slip", 2) -- held on the grid: it keeps its (zero) velocity
    end
    if state == "title" then
        if btnp("a") or btnp("start") then
            state, count = "countdown", COUNTDOWN
        end
    elseif state == "countdown" then
        if count % 60 == 0 then
            sfx(SFX.count, 5)
        end
        count -= 1
        if count == 0 then
            go()
        end
    elseif state == "race" then
        raced += 1
        local c = world:get(car)
        if c.laps > laps then
            laps = c.laps
            if laps < LAPS then
                sfx(SFX.lap, 5)
            end
        end
        if oils > 0 and btnp("r") then
            oils -= 1
            world:spawn("oil", OIL, c.x - m.cos(c.angle) * 14, c.y - m.sin(c.angle) * 14, "player")
            sfx(SFX.oil, 6)
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
    rectfill(0, 0, 320, 9, 1)
    spr(FLAG_ICON, 2, 1)
    print(`LAP {math.min(laps + 1, LAPS)}/{LAPS}`, 12, 2, 22)
    local n = world:get(nitro)
    for i = 1, (if n then n.charges else 0) do
        spr(NITRO_ICON, 230 + i * 9, 1)
    end
    for i = 1, oils do
        spr(OIL_ICON, 280 + i * 9, 1)
    end
    local secs = raced // 60
    centred(string.format("%d:%02d", secs // 60, secs % 60), 2, 37)
end

function cart.draw()
    cls(14) -- between the trees
    camera(world:camera())
    map(0, 0, 0, 0, MAP_W, MAP_H)
    world:draw()
    camera(0, 0)
    if state == "title" then
        panel(48, 76)
        centred("R A L L Y", 58, 4)
        centred("THREE LAPS. BEAT THE OTHER THREE.", 74, 37)
        centred("A GAS  B BRAKE  L NITRO  R OIL", 90, 39)
        if (tick() // 30) % 2 == 0 then
            centred("PRESS A TO START", 108, 22)
        end
        return
    end
    hud()
    if state == "countdown" then
        centred(tostring((count + 59) // 60), 70, 8)
    elseif state == "race" and raced < 60 then
        centred("GO!", 70, 12)
    elseif state == "won" or state == "lost" then
        panel(60, 44)
        centred(if state == "won" then "FIRST PLACE!" else "BEATEN", 72, if state == "won" then 8 else 4)
        centred(string.format("TIME %d:%02d", raced // 3600, raced // 60 % 60), 86, 37)
    end
end

function cart.hash()
    local s = ({ title = 0, countdown = 1, race = 2, won = 3, lost = 4 })[state]
    return s + count * 10 + oils * 10000 + laps * 100000 + raced * 1e6 + ending * 1e12
end
