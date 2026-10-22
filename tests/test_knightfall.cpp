#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// KNIGHTFALL's archetypes: knight, sword, skeleton, bomb.

namespace {

std::string avatar(const char* archetype, int x, int y, const char* params = "{}") {
    return std::string("[avatar]\narchetype = \"") + archetype + "\"\ndamage = \"hp\"\nhp = 30\nx = " +
           std::to_string(x) + "\ny = " + std::to_string(y) + "\nparams = " + params + "\n";
}

const char* const kNear = R"lua(
local function near(got, want, what)
    if math.abs(got - want) > 1e-9 then
        error(`{what}: expected {want}, got {got}`, 2)
    end
end
)lua";

}  // namespace

TEST(knightfall_knight_walks_8_ways_and_dodges) {
    const std::string toml = Toml{.avatar = "8dir", .extra = avatar("knight", 100, 90)}.text();
    const InputFn input = [](int t) { return hold(t < 10 ? ">" : t < 20 ? ">v" : t == 20 || t == 40 ? "b" : ""); };
    CHECK(wins(run_world("knight", toml, std::string(kNear) + R"lua(
local d = math.sqrt(0.5)
run(function() end, {
    [11] = function()
        local k = world:get(1)
        eq(k.x, 110, "walked right at 1 px/tick")
        eq(k.y, 90, "only right")
    end,
    [21] = function()
        local k = world:get(1)
        near(k.x, 110 + 10 * d, "diagonals are as fast as straight lines")
        near(k.y, 90 + 10 * d, "diagonal y")
    end,
    [25] = function()
        eq(world:damage(1, 3), false, "unhurtable while dodging")
    end,
    [34] = function()
        local k = world:get(1)
        near(k.x, 110 + 10 * d + 36 * d, "12 dodge steps at fast speed along the facing")
        eq(k.hp, 30, "unhurt")
    end,
    [45] = function()
        local k = world:get(1)
        near(k.x, 110 + 46 * d, "no second dodge within the cooldown")
    end,
})
)lua", 600, {}, input)));
}

TEST(knightfall_sword_hits_an_arc_ahead) {
    const std::string toml = Toml{.avatar = "8dir", .extra = avatar("knight", 160, 90)}.text();
    const InputFn input = [](int t) { return hold(t == 0 ? ">" : t == 1 || t == 3 ? "a" : ""); };
    CHECK(wins(run_world("sword", toml, R"lua(
run(function()
    world:spawn("sword", nil, 0, 0, "player")
    world:spawn("dummy", { hp = 30 }, 180, 90, "host") -- 3: ahead, in reach
    world:spawn("dummy", { hp = 30 }, 142, 90, "host") -- 4: behind
    world:spawn("dummy", { hp = 30 }, 200, 90, "host") -- 5: ahead, out of reach
    world:spawn("dummy", { hp = 30 }, 175, 95, "player") -- 6: ahead, own faction
end, {
    [4] = function()
        local hit = world:get(3)
        eq(hit.hp, 27, "tier 3 ahead")
        check(hit.status.knockback > 0, "knocked back")
        eq(world:get(4).hp, 30, "behind is safe")
        eq(world:get(5).hp, 30, "out of reach")
        eq(world:get(6).hp, 30, "own faction")
    end,
    [40] = function()
        eq(world:get(3).hp, 27, "one swing per press, 20 ticks apart")
    end,
})
)lua", 600, {}, input)));
}

TEST(knightfall_skeleton_chases_other_factions_and_swings) {
    const std::string toml =
        Toml{.surface = "ground", .spawn = "world", .extra = "[[spawn_points]]\nx = 10\ny = 10\n"}.text();
    CHECK(wins(run_world("skeleton", toml, R"lua(
local swings = {}
run(function()
    world:on("hit", function(id, tier) table.insert(swings, `{id}:{tier}@{tick()}`) end)
    world:spawn("skeleton", nil, 100, 90, "host")
    world:spawn("dummy", { hp = 30 }, 160, 90, "player") -- 2: the target
    world:spawn("dummy", { hp = 30 }, 120, 90, "host") -- 3: same faction, nearer, ignored
end, {
    [60] = function()
        check(world:get(1).x > 120, "walked towards the player dummy")
        eq(#swings, 0, "not in reach yet")
    end,
    [150] = function()
        eq(world:get(2).hp, 27, "one swing, tier 3")
        eq(world:get(3).hp, 30, "its own faction is never a target")
        eq(world:get(1).hp, 3, "skeletons have HP 3")
    end,
})
)lua")));

    // Under gravity it falls to the floor and walks along it.
    const std::string fall = Toml{.gravity = "down", .surface = "ground", .spawn = "world",
                                  .extra = "[[spawn_points]]\nx = 10\ny = 10\n"}
                                 .text();
    const std::string floor = solid_map(40, 22, [](int, int y) { return y == 20 ? 1 : 0; });
    CHECK(wins(run_world("skeleton_fall", fall, R"lua(
run(function()
    world:spawn("skeleton", nil, 100, 100, "host")
    world:spawn("dummy", { hp = 30, kinematic = false }, 200, 100, "player")
end, {
    [60] = function()
        eq(world:get(1).y, 153, "standing on the floor")
    end,
    [400] = function()
        check(world:get(2).hp < 30, "walked over and swung")
    end,
})
)lua", 600, {{"map.map", floor}})));
}

TEST(knightfall_stunned_skeletons_do_nothing) {
    const std::string toml =
        Toml{.surface = "ground", .spawn = "world", .extra = "[[spawn_points]]\nx = 10\ny = 10\n"}.text();
    CHECK(wins(run_world("skeleton_stun", toml, R"lua(
run(function()
    world:spawn("skeleton", nil, 100, 90, "host")
    world:spawn("dummy", { hp = 30 }, 115, 90, "player")
    world:status(1, "stun", 200)
end, {
    [150] = function()
        eq(world:get(1).x, 100, "stunned: no steps")
        eq(world:get(2).hp, 30, "stunned: no swings")
    end,
})
)lua")));
}

TEST(knightfall_bomb_blasts_everyone_and_breaks_tiles) {
    // Tile (13, 11) is damageable, (11, 11) damageable but critical, (16, 11) damageable but far.
    const std::string map = solid_map(40, 22, [](int x, int y) {
        return y != 11 ? 0 : x == 13 || x == 16 ? 2 : x == 11 ? 3 : 0;
    });
    CHECK(wins(run_world("bomb", Toml{.deploy = "true"}.text(), R"lua(
local bomb, stunned
run(function()
    bomb = world:spawn("bomb", nil, 100, 84, "player")
    world:spawn("dummy", { hp = 30 }, 112, 84, "player") -- 2: its own faction, in the blast
    world:spawn("dummy", { hp = 30 }, 140, 84, "host") -- 3: out of the blast
    stunned = world:spawn("bomb", nil, 250, 84, "player")
    world:spawn("dummy", { hp = 30 }, 260, 84, "host") -- 5: by the stunned bomb
    world:status(stunned, "stun", 50)
end, {
    [120] = function()
        eq(world:get(2).hp, 30, "the fuse is 120 ticks")
    end,
    [122] = function()
        eq(world:get(2).hp, 20, "tier 10 to its own faction too")
        eq(world:get(3).hp, 30, "2 tiles reach")
        eq(mget(13, 11), 0, "a damageable tile broke")
        eq(mget(11, 11), 3, "a critical tile never breaks")
        eq(mget(16, 11), 2, "out of reach")
        eq(world:get(5).hp, 30, "a stun holds the fuse")
    end,
    [135] = function()
        eq(world:get(bomb), nil, "gone after the blast")
    end,
    [175] = function()
        eq(world:get(5).hp, 20, "the stunned bomb went off 50 ticks late")
    end,
})
)lua", 600, {{"map.map", map}})));
}

TEST(knightfall_archetypes_check_requirements) {
    CHECK(wins(run_world("kf_requires", kTestToml, R"lua(
run(function() end, {
    [1] = function()
        has(fails(world.spawn, world, "knight", nil, 0, 0, "player"), "knight needs avatar=8dir, host has none", "knight")
        has(fails(world.spawn, world, "skeleton", nil, 0, 0, "host"), "skeleton needs surface=ground, host has none", "skeleton")
        has(fails(world.spawn, world, "sword", nil, 0, 0, "player"), "sword needs an avatar, host has avatar=none", "sword")
        has(fails(world.spawn, world, "bomb", nil, 0, 0, "player"), "bomb needs deploy=true, host has false", "bomb")
    end,
})
)lua")));
}
