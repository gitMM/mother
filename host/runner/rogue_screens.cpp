// The run's screens between carts (DECISIONS.md, run layer), drawn with the built-in font and the
// exports' own art. Text is written in lowercase: the font draws it in capitals.

#include <algorithm>

#include "gfx/font.h"
#include "runner/rogue.h"

namespace mother {

namespace {

// AAP-64 indices.
constexpr uint8_t kBack = 1;
constexpr uint8_t kPanel = 30;
constexpr uint8_t kEdge = 41;
constexpr uint8_t kDim = 40;
constexpr uint8_t kText = 37;
constexpr uint8_t kWhite = 22;
constexpr uint8_t kGold = 8;
constexpr uint8_t kRed = 4;
constexpr uint8_t kRedDark = 3;
constexpr uint8_t kGreen = 12;
constexpr uint8_t kGreenDark = 14;
constexpr uint8_t kBlue = 19;
constexpr uint8_t kOrange = 6;

constexpr int kCardW = 60, kCardH = 54, kCardGap = 8;

std::string spaced(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i > 0) out += ' ';
        out += s[i];
    }
    return out;
}

uint8_t kind_colour(const std::string& kind) {
    return kind == "tool" ? kBlue : kind == "drop" ? kGreen : kOrange;
}

const char* kind_label(const std::string& kind) {
    return kind == "tool" ? "tool" : kind == "drop" ? "drop" : "wild";
}

// What it does in a cart, short (the next-cart list) and long (the loot screen).
const char* kind_use(const std::string& kind) {
    if (kind == "tool") return "use it with x (key c)";
    if (kind == "drop") return "drop one with y (key v)";
    return "on nobody's side";
}

std::string kind_text(const std::string& kind) {
    const std::string label = std::string(kind_label(kind)) + " - ";
    if (kind == "tool") return label + "held from the start, used with x (key c)";
    if (kind == "drop") return label + "y (key v) leaves one behind you";
    if (kind == "creature") return label + "one comes in every 5 s; it hurts you and the cart's enemies";
    return label + "one turns up at the start; it hurts you and the cart's enemies";
}

}  // namespace

void Rogue::centred(const std::string& text, int y, uint8_t c, int cx) {
    painter_.print(text, cx - int(text.size()) * kGlyphW / 2, y, c);
}

void Rogue::draw_screen(Framebuffer& fb) {
    painter_.begin(fb);
    painter_.cls(kBack);
    draw_header();
    switch (screen_) {
    case Screen::Intro: draw_intro(); break;
    case Screen::Next:
    case Screen::Play: draw_next(); break;  // Play: the cart is waiting for the buttons to come up
    case Screen::Loot: draw_loot(); break;
    case Screen::Swap: draw_swap(); break;
    case Screen::Over:
    case Screen::Complete: draw_end(); break;
    }
    painter_.end();
}

// The title, the seed, and a box per cart: cleared ones by name, the current one (and on the
// loot screen the next) by name, the rest a question mark.
void Rogue::draw_header() {
    Painter& p = painter_;
    p.print("mother run", 8, 4, kGold);
    const std::string seed = "seed " + std::to_string(seed_);
    p.print(seed, kScreenW - 8 - int(seed.size()) * kGlyphW, 4, kDim);

    const bool won = screen_ == Screen::Loot || screen_ == Screen::Swap || screen_ == Screen::Complete;
    const int done = stage_ + (won ? 1 : 0);
    const bool playing = screen_ == Screen::Next || screen_ == Screen::Play || screen_ == Screen::Over;
    const int n = int(order_.size()), gap = 4;
    const int w = std::min(47, (kScreenW - 16 - (n - 1) * gap) / n);
    int x = (kScreenW - (n * w + (n - 1) * gap)) / 2;
    for (int i = 0; i < n; ++i, x += w + gap) {
        std::string label = carts_[size_t(order_[size_t(i)])].title;
        uint8_t fill = kPanel, edge = kEdge, ink = kEdge;
        if (i < done) {
            fill = kGreenDark, edge = kGreen, ink = kWhite;
        } else if (i == stage_ && playing && screen_ == Screen::Over) {
            fill = kRedDark, edge = kRed, ink = kWhite;
        } else if (i == stage_ && playing) {
            edge = kWhite, ink = kGold;
        } else if (i == stage_ + 1 && won && screen_ != Screen::Complete) {
            edge = kDim, ink = kText;
        } else {
            label = "?";
        }
        label.resize(std::min(label.size(), size_t((w - 2) / kGlyphW)));
        p.rectfill(x, 14, w, 11, fill);
        p.rect(x, 14, w, 11, edge);
        centred(label, 17, ink, x + w / 2 + 1);
    }
    p.line(8, 30, kScreenW - 9, 30, kEdge);
}

void Rogue::draw_intro() {
    centred(std::to_string(carts_.size()) + " carts, one after another, in a random order.", 42, kText);
    centred("win each one to go on. lose once and the run is over.", 52, kText);
    centred("after a win, take one of that cart's exports along.", 68, kText);
    centred("you hold up to three. a cart that can't use one leaves it out.", 78, kText);
    const char* kinds[][2] = {{"tool", "held from the start, used with x (key c)"},
                              {"drop", "y (key v) leaves one behind you"},
                              {"creature", "on nobody's side: it hurts you and the cart's enemies"}};
    for (int i = 0; i < 3; ++i) {
        const int y = 96 + i * 10;
        painter_.print(kind_label(kinds[i][0]), 44, y, kind_colour(kinds[i][0]));
        painter_.print(kinds[i][1], 68, y, kText);
    }
    if (screen_ticks_ / 30 % 2 == 0) centred("press a to start", 150, kWhite);
}

void Rogue::draw_next() {
    Painter& p = painter_;
    const int c = order_[size_t(stage_)];
    centred("cart " + std::to_string(stage_ + 1) + " of " + std::to_string(order_.size()), 40, kDim);
    centred(spaced(carts_[size_t(c)].title), 52, kGold);
    centred(carts_[size_t(c)].about, 64, kText);
    if (held_.empty()) {
        centred("you bring nothing yet", 96, kEdge);
    } else {
        p.print("you bring", 64, 82, kDim);
        for (size_t i = 0; i < held_.size(); ++i) {
            const Loot& l = loot_[size_t(held_[i])];
            const int y = 92 + int(i) * 22;
            draw_icon(l, 76, y + 6, 1);
            p.print(name(l.source.export_id), 96, y, kWhite);
            p.print("from " + carts_[size_t(l.cart)].title, 96, y + 8, kDim);
            const std::string& why = l.unfit[size_t(c)];
            if (why.empty()) {
                p.print(kind_label(l.kind), 170, y, kind_colour(l.kind));
                p.print(kind_use(l.kind), 170, y + 8, kText);
            } else {
                p.print("left out", 170, y, kEdge);
                p.print(why, 170, y + 8, kEdge);
            }
        }
    }
    if (screen_ == Screen::Next && screen_ticks_ / 30 % 2 == 0) centred("press a to play", 164, kWhite);
}

// Cards for the given loot, then one more labelled `last`; the cursor's is raised and white.
void Rogue::draw_cards(const std::vector<int>& cards, const char* last, int y) {
    Painter& p = painter_;
    const int n = int(cards.size()) + 1;
    int x = (kScreenW - (n * kCardW + (n - 1) * kCardGap)) / 2;
    for (int i = 0; i < n; ++i, x += kCardW + kCardGap) {
        const bool on = i == cursor_;
        const int top = on ? y - 2 : y;
        p.rectfill(x, top, kCardW, kCardH, kPanel);
        p.rect(x, top, kCardW, kCardH, on ? kWhite : kEdge);
        const int cx = x + kCardW / 2;
        if (i == n - 1) {
            centred(last, top + 24, on ? kWhite : kText, cx);
            continue;
        }
        const Loot& l = loot_[size_t(cards[size_t(i)])];
        draw_icon(l, cx, top + 20, 2);
        centred(name(l.source.export_id), top + 39, on ? kWhite : kText, cx);
        centred(kind_label(l.kind), top + 47, kind_colour(l.kind), cx);
    }
}

void Rogue::draw_icon(const Loot& l, int cx, int cy, int scale) {
    if (l.icon < 0) {
        centred("?", cy - 2, kDim, cx);
        return;
    }
    const int w = l.icon_w * kTileSize, h = l.icon_h * kTileSize;
    painter_.sspr((l.icon % kTilesPerRow) * kTileSize, (l.icon / kTilesPerRow) * kTileSize, w, h,
                  cx - w * scale / 2, cy - h * scale / 2, w * scale, h * scale);
}

void Rogue::draw_fit(const Loot& l, int y) {
    const int next = order_[size_t(stage_) + 1];
    const std::string& why = l.unfit[size_t(next)];
    const std::string verdict = why.empty() ? "takes it" : "leaves it out: " + why;
    centred("next: " + carts_[size_t(next)].title + " - " + verdict, y, why.empty() ? kGreen : kEdge);
    int left = 0, fits = 0;
    for (size_t i = size_t(stage_) + 1; i < order_.size(); ++i) {
        ++left;
        if (l.unfit[size_t(order_[i])].empty()) ++fits;
    }
    if (left > 1) {
        centred("fits " + std::to_string(fits) + " of the " + std::to_string(left) + " carts left", y + 10, kText);
    }
}

void Rogue::draw_loot() {
    centred(carts_[size_t(order_[size_t(stage_)])].title + " cleared!", 38, kGold);
    centred("take one of its exports along, or skip", 48, kText);
    draw_cards(offer_, "skip", 60);
    if (cursor_ < int(offer_.size())) {
        const Loot& l = loot_[size_t(offer_[size_t(cursor_)])];
        centred(kind_text(l.kind), 122, kind_colour(l.kind));
        draw_fit(l, 132);
    } else {
        centred("take nothing from this cart", 122, kText);
    }
    std::string holding = "you hold nothing yet";
    for (size_t i = 0; i < held_.size(); ++i) {
        holding = (i == 0 ? "you hold " : holding + ", ") + name(loot_[size_t(held_[i])].source.export_id);
    }
    if (!held_.empty()) holding += " (" + std::to_string(held_.size()) + " of " + std::to_string(kHold) + ")";
    centred(holding, 154, kDim);
    centred("left/right choose   a take   b skip", 168, kDim);
}

void Rogue::draw_swap() {
    const std::string taking = name(loot_[size_t(pending_)].source.export_id);
    centred("you hold three. drop one to take " + taking + "?", 38, kGold);
    centred("or keep what you have and leave it behind", 48, kText);
    draw_cards(held_, "keep", 60);
    if (cursor_ < int(held_.size())) {
        const Loot& l = loot_[size_t(held_[size_t(cursor_)])];
        centred("drop " + name(l.source.export_id), 122, kText);
        draw_fit(l, 132);
    } else {
        centred("leave " + taking + " behind", 122, kText);
        draw_fit(loot_[size_t(pending_)], 132);
    }
    centred("left/right choose   a confirm   b back", 168, kDim);
}

void Rogue::draw_end() {
    const bool won = screen_ == Screen::Complete;
    const std::string n = std::to_string(order_.size());
    centred(spaced(won ? "run complete" : "run over"), 44, won ? kGold : kRed);
    if (won) {
        centred("all " + n + " carts cleared", 58, kText);
    } else {
        const std::string& by = carts_[size_t(order_[size_t(stage_)])].title;
        centred("beaten by " + by + ", cart " + std::to_string(stage_ + 1) + " of " + n, 58, kText);
        std::string cleared;
        for (int i = 0; i < stage_; ++i) cleared += (i ? ", " : "cleared ") + carts_[size_t(order_[size_t(i)])].title;
        if (!cleared.empty()) centred(cleared, 70, kDim);
    }
    const int count = int(held_.size());
    for (int i = 0; i < count; ++i) {
        const int cx = kScreenW / 2 + (2 * i - (count - 1)) * 36;
        const Loot& l = loot_[size_t(held_[size_t(i)])];
        draw_icon(l, cx, 98, 2);
        centred(name(l.source.export_id), 118, kText, cx);
    }
    centred("time " + clock(run_ticks_), 134, kDim);
    if (screen_ticks_ / 30 % 2 == 0) centred("a new run   b quit", 152, kWhite);
}

}  // namespace mother
