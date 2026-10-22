#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "assets/sprites.h"
#include "gfx/draw.h"
#include "runner/program.h"
#include "vm/cart_vm.h"

namespace mother {

// A run (the run layer, DECISIONS.md): every cart in a folder once, in an order shuffled from the
// seed, which also seeds each cart. Between carts the host shows screens of its own. After each
// win but the last, the player takes one of that cart's exports along, or none, holding up to
// three; each later cart gets the held exports it can host, as `mother import` would give them.
// One loss ends the run. At the end, A starts a new run (seed + 1) and B quits with the result.
class Rogue : public Program {
public:
    enum class Screen : uint8_t { Intro, Next, Play, Loot, Swap, Over, Complete };

    static constexpr int kHold = 3;         // exports held at once: the import cap
    static constexpr int kInputDelay = 20;  // ticks each screen ignores input for

    struct Cart {
        std::string dir;    // folder name
        std::string title;
        std::string about;  // its family and objective, for the screens
        CartSource source;
    };

    // An export of one of the carts.
    struct Loot {
        int cart = 0;  // its cart's index
        ImportSource source;
        std::string kind;                // "tool" (equipment), "drop" (deployable), "creature" or "object"
        std::vector<std::string> unfit;  // per cart: why it can't host this export, "" when it can
        int icon = -1;                   // its icon's first tile in the screens' sheet 3; -1 for none
        int icon_w = 0, icon_h = 0;      // in tiles
    };

    // Reads every cart in dir (cart_folders) and starts a run; false with an error if it can't.
    bool open(const std::filesystem::path& dir, uint64_t seed, std::string& error);

    void update(const PadState& pad, uint64_t tick) override;
    void draw(Framebuffer& fb) override;
    const Status& status() const override { return status_; }
    const AudioSetup& audio_setup() const override;
    void take_audio(TickAudio& out) override;
    uint32_t audio_generation() const override { return audio_generation_; }
    uint64_t state_hash() const override { return state_hash_; }
    int budget_errors() const override { return budget_errors_ + (cart_ ? cart_->budget_errors() : 0); }

    Screen screen() const { return screen_; }
    uint64_t seed() const { return seed_; }
    const std::vector<Cart>& carts() const { return carts_; }
    const std::vector<Loot>& loot() const { return loot_; }
    const std::vector<int>& order() const { return order_; }  // cart indices in play order
    int stage() const { return stage_; }                      // the current cart's place in order()
    const std::vector<int>& held() const { return held_; }    // loot indices, in the order taken
    // The loot screen's cards: this cart's exports (loot indices), then one to skip. The swap
    // screen's are the held exports, then one to keep them and leave pending() behind.
    const std::vector<int>& offer() const { return offer_; }
    int pending() const { return pending_; }
    int cursor() const { return cursor_; }
    // The cart being played (null between carts); the next tick goes to it once it has started.
    CartVM* cart() { return cart_.get(); }
    bool cart_started() const { return cart_ != nullptr && !waiting_; }
    uint64_t cart_ticks() const { return cart_ticks_; }
    const std::vector<int>& playing_with() const { return playing_with_; }  // loot the cart got

private:
    void start(uint64_t seed);
    void show(Screen s);
    void play();
    void finished(Outcome outcome);
    void advance();  // on to the next cart's screen
    void take(int loot);
    void update_screen(const PadState& pad);
    void update_hash();
    void fail(std::string message);
    static std::string clock(uint64_t ticks);       // m:ss
    static std::string name(const std::string& id);  // an export id with spaces for underscores

    // rogue_screens.cpp
    void draw_screen(Framebuffer& fb);
    void draw_header();
    void draw_intro();
    void draw_next();
    void draw_loot();
    void draw_swap();
    void draw_end();
    void draw_cards(const std::vector<int>& cards, const char* last, int y);
    void draw_icon(const Loot& l, int cx, int cy, int scale);
    void draw_fit(const Loot& l, int y);  // whether the next cart, and how many of the rest, host it
    void centred(const std::string& text, int y, uint8_t c, int cx = kScreenW / 2);

    std::vector<Cart> carts_;
    std::vector<Loot> loot_;
    SpriteSheets icons_;
    Painter painter_;
    std::vector<int> order_, held_, offer_, playing_with_;
    std::unique_ptr<CartVM> cart_;
    Screen screen_ = Screen::Intro;
    uint64_t seed_ = 0;
    int stage_ = 0, cursor_ = 0;
    int pending_ = -1;      // the swap screen's new export
    int screen_ticks_ = 0;  // since the screen came up
    bool waiting_ = false;  // the cart is open, waiting for every face button to be up
    uint64_t cart_ticks_ = 0;
    uint64_t run_ticks_ = 0;  // carts' ticks this run, for its time
    Status status_;
    AudioSetup silence_;
    uint32_t audio_generation_ = 0;
    uint64_t state_hash_ = 0;
    int budget_errors_ = 0;  // carts' already closed
};

}  // namespace mother
