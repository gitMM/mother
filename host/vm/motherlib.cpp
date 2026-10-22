#include "vm/motherlib.h"

#include "lua.h"
#include "lualib.h"

#include "runtime/hash.h"
#include "vm/api.h"
#include "vm/cart_vm.h"
#include "vm/imports.h"
#include "vm/lib_sources.h"
#include "vm/sandbox.h"

namespace mother {

namespace {

const LibSource* find_source(const std::string& name) {
    for (size_t i = 0; i < kLibSourceCount; ++i) {
        if (name == kLibSources[i].name) return &kLibSources[i];
    }
    return nullptr;
}

int l_use(lua_State* L) {
    return CartVM::of(L).lib().use(L, luaL_checkstring(L, 1));
}

// host.modules() -> every MOTHERLIB module name, sorted.
int l_modules(lua_State* L) {
    lua_createtable(L, int(kLibSourceCount), 0);
    for (size_t i = 0; i < kLibSourceCount; ++i) {
        lua_pushstring(L, kLibSources[i].name);
        lua_rawseti(L, -2, int(i) + 1);
    }
    return 1;
}

// host.map_size(layer) -> width, height in tiles (0, 0 when the layer is absent).
int l_map_size(lua_State* L) {
    const int layer = arg_layer(L, 1, "host.map_size");
    const TileLayer& l = CartVM::of(L).tilemap().layers[size_t(layer)];
    lua_pushinteger(L, l.w);
    lua_pushinteger(L, l.h);
    return 2;
}

// host.in_draw() -> whether cart.draw() is running.
int l_in_draw(lua_State* L) {
    lua_pushboolean(L, CartVM::of(L).drawing());
    return 1;
}

// host.sfx(n, voice): sfx n on voice 1-8; MOTHERLIB plays imported sounds on 7 and 8.
int l_sfx(lua_State* L) {
    not_in_draw(L, "host.sfx");
    CartVM& vm = CartVM::of(L);
    const int n = int(luaL_checkinteger(L, 1)), voice = int(luaL_checkinteger(L, 2));
    if (n < 0 || n >= kSfxCount || vm.tracker().sfx[size_t(n)].speed == 0) luaL_error(L, "host.sfx: no sfx %d", n);
    if (voice < 1 || voice > kVoices) luaL_error(L, "host.sfx: voice must be 1-8, got %d", voice);
    vm.sequencer().sfx(n, voice - 1);
    return 0;
}

// host.import_tiles(tile, w, h) -> the first tile of its copy in sheet 3, while an import is
// being added.
int l_import_tiles(lua_State* L) {
    const int tile = int(luaL_checkinteger(L, 1)), w = int(luaL_checkinteger(L, 2));
    const int h = int(luaL_checkinteger(L, 3));
    lua_pushinteger(L, CartVM::of(L).import_tiles(L, tile, w, h));
    return 1;
}

// host.import_sfx(n) -> the slot of its copy, while an import is being added.
int l_import_sfx(lua_State* L) {
    lua_pushinteger(L, CartVM::of(L).import_sfx(L, int(luaL_checkinteger(L, 1))));
    return 1;
}

// host.outline(x, y, w, h): queues a Mother Gold outline (camera applied) during cart.draw().
int l_outline(lua_State* L) {
    const int x = arg_coord(L, 1), y = arg_coord(L, 2), w = arg_coord(L, 3), h = arg_coord(L, 4);
    CartVM::of(L).outline(x, y, w, h);
    return 0;
}

constexpr luaL_Reg kHost[] = {
    {"modules", l_modules},
    {"map_size", l_map_size},
    {"in_draw", l_in_draw},
    {"btn", lib_btn},
    {"btnp", lib_btnp},
    {"sfx", l_sfx},
    {"import_tiles", l_import_tiles},
    {"import_sfx", l_import_sfx},
    {"outline", l_outline},
};

// TOML values as Luau values: integers and floats become numbers, arrays 1-based tables.
void push_toml(lua_State* L, const TomlValue& v) {
    luaL_checkstack(L, 3, "manifest nesting");
    switch (v.type) {
    case TomlValue::Type::String: lua_pushlstring(L, v.string.data(), v.string.size()); break;
    case TomlValue::Type::Integer: lua_pushnumber(L, double(v.integer)); break;
    case TomlValue::Type::Float: lua_pushnumber(L, v.number); break;
    case TomlValue::Type::Bool: lua_pushboolean(L, v.boolean); break;
    case TomlValue::Type::Array:
        lua_createtable(L, int(v.items.size()), 0);
        for (size_t i = 0; i < v.items.size(); ++i) {
            push_toml(L, v.items[i]);
            lua_rawseti(L, -2, int(i) + 1);
        }
        break;
    case TomlValue::Type::Table:
        lua_createtable(L, 0, int(v.items.size()));
        for (size_t i = 0; i < v.items.size(); ++i) {
            push_toml(L, v.items[i]);
            lua_setfield(L, -2, v.keys[i].c_str());
        }
        break;
    }
}

int take_ref(lua_State* L, int table, const char* field) {
    lua_getfield(L, table, field);
    const int ref = lua_ref(L, -1);
    lua_pop(L, 1);
    return ref;
}

}  // namespace

void Motherlib::open(lua_State* L, int api_ref, const TomlValue& manifest) {
    // What MOTHERLIB globals read through to: use, host, then the cart API.
    lua_newtable(L);
    lua_pushcfunction(L, l_use, "use");
    lua_setfield(L, -2, "use");
    lua_newtable(L);
    for (const luaL_Reg& f : kHost) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setfield(L, -2, f.name);
    }
    lua_setreadonly(L, -1, true);
    lua_setfield(L, -2, "host");
    lua_newtable(L);
    lua_getref(L, api_ref);
    lua_setfield(L, -2, "__index");
    lua_setreadonly(L, -1, true);
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lib_ref_ = lua_ref(L, -1);
    lua_pop(L, 1);

    use(L, "core/world");
    const int hooks = lua_gettop(L);
    step_ref_ = take_ref(L, hooks, "step");
    hash_ref_ = take_ref(L, hooks, "hash");
    import_ref_ = take_ref(L, hooks, "import");
    audit_ref_ = take_ref(L, hooks, "audit");
    describe_ref_ = take_ref(L, hooks, "describe");
    check_ref_ = take_ref(L, hooks, "check");
    lua_getfield(L, hooks, "world");
    lua_setglobal(L, "world");
    lua_getfield(L, hooks, "create");
    push_toml(L, manifest);
    lua_call(L, 1, 0);
    lua_pop(L, 1);  // hooks
}

uint64_t Motherlib::take_hash(lua_State* L) {
    size_t len = 0;
    const void* data = lua_tobuffer(L, -2, &len);
    const double used = lua_tonumber(L, -1);
    Fnv64 h;
    if (data && used >= 0 && used <= double(len)) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size_t(used); ++i) h.byte(bytes[i]);
    }
    lua_pop(L, 2);
    return h.value();
}

std::string Motherlib::add_import(lua_State* L, const ImportSource& source) {
    lua_getref(L, import_ref_);
    lua_createtable(L, 0, 4);
    lua_pushstring(L, source.archetype.c_str());
    lua_setfield(L, -2, "archetype");
    push_toml(L, source.params);
    lua_setfield(L, -2, "params");
    lua_pushstring(L, source.cart.c_str());
    lua_setfield(L, -2, "source");
    lua_pushstring(L, source.export_id.c_str());
    lua_setfield(L, -2, "export");
    lua_call(L, 1, 1);
    std::string why = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
    lua_pop(L, 1);
    return why;
}

WorldAudit Motherlib::audit(lua_State* L) {
    lua_getref(L, audit_ref_);
    lua_call(L, 0, 4);
    WorldAudit out;
    out.out_most = int(lua_tointeger(L, -4));
    out.solids = lua_toboolean(L, -3) != 0;
    out.progress = lua_tonumber(L, -2);
    out.out_who = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
    lua_pop(L, 4);
    return out;
}

namespace {

std::string field_string(lua_State* L, const char* key) {
    lua_getfield(L, -1, key);
    std::string s = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
    lua_pop(L, 1);
    return s;
}

bool field_bool(lua_State* L, const char* key) {
    lua_getfield(L, -1, key);
    const bool b = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return b;
}

int field_int(lua_State* L, const char* key) {
    lua_getfield(L, -1, key);
    const int n = int(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return n;
}

constexpr const char* kAxes[] = {"gravity", "surface", "scroll", "avatar", "spawn", "deploy"};

}  // namespace

std::vector<ArchetypeInfo> Motherlib::describe(lua_State* L) {
    lua_getref(L, describe_ref_);
    lua_call(L, 0, 1);
    std::vector<ArchetypeInfo> out;
    const int n = lua_objlen(L, -1);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, -1, i);
        ArchetypeInfo a;
        a.name = field_string(L, "name");
        a.category = field_string(L, "category");
        a.layer = field_string(L, "layer");
        a.damage = field_string(L, "damage");
        a.deployable = field_bool(L, "deployable");
        a.solid = field_bool(L, "solid");
        a.hurts_owner = field_bool(L, "hurts_owner");
        a.tier = field_int(L, "tier");
        lua_getfield(L, -1, "sprites");
        for (int k = 1; k <= lua_objlen(L, -1); ++k) {
            lua_rawgeti(L, -1, k);
            a.sprites.push_back({field_string(L, "param"), field_int(L, "w"), field_int(L, "h"), field_int(L, "frames"),
                                 field_int(L, "default")});
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "requires");
        for (const char* axis : kAxes) {
            lua_getfield(L, -1, axis);
            std::vector<std::string>& values = a.accepts[axis];
            for (int k = 1; k <= lua_objlen(L, -1); ++k) {
                lua_rawgeti(L, -1, k);
                values.push_back(lua_tostring(L, -1));
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 2);  // requires, the record
        out.push_back(std::move(a));
    }
    lua_pop(L, 1);
    return out;
}

std::string Motherlib::check(lua_State* L, const std::string& archetype, const TomlValue& env) {
    lua_getref(L, check_ref_);
    lua_pushstring(L, archetype.c_str());
    push_toml(L, env);
    lua_call(L, 2, 1);
    std::string why = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
    lua_pop(L, 1);
    return why;
}

int Motherlib::use(lua_State* L, const std::string& name) {
    if (auto it = loaded_.find(name); it != loaded_.end()) {
        lua_getref(L, it->second);
        return 1;
    }
    const LibSource* source = find_source(name);
    if (!source) luaL_error(L, "use(\"%s\"): there is no lib/%s.luau", name.c_str(), name.c_str());
    if (loading_.count(name) != 0) luaL_error(L, "use(\"%s\"): a cycle of MOTHERLIB modules", name.c_str());

    std::string error;
    if (!load_module(L, lib_ref_, "lib/" + name, std::string(source->text, source->size), error)) {
        luaL_error(L, "%s", error.c_str());
    }
    struct Loading {
        std::set<std::string>& set;
        std::string name;
        ~Loading() { set.erase(name); }
    } loading{loading_, name};
    loading_.insert(name);
    lua_call(L, 0, 1);
    loaded_[name] = lua_ref(L, -1);
    return 1;
}

}  // namespace mother
