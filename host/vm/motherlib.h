#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "assets/toml.h"

struct lua_State;

namespace mother {

struct ImportSource;

// An archetype's declarations (lib/core/archetypes.luau), as the host sees them.
struct ArchetypeInfo {
    std::string name, category, layer, damage;
    bool deployable = false, solid = false, hurts_owner = false;
    int tier = 0;
    std::map<std::string, std::vector<std::string>> accepts;  // requires: axis -> values; empty = any
};

// The matrix test's checks after a step (lib/core/imports.luau).
struct WorldAudit {
    int out_most = 0;        // the most steps any entity has been wholly outside the bounds
    bool solids = false;     // an imported solid is in the world
    double progress = 0;     // grows as the objective gets nearer
    std::string out_who;     // the entity that has been outside longest, described
};

// MOTHERLIB (brief sections 2 and 6): the Luau library in lib/, compiled into the binary and
// loaded into every cart VM. Its modules run in their own global tables, which read through to
// two private names and then the cart API: use("core/x") loads another MOTHERLIB module, and
// `host` holds the few host functions the library needs (modules, map_size, in_draw; btn and
// btnp that also read X and Y; sfx that also plays on voices 7 and 8; and import_tiles,
// import_sfx and outline for imports). Carts reach only the `world` table.
class Motherlib {
public:
    // Loads lib/core/world.luau, creates the world from the checked manifest and sets the
    // `world` global. Call on a cart state that isn't frozen yet, inside a protected call: it
    // raises Luau errors. api_ref is a registry ref to the cart API (the globals table).
    void open(lua_State* L, int api_ref, const TomlValue& manifest);

    // Registry refs to the step and hash hooks. Calling the hash hook leaves a buffer and a byte
    // count on the stack; take_hash() pops them and returns their FNV-1a 64.
    int step_ref() const { return step_ref_; }
    int hash_ref() const { return hash_ref_; }
    static uint64_t take_hash(lua_State* L);

    // Hands an import to the world (after open, inside a protected call; raises Luau errors).
    // Returns "" when the cart takes it, else why not, e.g. "needs surface=ground, host has none".
    std::string add_import(lua_State* L, const ImportSource& source);
    // After a step, inside a protected call.
    WorldAudit audit(lua_State* L);
    // Every archetype, sorted by name, inside a protected call.
    std::vector<ArchetypeInfo> describe(lua_State* L);

    // For use(): pushes the module's value, running it the first time.
    int use(lua_State* L, const std::string& name);

private:
    int lib_ref_ = 0;  // the table MOTHERLIB globals read through to
    int step_ref_ = 0;
    int hash_ref_ = 0;
    int import_ref_ = 0;
    int audit_ref_ = 0;
    int describe_ref_ = 0;
    std::map<std::string, int> loaded_;  // module name -> registry ref of its value
    std::set<std::string> loading_;      // for cycle errors
};

}  // namespace mother
