#include <array>
#include <initializer_list>
#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// JUMPER's archetypes: runner, walker, turtle, shell, fire_flower and the fireball it throws.

namespace {

std::string runner(int x, int y, int hp = 1) {
    return "[avatar]\narchetype = \"runner\"\ndamage = \"hp\"\nhp = " + std::to_string(hp) + "\nx = " +
           std::to_string(x) + "\ny = " + std::to_string(y) + "\n[[spawn_points]]\nx = 300\ny = 150\n";
}

std::string platform(int x, int y, int hp = 1) {
    return Toml{.gravity = "down", .surface = "ground", .avatar = "lr", .spawn = "world", .deploy = "true",
                .extra = runner(x, y, hp)}
        .text();
}

// A floor of critical tiles from row 20 down, plus `more` (x, y, tile) cells.
TempCart::Files floor_map(std::initializer_list<std::array<int, 3>> more = {}) {
    return {{"map.map", solid_map(40, 22, [more](int x, int y) {
                 for (const auto& c : more) {
                     if (c[0] == x && c[1] == y) return c[2];
                 }
                 return y >= 20 ? 3 : 0;
             })}};
}

}  // namespace

TEST(jumper_runner_jumps_higher_the_longer_a_is_held) {
    const InputFn input = [](int t) { return hold((t >= 10 && t < 40) || (t >= 60 && t < 63) ? "a" : ""); };
    CHECK(wins(run_world("runner_jump", platform(100, 153), R"lua(
local top = 153
function cart.init() end
function cart.update()
    local r = world:get(1)
    top = math.min(top, r.y)
    if tick() == 50 then
        eq(top, 123, "a full jump: 30 px")
        eq(r.y, 153, "back on the floor")
        top = 153
    elseif tick() == 90 then
        eq(top, 141, "let go after 3 ticks: 12 px")
        mother.finish("win")
    end
end
function cart.draw() end
function cart.hash() return 0 end
)lua", 600, floor_map(), input)));
}

TEST(jumper_runner_head_bump_breaks_damageable_tiles_only) {
    const InputFn input = [](int t) { return hold(t >= 10 && t < 30 ? "a" : ""); };
    const char* const check_tile = R"lua(
run(function() end, {
    [40] = function() eq(mget(12, 15, 0), WANT, "the tile over its head") end,
})
)lua";
    std::string breaks = std::string("local WANT = 0\n") + check_tile;
    CHECK(wins(run_world("bump_breaks", platform(100, 153), breaks, 600, floor_map({{12, 15, 2}}), input)));
    std::string stays = std::string("local WANT = 3\n") + check_tile;
    CHECK(wins(run_world("bump_stays", platform(100, 153), stays, 600, floor_map({{12, 15, 3}}), input)));
}

TEST(jumper_runner_stomps_from_above_and_is_hurt_from_the_side) {
    CHECK(wins(run_world("stomp", platform(100, 100, 3), R"lua(
local dead = {}
run(function()
    world:spawn("walker", nil, 100, 154, "host")
    world:spawn("walker", nil, 220, 154, "host")
    world:on("death", function(id) dead[id] = tick() end)
end, {
    [40] = function()
        check(dead[2] ~= nil, "the walker it landed on died")
        local r = world:get(1)
        eq(r.hp, 3, "the stomp didn't hurt the runner")
    end,
    [300] = function()
        check(world:get(1).hp < 3, "the other walker walked into it: tier 1")
        check(world:get(3) ~= nil, "and the runner can't hurt it from the side")
    end,
})
)lua", 600, floor_map())));
}

TEST(jumper_turtle_leaves_a_shell_that_arms_after_30_ticks) {
    // A wall at column 24 sends the shell back.
    TempCart::Files map = {{"map.map", solid_map(40, 22, [](int x, int y) { return y >= 20 || x == 24 ? 3 : 0; })}};
    // The turtle is hit from above and to the left (as a stomp would), by a dummy standing in.
    CHECK(wins(run_world("shell", platform(20, 153, 30), R"lua(
local shell = nil
run(function()
    world:spawn("turtle", nil, 100, 153, "host")
    world:spawn("dummy", { hp = 10 }, 130, 156, "host")
    world:spawn("dummy", nil, 90, 130, "wild")
    world:on("spawn", function(id)
        if world:get(id).archetype == "shell" then
            shell = id
        end
    end)
end, {
    [2] = function() world:damage(2, 1, 4) end,
    [25] = function()
        local s = world:get(shell)
        check(s ~= nil, "a shell where the turtle died")
        check(s.vx > 0, "sliding away from what hit it")
        eq(world:get(3).hp, 10, "it passed the dummy before it was armed")
    end,
    [120] = function()
        eq(world:get(3).hp, 7, "on its way back, armed: tier 3, even to its own faction")
    end,
})
)lua", 600, map)));
}

TEST(jumper_fire_flower_powers_up_and_throws_two_bouncing_fireballs) {
    const InputFn input = [](int t) { return hold(t == 60 || t == 62 || t == 64 ? "b" : ""); };
    CHECK(wins(run_world("flower", platform(100, 153), R"lua(
local balls, bounced = 0, false
function cart.init()
    world:spawn("fire_flower", { hp = 2, button = "b", loose = true }, 100, 120, "player")
    world:spawn("walker", nil, 250, 154, "host")
    world:on("spawn", function(id)
        if world:get(id).archetype == "fireball" then
            balls += 1
        end
    end)
end
function cart.update()
    for _, id in world:query(0, 0, 320, 180, { archetype = "fireball" }) do
        bounced = bounced or world:get(id).vy < 0
    end
    if tick() == 50 then
        eq(world:get(1).hp, 2, "taking the flower raised its HP to 2")
    elseif tick() == 70 then
        eq(balls, 2, "3 presses, 2 fireballs out at most")
    elseif tick() == 200 then
        check(bounced, "they bounce along the floor")
        eq(world:get(3), nil, "and burn the walker (tier 3)")
        mother.finish("win")
    end
end
function cart.draw() end
function cart.hash() return 0 end
)lua", 600, floor_map(), input)));
}

TEST(jumper_fireball_ricochets_three_times_without_gravity) {
    TempCart::Files map = {{"map.map", solid_map(40, 22, [](int x, int) { return x == 5 || x == 25 ? 1 : 0; })}};
    CHECK(wins(run_world("ricochet", kTestToml, R"lua(
run(function()
    world:spawn("fireball", { vx = 3, life = 400 }, 100, 90, "player")
end, {
    [170] = function()
        local f = world:get(1)
        check(f ~= nil and f.vx < 0, "3 ricochets: right wall, left wall, right wall")
    end,
    [230] = function() eq(world:get(1), nil, "spent at the 4th wall") end,
})
)lua", 600, map)));
}

TEST(jumper_walker_heads_for_a_target_and_turns_at_walls) {
    TempCart::Files map = {{"map.map", solid_map(40, 22, [](int x, int) { return x == 5 || x == 25 ? 1 : 0; })}};
    const std::string toml = Toml{.surface = "ground", .spawn = "world", .extra = "[[spawn_points]]\nx = 1\ny = 1\n"}.text();
    CHECK(wins(run_world("walker", toml, R"lua(
run(function()
    world:spawn("dummy", { hp = 9 }, 60, 90, "player")
    world:spawn("walker", nil, 120, 90, "host")
end, {
    [2] = function() eq(world:get(2).vx, -0.5, "towards the dummy, at crawl speed") end,
    [200] = function()
        eq(world:get(2).vx, 0.5, "turned at the wall past the dummy")
        check(world:get(1).hp < 9, "and walked into the dummy on the way")
    end,
})
)lua", 600, map)));
}
