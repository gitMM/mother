-- Equipment (brief section 7): an entity held by the avatar that acts when its button is pressed.
-- Params every equipment archetype has (equip.PARAMS):
--   button  "a", "b", "l" or "r" when the cart binds it; "x" when the import system does
--   ticks   how long it stays once held; 0 = for good
--   loose   true: it starts as a pickup, drifting down at `drop` px/tick (a cart's native pickup,
--           such as a laser from a brick); the avatar takes it by touching it
--   drop    see loose
-- Taking a second copy of something already held renews the held one's time instead.
local S = use("core/state")
local entity = use("core/entity")
local act = use("core/act")

local equip = {}

equip.PARAMS = table.freeze({ button = "a", ticks = 0, loose = false, drop = 1 })

local BUTTONS = { a = true, b = true, l = true, r = true, x = true, y = true }

-- Copies the shared params into an archetype's own.
function equip.params(own)
    for k, v in equip.PARAMS do
        own[k] = v
    end
    return own
end

-- Holds e with `holder`, unless the holder already has one; returns true when it did.
function equip.attach(e, holder)
    local list = S.list
    for i = 1, #list do
        local o = list[i]
        if o ~= e and o.name == e.name and o.mem.holder == holder and entity.live(o) then
            o.mem.left = o.params.ticks
            entity.remove(e, "remove")
            return false
        end
    end
    local m = e.mem
    m.holder, m.left = holder, e.params.ticks
    m.vx, m.vy = 0, 0
    e.layer = "air"
    e.faction = holder.faction
    e.x, e.y = holder.x, holder.y
    return true
end

function equip.init(e)
    if not BUTTONS[e.params.button] then
        error(`{e.name}: button must be a, b, l, r, x or y, got {e.params.button}`, 0)
    end
    if e.params.loose then
        e.layer = "object"
        e.mem.vx, e.mem.vy = 0, e.params.drop
    else
        equip.attach(e, S.avatar)
    end
end

-- Every update. A loose pickup drifts. A held one follows its holder, runs out of time, and goes
-- when the holder goes. Returns the holder when it is free to act this tick, else nil.
function equip.update(e)
    local m = e.mem
    local h = m.holder
    if h == nil then
        act.drive(e)
        return nil
    end
    if not entity.live(h) then
        entity.remove(e, "remove")
        return nil
    end
    e.x, e.y, e.vx, e.vy = h.x, h.y, 0, 0
    if e.params.ticks > 0 then
        m.left -= 1
        if m.left <= 0 then
            entity.remove(e, "remove")
            return nil
        end
    end
    return if act.busy(h) then nil else h
end

-- Its button went down this tick.
function equip.pressed(e)
    return host.btnp(e.params.button)
end

-- A loose pickup touched by the avatar is taken (or renews the copy it holds). Returns true when
-- it was.
function equip.take(e, other)
    if e.mem.holder ~= nil or other ~= S.avatar then
        return false
    end
    equip.attach(e, other)
    return true
end

function equip.held(e)
    return e.mem.holder ~= nil
end

return table.freeze(equip)
