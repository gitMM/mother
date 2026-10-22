#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "bot.h"
#include "test.h"

using namespace mother;

// M6 and M7 acceptance: every cart can be won and lost. Bots (tests/bots/*.luau) play them through
// the pad; each win is played twice and must end on the same tick with the same state hash.

namespace {

const std::filesystem::path kCarts = MOTHER_CARTS;
const std::filesystem::path kBots = MOTHER_BOTS;
constexpr int kTenMinutes = 36000;

std::string bot(const char* name, const std::string& prelude = "") {
    std::ifstream in(kBots / (std::string(name) + ".luau"), std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return prelude + text.str();
}

// A win, twice alike.
bool wins_twice(const char* cart, const std::string& source) {
    const BotRun run = play(kCarts / cart, source, kTenMinutes);
    if (!ended(run, Outcome::Win)) return false;
    std::printf("  %s won after %llu ticks\n", cart, static_cast<unsigned long long>(run.ticks));
    const BotRun again = play(kCarts / cart, source, kTenMinutes);
    return again.ticks == run.ticks && again.hash == run.hash;
}

bool loses(const char* cart, const std::string& source) {
    return ended(play(kCarts / cart, source, kTenMinutes), Outcome::Lose);
}

}  // namespace

TEST(carts_knightfall_can_be_won) {
    CHECK(wins_twice("knightfall", bot("knightfall_winner")));
}

TEST(carts_knightfall_can_be_lost) {
    CHECK(loses("knightfall", bot("idler")));
}

TEST(carts_breakout_can_be_won) {
    CHECK(wins_twice("breakout", bot("breakout_player", "local away = false\n")));
}

TEST(carts_breakout_can_be_lost) {
    CHECK(loses("breakout", bot("breakout_player", "local away = true\n")));
}

TEST(carts_blockfall_can_be_won) {
    CHECK(wins_twice("blockfall", bot("blockfall_stacker")));
}

TEST(carts_blockfall_can_be_lost) {
    CHECK(loses("blockfall", bot("idler")));
}

TEST(carts_starfighter_can_be_won) {
    CHECK(wins_twice("starfighter", bot("starfighter_pilot")));
}

TEST(carts_starfighter_can_be_lost) {
    CHECK(loses("starfighter", bot("idler")));
}

TEST(carts_jumper_can_be_won) {
    CHECK(wins_twice("jumper", bot("jumper_runner")));
}

TEST(carts_jumper_can_be_lost) {
    CHECK(loses("jumper", bot("idler")));
}

TEST(carts_rally_can_be_won) {
    CHECK(wins_twice("rally", bot("rally_driver")));
}

TEST(carts_rally_can_be_lost) {
    CHECK(loses("rally", bot("idler")));
}
