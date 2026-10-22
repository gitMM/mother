#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

TEST(world_spawns_gets_and_removes) {
    CHECK(wins(run_world("basics", kTestToml, R"lua(
run(function() end, {
    [1] = function()
        local a = world:spawn("dummy", { w = 4, h = 6, hp = 3 }, 100, 50, "host")
        local b = world:spawn("dummy", nil, 10, 20, "wild")
        eq(a, 1, "first id")
        eq(b, 2, "second id")
        local v = world:get(a)
        eq(v.id, 1, "id")
        eq(v.archetype, "dummy", "archetype")
        eq(v.x, 100, "x")
        eq(v.y, 50, "y")
        eq(v.w, 4, "w")
        eq(v.h, 6, "h")
        eq(v.hp, 3, "hp")
        eq(v.faction, "host", "faction")
        eq(v.layer, "ground", "layer")
        eq(v.vx, 0, "vx")
        eq(v.imported, false, "imported")
        eq(v.status.stun + v.status.slip + v.status.knockback, 0, "statuses")
        check(fails(function() v.x = 5 end) ~= nil, "views are frozen")
        check(fails(function() v.status.stun = 5 end) ~= nil, "status views are frozen")
        eq(world:get(99), nil, "unknown id")
        eq(world:avatar(), nil, "no avatar")
        local cx, cy = world:camera()
        eq(cx + cy, 0, "camera")
        world:remove(b)
        eq(world:get(b), nil, "removed at once")
        world:remove(b)
        eq(world:spawn("dummy", nil, 0, 0, "player"), 3, "ids are never reused")
    end,
    [2] = function()
        eq(world:get(1).x, 100, "still there")
    end,
})
)lua")));
}

TEST(world_rejects_bad_calls) {
    CHECK(wins(run_world("errors", kTestToml, R"lua(
local function nothing() end
run(function() end, {
    [1] = function()
        has(fails(world.spawn, world, "nope", nil, 0, 0, "host"), "unknown archetype nope", "archetype")
        has(fails(world.spawn, world, "dummy", nil, 0, 0, "enemy"), "faction must be one of host, player, wild", "faction")
        has(fails(world.spawn, world, "dummy", nil, 0 / 0, 0, "host"), "x must be a finite number", "nan x")
        has(fails(world.spawn, world, "dummy", { speed = 1 }, 0, 0, "host"), "dummy params: unknown key speed", "param")
        has(fails(world.spawn, world, "dummy", { hp = "lots" }, 0, 0, "host"), "hp must be a number, got string", "type")
        has(fails(world.spawn, world, "dummy", { w = 3 }, 0, 0, "host"), "the collider must be even", "collider")
        has(fails(world.spawn, world, "dummy", { tier = 2 }, 0, 0, "host"), "tier must be one of", "tier param")
        has(fails(world.spawn, "dummy", nil, 0, 0, "host"), "call world:spawn(...) with a colon", "dot call")
        local id = world:spawn("dummy", nil, 0, 0, "host")
        has(fails(world.damage, world, id, 2), "the tier must be 1, 3, 10 or 30", "damage tier")
        has(fails(world.damage, world, id, 1, "me"), "the source must be an entity id", "source")
        has(fails(world.status, world, id, "burn", 10), "kind must be one of knockback, slip, stun", "kind")
        has(fails(world.status, world, id, "stun", -1), "ticks must be a whole number of at least 0", "ticks")
        has(fails(world.on, world, "explode", nothing), "event must be one of death, hit, objective, spawn", "event")
        has(fails(world.on, world, "death", 5), "the handler must be a function", "handler")
        has(fails(world.query, world, 0, 0, 10, 10, { colour = 1 }), "filter: unknown key colour", "filter")
        has(fails(world.nearest, world, 0, 0, 5), "filter must be a table", "filter type")
        has(fails(world.spawner, world, { archetype = "dummy", faction = "host" }), "spawn = none", "spawner")
        has(fails(world.draw, world), "can only be called from cart.draw()", "draw")
    end,
})
)lua")));
}

TEST(world_is_frozen_and_keeps_its_internals) {
    CHECK(wins(run_world("frozen", kTestToml, R"lua(
run(function() end, {
    [1] = function()
        check(fails(function() world.spawn = nil end) ~= nil, "world is writable")
        check(fails(function() world.extra = 1 end) ~= nil, "world takes new fields")
        eq(world.step, nil, "world.step")
        eq(world.create, nil, "world.create")
        eq(world.hash, nil, "world.hash")
        eq(use, nil, "use")
        eq(host, nil, "host")
    end,
})
)lua")));
}

TEST(world_queries_and_finds_nearest) {
    CHECK(wins(run_world("query", kTestToml, R"lua(
run(function()
    world:spawn("dummy", nil, 10, 10, "host")
    world:spawn("dummy", { layer = "air" }, 20, 10, "wild")
    world:spawn("dummy", { damage = "none", hp = 0 }, 30, 10, "player")
    world:spawn("dummy", nil, 100, 100, "host")
end, {
    [1] = function()
        local ids = world:query(0, 0, 40, 20)
        eq(#ids, 3, "count")
        eq(ids[1] * 100 + ids[2] * 10 + ids[3], 123, "id order")
        eq(#world:query(0, 0, 6, 6), 0, "touching edges don't overlap")
        eq(#world:query(0, 0, 6.5, 6.5), 1, "overlap")
        eq(world:query(0, 0, 200, 200, { faction = "host" })[2], 4, "faction")
        eq(#world:query(0, 0, 200, 200, { enemy_of = "host" }), 2, "enemy_of")
        eq(world:query(0, 0, 200, 200, { layer = "air" })[1], 2, "layer")
        eq(#world:query(0, 0, 200, 200, { damageable = false }), 1, "damageable")
        eq(#world:query(0, 0, 200, 200, { damageable = true, exclude = 1 }), 2, "exclude")
        eq(#world:query(0, 0, 200, 200, { archetype = "ball" }), 0, "archetype")
        eq(world:nearest(0, 0), 1, "nearest")
        eq(world:nearest(15, 10), 1, "a tie goes to the lower id")
        eq(world:nearest(15, 10, { exclude = 1 }), 2, "exclude")
        eq(world:nearest(0, 0, { enemy_of = "host" }), 2, "nearest of another faction")
        eq(world:nearest(0, 0, { faction = "player", damageable = true }), nil, "nothing matches")
    end,
})
)lua")));
}

TEST(world_delivers_events_at_the_end_of_the_step) {
    CHECK(wins(run_world("events", kTestToml, R"lua(
local log = {}
local function note(...)
    table.insert(log, table.concat({ ... }, " "))
end
run(function()
    world:on("spawn", function(id) note("spawn", id) end)
    world:on("spawn", function(id) note("spawn2", id) end)
    world:on("hit", function(id, tier, src) note("hit", id, tier, src or "-") end)
    world:on("death", function(id, view, src)
        note("death", id, view.hp, view.x, src or "-")
        if id == 2 then
            world:spawn("dummy", nil, 50, 50, "wild")
        end
    end)
    world:spawn("dummy", { hp = 3 }, 10, 10, "host")
    world:spawn("dummy", nil, 30, 10, "host")
end, {
    [1] = function()
        eq(#log, 0, "events wait for the step")
    end,
    [2] = function()
        eq(table.concat(log, ","), "spawn 1,spawn2 1,spawn 2,spawn2 2", "spawns in order, handlers in order")
        table.clear(log)
        eq(world:damage(1, 1, 2), true, "the hit lands")
        eq(world:damage(1, 1), false, "hurt ticks")
        world:damage(2, 3)
    end,
    [3] = function()
        eq(table.concat(log, ","), "hit 1 1 2,hit 2 3 -,death 2 0 30 -,spawn 3,spawn2 3", "hit, death, handler spawn")
        eq(world:get(2), nil, "the dead leave")
        eq(world:get(3).faction, "wild", "spawned by a handler")
    end,
})
)lua")));
}

TEST(world_draws_layers_only_in_draw) {
    const WorldRun run = run_world("draw", kTestToml, R"lua(
local draw_error
run(function()
    world:spawn("dummy", { w = 4, h = 4, layer = "air", colour = 10 }, 10, 10, "host")
    world:spawn("dummy", { w = 8, h = 8, colour = 9 }, 10, 10, "host")
end, {
    [3] = function()
        has(draw_error, "can't be called from cart.draw()", "spawn in draw")
    end,
}, function()
    cls(0)
    world:draw()
    draw_error = draw_error or fails(world.spawn, world, "dummy", nil, 0, 0, "host")
end)
)lua");
    CHECK(wins(run));
    CHECK_EQ(run.fb.pget(10, 10), 10);  // air on top of the ground body drawn after it
    CHECK_EQ(run.fb.pget(6, 6), 9);
    CHECK_EQ(run.fb.pget(14, 14), 0);
}

TEST(world_entities_join_the_state_hash) {
    auto cart = [](const std::string& spawn) { return "run(function() " + spawn + " end, { [10] = function() end })"; };
    const std::string still = "world:spawn('dummy', nil, 100, 50, 'host')";
    const WorldRun a = run_world("hash_a", kTestToml, cart(still));
    const WorldRun b = run_world("hash_b", kTestToml, cart(still));
    const WorldRun moving = run_world("hash_c", kTestToml, cart("world:spawn('dummy', { vx = 1 }, 100, 50, 'host')"));
    const WorldRun empty = run_world("hash_d", kTestToml, cart(""));
    CHECK(wins(a) && wins(b) && wins(moving) && wins(empty));
    CHECK_EQ(a.hash, b.hash);
    CHECK(a.hash != moving.hash);
    CHECK(a.hash != empty.hash);
}

TEST(world_creates_the_avatar_from_the_manifest) {
    const std::string toml = Toml{.avatar = "8dir",
                                  .extra = "[avatar]\narchetype = \"dummy\"\ndamage = \"stun\"\nhp = 7\nx = 40\ny = 60\n"
                                           "params = { w = 16, h = 16 }\n"}
                                 .text();
    CHECK(wins(run_world("avatar", toml, R"lua(
local spawned = {}
run(function()
    world:on("spawn", function(id) table.insert(spawned, id) end)
    eq(world:avatar(), 1, "avatar id")
    local a = world:get(1)
    eq(a.faction, "player", "faction")
    eq(a.hp, 7, "hp")
    eq(a.x + a.y, 100, "position")
    eq(a.w, 16, "params")
end, {
    [2] = function()
        eq(spawned[1], 1, "the avatar's spawn event")
        world:damage(1, 30)
    end,
    [3] = function()
        eq(world:get(1).status.stun, 59, "the manifest's stun response")
        eq(world:get(1).hp, 7, "stun bodies keep their HP")
    end,
})
)lua")));
    const std::string bad = Toml{.avatar = "up",
                                 .extra = "[avatar]\narchetype = \"nope\"\ndamage = \"hp\"\nhp = 1\nx = 0\ny = 0\n"}
                                .text();
    const WorldRun run = run_world("avatar_bad", bad, "run(function() end, { [1] = function() end })");
    CHECK(run.outcome == Outcome::Error);
    CHECK(run.message.find("cart.toml: [avatar] archetype nope doesn't exist") != std::string::npos);
}
