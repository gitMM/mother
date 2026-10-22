#pragma once

// MOTHERLIB tests are small carts: Luau checks inside a cart, run by CartVM for some ticks. The
// cart finishes "win" when every check passed; a failed check is a Luau error, printed here.

#include <cstdio>
#include <functional>
#include <string>
#include <utility>

#include "temp_cart.h"
#include "vm/cart_vm.h"

// Prepended to every test cart's main.luau.
//   eq(got, want, what)       check(cond, what)       fails(fn, ...) -> error text or nil
//   has(text, part, what)     text contains part
//   run(init, at, [draw])     cart.init runs init(); cart.update runs at[tick()] (where tick() k
//                             sees the world after k-1 steps) and finishes "win" after the last
inline const char* const kWorldPrelude = R"(
local function eq(got, want, what)
    if got ~= want then
        error(`{what}: expected {want}, got {got}`, 2)
    end
end
local function check(cond, what)
    if not cond then
        error(what, 2)
    end
end
local function fails(fn, ...)
    local ok, err = pcall(fn, ...)
    return if ok then nil else tostring(err)
end
local function has(text, part, what)
    if text == nil or string.find(text, part, 1, true) == nil then
        error(`{what}: "{part}" not in {text}`, 2)
    end
end
local function run(init, at, draw)
    local last = 0
    for t in at do
        last = math.max(last, t)
    end
    function cart.init()
        init()
    end
    function cart.update()
        local f = at[tick()]
        if f then
            f()
        end
        if tick() == last then
            mother.finish("win")
        end
    end
    function cart.draw()
        if draw then
            draw()
        end
    end
    function cart.hash()
        return 0
    end
end
)";

struct WorldRun {
    mother::Outcome outcome = mother::Outcome::Error;
    std::string message;
    mother::Framebuffer fb;  // after the last draw
    uint64_t hash = 0;       // the state hash after the last tick
};

// Pad input for tick t (0-based; tick() in the cart is t + 1).
using InputFn = std::function<mother::RawInput(int t)>;

// RawInput with the named buttons held: a b x y l r s(tart), and < > ^ v for directions.
inline mother::RawInput hold(const std::string& keys) {
    mother::RawInput raw;
    for (char k : keys) {
        switch (k) {
        case '<': raw.left = true; break;
        case '>': raw.right = true; break;
        case '^': raw.up = true; break;
        case 'v': raw.down = true; break;
        case 'a': raw.buttons[mother::face_index(mother::Button::A)] = true; break;
        case 'b': raw.buttons[mother::face_index(mother::Button::B)] = true; break;
        case 'x': raw.buttons[mother::face_index(mother::Button::X)] = true; break;
        case 'y': raw.buttons[mother::face_index(mother::Button::Y)] = true; break;
        case 'l': raw.buttons[mother::face_index(mother::Button::L)] = true; break;
        case 'r': raw.buttons[mother::face_index(mother::Button::R)] = true; break;
        case 's': raw.buttons[mother::face_index(mother::Button::Start)] = true; break;
        default: break;
        }
    }
    return raw;
}

// Runs a test cart for up to `ticks` ticks (drawing after each) and returns how it ended.
inline WorldRun run_world(const std::string& name, const std::string& toml, const std::string& main,
                          int ticks = 600, const TempCart::Files& more = {}, const InputFn& input = {}) {
    WorldRun out;
    TempCart::Files files = {{"cart.toml", toml}, {"main.luau", std::string(kWorldPrelude) + main}};
    files.insert(files.end(), more.begin(), more.end());
    TempCart cart("world_" + name, files);
    mother::CartSource source;
    if (!mother::load_cart_source(cart.dir(), source, out.message)) return out;
    mother::CartVM vm;
    vm.open(std::move(source));
    mother::PadState pad;
    for (int t = 0; t < ticks && vm.status().outcome == mother::Outcome::Running; ++t) {
        if (input) pad = mother::build_pad(input(t), pad);
        vm.update(pad, uint64_t(t));
        vm.draw(out.fb);
    }
    out.outcome = vm.status().outcome;
    if (out.message.empty()) out.message = vm.status().message;
    out.hash = vm.state_hash();
    return out;
}

// True when the cart won; otherwise prints how it ended.
inline bool wins(const WorldRun& run) {
    if (run.outcome == mother::Outcome::Win) return true;
    std::printf("  cart ended %s: %s\n", mother::result_name(run.outcome), run.message.c_str());
    return false;
}

// A map.map: layer 0 is w x h tiles from tile_at(x, y); tile 1 is solid (flag bit 0), tile 2
// solid and damageable, tile 3 solid, damageable and critical (so never damaged).
template <typename TileAt>
std::string solid_map(int w, int h, TileAt tile_at) {
    std::string text = "layer 0 " + std::to_string(w) + " " + std::to_string(h) + "\n";
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            char cell[8];
            std::snprintf(cell, sizeof cell, "%04d", tile_at(x, y));
            text += cell;
            text += x + 1 < w ? ' ' : '\n';
        }
    }
    text += "flags\n";
    for (int t = 0; t < 3072; ++t) text += t == 1 ? "01\n" : t == 2 ? "03\n" : t == 3 ? "07\n" : "00\n";
    return text;
}
