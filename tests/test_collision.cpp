#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// M5 acceptance: no tunnelling at 4 px/tick. A 2x2 projectile at 4 px/tick against a 2x2
// target, from 16 sub-pixel starting offsets: at some offsets the projectile's positions at
// whole ticks never overlap the target, so only the sweep can find the hit.
TEST(collision_no_tunnelling_through_entities) {
    CHECK(wins(run_world("tunnel_entities", kTestToml, R"lua(
local deaths = 0
run(function()
    world:on("death", function() deaths += 1 end)
    local shot = { w = 2, h = 2, layer = "projectile", tier = 1, vx = 4 }
    for k = 0, 15 do
        local y = 6 + k * 5
        -- against a target standing still, and against one coming the other way at 4 px/tick
        world:spawn("dummy", shot, 20 + k * 0.25, y, "player")
        world:spawn("dummy", { w = 2, h = 2 }, 60, y, "host")
        world:spawn("dummy", shot, 20 + k * 0.25, y + 85, "player")
        world:spawn("dummy", { w = 2, h = 2, vx = -4 }, 100, y + 85, "host")
    end
end, {
    [30] = function()
        eq(deaths, 32, "targets hit")
    end,
})
)lua")));
}

// Diagonal shots at (4, 4) px/tick that only graze their targets between two ticks.
TEST(collision_no_tunnelling_on_diagonals) {
    CHECK(wins(run_world("tunnel_diagonal", kTestToml, R"lua(
local deaths = 0
run(function()
    world:on("death", function() deaths += 1 end)
    for k = 0, 7 do
        local x = 10 + 30 * k
        world:spawn("dummy", { w = 2, h = 2, layer = "projectile", tier = 1, vx = 4, vy = 4 }, x, 10, "player")
        world:spawn("dummy", { w = 2, h = 2 }, x + 40.25 + 0.5 * k, 52, "host")
    end
end, {
    [30] = function()
        eq(deaths, 8, "targets hit")
    end,
})
)lua")));
}

namespace {

// 40x22 tiles: a solid wall down column 20 (x 160-168).
std::string wall_map() {
    return solid_map(40, 22, [](int x, int) { return x == 20 ? 1 : 0; });
}

// 40x22 tiles: a lone solid tile at (10, 10) (80-88 px) and a solid floor at row 21 (y 168).
std::string pillar_map() {
    return solid_map(40, 22, [](int x, int y) { return (x == 10 && y == 10) || y == 21 ? 1 : 0; });
}

}  // namespace

TEST(collision_no_tunnelling_through_tiles) {
    CHECK(wins(run_world("tunnel_tiles", kTestToml, R"lua(
local right, left = {}, {}
run(function()
    for k = 0, 15 do
        right[k] = world:spawn("dummy", { w = 2, h = 2, vx = 4 }, 120 + k * 0.25, 4 + k * 10, "host")
        left[k] = world:spawn("dummy", { w = 2, h = 2, vx = -4 }, 200 + k * 0.25, 4 + k * 10, "host")
    end
end, {
    [20] = function()
        for k = 0, 15 do
            local r, l = world:get(right[k]), world:get(left[k])
            eq(r.x, 159, `body {k} moving right stops at the wall`)
            eq(r.vx, 0, `body {k} moving right is stopped`)
            eq(l.x, 169, `body {k} moving left stops at the wall`)
        end
    end,
})
)lua", 600, {{"map.map", wall_map()}})));
}

// Diagonal movers at a lone tile's corner never overlap it; they slide along its faces and
// stop on the floor or at the right edge.
TEST(collision_slides_round_tile_corners) {
    CHECK(wins(run_world("corners", kTestToml, R"lua(
local ids = {}
local function overlaps(v, x0, y0, x1, y1)
    return v.x - 1 < x1 and x0 < v.x + 1 and v.y - 1 < y1 and y0 < v.y + 1
end
run(function()
    for k = 0, 15 do
        ids[k] = world:spawn("dummy", { w = 2, h = 2, vx = 4, vy = 4 }, 46 + k * 0.5, 50, "host")
    end
end, setmetatable({
    [150] = function()
        for k = 0, 15 do
            local v = world:get(ids[k])
            eq(v.vx + v.vy, 0, `body {k} comes to rest`)
            check(v.y == 167 or v.x == 319, `body {k} rests on the floor or at the edge`)
        end
    end,
}, {
    __index = function(_, t)
        if t < 150 then
            return function()
                for k = 0, 15 do
                    local v = world:get(ids[k])
                    check(not overlaps(v, 80, 80, 88, 88), `body {k} entered the tile at tick {t}`)
                    check(v.y + 1 <= 168, `body {k} entered the floor at tick {t}`)
                end
            end
        end
        return nil
    end,
}))
)lua", 600, {{"map.map", pillar_map()}})));
}

TEST(collision_solid_entities_block_and_bumps_are_contacts) {
    CHECK(wins(run_world("solids", kTestToml, R"lua(
local hits = 0
local air
run(function()
    world:on("hit", function() hits += 1 end)
    for k = 0, 15 do
        local y = 6 + k * 10
        world:spawn("dummy", { w = 2, h = 8, solid = true, damage = "stun" }, 100, y, "host")
        world:spawn("dummy", { w = 4, h = 4, tier = 1, vx = 4 }, 50 + k * 0.25, y, "player")
    end
    air = world:spawn("dummy", { w = 4, h = 4, layer = "air", vx = 4 }, 50, 170, "player")
    world:spawn("dummy", { w = 2, h = 8, solid = true }, 100, 170, "host")
end, {
    [30] = function()
        for k = 0, 15 do
            eq(world:get(2 + 2 * k).x, 97, `body {k} stops at the solid's face`)
        end
        eq(hits, 16, "each bump is a contact")
        check(world:get(air).x > 120, "air bodies pass solids")
    end,
})
)lua", 600, {{"map.map", wall_map()}})));
}

TEST(collision_air_passes_over_solid_tiles) {
    CHECK(wins(run_world("air_tiles", kTestToml, R"lua(
local air
run(function()
    air = world:spawn("dummy", { layer = "air", vx = 4 }, 140, 50, "host")
end, {
    [20] = function()
        eq(world:get(air).x, 216, "flew over the wall")
    end,
})
)lua", 600, {{"map.map", wall_map()}})));
}

TEST(collision_on_block_can_bounce) {
    CHECK(wins(run_world("bounce", kTestToml, R"lua(
local id
run(function()
    id = world:spawn("dummy", { vx = 3, bounce = true }, 300, 90, "host")
end, {
    [8] = function()
        local v = world:get(id)
        eq(v.vx, -3, "bounced off the right edge")
        eq(v.x, 313, "and came back")
    end,
})
)lua")));
}

TEST(collision_gravity_terminal_velocity_and_speed_cap) {
    const std::string floor = solid_map(40, 22, [](int, int y) { return y == 20 ? 1 : 0; });
    CHECK(wins(run_world("gravity", Toml{.gravity = "down"}.text(), R"lua(
local faller, floater, fast
run(function()
    faller = world:spawn("dummy", { kinematic = false }, 40, 20, "host")
    floater = world:spawn("dummy", nil, 80, 20, "host")
    fast = world:spawn("dummy", { vx = 100 }, 120, 20, "host")
end, {
    [2] = function()
        eq(world:get(faller).vy, 0.25, "one step of gravity")
        eq(world:get(faller).y, 20.25, "fell")
    end,
    [3] = function()
        eq(world:get(fast).x, 128, "4 px/tick at most")
        eq(world:get(fast).vx, 4, "velocity capped")
    end,
    [5] = function()
        eq(world:get(faller).vy, 1, "0.25 px/tick² ")
        eq(world:get(faller).y, 22.5, "position")
    end,
    [30] = function()
        eq(world:get(faller).vy, 4, "terminal velocity")
        eq(world:get(faller).y, 106, "falling at 4")
    end,
    [100] = function()
        eq(world:get(faller).y, 156, "landed on the floor")
        eq(world:get(faller).vy, 0, "at rest")
        eq(world:get(floater).y, 20, "kinematic bodies ignore gravity")
    end,
})
)lua", 600, {{"map.map", floor}})));
}
