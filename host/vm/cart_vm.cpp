#include "vm/cart_vm.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <utility>

#include "lua.h"
#include "lualib.h"

#include "runtime/hash.h"
#include "vm/api.h"

namespace mother {

namespace {

constexpr const char* kLifecycle[] = {"init", "update", "draw", "hash"};

// Error handler for protected calls: the message plus a Luau stack traceback.
int traceback(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    std::string text = message ? message : "(error value is not a string)";
    text += "\nstack traceback:\n";
    text += lua_debugtrace(L);
    lua_pushlstring(L, text.data(), text.size());
    return 1;
}

void on_interrupt(lua_State* L, int) {
    CartVM::of(L).check_budget();
}

int boot(lua_State* L) {
    CartVM::of(L).run_main(L);
    return 0;
}

}  // namespace

CartVM::~CartVM() {
    if (L_) lua_close(L_);
}

CartVM& CartVM::of(lua_State* L) {
    return *static_cast<CartVM*>(lua_callbacks(L)->userdata);
}

bool CartVM::open(CartSource source, uint64_t seed) {
    source_ = std::move(source);
    source_.map.rehash();
    rng_ = Rng(seed);
    L_ = new_sandbox_state(heap_);
    if (!L_) {
        fail("could not create the Luau state");
        return false;
    }
    lua_callbacks(L_)->userdata = this;
    lua_callbacks(L_)->interrupt = on_interrupt;

    painter_.set_assets(&source_.art.sheets, &source_.map);
    open_gfx_api(L_);
    open_sprite_api(L_);
    open_map_api(L_);
    open_input_api(L_);
    open_audio_api(L_);
    open_math_api(L_);
    open_host_api(L_);
    luaL_sandbox(L_);  // freezes every library table, the API and the globals

    // `cart` is the one writable table: main.luau fills in its lifecycle functions.
    lua_setreadonly(L_, LUA_GLOBALSINDEX, false);
    lua_newtable(L_);
    lua_setglobal(L_, "cart");
    lua_setreadonly(L_, LUA_GLOBALSINDEX, true);
    lua_pushvalue(L_, LUA_GLOBALSINDEX);
    api_ref_ = lua_ref(L_, -1);
    lua_pop(L_, 1);

    lua_pushcfunction(L_, boot, "boot");
    if (!protected_call(0)) return false;

    lua_getglobal(L_, "cart");
    for (const char* fn : kLifecycle) {
        lua_getfield(L_, -1, fn);
        const bool ok = lua_isfunction(L_, -1);
        lua_pop(L_, 1);
        if (!ok) {
            lua_pop(L_, 1);
            fail(std::string("main.luau must define cart.") + fn + " as a function");
            return false;
        }
    }
    lua_pop(L_, 1);
    return call("init");
}

void CartVM::update(const PadState& pad, uint64_t tick) {
    if (status_.outcome != Outcome::Running) return;
    pad_ = pad;
    ticks_ = tick + 1;
    budget_logged_ = false;
    tick_start_ = std::chrono::steady_clock::now();
    in_update_ = true;
    call("update");
    in_update_ = false;
    if (!budget_logged_ && std::chrono::steady_clock::now() - tick_start_ > kTickBudget) report_budget();
    if (status_.outcome == Outcome::Error) return;
    sequencer_.tick(audio_);
    update_hash();
}

void CartVM::update_hash() {
    if (!call("hash", 1)) return;
    if (lua_type(L_, -1) != LUA_TNUMBER) {
        lua_pop(L_, 1);
        fail("cart.hash() must return a number");
        return;
    }
    const double cart_value = lua_tonumber(L_, -1);
    lua_pop(L_, 1);
    Fnv64 h;
    h.u64(ticks_);
    for (uint32_t word : rng_.state()) h.u32(word);
    h.u64(source_.map.hash);
    h.number(cart_value);
    state_hash_ = h.value();
}

void CartVM::take_audio(TickAudio& out) {
    out.swap(audio_);
    audio_.clear();
}

void CartVM::draw(Framebuffer& fb) {
    if (status_.outcome != Outcome::Running) return;
    painter_.begin(fb);
    call("draw");
    painter_.end();
}

Painter& CartVM::painter_in_draw(lua_State* L, const char* fn) {
    if (!painter_.drawing()) luaL_error(L, "%s() can only be called from cart.draw()", fn);
    return painter_;
}

void CartVM::finish(Outcome outcome) {
    if (status_.outcome == Outcome::Running) status_.outcome = outcome;
}

void CartVM::log(const std::string& line) {
    std::printf("[cart] %s\n", line.c_str());
    std::fflush(stdout);
}

void CartVM::run_main(lua_State* L) {
    run_module(L, "main");
}

int CartVM::import_module(lua_State* L, const char* name) {
    const std::string key = name;
    if (!valid_module_name(key)) {
        luaL_error(L, "import(\"%s\"): a module name is letters, digits and _ only", name);
    }
    if (key == "main") {
        luaL_error(L, "import(\"main\"): main.luau is the entry point and can't be imported");
    }
    if (auto it = imported_.find(key); it != imported_.end()) {
        lua_getref(L, it->second);
        return 1;
    }
    if (source_.modules.count(key) == 0) {
        luaL_error(L, "import(\"%s\"): there is no %s.luau in the cart folder", name, name);
    }
    if (importing_.count(key) != 0) luaL_error(L, "import(\"%s\"): import cycle", name);

    run_module(L, key);
    imported_[key] = lua_ref(L, -1);
    return 1;
}

void CartVM::run_module(lua_State* L, const std::string& name) {
    std::string error;
    if (!load_module(L, api_ref_, name, source_.modules.at(name), error)) {
        luaL_error(L, "%s", error.c_str());
    }
    struct Loading {
        std::set<std::string>& set;
        std::string name;
        ~Loading() { set.erase(name); }
    } loading{importing_, name};
    importing_.insert(name);
    lua_call(L, 0, 1);
}

void CartVM::check_budget() {
    if (!in_update_ || budget_logged_ || (++interrupts_ & 255) != 0) return;
    if (std::chrono::steady_clock::now() - tick_start_ > kTickBudget) report_budget();
}

void CartVM::report_budget() {
    budget_logged_ = true;
    ++budget_errors_;
    std::fprintf(stderr, "budget error: tick %" PRIu64 ": cart.update ran over %d ms\n", ticks_,
                 int(kTickBudget.count()));
}

bool CartVM::call(const char* fn, int nresults) {
    lua_getglobal(L_, "cart");
    lua_getfield(L_, -1, fn);
    lua_remove(L_, -2);
    return protected_call(0, nresults);
}

// Calls the function below the top nargs values; on success leaves its nresults results.
bool CartVM::protected_call(int nargs, int nresults) {
    const int func = lua_gettop(L_) - nargs;
    lua_pushcfunction(L_, traceback, "traceback");
    lua_insert(L_, func);
    const int status = lua_pcall(L_, nargs, nresults, func);
    if (status != LUA_OK) {
        const char* message = lua_tostring(L_, -1);
        std::string text = message ? message : "unknown error";
        if (status == LUA_ERRMEM) text += " (the cart heap is limited to 16 MB)";
        lua_pop(L_, 2);  // error, handler
        fail(std::move(text));
        return false;
    }
    lua_remove(L_, func);  // handler
    return true;
}

void CartVM::fail(std::string message) {
    status_.outcome = Outcome::Error;
    status_.message = std::move(message);
}

}  // namespace mother
