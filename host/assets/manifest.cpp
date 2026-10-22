#include "assets/manifest.h"

#include <initializer_list>
#include <set>
#include <utility>

namespace mother {

namespace {

using Type = TomlValue::Type;
using Words = std::initializer_list<const char*>;

std::string join(Words words) {
    std::string out;
    for (const char* w : words) {
        if (!out.empty()) out += '|';
        out += w;
    }
    return out;
}

bool name_chars(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

// Schema checks that stop at the first problem, with "file:line: [table] key ..." messages.
class Checker {
public:
    Checker(const std::string& filename, std::string& error) : filename_(filename), error_(error) {}

    bool fail(int line, const std::string& message) {
        error_ = filename_ + ":" + std::to_string(line) + ": " + message;
        return false;
    }

    bool only(const TomlValue& t, Words allowed, const std::string& where) {
        for (size_t i = 0; i < t.keys.size(); ++i) {
            bool known = false;
            for (const char* a : allowed) known = known || t.keys[i] == a;
            if (!known) return fail(t.items[i].line, where + ": unknown key " + t.keys[i]);
        }
        return true;
    }

    // The value of a required key of the given type; Type::Float accepts any number.
    const TomlValue* get(const TomlValue& t, const char* key, Type type, const std::string& where) {
        const TomlValue* v = t.find(key);
        if (!v) {
            fail(t.line, where + ": missing " + key);
            return nullptr;
        }
        if (type == Type::Float ? !v->is_number() : v->type != type) {
            fail(v->line, where + " " + key + " must be " + (type == Type::Float ? "a number" : toml_type_name(type)) +
                              ", not " + toml_type_name(v->type));
            return nullptr;
        }
        return v;
    }

    bool word(const TomlValue& t, const char* key, Words words, const std::string& where) {
        const TomlValue* v = get(t, key, Type::String, where);
        if (!v) return false;
        for (const char* w : words) {
            if (v->string == w) return true;
        }
        return fail(v->line, where + " " + key + " must be " + join(words) + ", not \"" + v->string + "\"");
    }

    bool name(const TomlValue& t, const char* key, const std::string& where) {
        const TomlValue* v = get(t, key, Type::String, where);
        if (!v) return false;
        if (name_chars(v->string)) return true;
        return fail(v->line, where + " " + key + " must be letters, digits and _ only");
    }

    bool integer(const TomlValue& t, const char* key, int64_t min, const std::string& where) {
        const TomlValue* v = get(t, key, Type::Integer, where);
        if (!v) return false;
        if (v->integer >= min) return true;
        return fail(v->line, where + " " + key + " must be at least " + std::to_string(min));
    }

    bool numbers(const TomlValue& t, Words keys, const std::string& where) {
        for (const char* key : keys) {
            if (!get(t, key, Type::Float, where)) return false;
        }
        return true;
    }

    bool optional_table(const TomlValue& t, const char* key, const std::string& where) {
        return !t.find(key) || get(t, key, Type::Table, where);
    }

    // A [name] section: present (or absent, when allowed) and a table.
    const TomlValue* section(const TomlValue& doc, const char* name, bool required) {
        const TomlValue* v = doc.find(name);
        if (!v) {
            if (required) fail(1, std::string("missing [") + name + "]");
            return nullptr;
        }
        if (v->type != Type::Table) {
            fail(v->line, std::string(name) + " must be a [" + name + "] table");
            return nullptr;
        }
        return v;
    }

    // A [[name]] list: every element a table. Returns nullptr when absent.
    const TomlValue* list(const TomlValue& doc, const char* name) {
        const TomlValue* v = doc.find(name);
        if (!v) return nullptr;
        bool ok = v->type == Type::Array;
        for (size_t i = 0; ok && i < v->items.size(); ++i) ok = v->items[i].type == Type::Table;
        if (!ok) fail(v->line, std::string(name) + " must be a list of [[" + name + "]] tables");
        return ok ? v : nullptr;
    }

    bool failed() const { return !error_.empty(); }

private:
    const std::string& filename_;
    std::string& error_;
};

bool check_env(Checker& c, const TomlValue& env) {
    const std::string w = "[env]";
    if (!c.only(env, {"gravity", "surface", "scroll", "avatar", "spawn", "deploy", "bounds"}, w)) return false;
    if (!c.word(env, "gravity", {"none", "down"}, w) || !c.word(env, "surface", {"none", "ground"}, w) ||
        !c.word(env, "scroll", {"none", "follow", "auto"}, w) ||
        !c.word(env, "avatar", {"none", "up", "lr", "8dir", "heading"}, w) ||
        !c.word(env, "spawn", {"none", "world", "camera"}, w) || !c.get(env, "deploy", Type::Bool, w)) {
        return false;
    }
    const TomlValue* bounds = c.get(env, "bounds", Type::Table, w);
    if (!bounds) return false;
    const std::string wb = "[env] bounds";
    if (!c.only(*bounds, {"left", "right", "top", "bottom"}, wb)) return false;
    for (const char* edge : {"left", "right", "top", "bottom"}) {
        if (!c.word(*bounds, edge, {"solid", "kill", "despawn"}, wb)) return false;
    }
    return true;
}

bool check_objective(Checker& c, const TomlValue& obj) {
    const std::string w = "[objective]";
    if (!c.word(obj, "type", {"kill_all", "escape", "survive", "race", "collect"}, w)) return false;
    const std::string& type = obj.find("type")->string;
    if (type == "kill_all") return c.only(obj, {"type"}, w);
    if (type == "survive") return c.only(obj, {"type", "ticks"}, w) && c.integer(obj, "ticks", 1, w);
    if (type == "race") return c.only(obj, {"type", "laps"}, w) && c.integer(obj, "laps", 1, w);
    if (type == "collect") return c.only(obj, {"type", "archetype"}, w) && c.name(obj, "archetype", w);
    if (!c.only(obj, {"type", "exit"}, w)) return false;
    const TomlValue* exit = c.get(obj, "exit", Type::Table, w);
    const std::string we = "[objective] exit";
    if (!exit || !c.only(*exit, {"x", "y", "w", "h"}, we) || !c.numbers(*exit, {"x", "y", "w", "h"}, we)) return false;
    if (exit->find("w")->as_number() > 0 && exit->find("h")->as_number() > 0) return true;
    return c.fail(exit->line, we + " w and h must be positive");
}

bool check_avatar(Checker& c, const TomlValue& avatar) {
    const std::string w = "[avatar]";
    return c.only(avatar, {"archetype", "damage", "hp", "x", "y", "params"}, w) && c.name(avatar, "archetype", w) &&
           c.word(avatar, "damage", {"hp", "stun"}, w) && c.integer(avatar, "hp", 1, w) &&
           c.numbers(avatar, {"x", "y"}, w) && c.optional_table(avatar, "params", w);
}

bool check_spawn_points(Checker& c, const TomlValue* points, bool wanted, int line) {
    if (!wanted) {
        if (points) return c.fail(points->line, "[[spawn_points]] need [env] spawn = world or camera");
        return true;
    }
    if (!points || points->items.empty()) return c.fail(line, "[env] spawn needs at least one [[spawn_points]]");
    for (const TomlValue& p : points->items) {
        if (!c.only(p, {"x", "y"}, "[[spawn_points]]") || !c.numbers(p, {"x", "y"}, "[[spawn_points]]")) return false;
    }
    return true;
}

bool check_exports(Checker& c, const TomlValue& exports) {
    const std::string w = "[[exports]]";
    std::set<std::string> ids;
    for (const TomlValue& e : exports.items) {
        if (!c.only(e, {"id", "archetype", "params"}, w) || !c.name(e, "id", w) || !c.name(e, "archetype", w) ||
            !c.optional_table(e, "params", w)) {
            return false;
        }
        if (!ids.insert(e.find("id")->string).second) return c.fail(e.line, w + " id " + e.find("id")->string + " is used twice");
    }
    return true;
}

bool check_manifest(Checker& c, const TomlValue& doc) {
    for (size_t i = 0; i < doc.keys.size(); ++i) {
        const std::string& k = doc.keys[i];
        const bool known = k == "cart" || k == "env" || k == "objective" || k == "avatar" || k == "spawn_points" ||
                           k == "exports";
        if (!known) return c.fail(doc.items[i].line, "unknown table [" + k + "]");
    }
    const TomlValue* cart = c.section(doc, "cart", true);
    if (!cart || !c.only(*cart, {"id", "title", "family"}, "[cart]") || !c.name(*cart, "id", "[cart]") ||
        !c.get(*cart, "title", Type::String, "[cart]") ||
        !c.word(*cart, "family", {"action", "platform", "driving", "shooter", "puzzle"}, "[cart]")) {
        return false;
    }
    const TomlValue* env = c.section(doc, "env", true);
    if (!env || !check_env(c, *env)) return false;
    const TomlValue* objective = c.section(doc, "objective", true);
    if (!objective || !check_objective(c, *objective)) return false;

    const bool wants_avatar = env->find("avatar")->string != "none";
    const TomlValue* avatar = c.section(doc, "avatar", false);
    if (c.failed()) return false;
    if (avatar && !wants_avatar) return c.fail(avatar->line, "[avatar] needs [env] avatar other than none");
    if (!avatar && wants_avatar) return c.fail(env->find("avatar")->line, "[env] avatar needs an [avatar] table");
    if (avatar && !check_avatar(c, *avatar)) return false;

    const TomlValue* points = c.list(doc, "spawn_points");
    if (c.failed()) return false;
    const TomlValue& spawn = *env->find("spawn");
    if (!check_spawn_points(c, points, spawn.string != "none", spawn.line)) return false;

    const TomlValue* exports = c.list(doc, "exports");
    if (c.failed()) return false;
    return !exports || check_exports(c, *exports);
}

}  // namespace

bool parse_manifest(std::string_view text, const std::string& filename, TomlValue& out, std::string& error) {
    TomlValue doc;
    if (!parse_toml(text, filename, doc, error)) return false;
    std::string problem;
    Checker checker(filename, problem);
    if (!check_manifest(checker, doc)) {
        error = problem;
        return false;
    }
    out = std::move(doc);
    return true;
}

}  // namespace mother
