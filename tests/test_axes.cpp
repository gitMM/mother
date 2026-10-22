#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "input/fuzz.h"
#include "test.h"
#include "world_cart.h"

using namespace mother;

// M8: the archetype axis tests (brief section 9). Every MOTHERLIB archetype (as MOTHERLIB describes
// it) gets a generated world for each value of each [env] axis it accepts, and for each bounds
// type. It is spawned with dummies of every faction and runs 600 ticks under fuzz input, checked
// every tick:
//   no Luau errors;
//   nothing stays wholly outside the bounds for more than 60 ticks (it stays in, or is removed);
//   it takes damage per its response: hp loses HP (and dies at 0), stun is stunned for 60 ticks,
//   none is never hurt;
//   contact and swing damage land only across factions, unless the source hurts its owner, and
//   blasts (bomb, mine) hit every faction.

namespace {

const char* const kAxes[] = {"gravity", "surface", "scroll", "avatar", "spawn", "deploy"};
const std::map<std::string, std::vector<std::string>> kValues = {
    {"gravity", {"none", "down"}},
    {"surface", {"ground", "none"}},
    {"scroll", {"none", "follow", "auto"}},
    {"avatar", {"8dir", "up", "lr", "heading", "none"}},
    {"spawn", {"world", "camera", "none"}},
    {"deploy", {"true", "false"}},
};

std::vector<ArchetypeInfo> describe_archetypes() {
    CartSource source;
    std::string error;
    TempCart cart("axes_probe", {{"cart.toml", kTestToml}, {"main.luau", lifecycle("")}});
    if (!load_cart_source(cart.dir(), source, error)) return {};
    CartVM vm;
    if (!vm.open(std::move(source))) return {};
    return vm.describe();
}

// The values of an axis the archetype runs under: what it requires, less what its category rules
// out (equipment needs an avatar, deployables need deploy).
std::vector<std::string> accepted(const ArchetypeInfo& a, const std::string& axis) {
    const std::vector<std::string>& listed = a.accepts.at(axis);
    std::vector<std::string> out;
    for (const std::string& v : kValues.at(axis)) {
        bool ok = listed.empty();
        for (const std::string& l : listed) ok = ok || l == v;
        if (axis == "avatar" && v == "none" && a.category == "equipment") ok = false;
        if (axis == "deploy" && v == "false" && a.deployable) ok = false;
        if (ok) out.push_back(v);
    }
    return out;
}

bool is_avatar_body(const ArchetypeInfo& a) {
    return a.category == "body" && !a.accepts.at("avatar").empty() && a.accepts.at("avatar")[0] != "none";
}

struct World {
    std::string label;
    std::map<std::string, std::string> env;
    std::string bounds = "solid";
};

std::string manifest(const ArchetypeInfo& a, const World& w) {
    const std::map<std::string, std::string>& e = w.env;
    std::string toml = "[cart]\nid = \"axes\"\ntitle = \"AXES\"\nfamily = \"action\"\n\n[env]\n";
    for (const char* axis : kAxes) {
        const bool word = std::string(axis) != "deploy";
        toml += std::string(axis) + " = " + (word ? "\"" + e.at(axis) + "\"" : e.at(axis)) + "\n";
    }
    const std::string b = "\"" + w.bounds + "\"";
    toml += "bounds = { left = " + b + ", right = " + b + ", top = " + b + ", bottom = " + b + " }\n\n";
    toml += "[objective]\ntype = \"survive\"\nticks = 36000\n\n";
    if (e.at("avatar") != "none") {
        toml += "[avatar]\narchetype = \"" + std::string(is_avatar_body(a) ? a.name : "dummy") +
                "\"\ndamage = \"" + std::string(is_avatar_body(a) ? (a.damage == "stun" ? "stun" : "hp") : "hp") +
                "\"\nhp = 30\nx = 160\ny = 100\n\n";
    }
    if (e.at("spawn") != "none") {
        for (const char* p : {"x = 40\ny = 40", "x = 280\ny = 40", "x = 40\ny = 140", "x = 280\ny = 140"}) {
            toml += std::string("[[spawn_points]]\n") + p + "\n\n";
        }
    }
    return toml;
}

// Checks every tick (see the top of the file); finishes "win" after 600 ticks.
const char* const kChecks = R"lua(
local target = nil
local outside = {}
local function bounds_rect()
    if AUTO then
        local cx, cy = world:camera()
        return cx, cy, cx + 320, cy + 180
    end
    return 0, 0, 320, 176
end
function cart.init()
    if not AVATAR then
        target = world:spawn(NAME, nil, 160, 100, FACTION)
    end
    for i, f in { "player", "host", "wild" } do
        world:spawn("dummy", { hp = 30, tier = 1, vx = i - 2, vy = 0.5, bounce = true }, 60 + 100 * (i - 1), 70, f)
    end
    world:on("hit", function(id, tier, source)
        local v, s = world:get(id), source and world:get(source)
        if v ~= nil and s ~= nil and v.faction == s.faction and v.id ~= s.id
            and not HURTS_OWNER[s.archetype] and s.archetype ~= "bomb" and s.archetype ~= "mine" then
            error(`{s.archetype} {s.id} hurt {v.archetype} {v.id} of its own faction ({v.faction})`)
        end
    end)
end
function cart.update()
    local t = tick()
    if t == 2 then
        target = target or world:avatar()
        local before = world:get(target)
        if before ~= nil then
            local landed = world:damage(target, 1)
            local after = world:get(target)
            if RESPONSE == "none" then
                check(not landed, "a 'none' response takes no damage")
            elseif RESPONSE == "hp" then
                check(landed, "an 'hp' body takes the hit")
                check(after == nil or after.hp == before.hp - 1, "and loses 1 HP (dying at 0)")
            else
                check(landed and after ~= nil and after.status.stun >= 59, "a 'stun' body is stunned for 60 ticks")
                check(after.hp == before.hp, "and loses no HP")
            end
        end
    end
    local x0, y0, x1, y1 = bounds_rect()
    for _, id in world:query(-100000, -100000, 200000, 200000) do
        local v = world:get(id)
        local out = v.x + v.w / 2 <= x0 or v.x - v.w / 2 >= x1 or v.y + v.h / 2 <= y0 or v.y - v.h / 2 >= y1
        outside[id] = if out then (outside[id] or 0) + 1 else 0
        check(outside[id] <= 60, `{v.archetype} {id} has been outside the bounds for 60 ticks at {v.x},{v.y}`)
    end
    if t == 600 then
        mother.finish("win")
    end
end
function cart.draw()
    cls(0)
    world:draw()
end
function cart.hash() return 0 end
)lua";

std::string checks(const ArchetypeInfo& a, const World& w, const std::vector<ArchetypeInfo>& all) {
    std::string pre = "local NAME = \"" + a.name + "\"\n";
    pre += "local FACTION = \"" + std::string(a.category == "creature" ? "host" : "player") + "\"\n";
    pre += "local RESPONSE = \"" + a.damage + "\"\n";
    pre += std::string("local AVATAR = ") + (is_avatar_body(a) && w.env.at("avatar") != "none" ? "true" : "false") + "\n";
    pre += std::string("local AUTO = ") + (w.env.at("scroll") == "auto" ? "true" : "false") + "\n";
    pre += "local HURTS_OWNER = {";
    for (const ArchetypeInfo& o : all) {
        if (o.hurts_owner) pre += " " + o.name + " = true,";
    }
    return pre + " }\n" + kChecks;
}

// A floor of critical tiles (rows 20-21) and a row of damageable blocks.
const std::string kMap = solid_map(40, 22, [](int x, int y) { return y >= 20 ? 3 : (y == 15 && x % 6 == 0) ? 2 : 0; });

}  // namespace

TEST(axes_every_archetype_runs_under_every_env_value_it_accepts) {
    const std::vector<ArchetypeInfo> all = describe_archetypes();
    CHECK(all.size() >= 27);
    int worlds = 0;
    for (const ArchetypeInfo& a : all) {
        // The base world: the first value of each axis it accepts (kValues lists the preferred first).
        World base;
        for (const char* axis : kAxes) base.env[axis] = accepted(a, axis).at(0);
        std::vector<World> list;
        for (const char* axis : kAxes) {
            for (const std::string& v : accepted(a, axis)) {
                World w = base;
                w.env[axis] = v;
                w.label = std::string(axis) + "=" + v;
                list.push_back(w);
            }
        }
        for (const char* b : {"solid", "kill", "despawn"}) {
            World w = base;
            w.bounds = b;
            w.label = std::string("bounds=") + b;
            list.push_back(w);
        }
        for (const World& w : list) {
            ++worlds;
            FuzzInput random_pad{uint64_t(worlds)};
            const InputFn input = [&random_pad](int) { return random_pad.next(); };
            const WorldRun run = run_world("axes", manifest(a, w), checks(a, w, all), 600, {{"map.map", kMap}}, input);
            if (run.outcome != Outcome::Win) {
                std::printf("  %s, %s: %s\n", a.name.c_str(), w.label.c_str(), run.message.c_str());
                CHECK(run.outcome == Outcome::Win);
            }
        }
    }
    std::printf("  %zu archetypes, %d worlds\n", all.size(), worlds);
}
