#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

TEST(damage_contact_only_across_factions) {
    CHECK(wins(run_world("factions", kTestToml, R"lua(
local hits = {}
run(function()
    world:on("hit", function(id, tier, src) table.insert(hits, `{id}<{src}`) end)
    world:spawn("dummy", { w = 2, h = 2, layer = "projectile", tier = 1, vx = 4 }, 10, 20, "player")
    world:spawn("dummy", { hp = 3 }, 40, 20, "player") -- 2: same faction as the shot
    world:spawn("dummy", { hp = 3 }, 80, 20, "host") -- 3
    world:spawn("dummy", { w = 2, h = 2, layer = "projectile", tier = 3, hurts_owner = true, vx = 4 }, 10, 60, "player")
    world:spawn("dummy", { hp = 3 }, 40, 60, "player") -- 5: hit by its own side's shot
    world:spawn("dummy", { tier = 10, hp = 30 }, 200, 100, "wild") -- 6
    world:spawn("dummy", { tier = 1, hp = 30, vx = -2 }, 220, 100, "host") -- 7: they hurt each other once
end, {
    [40] = function()
        eq(table.concat(hits, " "), "5<4 7<6 6<7 3<1", "who hit whom, in order")
        eq(world:get(2).hp, 3, "same faction is safe")
        eq(world:get(3).hp, 2, "other faction is hit")
        eq(world:get(5), nil, "hurts_owner hits its own faction")
        eq(world:get(6).hp, 29, "contact damage goes both ways")
        eq(world:get(7).hp, 20, "tier 10")
    end,
})
)lua")));
}

TEST(damage_hp_stun_none_and_hurt_ticks) {
    CHECK(wins(run_world("responses", kTestToml, R"lua(
local deaths = {}
run(function()
    world:on("death", function(id, view, src) table.insert(deaths, `{id}:{view.hp}:{src}`) end)
    world:spawn("dummy", { hp = 30 }, 20, 20, "host")
    world:spawn("dummy", { damage = "stun" }, 60, 20, "host")
    world:spawn("dummy", { damage = "none", hp = 0 }, 100, 20, "host")
    world:spawn("dummy", nil, 140, 20, "player")
end, {
    [1] = function()
        eq(world:damage(1, 10, 4), true, "an hp hit")
        eq(world:get(1).hp, 20, "HP lost")
        eq(world:damage(1, 10), false, "no damage during the hurt ticks")
        eq(world:damage(2, 30), true, "a stun hit")
        eq(world:get(2).status.stun, 60, "stunned for 60 ticks")
        eq(world:get(2).hp, 1, "stun bodies keep their HP")
        eq(world:damage(3, 30), false, "immune")
        eq(world:damage(99, 1), false, "no such entity")
    end,
    [30] = function()
        eq(world:damage(1, 10), false, "still hurt after 29 steps")
    end,
    [31] = function()
        eq(world:damage(1, 30, 4), true, "the hurt ticks are over after 30 steps")
    end,
    [32] = function()
        eq(table.concat(deaths, " "), "1:0:4", "died at 0 HP, with the source")
        eq(world:get(1), nil, "the dead leave")
        eq(world:get(2).status.stun, 29, "the stun counts down")
    end,
})
)lua")));
}

// stun stops a body; knockback pushes it away from its attacker at run speed; slip keeps the
// velocity it has (here the knockback's) and ignores its steering until it ends.
TEST(damage_statuses_stun_knockback_slip) {
    CHECK(wins(run_world("statuses", kTestToml, R"lua(
local m
run(function()
    m = world:spawn("dummy", { vx = 1, hp = 30 }, 100, 50, "host")
    world:spawn("dummy", nil, 60, 50, "player")
end, {
    [1] = function()
        world:status(m, "stun", 3)
    end,
    [2] = function()
        local v = world:get(m)
        eq(v.vx, 0, "stunned: no motion")
        eq(v.x, 100, "stayed")
        eq(v.status.stun, 2, "the stun counts down")
    end,
    [4] = function()
        eq(world:get(m).status.stun, 0, "stun over")
        eq(world:get(m).x, 100, "still for 3 steps")
    end,
    [5] = function()
        eq(world:get(m).x, 101, "acts again")
        world:damage(m, 1, 2)
        world:status(m, "knockback", 4)
        world:status(m, "slip", 8)
    end,
    [6] = function()
        eq(world:get(m).vx, 2, "knocked away from the attacker at run speed")
        eq(world:get(m).x, 103, "moved")
    end,
    [10] = function()
        local v = world:get(m)
        eq(v.x, 111, "slid on after the knockback")
        eq(v.vx, 2, "slip keeps the velocity")
        world:status(m, "slip", 1)
        eq(world:get(m).status.slip, 3, "a status is never shortened")
    end,
    [13] = function()
        eq(world:get(m).x, 117, "slipping")
    end,
    [14] = function()
        eq(world:get(m).x, 118, "steering again")
        eq(world:get(m).vx, 1, "own velocity")
    end,
})
)lua")));
}
