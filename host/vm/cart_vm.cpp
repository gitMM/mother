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

int boot_lib(lua_State* L) {
    CartVM::of(L).open_lib(L);
    return 0;
}

int audit_world(lua_State* L) {
    *static_cast<WorldAudit*>(lua_tolightuserdata(L, 1)) = CartVM::of(L).lib().audit(L);
    return 0;
}

int describe_lib(lua_State* L) {
    *static_cast<std::vector<ArchetypeInfo>*>(lua_tolightuserdata(L, 1)) = CartVM::of(L).lib().describe(L);
    return 0;
}

struct ImportCheck {
    const std::string* archetype;
    const TomlValue* env;
    std::string* why;
};

int check_lib(lua_State* L) {
    const auto* c = static_cast<const ImportCheck*>(lua_tolightuserdata(L, 1));
    *c->why = CartVM::of(L).lib().check(L, *c->archetype, *c->env);
    return 0;
}

}  // namespace

CartVM::~CartVM() {
    if (L_) lua_close(L_);
}

CartVM& CartVM::of(lua_State* L) {
    return *static_cast<CartVM*>(lua_callbacks(L)->userdata);
}

bool CartVM::open(CartSource source, uint64_t seed, std::vector<ImportSource> imports) {
    source_ = std::move(source);
    imports_ = std::move(imports);
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
    lua_pushvalue(L_, LUA_GLOBALSINDEX);
    api_ref_ = lua_ref(L_, -1);
    lua_pop(L_, 1);

    // MOTHERLIB creates the world from cart.toml before any cart code runs.
    lua_pushcfunction(L_, boot_lib, "motherlib");
    if (!protected_call(0)) return false;
    luaL_sandbox(L_);  // freezes every library table, the API, `world` and the globals

    // `cart` is the one writable table: main.luau fills in its lifecycle functions.
    lua_setreadonly(L_, LUA_GLOBALSINDEX, false);
    lua_newtable(L_);
    lua_setglobal(L_, "cart");
    lua_setreadonly(L_, LUA_GLOBALSINDEX, true);

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
    if (call("update")) {
        lua_getref(L_, lib_.step_ref());  // world:step()
        protected_call(0);
    }
    in_update_ = false;
    if (!budget_logged_ && std::chrono::steady_clock::now() - tick_start_ > kTickBudget) report_budget();
    if (status_.outcome == Outcome::Error) return;
    sequencer_.tick(audio_);
    update_hash();
}

void CartVM::update_hash() {
    lua_getref(L_, lib_.hash_ref());
    if (!protected_call(0, 2)) return;
    const uint64_t world_hash = Motherlib::take_hash(L_);
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
    h.u64(world_hash);
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
    outlines_.clear();
    call("draw");
    painter_.end();
    for (const Outline& o : outlines_) fb.rect(o.x, o.y, o.w, o.h, kMotherGold);  // brief section 4, step 5
}

void CartVM::outline(int x, int y, int w, int h) {
    if (!painter_.drawing()) luaL_error(L_, "host.outline: only during cart.draw()");
    outlines_.push_back({x - painter_.camera_x(), y - painter_.camera_y(), w, h});
}

bool CartVM::audit(WorldAudit& out) {
    lua_pushcfunction(L_, audit_world, "audit");
    lua_pushlightuserdata(L_, &out);
    return protected_call(1);
}

std::vector<ArchetypeInfo> CartVM::describe() {
    std::vector<ArchetypeInfo> out;
    lua_pushcfunction(L_, describe_lib, "describe");
    lua_pushlightuserdata(L_, &out);
    if (!protected_call(1)) out.clear();
    return out;
}

bool CartVM::check_import(const std::string& archetype, const TomlValue& env, std::string& why) {
    ImportCheck check{&archetype, &env, &why};
    lua_pushcfunction(L_, check_lib, "check");
    lua_pushlightuserdata(L_, &check);
    return protected_call(1);
}

int CartVM::import_tiles(lua_State* L, int tile, int w, int h) {
    if (!adding_) luaL_error(L, "host.import_tiles: no import is being added");
    const int first = import_assets_.copy_tiles(*adding_, tile, w, h, source_.art.sheets);
    if (first < 0) luaL_error(L, "import %s: its art doesn't fit in sheet 3", adding_->label().c_str());
    return first;
}

int CartVM::import_sfx(lua_State* L, int n) {
    if (!adding_) luaL_error(L, "host.import_sfx: no import is being added");
    const int slot = import_assets_.copy_sfx(*adding_, n, source_.tracker);
    if (slot < 0) {
        luaL_error(L, "import %s: its cart has no sfx %d, or sfx slots 48-63 are full", adding_->label().c_str(), n);
    }
    return slot;
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

void CartVM::open_lib(lua_State* L) {
    lib_.open(L, api_ref_, source_.manifest);
    for (const ImportSource& import : imports_) {
        adding_ = &import;
        const std::string why = lib_.add_import(L, import);
        adding_ = nullptr;
        if (!why.empty()) {
            incompatible_ = why;
            luaL_error(L, "can't import %s: %s", import.label().c_str(), why.c_str());
        }
    }
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
    std::fprintf(stderr, "budget error: tick %" PRIu64 ": cart.update and world:step ran over %d ms\n", ticks_,
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
