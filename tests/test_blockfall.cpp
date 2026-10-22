#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

// BLOCKFALL's archetypes: falling_piece and piece_controller.

namespace {

const std::string kToml = Toml{.gravity = "down"}.text();

// A 12x22 map: a well (columns 1-10, rows 1-20) between critical walls (tile 8) and a critical
// floor (row 21). Tiles 1-7 are the pieces' cells: solid and damageable.
std::string well_map() {
    std::string text = "layer 0 12 22\n";
    for (int y = 0; y < 22; ++y) {
        for (int x = 0; x < 12; ++x) {
            text += (x == 0 || x == 11 || y == 21) ? "0008" : "0000";
            text += x < 11 ? ' ' : '\n';
        }
    }
    text += "flags\n";
    for (int t = 0; t < 3072; ++t) text += t >= 1 && t <= 7 ? "03\n" : t == 8 ? "05\n" : "00\n";
    return text;
}

const TempCart::Files kWell = {{"map.map", well_map()}};

const char* const kController = R"lua(
local WELL = { well_x = 1, well_y = 1, well_w = 10, well_h = 20, tiles = 1, fall = 48 }
local function row_text(y)
    local s = ""
    for x = 1, 10 do
        s ..= tostring(mget(x, y, 0))
    end
    return s
end
)lua";

}  // namespace

TEST(blockfall_piece_steps_down_the_grid_and_lands_as_tiles) {
    CHECK(wins(run_world("piece_lands", kToml, R"lua(
run(function()
    world:spawn("falling_piece", { shape = 2, fall = 4, tiles = 1 }, 5 * 8, 1 * 8, "player")
end, {
    [2] = function()
        local p = world:get(1)
        eq(p.shape, 2, "an O")
        eq(p.col, 5, "the column of its spawn point")
        eq(p.row, 1, "the row of its spawn point")
        eq(p.w, 16, "its collider covers its cells")
    end,
    [5] = function() eq(world:get(1).row, 2, "a cell after 4 steps") end,
    [73] = function() eq(world:get(1).row, 19, "resting on the floor") end,
    [75] = function()
        eq(world:get(1), nil, "landed")
        for _, c in { { 5, 19 }, { 6, 19 }, { 5, 20 }, { 6, 20 } } do
            eq(mget(c[1], c[2], 0), 2, `cell {c[1]},{c[2]} is the O's tile (tiles + shape - 1)`)
        end
        eq(mget(4, 20, 0), 0, "and nothing else")
    end,
})
)lua", 600, kWell)));
}

TEST(blockfall_piece_waits_out_its_lock_delay) {
    CHECK(wins(run_world("piece_lock", kToml, R"lua(
run(function()
    world:spawn("falling_piece", { shape = 2, fall = 1, lock = 30, tiles = 1 }, 5 * 8, 1 * 8, "player")
end, {
    [20] = function() eq(world:get(1).row, 19, "at rest after 18 cells") end,
    [48] = function() check(world:get(1) ~= nil, "still waiting after 29 ticks at rest") end,
    [50] = function()
        eq(world:get(1), nil, "landed after 30")
        eq(mget(5, 20, 0), 2, "as tiles")
    end,
})
)lua", 600, kWell)));
}

TEST(blockfall_controller_deals_7_bags_and_clears_full_rows) {
    // Each piece: four taps to one wall (left, then right, by turns), then a hard drop.
    const InputFn input = [](int t) {
        const int k = (t - 10) / 30, at = (t - 10) % 30;
        if (t < 10 || k > 8) return hold("");
        if (at < 8) return hold(at % 2 == 0 ? (k % 2 == 0 ? "<" : ">") : "");
        return hold(at == 10 ? "^" : "");
    };
    CHECK(wins(run_world("controller", kToml, std::string(kController) + R"lua(
local shapes, controller = {}, nil
run(function()
    for x = 1, 10 do
        mset(x, 20, 3, 0) -- a full bottom row
        mset(x, 19, if x == 10 then 0 else 5, 0) -- and one with a hole on the right
    end
    controller = world:spawn("piece_controller", WELL, 200, 40, "player")
    world:on("spawn", function(id)
        local v = world:get(id)
        if v.archetype == "falling_piece" then
            table.insert(shapes, v.shape)
        end
    end)
end, {
    [30] = function()
        local c = world:get(controller)
        eq(c.lines, 1, "the full row cleared once the first piece landed")
        eq(row_text(20), "5555555550", "the row above moved down, hole and all")
    end,
    [260] = function()
        check(#shapes >= 8, "8 pieces dealt")
        local seen = {}
        for i = 1, 7 do
            seen[shapes[i]] = true
        end
        for s = 1, 7 do
            check(seen[s], `shape {s} in the first 7 (one bag)`)
        end
        check(world:get(controller) ~= nil, "no top-out")
    end,
})
)lua", 600, kWell, input)));
}

TEST(blockfall_controller_moves_rotates_and_soft_drops) {
    const InputFn input = [](int t) {
        if (t >= 5 && t < 25) return hold(t % 2 == 0 ? "<" : "");  // to the left wall
        if (t == 30) return hold("a");
        if (t >= 40 && t < 60) return hold("v");
        return hold("");
    };
    CHECK(wins(run_world("controls", kToml, std::string(kController) + R"lua(
local controller
run(function()
    controller = world:spawn("piece_controller", WELL, 200, 40, "player")
end, {
    [29] = function()
        local p = world:get(world:get(controller).piece)
        eq(p.rot, 0, "not turned yet")
        eq(p.col, 1, "pushed against the left wall (every shape's rotation 0 starts at its box's left)")
    end,
    [32] = function()
        local p = world:get(world:get(controller).piece)
        eq(p.rot, 1, "turned clockwise (with a kick if the wall was in the way)")
        check(p.col >= 0, "inside the well")
    end,
    [61] = function()
        local p = world:get(world:get(controller).piece)
        eq(p.row, 11, "soft drop: a cell every 2 ticks for 20 ticks")
    end,
})
)lua", 600, kWell, input)));
}

TEST(blockfall_controller_dies_when_a_piece_cannot_appear) {
    CHECK(wins(run_world("top_out", kToml, std::string(kController) + R"lua(
local controller, died = nil, nil
run(function()
    for x = 1, 10 do
        mset(x, 1, 4, 0)
    end
    controller = world:spawn("piece_controller", WELL, 200, 40, "player")
    world:on("death", function(id) died = id end)
end, {
    [5] = function()
        eq(died, controller, "topped out")
        eq(world:get(controller), nil, "gone")
    end,
})
)lua", 600, kWell)));
}
