#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// RALLY's archetypes: car, rival_ai, oil, nitro; and the racing line that counts laps.

namespace {

std::string driving(int x, int y, const std::string& objective = "type = \"survive\"\nticks = 36000") {
    return Toml{.surface = "ground", .avatar = "heading", .spawn = "world", .deploy = "true", .objective = objective,
                .extra = "[avatar]\narchetype = \"car\"\ndamage = \"stun\"\nhp = 1\nx = " + std::to_string(x) +
                         "\ny = " + std::to_string(y) + "\n[[spawn_points]]\nx = 40\ny = 40\n"}
        .text();
}

// A 40x22 map: columns 25 on are offroad grass (tile 4, flag bit 3).
std::string grass_map() {
    std::string text = "layer 0 40 22\n";
    for (int y = 0; y < 22; ++y) {
        for (int x = 0; x < 40; ++x) {
            text += x >= 25 ? "0004" : "0000";
            text += x < 39 ? ' ' : '\n';
        }
    }
    return text + flags_block(4, "08");
}

const char* const kSpeed = R"lua(
local function speed(v)
    return math.sqrt(v.vx * v.vx + v.vy * v.vy)
end
)lua";

}  // namespace

TEST(rally_car_speeds_up_steers_and_slows_offroad) {
    const InputFn input = [](int) { return hold("a"); };
    CHECK(wins(run_world("car", driving(40, 90), std::string(kSpeed) + R"lua(
run(function() end, {
    [2] = function() eq(world:get(1).angle, 0, "facing right with no racing line") end,
    [60] = function()
        local c = world:get(1)
        eq(c.vx, 2, "top speed: run")
        eq(c.vy, 0, "straight ahead")
    end,
    [140] = function()
        local c = world:get(1)
        check(c.x > 200, "on the grass")
        eq(c.vx, 1, "half speed offroad")
    end,
})
)lua", 600, {{"map.map", grass_map()}}, input)));

    const InputFn turning = [](int) { return hold("a>"); };
    CHECK(wins(run_world("car_steer", driving(100, 40), R"lua(
run(function() end, {
    [40] = function()
        local c = world:get(1)
        check(c.angle > 0.5, "turned clockwise")
        check(c.vy > 0, "and is heading down the screen")
    end,
})
)lua", 600, {}, turning)));
}

TEST(rally_rival_rams_and_only_the_car_it_hits_spins) {
    CHECK(wins(run_world("ram", driving(200, 90), R"lua(
local rammed = nil
run(function()
    world:spawn("rival_ai", nil, 60, 90, "host")
    world:on("hit", function(id, tier, source) rammed = rammed or { id = id, source = source, at = tick() } end)
end, {
    [200] = function()
        check(rammed ~= nil, "a ram")
        eq(rammed.id, 1, "the car at rest was hit")
        eq(rammed.source, 2, "by the rival")
        eq(world:get(2).status.stun, 0, "the rival, closing in, wasn't")
    end,
})
)lua")));
}

TEST(rally_rival_drives_the_racing_line_and_laps) {
    const std::string race = "type = \"race\"\nlaps = 1\nline = [[60, 40], [260, 40], [260, 140], [60, 140]]";
    CHECK(wins(run_world("laps", driving(300, 165, race), R"lua(
local winner = nil
run(function()
    world:spawn("rival_ai", nil, 30, 40, "host")
    world:on("objective", function(kind, id) winner = winner or { id = id, laps = world:get(id).laps } end)
end, {
    [100] = function() eq(world:get(2).laps, 0, "not round yet") end,
    [900] = function()
        check(winner ~= nil, "a lap done")
        eq(winner.id, 2, "by the rival")
        eq(winner.laps, 1, "its view counts the lap")
        eq(world:get(1).laps, 0, "the car that stayed put has none")
    end,
})
)lua", 1000)));
}

TEST(rally_oil_slips_ground_bodies_and_three_lie_about_at_most) {
    CHECK(wins(run_world("oil", driving(20, 160), R"lua(
run(function()
    for i = 0, 3 do
        world:spawn("oil", nil, 100 + 30 * i, 90, "player")
    end
    world:spawn("dummy", { vx = -1 }, 240, 90, "wild")
    world:spawn("dummy", { vx = -1, layer = "air" }, 240, 94, "wild")
end, {
    [2] = function() eq(#world:query(0, 0, 320, 180, { archetype = "oil" }), 3, "the 4th cleared the 1st") end,
    [60] = function()
        local ground, air = world:get(6), world:get(7)
        check(ground.status.slip > 0, "the ground body crossed a slick and slips")
        eq(air.status.slip, 0, "air bodies don't")
    end,
})
)lua")));
}

TEST(rally_nitro_doubles_speed_and_runs_out_of_charges) {
    const InputFn input = [](int t) {
        if (t == 60 || t == 150 || t == 400) return hold("a>l");
        return hold("a>");
    };
    CHECK(wins(run_world("nitro", driving(160, 50), std::string(kSpeed) + R"lua(
local nitro, fastest = nil, 0
function cart.init()
    nitro = world:spawn("nitro", { button = "l", charges = 2 }, 0, 0, "player")
end
function cart.update()
    local c = world:get(1)
    if tick() > 61 and tick() < 120 then
        fastest = math.max(fastest, speed(c))
    end
    if tick() == 59 then
        check(speed(c) < 2.2, "about run speed before (turning adds a little slide)")
    elseif tick() == 125 then
        check(fastest > 3, "up to twice run speed while boosted")
    elseif tick() == 200 then
        eq(world:get(nitro).charges, 1, "the press at 150 came within the 300-tick recharge")
    elseif tick() == 410 then
        eq(world:get(nitro), nil, "the second boost used it up")
        mother.finish("win")
    end
end
function cart.draw() end
function cart.hash() return 0 end
)lua", 600, {}, input)));
}
