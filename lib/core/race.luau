-- The racing line (brief section 6): a race objective's `line`, a closed loop of points (px) laid
-- along the middle of the track. Every racer heads for one point at a time, in order, and has
-- passed it once its centre comes within RADIUS px. Passing point 1 starts its race; each later
-- pass of point 1, after all the others, is a lap. Laps feed the race objective.
local S = use("core/state")
local entity = use("core/entity")
local objective = use("core/objective")

local race = {}

race.RADIUS = 64

-- The line's points as { x, y } pairs, or nil when the cart has no race objective.
function race.line()
    local o = S.objective
    return if o.type == "race" then o.line else nil
end

-- The point e heads for next, or nil without a line.
function race.next(e)
    local line = race.line()
    if line == nil then
        return nil
    end
    local p = line[e.wp]
    return p[1], p[2]
end

-- Once per step, after movement: racers that reached their next point move on to the one after.
function race.step()
    local line = race.line()
    if line == nil then
        return
    end
    local r2 = race.RADIUS * race.RADIUS
    local list = S.list
    for i = 1, #list do
        local e = list[i]
        if e.racer and entity.live(e) and not e.ghost then
            local p = line[e.wp]
            local dx, dy = p[1] - e.x, p[2] - e.y
            if dx * dx + dy * dy <= r2 then
                if e.wp == 1 then
                    if e.started then
                        e.laps += 1
                    end
                    e.started = true
                end
                e.wp = e.wp % #line + 1
                if objective.counts(e) then
                    S.progress += 1
                end
            end
        end
    end
end

return table.freeze(race)
