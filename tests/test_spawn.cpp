#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

namespace {

const char* const kThreePoints =
    "[[spawn_points]]\nx = 10\ny = 20\n[[spawn_points]]\nx = 30\ny = 40\n[[spawn_points]]\nx = 50\ny = 60\n";

// Logs every spawn as "tick:x,y:faction:hp".
const char* const kSpawnLog = R"lua(
local log = {}
local function logging()
    world:on("spawn", function(id)
        local v = world:get(id)
        table.insert(log, `{tick()}:{v.x},{v.y}:{v.faction}:{v.hp}`)
    end)
end
)lua";

}  // namespace

TEST(spawn_spawners_release_over_time_cycling_points) {
    const std::string toml = Toml{.spawn = "world", .extra = kThreePoints}.text();
    CHECK(wins(run_world("spawner", toml, std::string(kSpawnLog) + R"lua(
run(function()
    logging()
    world:spawner({ archetype = "dummy", faction = "host", count = 5, every = 2 })
end, {
    [12] = function()
        eq(table.concat(log, " "), "1:10,20:host:1 3:30,40:host:1 5:50,60:host:1 7:10,20:host:1 9:30,40:host:1",
            "one every 2 steps, cycling the points")
    end,
})
)lua")));
}

TEST(spawn_every_zero_releases_at_once) {
    const std::string toml = Toml{.spawn = "world", .extra = kThreePoints}.text();
    CHECK(wins(run_world("spawner_now", toml, std::string(kSpawnLog) + R"lua(
run(function()
    logging()
    world:spawner({ archetype = "dummy", faction = "wild", count = 3, points = { 3, 1 }, params = { hp = 5 } })
end, {
    [2] = function()
        eq(table.concat(log, " "), "1:50,60:wild:5 1:10,20:wild:5 1:50,60:wild:5", "all at once, on chosen points")
        has(fails(world.spawner, world, { archetype = "dummy", faction = "host", points = { 4 } }),
            "there is no spawn point 4", "point")
        has(fails(world.spawner, world, { archetype = "dummy", faction = "host", count = 0 }),
            "count must be a whole number of at least 1", "count")
        has(fails(world.spawner, world, { archetype = "dummy", faction = "host", rate = 1 }),
            "world:spawner spec: unknown key rate", "spec")
        has(fails(world.spawner, world, { archetype = "nope", faction = "host" }), "unknown archetype nope", "archetype")
    end,
})
)lua")));
}

TEST(spawn_camera_points_follow_the_camera) {
    const std::string toml = Toml{.scroll = "auto", .spawn = "camera", .extra = "[[spawn_points]]\nx = 160\ny = -10\n"}.text();
    CHECK(wins(run_world("spawner_camera", toml, std::string(kSpawnLog) + R"lua(
run(function()
    logging()
    world:spawner({ archetype = "dummy", faction = "host", count = 2, every = 10 })
end, {
    [12] = function()
        eq(table.concat(log, " "), "1:160,-10.5:host:1 11:160,-15.5:host:1", "relative to the camera")
        local x, y = world:camera()
        eq(x, 0, "camera x")
        eq(y, -5.5, "the camera rises 0.5 px/tick")
    end,
})
)lua")));
}
