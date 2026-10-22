#pragma once

// Plays a real cart with input from a bot: a Luau function that runs inside the cart's VM between
// ticks, reads the world (world:get, world:query, ...) and returns the pad for the next tick as
// (stick x, stick y, buttons), with x and y in -1..1 and buttons a string of a b l r s(tart).
// It only reads, so the cart runs exactly as it would with that input from a player.

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

#include "lua.h"
#include "lualib.h"

#include "vm/cart_vm.h"
#include "vm/sandbox.h"

struct BotRun {
    mother::Outcome outcome = mother::Outcome::Error;
    uint64_t ticks = 0;
    uint64_t hash = 0;  // the state hash after the last tick
    std::string message;
};

inline BotRun play(const std::filesystem::path& cart, const std::string& bot, int max_ticks, uint64_t seed = 0) {
    BotRun out;
    mother::CartSource source;
    if (!mother::load_cart_source(cart, source, out.message)) return out;
    mother::CartVM vm;
    if (!vm.open(std::move(source), seed)) {
        out.message = vm.status().message;
        return out;
    }
    lua_State* L = vm.state();
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    const int globals = lua_ref(L, -1);
    lua_pop(L, 1);
    if (!mother::load_module(L, globals, "bot", bot, out.message)) return out;
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        out.message = lua_tostring(L, -1);
        return out;
    }
    const int fn = lua_ref(L, -1);
    lua_pop(L, 1);

    mother::PadState pad;
    mother::Framebuffer fb;
    for (int t = 0; t < max_ticks && vm.status().outcome == mother::Outcome::Running; ++t) {
        lua_getref(L, fn);
        lua_pushinteger(L, t);
        if (lua_pcall(L, 1, 3, 0) != LUA_OK) {
            out.message = std::string("bot: ") + lua_tostring(L, -1);
            return out;
        }
        mother::RawInput raw;
        raw.axis_x = int8_t(std::lround(lua_tonumber(L, -3) * 127));
        raw.axis_y = int8_t(std::lround(lua_tonumber(L, -2) * 127));
        const char* keys = lua_tostring(L, -1);
        for (const char* k = keys ? keys : ""; *k; ++k) {
            using mother::Button;
            const Button b = *k == 'a' ? Button::A : *k == 'b' ? Button::B : *k == 'l' ? Button::L
                           : *k == 'r' ? Button::R : Button::Start;
            raw.buttons[mother::face_index(b)] = true;
        }
        lua_pop(L, 3);
        pad = mother::build_pad(raw, pad);
        vm.update(pad, uint64_t(t));
        vm.draw(fb);
        out.ticks = uint64_t(t) + 1;
    }
    out.outcome = vm.status().outcome;
    out.message = vm.status().message;
    out.hash = vm.state_hash();
    return out;
}

// True when the run ended with `want`; otherwise prints how it did end.
inline bool ended(const BotRun& run, mother::Outcome want) {
    if (run.outcome == want) return true;
    std::printf("  ended %s after %llu ticks: %s\n", mother::result_name(run.outcome),
                static_cast<unsigned long long>(run.ticks), run.message.c_str());
    return false;
}
