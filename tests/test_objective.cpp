#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

namespace {

const char* const kMetLog = R"lua(
local met = {}
local function logging()
    world:on("objective", function(kind, id) table.insert(met, `{kind}@{tick()}` .. (if id then `:{id}` else "")) end)
end
)lua";

}  // namespace

// M5 acceptance: kill_all counts host-faction entities only.
TEST(objective_kill_all_counts_host_faction_only) {
    const std::string toml = Toml{.objective = "type = \"kill_all\""}.text();
    CHECK(wins(run_world("kill_all", toml, std::string(kMetLog) + R"lua(
run(function()
    logging()
    world:spawn("dummy", nil, 20, 20, "host")
    world:spawn("dummy", nil, 40, 20, "host")
    world:spawn("dummy", nil, 60, 20, "wild")
    world:spawn("dummy", nil, 80, 20, "player")
    world:spawn("dummy", { damage = "stun" }, 100, 20, "host")
end, {
    [3] = function()
        eq(#met, 0, "two host entities are left")
        world:damage(1, 1)
    end,
    [4] = function()
        eq(#met, 0, "one host entity is left")
        world:damage(2, 1)
    end,
    [5] = function()
        eq(table.concat(met, " "), "kill_all@4", "met with wild, player and stun-only host entities alive")
        check(world:get(3) and world:get(4) and world:get(5), "the others are alive")
        world:spawn("dummy", nil, 20, 60, "host")
    end,
    [6] = function()
        world:damage(6, 1)
    end,
    [7] = function()
        eq(table.concat(met, " "), "kill_all@4 kill_all@6", "met again after a new host entity died")
    end,
})
)lua")));
}

TEST(objective_kill_all_waits_for_host_spawners) {
    const std::string toml =
        Toml{.spawn = "world", .objective = "type = \"kill_all\"", .extra = "[[spawn_points]]\nx = 20\ny = 20\n"}.text();
    CHECK(wins(run_world("kill_all_spawners", toml, std::string(kMetLog) + R"lua(
run(function()
    logging()
    world:on("spawn", function(id)
        if world:get(id).faction == "host" then
            world:damage(id, 1)
        end
    end)
    world:spawner({ archetype = "dummy", faction = "host", count = 2, every = 5 })
    world:spawner({ archetype = "dummy", faction = "wild", count = 100, every = 1 })
end, {
    [12] = function()
        eq(table.concat(met, " "), "kill_all@7", "met once the host spawner was done")
    end,
})
)lua")));
}

TEST(objective_survive_escape_collect) {
    CHECK(wins(run_world("survive", Toml{.objective = "type = \"survive\"\nticks = 30"}.text(),
                         std::string(kMetLog) + "run(logging, { [40] = function() eq(table.concat(met), 'survive@30', 'survived') end })")));

    const std::string escape = Toml{.avatar = "lr",
                                    .objective = "type = \"escape\"\nexit = { x = 200, y = 0, w = 8, h = 180 }",
                                    .extra = "[avatar]\narchetype = \"dummy\"\ndamage = \"hp\"\nhp = 1\nx = 100\n"
                                             "y = 90\nparams = { vx = 2 }\n"}
                                   .text();
    CHECK(wins(run_world("escape", escape,
                         std::string(kMetLog) + "run(logging, { [60] = function() eq(table.concat(met), 'escape@49:1', 'escaped') end })")));

    const std::string collect = Toml{.objective = "type = \"collect\"\narchetype = \"dummy\""}.text();
    CHECK(wins(run_world("collect", collect, std::string(kMetLog) + R"lua(
run(function()
    logging()
    world:spawn("dummy", nil, 20, 20, "host")
    world:spawn("dummy", nil, 40, 20, "wild")
end, {
    [2] = function() world:remove(1) end,
    [4] = function() world:remove(2) end,
    [6] = function() eq(table.concat(met), "collect@4", "nothing left to collect") end,
})
)lua")));
}

TEST(objective_race_counts_the_avatar_and_host_racers) {
    const std::string toml = Toml{.avatar = "heading",
                                  .objective = "type = \"race\"\nlaps = 3",
                                  .extra = "[avatar]\narchetype = \"dummy\"\ndamage = \"hp\"\nhp = 1\nx = 20\ny = 20\n"
                                           "params = { racer = true, lap_every = 10 }\n"}
                                 .text();
    CHECK(wins(run_world("race", toml, std::string(kMetLog) + R"lua(
run(function()
    logging()
    world:spawn("dummy", { racer = true, lap_every = 5 }, 40, 20, "wild")
    world:spawn("dummy", { racer = true, lap_every = 7 }, 60, 20, "host")
    world:spawn("dummy", { lap_every = 1 }, 80, 20, "host")
end, {
    [40] = function()
        eq(table.concat(met, " "), "race@21:3", "the host racer won; wild racers and non-racers don't count")
    end,
})
)lua")));
}
