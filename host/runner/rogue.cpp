#include "runner/rogue.h"

#include <cstdio>
#include <map>
#include <utility>

#include "runtime/hash.h"
#include "runtime/rng.h"

namespace mother {

namespace {

namespace fs = std::filesystem;

constexpr uint16_t kFaceButtons = uint16_t((1u << int(Button::Count)) - (1u << int(Button::A)));

std::string clock_text(int64_t ticks) {
    char text[24];
    std::snprintf(text, sizeof text, "%d:%02d", int(ticks / 3600), int(ticks / 60 % 60));
    return text;
}

std::string spoken(std::string id) {
    for (char& c : id) {
        if (c == '_') c = ' ';
    }
    return id;
}

// "driving - win a 3-lap race", from [cart] family and [objective].
std::string about(const TomlValue& manifest) {
    static const std::map<std::string, std::string> families = {
        {"action", "action"}, {"platform", "platformer"}, {"driving", "driving"},
        {"shooter", "shooter"}, {"puzzle", "puzzle"}};
    const TomlValue& goal = *manifest.find("objective");
    const std::string& type = goal.find("type")->string;
    std::string text;
    if (type == "kill_all") {
        text = "clear out every enemy";
    } else if (type == "survive") {
        text = "survive " + clock_text(goal.find("ticks")->integer);
    } else if (type == "race") {
        text = "win a " + std::to_string(goal.find("laps")->integer) + "-lap race";
    } else if (type == "escape") {
        text = "reach the exit";
    } else {
        text = "collect every " + spoken(goal.find("archetype")->string);
    }
    const std::string& family = manifest.find("cart")->find("family")->string;
    const auto it = families.find(family);
    return (it == families.end() ? family : it->second) + " - " + text;
}

// The first frame of the export's main sprite (`sprite`, else its first block), copied into the
// screens' sheet 3.
void find_icon(Rogue::Loot& l, const ArchetypeInfo& a, ImportAssets& assets, SpriteSheets& sheets) {
    if (a.sprites.empty()) return;
    const ArchetypeInfo::Sprite* block = &a.sprites[0];
    for (const ArchetypeInfo::Sprite& s : a.sprites) {
        if (s.param == "sprite") block = &s;
    }
    int tile = block->fallback;
    if (const TomlValue* given = l.source.params.find(block->param); given && given->is_number()) {
        tile = int(given->as_number());
    }
    if (tile <= 0) return;
    l.icon = assets.copy_tiles(l.source, tile, block->w, block->h, sheets);
    l.icon_w = block->w;
    l.icon_h = block->h;
}

}  // namespace

std::string Rogue::clock(uint64_t ticks) {
    return clock_text(int64_t(ticks));
}

std::string Rogue::name(const std::string& id) {
    return spoken(id);
}

bool Rogue::open(const fs::path& dir, uint64_t seed, std::string& error) {
    for (const fs::path& d : cart_folders(dir)) {
        Cart c;
        if (!load_cart_source(d, c.source, error)) {
            error = d.string() + ": " + error;
            return false;
        }
        c.dir = d.filename().string();
        c.title = c.source.manifest.find("cart")->find("title")->string;
        c.about = about(c.source.manifest);
        carts_.push_back(std::move(c));
    }
    if (carts_.empty()) {
        error = "no carts in " + dir.string();
        return false;
    }

    // MOTHERLIB, in one of the carts, says what each export is and which carts can host it.
    CartVM probe;
    if (!probe.open(carts_[0].source)) {
        error = carts_[0].dir + ": " + probe.status().message;
        return false;
    }
    std::map<std::string, ArchetypeInfo> archetypes;
    for (ArchetypeInfo& a : probe.describe()) archetypes[a.name] = std::move(a);
    ImportAssets assets;
    for (size_t c = 0; c < carts_.size(); ++c) {
        const TomlValue* exports = carts_[c].source.manifest.find("exports");
        for (size_t i = 0; exports && i < exports->items.size(); ++i) {
            Loot l;
            l.cart = int(c);
            import_from(carts_[c].source, exports->items[i].find("id")->string, l.source);
            const auto it = archetypes.find(l.source.archetype);
            if (it == archetypes.end()) {
                error = carts_[c].dir + ": export " + l.source.export_id + ": there is no archetype " + l.source.archetype;
                return false;
            }
            const ArchetypeInfo& a = it->second;
            l.kind = a.category == "equipment" ? "tool" : a.deployable ? "drop" : a.category == "creature" ? "creature" : "object";
            for (size_t host = 0; host < carts_.size(); ++host) {
                std::string why = "its own cart";
                if (host != c && !probe.check_import(a.name, *carts_[host].source.manifest.find("env"), why)) {
                    error = probe.status().message;
                    return false;
                }
                l.unfit.push_back(why);
            }
            find_icon(l, a, assets, icons_);
            loot_.push_back(std::move(l));
        }
    }
    painter_.set_assets(&icons_, nullptr);
    start(seed);
    update_hash();
    return true;
}

void Rogue::start(uint64_t seed) {
    seed_ = seed;
    order_.clear();
    for (size_t i = 0; i < carts_.size(); ++i) order_.push_back(int(i));
    Rng rng(seed);
    for (size_t i = order_.size(); i > 1; --i) std::swap(order_[i - 1], order_[size_t(rng.range(0, int64_t(i) - 1))]);
    held_.clear();
    offer_.clear();
    playing_with_.clear();
    stage_ = 0;
    pending_ = -1;
    run_ticks_ = 0;
    show(Screen::Intro);
}

void Rogue::show(Screen s) {
    screen_ = s;
    screen_ticks_ = 0;
    cursor_ = 0;
}

void Rogue::play() {
    const int c = order_[size_t(stage_)];
    std::vector<ImportSource> imports;
    playing_with_.clear();
    for (int l : held_) {
        if (!loot_[size_t(l)].unfit[size_t(c)].empty()) continue;
        imports.push_back(loot_[size_t(l)].source);
        playing_with_.push_back(l);
    }
    cart_ = std::make_unique<CartVM>();
    ++audio_generation_;
    if (!cart_->open(carts_[size_t(c)].source, seed_, std::move(imports))) {
        fail(carts_[size_t(c)].dir + ": " + cart_->status().message);
        return;
    }
    screen_ = Screen::Play;
    waiting_ = true;
    cart_ticks_ = 0;
}

void Rogue::finished(Outcome outcome) {
    budget_errors_ += cart_->budget_errors();
    cart_.reset();
    playing_with_.clear();
    ++audio_generation_;
    if (outcome == Outcome::Lose) {
        show(Screen::Over);
        return;
    }
    if (size_t(stage_) + 1 == order_.size()) {
        show(Screen::Complete);
        return;
    }
    offer_.clear();
    for (size_t l = 0; l < loot_.size(); ++l) {
        if (loot_[l].cart == order_[size_t(stage_)]) offer_.push_back(int(l));
    }
    if (offer_.empty()) {
        advance();
    } else {
        show(Screen::Loot);
    }
}

void Rogue::advance() {
    ++stage_;
    pending_ = -1;
    show(Screen::Next);
}

void Rogue::take(int loot) {
    if (int(held_.size()) < kHold) {
        held_.push_back(loot);
        advance();
        return;
    }
    pending_ = loot;
    show(Screen::Swap);
}

void Rogue::update(const PadState& pad, uint64_t) {
    if (status_.outcome != Outcome::Running) return;
    if (screen_ != Screen::Play) {
        update_screen(pad);
    } else if (waiting_) {
        waiting_ = (pad.held & kFaceButtons) != 0;
    } else {
        cart_->update(pad, cart_ticks_++);
        ++run_ticks_;
        const Status& s = cart_->status();
        if (s.outcome == Outcome::Error) {
            fail(carts_[size_t(order_[size_t(stage_)])].dir + ": " + s.message);
        } else if (s.outcome != Outcome::Running) {
            finished(s.outcome);
        }
    }
    update_hash();
}

void Rogue::update_screen(const PadState& pad) {
    if (++screen_ticks_ <= kInputDelay) return;
    const bool ok = pad.btnp(Button::A), back = pad.btnp(Button::B);
    const int step = (pad.btnp(Button::Right) ? 1 : 0) - (pad.btnp(Button::Left) ? 1 : 0);
    switch (screen_) {
    case Screen::Intro:
        if (ok || pad.btnp(Button::Start)) show(Screen::Next);
        break;
    case Screen::Next:
        if (ok || pad.btnp(Button::Start)) play();
        break;
    case Screen::Loot: {
        const int cards = int(offer_.size()) + 1;
        cursor_ = (cursor_ + step + cards) % cards;
        if (back || (ok && cursor_ == cards - 1)) {
            advance();
        } else if (ok) {
            take(offer_[size_t(cursor_)]);
        }
        break;
    }
    case Screen::Swap: {
        const int cards = int(held_.size()) + 1;
        cursor_ = (cursor_ + step + cards) % cards;
        if (back) {
            const int was = pending_;
            pending_ = -1;
            show(Screen::Loot);
            screen_ticks_ = kInputDelay;
            while (offer_[size_t(cursor_)] != was) ++cursor_;
        } else if (ok) {
            if (cursor_ < cards - 1) {
                held_.erase(held_.begin() + cursor_);
                held_.push_back(pending_);
            }
            advance();
        }
        break;
    }
    case Screen::Over:
    case Screen::Complete:
        if (ok) {
            start(seed_ + 1);
        } else if (back) {
            status_.outcome = screen_ == Screen::Complete ? Outcome::Win : Outcome::Lose;
        }
        break;
    case Screen::Play:
        break;
    }
}

void Rogue::update_hash() {
    Fnv64 h;
    h.u64(seed_);
    h.byte(uint8_t(screen_));
    h.u32(uint32_t(stage_));
    h.u32(uint32_t(cursor_));
    h.u32(uint32_t(pending_ + 1));
    h.u32(uint32_t(screen_ticks_));
    h.byte(waiting_ ? 1 : 0);
    h.u32(uint32_t(held_.size()));
    for (int l : held_) h.u32(uint32_t(l));
    h.u64(cart_ticks_);
    h.u64(run_ticks_);
    h.u64(cart_ ? cart_->state_hash() : 0);
    state_hash_ = h.value();
}

void Rogue::fail(std::string message) {
    status_.outcome = Outcome::Error;
    status_.message = std::move(message);
}

const AudioSetup& Rogue::audio_setup() const {
    return cart_ ? cart_->audio_setup() : silence_;
}

void Rogue::take_audio(TickAudio& out) {
    if (cart_) {
        cart_->take_audio(out);
    } else {
        out.clear();
    }
}

void Rogue::draw(Framebuffer& fb) {
    if (cart_started()) {
        cart_->draw(fb);
    } else {
        draw_screen(fb);
    }
}

}  // namespace mother
