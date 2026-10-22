#include <filesystem>
#include <string>

#include "bot.h"
#include "test.h"

using namespace mother;

// M6 acceptance: KNIGHTFALL and BREAKOUT can be won and lost. Bots play them through the pad.

namespace {

const std::filesystem::path kCarts = MOTHER_CARTS;
constexpr int kTenMinutes = 36000;

// Walks to the nearest skeleton and swings when close; taps A to start.
const char* const kKnightfallWinner = R"lua(
local function sgn(v)
    return if v > 2 then 1 elseif v < -2 then -1 else 0
end
return function(t)
    local tap = if t % 2 == 0 then "a" else ""
    local me = world:get(world:avatar())
    if me == nil then
        return 0, 0, ""
    end
    local id = world:nearest(me.x, me.y, { faction = "host", damageable = true })
    if id == nil then
        return 0, 0, tap
    end
    local s = world:get(id)
    local dx, dy = s.x - me.x, s.y - me.y
    return sgn(dx), sgn(dy), if dx * dx + dy * dy > 16 * 16 then "" else tap
end
)lua";

// Starts the game, then stands still.
const char* const kIdler = R"lua(
return function(t)
    return 0, 0, if t == 5 then "a" else ""
end
)lua";

// Keeps the paddle under the lowest ball, off-centre by a slowly changing amount so the ball
// leaves at different angles; taps A to start and to fire lasers. With away = true it does the
// opposite and keeps the paddle away from the ball (breakout_player prepends "local away = ...").
const char* const kBreakoutPlayer = R"lua(
return function(t)
    local tap = if t % 2 == 0 then "a" else ""
    local me = world:get(world:avatar())
    local ids = world:query(0, 0, 320, 180, { archetype = "ball" })
    if #ids == 0 then
        return 0, 0, tap
    end
    local b = world:get(ids[1])
    for i = 2, #ids do
        local v = world:get(ids[i])
        if v.y > b.y then
            b = v
        end
    end
    local aim = b.x + 10 * m.sin(t / 97)
    if away then
        aim = if b.x < 160 then 300 else 20
    end
    return math.clamp((aim - me.x) / 3, -1, 1), 0, tap
end
)lua";

std::string breakout_player(bool away) {
    return std::string("local away = ") + (away ? "true" : "false") + "\n" + kBreakoutPlayer;
}

}  // namespace

TEST(carts_knightfall_can_be_won) {
    const BotRun run = play(kCarts / "knightfall", kKnightfallWinner, kTenMinutes);
    CHECK(ended(run, Outcome::Win));
    const BotRun again = play(kCarts / "knightfall", kKnightfallWinner, kTenMinutes);
    CHECK(again.ticks == run.ticks && again.hash == run.hash);
}

TEST(carts_knightfall_can_be_lost) {
    CHECK(ended(play(kCarts / "knightfall", kIdler, kTenMinutes), Outcome::Lose));
}

TEST(carts_breakout_can_be_won) {
    const BotRun run = play(kCarts / "breakout", breakout_player(false), kTenMinutes);
    CHECK(ended(run, Outcome::Win));
    const BotRun again = play(kCarts / "breakout", breakout_player(false), kTenMinutes);
    CHECK(again.ticks == run.ticks && again.hash == run.hash);
}

TEST(carts_breakout_can_be_lost) {
    CHECK(ended(play(kCarts / "breakout", breakout_player(true), kTenMinutes), Outcome::Lose));
}
