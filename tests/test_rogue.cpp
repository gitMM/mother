#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "bot.h"
#include "runner/rogue.h"
#include "test.h"

using namespace mother;

// The run layer (runner/rogue.h): every cart once in a seeded order, loot after each win, one loss
// ends it. Most cases end carts by hand (CartVM::finish) to reach the screens quickly; one plays
// a whole run with the carts' bots. With MOTHER_SNAPSHOTS set to a folder, the cases also write
// each screen they reach there as a PPM image.

namespace {

const std::filesystem::path kCarts = MOTHER_CARTS;
const std::filesystem::path kBots = MOTHER_BOTS;

std::string bot(const char* name, const std::string& prelude = "") {
    std::ifstream in(kBots / (std::string(name) + ".luau"), std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return prelude + text.str();
}

bool open(Rogue& r, uint64_t seed) {
    std::string error;
    if (r.open(kCarts, seed, error)) return true;
    std::printf("  open: %s\n", error.c_str());
    return false;
}

std::string snapshot_dir() {
#ifdef _MSC_VER
    char* dir = nullptr;
    size_t size = 0;
    if (_dupenv_s(&dir, &size, "MOTHER_SNAPSHOTS") != 0 || !dir) return "";
    const std::string out = dir;
    std::free(dir);
    return out;
#else
    const char* dir = std::getenv("MOTHER_SNAPSHOTS");
    return dir ? dir : "";
#endif
}

void snapshot(Rogue& r, const char* name) {
    const std::string dir = snapshot_dir();
    if (dir.empty()) return;
    Framebuffer fb;
    r.draw(fb);
    std::ofstream out(std::filesystem::path(dir) / (std::string(name) + ".ppm"), std::ios::binary);
    out << "P6 " << kScreenW << " " << kScreenH << " 255\n";
    for (int y = 0; y < kScreenH; ++y) {
        for (int x = 0; x < kScreenW; ++x) {
            const uint32_t c = kPalette[fb.pget(x, y)];
            out.put(char(c >> 16)).put(char(c >> 8)).put(char(c));
        }
    }
}

// Pads for a run, one tick at a time. Keys: a b x y l r s(tart), < and > for left and right.
struct Driver {
    Rogue& r;
    PadState pad;
    uint64_t tick = 0;

    void step(const char* keys = "") {
        RawInput raw;
        for (const char* k = keys; *k; ++k) {
            if (*k == '<') raw.left = true;
            if (*k == '>') raw.right = true;
            const Button b = *k == 'a' ? Button::A : *k == 'b' ? Button::B : *k == 's' ? Button::Start : Button::Count;
            if (b != Button::Count) raw.buttons[face_index(b)] = true;
        }
        pad = build_pad(raw, pad);
        r.update(pad, tick++);
    }

    // Waits out the screen's input delay, then presses and releases.
    void press(const char* keys) {
        for (int i = 0; i < Rogue::kInputDelay; ++i) step();
        step(keys);
        step();
    }

    // From the intro or a next-cart screen into the cart, started.
    void into_cart() {
        if (r.screen() == Rogue::Screen::Intro) press("a");
        press("a");
    }

    // Ends the cart being played with `outcome`.
    void end_cart(Outcome outcome) {
        r.cart()->finish(outcome);
        step();
    }
};

int index_in(const std::vector<int>& v, int x) {
    return int(std::find(v.begin(), v.end(), x) - v.begin());
}

}  // namespace

TEST(rogue_reads_every_cart_and_each_export_with_its_icon) {
    Rogue r;
    CHECK(open(r, 0));
    CHECK_EQ(r.carts().size(), size_t(6));
    CHECK_EQ(r.loot().size(), size_t(16));
    std::map<std::string, std::string> kinds;
    for (const Rogue::Loot& l : r.loot()) {
        CHECK(l.icon >= kCartTiles);
        CHECK(l.icon_w > 0 && l.icon_h > 0);
        kinds[l.source.label()] = l.kind;
    }
    CHECK_EQ(kinds["knightfall:sword"], std::string("tool"));
    CHECK_EQ(kinds["knightfall:bomb"], std::string("drop"));
    CHECK_EQ(kinds["knightfall:skeleton"], std::string("creature"));
    CHECK_EQ(kinds["breakout:ball"], std::string("object"));
}

TEST(rogue_order_is_a_seeded_shuffle_of_every_cart) {
    std::set<std::vector<int>> orders;
    for (uint64_t seed = 0; seed < 8; ++seed) {
        Rogue a, b;
        CHECK(open(a, seed) && open(b, seed));
        CHECK(a.order() == b.order());
        std::vector<int> sorted = a.order();
        std::sort(sorted.begin(), sorted.end());
        CHECK(sorted == (std::vector<int>{0, 1, 2, 3, 4, 5}));
        orders.insert(a.order());
    }
    CHECK(orders.size() >= 6);
}

// The run asks MOTHERLIB which cart takes which export; that must be the import matrix.
TEST(rogue_fits_match_the_import_matrix) {
    Rogue r;
    CHECK(open(r, 0));
    std::ifstream in(kBots.parent_path() / "matrix_expected.md", std::ios::binary);
    std::vector<std::vector<std::string>> rows;
    for (std::string line; std::getline(in, line);) {
        if (!line.starts_with("| ") || line.starts_with("| Export") || line.starts_with("| ---")) continue;
        std::vector<std::string> cells;
        std::istringstream cols(line);
        for (std::string cell; std::getline(cols, cell, '|');) {
            cell.erase(0, cell.find_first_not_of(' '));
            cell.erase(cell.find_last_not_of(" \r") + 1);
            cells.push_back(cell);
        }
        rows.push_back({cells.begin() + 2, cells.end()});  // past the empty cell and the export id
    }
    CHECK_EQ(rows.size(), r.loot().size());
    for (size_t i = 0; i < rows.size() && i < r.loot().size(); ++i) {
        const Rogue::Loot& l = r.loot()[i];
        for (size_t c = 0; c < r.carts().size(); ++c) {
            const std::string want = int(c) == l.cart ? "H" : l.unfit[c].empty() ? "✓" : "✗";
            if (rows[i][c] != want) std::printf("  %s in %s\n", l.source.label().c_str(), r.carts()[c].dir.c_str());
            CHECK_EQ(rows[i][c], want);
        }
    }
}

TEST(rogue_screens_wait_then_the_cart_starts_once_the_buttons_are_up) {
    Rogue r;
    CHECK(open(r, 0));
    Driver d{r};
    CHECK(r.screen() == Rogue::Screen::Intro);
    snapshot(r, "intro");
    for (int i = 0; i < Rogue::kInputDelay; ++i) d.step("a");  // too soon: ignored
    CHECK(r.screen() == Rogue::Screen::Intro);
    d.step();
    d.step("a");
    CHECK(r.screen() == Rogue::Screen::Next);
    snapshot(r, "next_first");

    const uint32_t silent = r.audio_generation();
    for (int i = 0; i < Rogue::kInputDelay; ++i) d.step();
    d.step("a");
    CHECK(r.screen() == Rogue::Screen::Play);
    CHECK(r.cart() != nullptr && !r.cart_started());
    CHECK(r.audio_generation() != silent);
    CHECK(&r.audio_setup() == &r.cart()->audio_setup());
    d.step("a");  // still held: waits
    CHECK(!r.cart_started());
    d.step();
    CHECK(r.cart_started());
    CHECK_EQ(r.cart_ticks(), uint64_t(0));
    d.step();
    d.step();
    CHECK_EQ(r.cart_ticks(), uint64_t(2));

    const uint32_t playing = r.audio_generation();
    d.end_cart(Outcome::Win);
    CHECK(r.cart() == nullptr);
    CHECK(r.audio_generation() != playing);
    CHECK(r.screen() == Rogue::Screen::Loot);
}

// Wins every cart by hand, taking the first card each time (dropping the oldest when full): the
// loot screen offers the cart's own exports, and each cart gets exactly the held ones it hosts.
TEST(rogue_loot_goes_along_into_the_carts_that_can_host_it) {
    Rogue r;
    CHECK(open(r, 0));
    Driver d{r};
    int left_out = 0, taken_in = 0, swaps = 0;
    for (size_t stage = 0; stage < r.order().size(); ++stage) {
        if (stage == 4) snapshot(r, "next_with_loot");
        d.into_cart();
        CHECK(r.cart_started());
        CHECK_EQ(r.stage(), int(stage));
        const int c = r.order()[stage];
        std::vector<int> want;
        for (int l : r.held()) {
            if (r.loot()[size_t(l)].unfit[size_t(c)].empty()) {
                want.push_back(l);
            } else {
                ++left_out;
            }
        }
        CHECK(r.playing_with() == want);
        taken_in += int(want.size());
        d.end_cart(Outcome::Win);
        if (stage + 1 == r.order().size()) break;

        CHECK(r.screen() == Rogue::Screen::Loot);
        CHECK(!r.offer().empty());
        for (int l : r.offer()) CHECK_EQ(r.loot()[size_t(l)].cart, c);
        if (stage == 0) snapshot(r, "loot");
        const int pick = r.offer()[0];
        const std::vector<int> before = r.held();
        d.press("a");
        if (before.size() < size_t(Rogue::kHold)) {
            CHECK(r.screen() == Rogue::Screen::Next);
            CHECK_EQ(r.held().back(), pick);
            continue;
        }
        CHECK(r.screen() == Rogue::Screen::Swap);
        CHECK_EQ(r.pending(), pick);
        snapshot(r, "swap");
        d.press("a");  // drops the first held
        ++swaps;
        CHECK(r.screen() == Rogue::Screen::Next);
        CHECK(r.held() == (std::vector<int>{before[1], before[2], pick}));
    }
    CHECK(r.screen() == Rogue::Screen::Complete);
    snapshot(r, "complete");
    CHECK(left_out > 0);
    CHECK(taken_in > 0);
    CHECK_EQ(swaps, 2);
    d.press("b");
    CHECK(r.status().outcome == Outcome::Win);
}

TEST(rogue_skip_and_back_leave_the_held_exports_alone) {
    uint64_t seed = 0;  // the first whose fourth cart has two exports or more (BLOCKFALL has one)
    for (;; ++seed) {
        Rogue probe;
        CHECK(open(probe, seed));
        if (probe.carts()[size_t(probe.order()[3])].dir != "blockfall") break;
    }
    Rogue r;
    CHECK(open(r, seed));
    Driver d{r};
    for (int stage = 0; stage < 3; ++stage) {  // three wins, three exports taken
        d.into_cart();
        d.end_cart(Outcome::Win);
        d.press("a");
    }
    const std::vector<int> three = r.held();
    CHECK_EQ(three.size(), size_t(3));

    d.into_cart();
    d.end_cart(Outcome::Win);
    CHECK(r.screen() == Rogue::Screen::Loot);
    d.press(">");
    CHECK_EQ(r.cursor(), 1);
    const int second = r.offer()[1];
    d.press("a");
    CHECK(r.screen() == Rogue::Screen::Swap);
    d.press("b");  // back to the loot, on the same card
    CHECK(r.screen() == Rogue::Screen::Loot);
    CHECK_EQ(r.offer()[size_t(r.cursor())], second);
    d.press("a");
    d.press("<");  // the keep card
    CHECK_EQ(r.cursor(), 3);
    d.press("a");
    CHECK(r.screen() == Rogue::Screen::Next);
    CHECK(r.held() == three);

    d.into_cart();
    d.end_cart(Outcome::Win);
    d.press("b");  // skip
    CHECK(r.screen() == Rogue::Screen::Next);
    CHECK(r.held() == three);
}

TEST(rogue_a_loss_ends_the_run_and_a_starts_another) {
    Rogue r;
    CHECK(open(r, 5));
    Driver d{r};
    d.into_cart();
    d.end_cart(Outcome::Win);
    d.press("a");
    d.into_cart();
    d.end_cart(Outcome::Lose);
    CHECK(r.screen() == Rogue::Screen::Over);
    snapshot(r, "over");
    const std::vector<int> first = r.order();
    d.press("a");
    CHECK(r.screen() == Rogue::Screen::Intro);
    CHECK_EQ(r.seed(), uint64_t(6));
    CHECK(r.held().empty());
    CHECK_EQ(r.stage(), 0);
    Rogue six;
    CHECK(open(six, 6));
    CHECK(r.order() == six.order());
    CHECK(r.order() != first);

    d.into_cart();
    d.end_cart(Outcome::Lose);
    d.press("b");
    CHECK(r.status().outcome == Outcome::Lose);
}

// The carts' own bots play a whole run from seed 0, skipping the loot so every cart runs as the
// bot knows it (seed 0 too); each bot starts on a fresh pad, as in unit.carts.
TEST(rogue_bots_win_a_whole_run) {
    const std::map<std::string, std::string> bots = {
        {"blockfall", bot("blockfall_stacker")},     {"breakout", bot("breakout_player", "local away = false\n")},
        {"jumper", bot("jumper_runner")},            {"knightfall", bot("knightfall_winner")},
        {"rally", bot("rally_driver")},              {"starfighter", bot("starfighter_pilot")}};
    Rogue r;
    CHECK(open(r, 0));
    Bot player;
    int bound = -1;  // the stage the bot was loaded for
    PadState pad;
    std::string error;
    uint64_t tick = 0;
    int carts = 0;
    while (r.status().outcome == Outcome::Running && r.screen() != Rogue::Screen::Over && tick < 400000) {
        if (r.cart_started()) {
            if (r.stage() != bound) {
                bound = r.stage();
                ++carts;
                CHECK(player.load(*r.cart(), bots.at(r.carts()[size_t(r.order()[size_t(r.stage())])].dir), error));
            }
            if (!player.pad(int(r.cart_ticks()), pad, pad, error)) break;
        } else {
            RawInput raw;
            const bool skip = r.screen() == Rogue::Screen::Loot || r.screen() == Rogue::Screen::Complete;
            if (tick % 2 == 0) raw.buttons[face_index(skip ? Button::B : Button::A)] = true;
            pad = build_pad(raw, pad);
        }
        r.update(pad, tick++);
    }
    if (!error.empty()) std::printf("  %s\n", error.c_str());
    CHECK(error.empty());
    CHECK_EQ(carts, 6);
    CHECK(r.status().outcome == Outcome::Win);
    std::printf("  won a run in %llu ticks\n", static_cast<unsigned long long>(tick));
}
