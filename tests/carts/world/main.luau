-- WORLD: a MOTHERLIB smoke test. Waves of host dummies spawn at the corners and bounce round the
-- arena; A fires a shot from the avatar along the stick (or upwards); wild bouncers wander and
-- hurt everyone. Clearing three waves wins; the avatar dying loses.

local wave, kills, shots = 0, 0, 0
local avatar = world:avatar()
local live_shots = {}

local CORNERS = { { 1, 0.5 }, { -1, 0.5 }, { 1, -0.5 }, { -1, -0.5 } }

local function next_wave()
    wave += 1
    for p = 1, 4 do
        world:spawner({
            archetype = "dummy",
            faction = "host",
            count = wave + 1,
            every = 30,
            points = { p },
            params = { vx = CORNERS[p][1], vy = CORNERS[p][2], bounce = true, tier = 1, hp = 3, colour = 8 },
        })
    end
end

function cart.init()
    world:on("objective", function()
        if wave == 3 then
            mother.finish("win")
        else
            next_wave()
        end
    end)
    world:on("death", function(id, view)
        if id == avatar then
            mother.finish("lose")
        elseif view.faction == "host" then
            kills += 1
        end
    end)
    for i = 1, 2 do
        world:spawn("dummy", { vx = rnd(2) - 1, vy = rnd(2) - 1, bounce = true, tier = 1, hp = 10, colour = 27 },
            80 * i + 40, 60, "wild")
    end
    next_wave()
end

function cart.update()
    local me = world:get(avatar)
    if me ~= nil and btnp("a") then
        local sx, sy = stick()
        if sx == 0 and sy == 0 then
            sy = -1
        end
        local len = m.sqrt(sx * sx + sy * sy)
        local spread = rnd(0.2) - 0.1
        local id = world:spawn("dummy", {
            w = 2, h = 2, layer = "projectile", tier = 3, damage = "none", hp = 0, colour = 10,
            vx = 4 * sx / len + spread, vy = 4 * sy / len - spread,
        }, me.x, me.y, "player")
        table.insert(live_shots, { id = id, born = tick() })
        shots += 1
    end
    -- Shots last a second.
    while #live_shots > 0 and tick() - live_shots[1].born >= 60 do
        world:remove(table.remove(live_shots, 1).id)
    end
end

function cart.draw()
    cls(1)
    world:draw()
    local me = world:get(avatar)
    print(`WAVE {wave}  KILLS {kills}  HP {if me then me.hp else 0}`, 4, 4, 7)
end

function cart.hash()
    return kills * 100000 + wave * 1000 + shots
end
