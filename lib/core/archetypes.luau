-- The archetype registry: every lib/archetypes/*.luau module, checked when the world is created.
-- An archetype module returns a table with:
--   category     "body" | "creature" | "object" | "equipment"
--   layer        "ground" | "air" | "object" | "projectile"
--   size         sprite tier: "S" (8x8), "M" (16x16) or "L" (32x32)
--   w, h         collider in px: even, 2-32
--   kinematic    true when gravity never moves it; required, so the choice is explicit
--   damage       "hp" (dies at 0), "stun" (stunned per hit, never dies) or "none" (immune)
--   hp           starting HP; 0 unless damage is "hp"
--   tier         contact damage tier: 0 (none), 1, 3, 10 or 30
--   requires     { axis = { accepted values } } over gravity, surface, scroll, avatar, spawn and
--                deploy; an axis left out accepts every value
--   params       default parameters; world:spawn may override exactly these keys
-- and optionally:
--   solid        blocks other entities' movement (default false)
--   hurts_owner  its contact damage also hits its own faction (default false)
--   racer        counts as a racer for a race objective (default false)
--   deployable   needs [env] deploy = true (default false)
--   init(e) update(e) draw(e) on_contact(e, other) on_block(e, nx, ny)
local check = use("core/check")

local archetypes = {}
local defs = {}

local AXES = { "gravity", "surface", "scroll", "avatar", "spawn", "deploy" }
local AXIS_VALUES = {
    gravity = { none = true, down = true },
    surface = { none = true, ground = true },
    scroll = { none = true, follow = true, auto = true },
    avatar = { none = true, up = true, lr = true, ["8dir"] = true, heading = true },
    spawn = { none = true, world = true, camera = true },
    deploy = { [true] = true, [false] = true },
}

archetypes.CATEGORIES = table.freeze({ body = true, creature = true, object = true, equipment = true })
archetypes.LAYERS = table.freeze({ ground = true, air = true, object = true, projectile = true })
archetypes.DAMAGE = table.freeze({ hp = true, stun = true, none = true })
archetypes.TIERS = table.freeze({ [0] = true, [1] = true, [3] = true, [10] = true, [30] = true })
local SIZES = { S = true, M = true, L = true }
local FIELDS = {
    category = true, layer = true, size = true, w = true, h = true, kinematic = true, damage = true,
    hp = true, tier = true, requires = true, params = true, solid = true, hurts_owner = true,
    racer = true, deployable = true, init = true, update = true, draw = true, on_contact = true,
    on_block = true,
}
local HOOKS = { "init", "update", "draw", "on_contact", "on_block" }
local FLAGS = { "solid", "hurts_owner", "racer", "deployable" }

function archetypes.collider(v)
    return type(v) == "number" and v >= 2 and v <= 32 and v % 2 == 0
end

local function validate(name, d)
    local where = `archetypes/{name}.luau`
    local function bad(message)
        error(`{where}: {message}`, 0)
    end
    check.table(d, where, 0)
    check.keys(d, FIELDS, where, 0)
    check.one_of(d.category, archetypes.CATEGORIES, `{where} category`, 0)
    check.one_of(d.layer, archetypes.LAYERS, `{where} layer`, 0)
    check.one_of(d.size, SIZES, `{where} size`, 0)
    check.one_of(d.damage, archetypes.DAMAGE, `{where} damage`, 0)
    check.one_of(d.tier, archetypes.TIERS, `{where} tier`, 0)
    if not archetypes.collider(d.w) or not archetypes.collider(d.h) then
        bad("w and h must be even, 2-32")
    end
    if type(d.kinematic) ~= "boolean" then
        bad("kinematic must be true or false")
    end
    check.integer(d.hp, `{where} hp`, 0, 0)
    if (d.damage == "hp") ~= (d.hp > 0) then
        bad("hp must be positive exactly when damage is \"hp\"")
    end
    for _, flag in FLAGS do
        if d[flag] ~= nil and type(d[flag]) ~= "boolean" then
            bad(`{flag} must be true or false`)
        end
    end
    for _, hook in HOOKS do
        if d[hook] ~= nil and type(d[hook]) ~= "function" then
            bad(`{hook} must be a function`)
        end
    end
    check.table(d.params, `{where} params`, 0)
    for k, v in d.params do
        if type(k) ~= "string" or type(v) == "function" then
            bad("params must map names to values")
        end
    end
    check.table(d.requires, `{where} requires`, 0)
    check.keys(d.requires, AXIS_VALUES, `{where} requires`, 0)
    for _, axis in AXES do
        local accepted = d.requires[axis]
        if accepted ~= nil then
            if type(accepted) ~= "table" or #accepted == 0 then
                bad(`requires.{axis} must be a list of accepted values`)
            end
            for _, v in accepted do
                check.one_of(v, AXIS_VALUES[axis], `{where} requires.{axis}`, 0)
            end
            table.freeze(accepted)
        end
    end
    table.freeze(d.params)
    table.freeze(d.requires)
    return table.freeze(d)
end

-- Loads and checks every archetype module, once the core is loaded (archetypes use it).
function archetypes.load()
    for _, module in host.modules() do
        if string.sub(module, 1, 11) == "archetypes/" then
            local name = string.sub(module, 12)
            defs[name] = validate(name, use(module))
        end
    end
end

function archetypes.get(name)
    return if type(name) == "string" then defs[name] else nil
end

-- Why `def` can't run under `env`, as "needs surface=ground, host has none", or nil when it can.
-- Category rules come on top of `requires`: equipment needs an avatar, deployables need deploy.
function archetypes.check(def, env)
    for _, axis in AXES do
        local accepted = def.requires[axis]
        if accepted ~= nil and table.find(accepted, env[axis]) == nil then
            local names = {}
            for _, v in accepted do
                table.insert(names, tostring(v))
            end
            return `needs {axis}={table.concat(names, "|")}, host has {env[axis]}`
        end
    end
    if def.category == "equipment" and env.avatar == "none" then
        return "needs an avatar, host has avatar=none"
    end
    if def.deployable and env.deploy ~= true then
        return "needs deploy=true, host has false"
    end
    return nil
end

-- The archetype's default params with `given` (a table or nil) laid over them. Unknown keys and
-- values of the wrong type are errors at `level`; 0 when no cart code is to blame.
function archetypes.params(name, def, given, level)
    local inner = if level == 0 then 0 else level + 1
    local p = table.clone(def.params)
    if given ~= nil then
        check.table(given, `{name} params`, inner)
        check.keys(given, def.params, `{name} params`, inner)
        for k, default in def.params do
            local v = given[k]
            if v ~= nil then
                if type(v) ~= type(default) then
                    error(`{name} params: {k} must be a {type(default)}, got {type(v)}`, level)
                end
                p[k] = if type(v) == "table" then table.freeze(table.clone(v)) else v
            end
        end
    end
    return table.freeze(p)
end

return table.freeze(archetypes)
