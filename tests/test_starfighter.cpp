#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// STARFIGHTER's archetypes: ship, diver, mine, homing, and the missile homing launches.

namespace {

std::string ship(int x, int y) {
    return "[avatar]\narchetype = \"ship\"\ndamage = \"hp\"\nhp = 3\nx = " + std::to_string(x) + "\ny = " +
           std::to_string(y) + "\n";
}

const std::string kPoint = "[[spawn_points]]\nx = 100\ny = -10\n";

}  // namespace

TEST(starfighter_ship_keeps_on_screen_and_fires_two_at_a_time) {
    const std::string toml = Toml{.scroll = "auto", .avatar = "up", .spawn = "camera", .bounds = "despawn",
                                  .extra = ship(160, 150) + kPoint}
                                 .text();
    const InputFn input = [](int t) {
        if (t < 80) return hold("<");
        if (t < 100) return hold("v");
        return hold(t == 110 || t == 112 || t == 114 ? "a" : "");
    };
    CHECK(wins(run_world("ship", toml, R"lua(
local function shots()
    local cx, cy = world:camera()
    return #world:query(cx, cy - 40, 320, 260, { archetype = "shot" })
end
run(function() end, {
    [100] = function()
        local s = world:get(1)
        local cx, cy = world:camera()
        eq(cy, -49.5, "the camera has risen 0.5 px a tick")
        eq(s.x, cx + 6, "held at the left edge")
        eq(s.y, cy + 174, "held at the bottom edge as the screen scrolls")
    end,
    [118] = function() eq(shots(), 2, "3 presses, 2 shots on screen at most") end,
})
)lua", 600, {}, input)));
}

TEST(starfighter_diver_swoops_dives_fires_once_and_comes_back) {
    const std::string toml = Toml{.avatar = "up", .spawn = "camera", .bounds = "despawn",
                                  .extra = ship(160, 160) + kPoint}
                                 .text();
    CHECK(wins(run_world("diver", toml, R"lua(
local diver, bullets, phase = nil, {}, "in"
function cart.init()
    diver = world:spawn("diver", { swoop = 30 }, 100, -10, "host")
    world:on("spawn", function(id)
        local v = world:get(id)
        if v.archetype == "shot" and v.faction == "host" then
            table.insert(bullets, v)
        end
    end)
end
function cart.update()
    local d = world:get(diver)
    check(d ~= nil, `the diver is still about at tick {tick()}`)
    if phase == "in" and tick() == 32 then
        eq(#bullets, 1, "one bullet as it starts its dive")
        check(bullets[1].vx > 0 and bullets[1].vy > 0, "aimed at the ship, down and to the right")
        check(d.vx > 0 and d.vy > 0, "diving at the ship")
        phase = "down"
    elseif phase == "down" and d.y < 0 then
        eq(#bullets, 1, "still one bullet for the dive")
        check(math.abs(d.x - 100) < 3, "back in where it first appeared")
        mother.finish("win")
    end
    check(tick() < 400, "it never came back")
end
function cart.draw() end
function cart.hash() return 0 end
)lua")));
}

TEST(starfighter_mine_blasts_everything_near_when_touched) {
    const std::string toml = Toml{.spawn = "world", .extra = kPoint}.text();
    CHECK(wins(run_world("mine", toml, R"lua(
run(function()
    world:spawn("mine", nil, 100, 90, "host")
    world:spawn("dummy", { hp = 30, vx = 1 }, 80, 90, "player")
    world:spawn("dummy", { hp = 30 }, 100, 102, "host")
    world:spawn("dummy", { hp = 30 }, 100, 130, "wild")
end, {
    [20] = function()
        eq(world:get(2).hp, 20, "the one that touched it: tier 10")
        eq(world:get(3).hp, 20, "its own faction, 8 px from its middle, too")
        eq(world:get(4).hp, 30, "not one further off")
        eq(#world:query(0, 0, 320, 180, { archetype = "mine" }), 0, "out of play while it shows its blast")
    end,
    [40] = function() eq(world:get(1), nil, "then gone") end,
})
)lua")));
}

TEST(starfighter_homing_missile_seeks_a_target) {
    const std::string toml = Toml{.avatar = "up", .extra = ship(160, 150)}.text();
    const InputFn input = [](int t) { return hold(t == 5 || t == 10 ? "b" : ""); };
    CHECK(wins(run_world("homing", toml, R"lua(
local missiles = 0
run(function()
    world:spawn("homing", { button = "b" }, 0, 0, "player")
    world:spawn("dummy", { hp = 30 }, 260, 60, "wild")
    world:on("spawn", function(id)
        if world:get(id).archetype == "missile" then
            missiles += 1
        end
    end)
end, {
    [20] = function()
        eq(missiles, 1, "one launch; the second press came within the 60-tick cooldown")
        local m = world:get(world:query(0, 0, 320, 180, { archetype = "missile" })[1])
        check(m.vx > 0, "turning towards the target on the right")
    end,
    [120] = function() eq(world:get(3).hp, 27, "hit for tier 3") end,
})
)lua", 600, {}, input)));
}
