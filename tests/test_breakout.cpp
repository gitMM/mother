#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// BREAKOUT's archetypes: paddle_avatar, ball, brick, laser, paddle, and the shot the laser fires.

namespace {

std::string avatar(const char* archetype, int x, int y) {
    return std::string("[avatar]\narchetype = \"") + archetype + "\"\ndamage = \"hp\"\nhp = 30\nx = " +
           std::to_string(x) + "\ny = " + std::to_string(y) + "\n";
}

const char* const kNear = R"lua(
local function near(got, want, what)
    if math.abs(got - want) > 1e-9 then
        error(`{what}: expected {want}, got {got}`, 2)
    end
end
)lua";

}  // namespace

TEST(breakout_paddle_reflects_by_where_it_is_hit) {
    const std::string toml = Toml{.avatar = "up", .extra = avatar("paddle_avatar", 160, 160)}.text();
    CHECK(wins(run_world("paddle_avatar", toml, std::string(kNear) + R"lua(
local shots = {}
run(function()
    for i, x in { 148, 160, 172 } do
        shots[i] = world:spawn("shot", { vx = 0, vy = 3, life = 200 }, x, 120, "wild")
    end
end, {
    [30] = function()
        local l, c, r = world:get(shots[1]), world:get(shots[2]), world:get(shots[3])
        check(l.vy < 0 and c.vy < 0 and r.vy < 0, "all sent back up")
        check(l.vx < 0, "left of centre leaves to the left")
        eq(c.vx, 0, "the centre leaves straight up")
        check(r.vx > 0, "right of centre leaves to the right")
        near(math.sqrt(r.vx * r.vx + r.vy * r.vy), 3, "at the same speed")
        eq(c.faction, "player", "the paddle's faction owns them now")
    end,
})
)lua")));
}

TEST(breakout_ball_is_served_again_after_it_is_lost) {
    const std::string toml =
        Toml{.bounds = "{ left = \"solid\", right = \"solid\", top = \"solid\", bottom = \"kill\" }"}.text();
    CHECK(wins(run_world("ball", toml, std::string(kNear) + R"lua(
local lost_at = nil
function cart.init()
    world:spawn("ball", nil, 160, 100, "player")
    world:on("death", function(id, view)
        if view.archetype == "ball" then
            check(lost_at == nil, "lost once")
            lost_at = tick()
        end
    end)
end
function cart.update()
    local balls = world:query(0, 0, 320, 180, { archetype = "ball" })
    if lost_at == nil then
        local b = world:get(balls[1])
        near(math.sqrt(b.vx * b.vx + b.vy * b.vy), 3, "fast speed")
        check(tick() < 3000, "the ball was never lost")
    elseif tick() <= lost_at + 180 then
        eq(#balls, 0, "out of play while it waits")
    else
        eq(#balls, 1, "served again after 180 ticks")
        local b = world:get(balls[1])
        check(b.y < 100 and b.vy < 0, "from where the first one started, heading up")
        mother.finish("win")
    end
end
function cart.draw() end
function cart.hash() return 0 end
)lua", 4000)));
}

TEST(breakout_ball_bounces_off_player_bodies_and_hurts_others) {
    CHECK(wins(run_world("ball_bodies", kTestToml, R"lua(
run(function()
    world:spawn("ball", nil, 60, 100, "player")
    world:spawn("dummy", { w = 32, h = 8, hp = 3 }, 60, 80, "player")
    world:spawn("ball", nil, 260, 100, "player")
    world:spawn("dummy", { w = 32, h = 8, hp = 3 }, 260, 80, "host")
end, {
    [8] = function()
        check(world:get(1).vy > 0, "bounced off a player-faction body")
        eq(world:get(2).hp, 3, "and did not hurt it")
        check(world:get(3).vy < 0, "passed through a host body")
        eq(world:get(4).hp, 2, "tier 1 on contact")
    end,
})
)lua")));
}

TEST(breakout_brick_drops_a_laser_that_fires_pairs) {
    const std::string toml = Toml{.avatar = "up", .extra = avatar("paddle_avatar", 160, 164)}.text();
    const InputFn input = [](int t) { return hold(t == 80 || t == 90 || t == 120 || t == 190 ? "a" : ""); };
    CHECK(wins(run_world("laser", toml, R"lua(
local shots, lasers = {}, 0
run(function()
    world:on("spawn", function(id)
        local v = world:get(id)
        if v.archetype == "shot" then
            table.insert(shots, v)
        elseif v.archetype == "laser" then
            lasers += 1
        end
    end)
    world:spawn("brick", { drop = "laser", drop_params = { loose = true, ticks = 120 } }, 160, 100, "host")
end, {
    [2] = function()
        world:damage(2, 1)
    end,
    [79] = function()
        eq(lasers, 1, "the brick dropped a laser")
        local l = world:get(3)
        eq(l.x * 1000 + l.y, 160164, "taken: it rides on the paddle")
    end,
    [82] = function()
        eq(#shots, 2, "a pair of shots")
        eq(shots[1].x + shots[2].x, 320, "either side of the paddle")
        eq(shots[1].vy, -4, "along the facing at projectile speed")
    end,
    [92] = function()
        eq(#shots, 2, "30 ticks between pairs")
    end,
    [122] = function()
        eq(#shots, 4, "the next pair")
    end,
    [195] = function()
        eq(world:get(3), nil, "it ran out of time")
        eq(#shots, 4, "nothing left to fire")
    end,
})
)lua", 600, {}, input)));
}

TEST(breakout_shots_break_solid_bricks_and_hit_once) {
    CHECK(wins(run_world("shots", kTestToml, R"lua(
run(function()
    world:spawn("brick", nil, 100, 50, "host")
    world:spawn("shot", { vy = -4 }, 100, 90, "player")
    world:spawn("dummy", { hp = 3 }, 200, 60, "host")
    world:spawn("dummy", { hp = 3 }, 200, 60, "host")
    world:spawn("shot", { vy = -4 }, 200, 90, "player")
end, {
    [20] = function()
        eq(world:get(1), nil, "the solid brick broke")
        eq(world:get(2), nil, "the shot is spent")
        eq(world:get(3).hp + world:get(4).hp, 5, "one hit between two overlapping targets")
        eq(world:get(5), nil, "spent on its first hit")
    end,
})
)lua")));
}

TEST(breakout_paddle_equipment_reflects_ahead_of_its_holder) {
    const std::string toml = Toml{.avatar = "8dir", .extra = avatar("knight", 100, 90)}.text();
    const InputFn input = [](int t) { return hold(t == 0 ? ">" : ""); };
    CHECK(wins(run_world("paddle", toml, R"lua(
local shot
run(function()
    world:spawn("paddle", nil, 0, 0, "player")
    shot = world:spawn("shot", { vx = -3, vy = 0, life = 200 }, 200, 90, "wild")
end, {
    [60] = function()
        local s = world:get(shot)
        eq(s.vx, 3, "sent straight back")
        eq(s.faction, "player", "owned by the paddle's faction")
        eq(world:get(1).hp, 30, "the knight behind it is unhurt")
    end,
})
)lua", 600, {}, input)));
}

TEST(breakout_archetypes_check_requirements) {
    CHECK(wins(run_world("bo_requires", kTestToml, R"lua(
run(function() end, {
    [1] = function()
        has(fails(world.spawn, world, "paddle_avatar", nil, 0, 0, "player"), "needs avatar=up, host has none", "avatar")
        has(fails(world.spawn, world, "laser", nil, 0, 0, "player"), "laser needs an avatar", "laser")
        has(fails(world.spawn, world, "paddle", nil, 0, 0, "player"), "paddle needs an avatar", "paddle")
        check(world:spawn("ball", nil, 10, 10, "wild") > 0, "balls go anywhere")
        check(world:spawn("brick", nil, 40, 10, "host") > 0, "bricks go anywhere")
    end,
})
)lua")));
}
