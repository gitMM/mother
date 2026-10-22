#include <filesystem>
#include <string>

#include "bot.h"
#include "test.h"
#include "vm/imports.h"

using namespace mother;

// The import harness (brief section 9): loading exports, copying their art and sounds, and how
// MOTHERLIB runs them in another cart (core/imports.luau). Bots check what they see and raise an
// error when it's wrong.

namespace {

const std::filesystem::path kCarts = MOTHER_CARTS;

// Ends without a bot error, having reached `ticks`.
bool clean(const BotRun& run, uint64_t ticks) {
    if (run.message.empty() && run.ticks == ticks) return true;
    std::printf("  ended %s after %llu ticks: %s\n", result_name(run.outcome),
                static_cast<unsigned long long>(run.ticks), run.message.c_str());
    return false;
}

bool has_gold(const Framebuffer& fb) {
    for (int y = 0; y < kScreenH; ++y) {
        for (int x = 0; x < kScreenW; ++x) {
            if (fb.pget(x, y) == kMotherGold) return true;
        }
    }
    return false;
}

}  // namespace

TEST(import_copies_tile_blocks_into_sheet_3_once) {
    ImportSource src;
    src.cart = "src";
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 16; ++x) src.art.sheets.set(5 * 8 + x, y, uint8_t(1 + (x + y) % 60));
    }
    SpriteSheets dst;
    ImportAssets assets;
    CHECK_EQ(assets.copy_tiles(src, 5, 2, 1, dst), 3072);
    CHECK_EQ(assets.copy_tiles(src, 5, 2, 1, dst), 3072);  // already there
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 16; ++x) CHECK_EQ(dst.get(x, 768 + y), src.art.sheets.get(40 + x, y));
    }
    CHECK_EQ(assets.copy_tiles(src, 40, 3, 2, dst), 3074);  // packed to its right
    CHECK_EQ(assets.copy_tiles(src, 100, 30, 1, dst), 3072 + 2 * 32);  // a new row under the tallest
    CHECK_EQ(assets.copy_tiles(src, 3072, 1, 1, dst), -1);             // only sheets 0-2
}

TEST(import_copies_sfx_with_their_instruments) {
    ImportSource src;
    src.cart = "src";
    src.tracker.instruments[0].volume = 11;
    src.tracker.instruments[3].volume = 33;
    Sfx& s = src.tracker.sfx[7];
    s.speed = 2;
    s.rows.resize(3);
    s.rows[0].note = 40;  // relies on the default instrument 0
    s.rows[1].note = 42, s.rows[1].inst = 3;
    s.rows[2].note = kNoteOff;
    TrackerData dst;
    ImportAssets assets;
    CHECK_EQ(assets.copy_sfx(src, 7, dst), kImportSfx);
    const Sfx& copy = dst.sfx[kImportSfx];
    CHECK_EQ(copy.speed, 2);
    CHECK_EQ(int(copy.rows[0].inst), kImportInstruments);
    CHECK_EQ(int(copy.rows[1].inst), kImportInstruments + 1);
    CHECK_EQ(int(copy.rows[2].inst), -1);
    CHECK_EQ(int(dst.instruments[kImportInstruments].volume), 11);
    CHECK_EQ(int(dst.instruments[kImportInstruments + 1].volume), 33);
    CHECK_EQ(assets.copy_sfx(src, 7, dst), kImportSfx);  // already there
    CHECK_EQ(assets.copy_sfx(src, 9, dst), -1);          // no sfx 9
}

TEST(import_reads_exports_and_says_why_a_cart_refuses_one) {
    ImportSource src;
    std::string error;
    CHECK(!load_import(kCarts / "knightfall", "breakout", src, error));
    CHECK(error.find("SOURCE:EXPORT") != std::string::npos);
    CHECK(!load_import(kCarts / "knightfall", "breakout:brick", src, error));
    CHECK(error.find("has no [[exports]] with id brick") != std::string::npos);
    CHECK(load_import(kCarts / "knightfall", "breakout:laser", src, error));
    CHECK_EQ(src.archetype, std::string("laser"));
    CHECK_EQ(src.params.find("sprite")->integer, int64_t(44));

    const BotRun refused = play(kCarts / "breakout", "return function() return 0, 0, '' end", 10, 0, {"jumper:walker"});
    CHECK_EQ(refused.refused, std::string("needs surface=ground, host has none"));
    const BotRun body = play(kCarts / "knightfall", "return function() return 0, 0, '' end", 10, 0,
                             {"breakout:ball", "breakout:laser", "breakout:paddle", "jumper:fire_flower"});
    CHECK(body.message.find("at most 3 exports") != std::string::npos);
}

TEST(import_equipment_is_held_and_used_with_x_on_voices_7_and_8) {
    const char* bot = R"lua(
local shots = 0
return function(t)
    local lasers = world:query(0, 0, 320, 180, { archetype = "laser" })
    if t == 2 then
        if #lasers ~= 1 then error("one imported laser") end
        local l = world:get(lasers[1])
        local k = world:get(world:avatar())
        if not l.imported or l.faction ~= "player" or l.x ~= k.x or l.y ~= k.y then error("held by the knight") end
    end
    for _, id in world:query(0, 0, 320, 180, { archetype = "shot" }) do
        local s = world:get(id)
        if not s.imported or s.faction ~= "player" then error("its shots are imported, the player's") end
        shots += 1
    end
    if t == 60 and shots == 0 then error("X never fired it") end
    return 0, 0, if t == 10 then "x" else ""
end
)lua";
    const BotRun run = play(kCarts / "knightfall", bot, 61, 0, {"breakout:laser"});
    CHECK(clean(run, 61));
    CHECK((run.voices & 0xC0) != 0);    // its fire sfx on voice 7 or 8
}

TEST(import_deployables_drop_behind_the_avatar_on_y) {
    const char* bot = R"lua(
return function(t)
    local bombs = world:query(0, 0, 320, 180, { archetype = "bomb" })
    if t == 9 and #bombs ~= 0 then error("nothing before Y") end
    if t == 12 then
        if #bombs ~= 1 then error("Y dropped one") end
        local b, p = world:get(bombs[1]), world:get(world:avatar())
        if not b.imported or b.faction ~= "player" then error("an imported player-faction bomb") end
        if not (b.y > p.y) then error("behind the paddle, which faces up") end
    end
    return 0, 0, if t == 10 then "y" else ""
end
)lua";
    CHECK(clean(play(kCarts / "breakout", bot, 13, 0, {"knightfall:bomb"}), 13));
}

TEST(import_creatures_come_from_spawn_points_every_300_ticks) {
    const char* bot = R"lua(
local seen, count = {}, 0 -- skeletons ever seen (one may die on the parked car)
return function(t)
    local new = nil
    for _, id in world:query(0, 0, 960, 640, { archetype = "skeleton" }) do
        if not seen[id] then
            seen[id], count, new = true, count + 1, world:get(id)
        end
    end
    if t == 1 then
        if count ~= 1 then error(`one skeleton at once, saw {count}`) end
        if new.faction ~= "wild" or not new.imported then error("an imported wild skeleton") end
        if new.x ~= 264 or new.y ~= 68 then error(`at spawn point 1, got {new.x},{new.y}`) end
    elseif t == 300 and count ~= 1 then
        error("no second one yet")
    elseif t == 301 then
        if count ~= 2 then error("a second one 300 ticks on") end
        if new.x ~= 264 or new.y ~= 100 then error("at spawn point 2") end
    end
    return 0, 0, ""
end
)lua";
    CHECK(clean(play(kCarts / "rally", bot, 302, 0, {"knightfall:skeleton"}), 302));
}

TEST(import_auto_objects_start_at_once_with_a_mother_gold_outline) {
    const char* bot = R"lua(
return function(t)
    local balls = world:query(0, 0, 320, 180, { archetype = "ball" })
    if #balls ~= 1 then error("one ball from the start") end
    local b = world:get(balls[1])
    if b.faction ~= "wild" or not b.imported then error("an imported wild ball") end
    return 0, 0, ""
end
)lua";
    const BotRun run = play(kCarts / "blockfall", bot, 30, 0, {"breakout:ball"});
    CHECK(clean(run, 30));
    CHECK(has_gold(run.fb));
    // Without the import, BLOCKFALL draws no Mother Gold.
    CHECK(!has_gold(play(kCarts / "blockfall", "return function() return 0, 0, '' end", 30).fb));
}

// Mines behind JUMPER's title screen: its 18 spawn points are apart, so they never touch.
TEST(import_instances_are_capped_at_8) {
    const char* bot = R"lua(
return function(t)
    local n = #world:query(0, 0, 1600, 176, { archetype = "mine" })
    if n > 8 then error(`{n} imported mines alive`) end
    if t == 2800 and n ~= 8 then error(`8 mines by now, saw {n}`) end
    return 0, 0, ""
end
)lua";
    CHECK(clean(play(kCarts / "jumper", bot, 2801, 0, {"starfighter:mine"}), 2801));
}

TEST(import_falling_pieces_land_as_sheet_3_tiles_that_decay) {
    const char* bot = R"lua(
local landed = nil
local function imported_cells()
    local n = 0
    for y = 0, 21 do
        for x = 0, 20 do
            local t = mget(x, y, 0)
            if t >= 3072 then
                n += 1
                if not (fget(t, 0) and fget(t, 1)) then error("flagged solid and damageable") end
            end
        end
    end
    return n
end
return function(t)
    local n = imported_cells()
    if landed == nil and n > 0 then
        if n ~= 4 then error(`4 cells, saw {n}`) end
        landed = t
    elseif landed ~= nil and t < landed + 1195 and n ~= 4 then
        error("the cells stay until they decay")
    elseif landed ~= nil and t > landed + 1205 and n ~= 0 then
        error("decayed after 1200 ticks")
    end
    if t == 2400 and (landed == nil or n ~= 0) then error("landed and decayed by now") end
    return 0, 0, ""
end
)lua";
    CHECK(clean(play(kCarts / "jumper", bot, 2401, 0, {"blockfall:falling_piece"}), 2401));
}
