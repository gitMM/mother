-- The cart's [objective] (brief section 6), checked once per step. The "objective" event fires
-- each time it becomes met; the cart decides what that means (win, next wave, ...).
--   kill_all  no host-faction entity with HP is left, and no host spawner has spawns left
--   survive   `ticks` steps have run since the game started: the cart's first world:spawn or
--             world:spawner call, so a title screen's time doesn't count
--   escape    the avatar overlaps `exit`
--   race      a racer has `laps` laps (counted along the racing line, core/race.luau): the
--             avatar, or a host-faction racer (wild racers don't count); the event names the winner
--   collect   no entity of `archetype` is left
local S = use("core/state")
local entity = use("core/entity")
local events = use("core/events")

local objective = {}

-- Objective entities: the ones the objective counts. Non-native (imported) entities can't
-- damage or block them.
function objective.counts(e)
    local o = S.objective
    if o.type == "kill_all" then
        return e.faction == "host" and e.damage == "hp"
    elseif o.type == "race" then
        return e == S.avatar or (e.faction == "host" and e.racer)
    elseif o.type == "collect" then
        return e.name == o.archetype
    end
    return false
end

function objective.lap(e)
    e.laps += 1
end

-- The game starts with the cart's first spawn (world:spawn or world:spawner); survive counts from
-- there.
function objective.start()
    if S.started == nil then
        S.started = S.tick
    end
end

local function met()
    local o = S.objective
    local list = S.list
    if o.type == "survive" then
        return S.started ~= nil and S.tick >= S.started + o.ticks, nil
    elseif o.type == "escape" then
        local a = S.avatar
        local x = o.exit
        return entity.live(a) and entity.overlaps(a, x.x, x.y, x.x + x.w, x.y + x.h), a and a.id
    elseif o.type == "race" then
        for i = 1, #list do
            local e = list[i]
            if entity.live(e) and objective.counts(e) and e.laps >= o.laps then
                return true, e.id
            end
        end
        return false, nil
    end
    -- kill_all and collect: nothing counted is left
    for i = 1, #list do
        if entity.live(list[i]) and objective.counts(list[i]) then
            return false, nil
        end
    end
    if o.type == "kill_all" then
        for i = 1, #S.spawners do
            local sp = S.spawners[i]
            if sp.faction == "host" and sp.left > 0 then
                return false, nil
            end
        end
    end
    return true, nil
end

-- The objective's progress, for the matrix test: S.progress only grows, by one for each counted
-- entity (or spawn still to come) fewer than the step before under kill_all and collect, each
-- step under survive, and each px the avatar gets nearer the exit than ever under escape. Under
-- race, core/race.luau adds one for each racing-line point a counted racer passes.
local function note_progress()
    local o = S.objective
    if o.type == "survive" then
        if S.started ~= nil then
            S.progress += 1
        end
    elseif o.type == "escape" then
        local a, x = S.avatar, o.exit
        if entity.live(a) then
            local dx = math.max(x.x - (a.x + a.hw), a.x - a.hw - (x.x + x.w), 0)
            local dy = math.max(x.y - (a.y + a.hh), a.y - a.hh - (x.y + x.h), 0)
            local d = math.sqrt(dx * dx + dy * dy)
            if d <= S.best_exit - 1 then
                S.progress += 1
                S.best_exit = d
            end
        end
    elseif o.type ~= "race" then
        local n = 0
        for _, e in S.list do
            if entity.live(e) and objective.counts(e) then
                n += 1
            end
        end
        if o.type == "kill_all" then
            for _, sp in S.spawners do
                if sp.faction == "host" then
                    n += sp.left
                end
            end
        end
        if S.last_count ~= nil and n < S.last_count then
            S.progress += S.last_count - n
        end
        S.last_count = n
    end
end

function objective.check()
    note_progress()
    local done, who = met()
    if done and not S.objective_met then
        events.emit("objective", S.objective.type, who)
    end
    S.objective_met = done
end

return table.freeze(objective)
