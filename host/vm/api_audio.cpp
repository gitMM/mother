#include "lua.h"
#include "lualib.h"

#include "assets/trk_cell.h"
#include "vm/api.h"
#include "vm/cart_vm.h"

// sfx, music, stop, beat, song_get, song_set. The calls that change what plays are simulation
// state, so like the tilemap they belong in cart.update; they raise an error inside cart.draw().

namespace mother {

void not_in_draw(lua_State* L, const char* fn) {
    if (CartVM::of(L).drawing()) luaL_error(L, "%s() can't be called from cart.draw()", fn);
}

namespace {

int opt_voice(lua_State* L, int idx, const char* fn) {
    if (lua_isnoneornil(L, idx)) return -1;
    const int voice = arg_coord(L, idx);
    if (voice == 7 || voice == 8) luaL_error(L, "%s: voices 7 and 8 are reserved for imports and the host", fn);
    if (voice < 1 || voice > kCartVoices) luaL_error(L, "%s: voice must be 1-6, got %d", fn, voice);
    return voice - 1;
}

Cell& arg_cell(lua_State* L, const char* fn) {
    const int pattern = arg_coord(L, 1), row = arg_coord(L, 2), ch = arg_coord(L, 3);
    if (pattern < 0 || pattern >= kPatterns) luaL_error(L, "%s: pattern must be 0-63, got %d", fn, pattern);
    if (row < 0 || row >= kPatternRows) luaL_error(L, "%s: row must be 0-63, got %d", fn, row);
    if (ch < 1 || ch > kChannels) luaL_error(L, "%s: channel must be 1-6, got %d", fn, ch);
    return CartVM::of(L).tracker().patterns[size_t(pattern)][size_t(row)][size_t(ch - 1)];
}

int l_sfx(lua_State* L) {
    not_in_draw(L, "sfx");
    const int n = arg_coord(L, 1);
    if (n < 0 || n >= kSfxCount || CartVM::of(L).tracker().sfx[size_t(n)].speed == 0) {
        luaL_error(L, "sfx(%d): audio.trk has no sfx %d", n, n);
    }
    const int voice = opt_voice(L, 2, "sfx");
    CartVM::of(L).sequencer().sfx(n, voice);
    return 0;
}

int l_music(lua_State* L) {
    not_in_draw(L, "music");
    const int song = arg_coord(L, 1);
    const int fade = opt_coord(L, 2, 0);
    if (fade < 0) luaL_error(L, "music: fade_ticks must be 0 or more, got %d", fade);
    if (song != -1) {
        if (song < 0 || song >= kSongs || CartVM::of(L).tracker().songs[size_t(song)].tempo == 0) {
            luaL_error(L, "music(%d): audio.trk has no song %d (use -1 to stop)", song, song);
        }
    }
    CartVM::of(L).sequencer().music(song, fade);
    return 0;
}

int l_stop(lua_State* L) {
    not_in_draw(L, "stop");
    CartVM::of(L).sequencer().stop(opt_voice(L, 1, "stop"));
    return 0;
}

int l_beat(lua_State* L) {
    int bar = 0, beat = 0, row = 0;
    if (!CartVM::of(L).sequencer().beat(bar, beat, row)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, bar);
    lua_pushinteger(L, beat);
    lua_pushinteger(L, row);
    return 3;
}

int l_song_get(lua_State* L) {
    const Cell& cell = arg_cell(L, "song_get");
    lua_pushinteger(L, cell.note);
    lua_pushinteger(L, cell.inst);
    lua_pushinteger(L, cell.vol);
    lua_pushstring(L, format_fx(cell.fx, cell.param).c_str());
    return 4;
}

int l_song_set(lua_State* L) {
    not_in_draw(L, "song_set");
    Cell& cell = arg_cell(L, "song_set");
    const int note = arg_coord(L, 4), inst = arg_coord(L, 5), vol = arg_coord(L, 6);
    if (note < kNoteOff || note > kNoteCount) luaL_error(L, "song_set: note must be -1 (off), 0 (empty) or 1-96, got %d", note);
    if (inst < -1 || inst >= kInstruments) luaL_error(L, "song_set: instrument must be -1 (none) or 0-63, got %d", inst);
    if (vol < -1 || vol > kMaxVolume) luaL_error(L, "song_set: volume must be -1 (none) or 0-64, got %d", vol);
    char fx = 0;
    uint8_t param = 0;
    if (!lua_isnoneornil(L, 7)) {
        const char* text = luaL_checkstring(L, 7);
        if (!parse_fx(text, fx, param)) luaL_error(L, "song_set: '%s' is not an effect (Pxx Uxx Dxx Vxy Txx or ...)", text);
    }
    cell = {int8_t(note), int8_t(inst), int8_t(vol), fx, param};
    return 0;
}

constexpr luaL_Reg kFunctions[] = {
    {"sfx", l_sfx},   {"music", l_music},       {"stop", l_stop},
    {"beat", l_beat}, {"song_get", l_song_get}, {"song_set", l_song_set},
};

}  // namespace

void open_audio_api(lua_State* L) {
    for (const luaL_Reg& f : kFunctions) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setglobal(L, f.name);
    }
}

}  // namespace mother
