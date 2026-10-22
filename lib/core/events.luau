-- world:on() handlers. Events queue up as they happen and are delivered at the end of each
-- step, in order, to handlers in the order they were added; events raised by handlers are
-- delivered in the same pass.
--   spawn(id)
--   hit(id, tier, source_id)          the damage landed
--   death(id, view, source_id)        view: the entity as it died
--   objective(type, id)               the objective became met; id: race winner or escapee
local events = {}

local handlers = { spawn = {}, hit = {}, death = {}, objective = {} }
local queue = {}

events.names = table.freeze({ spawn = true, hit = true, death = true, objective = true })

function events.on(name, fn)
    table.insert(handlers[name], fn)
end

function events.emit(name, a, b, c)
    table.insert(queue, { name, a, b, c })
end

function events.flush()
    local i = 1
    while i <= #queue do
        local ev = queue[i]
        local list = handlers[ev[1]]
        for k = 1, #list do
            list[k](ev[2], ev[3], ev[4])
        end
        i += 1
    end
    table.clear(queue)
end

return table.freeze(events)
