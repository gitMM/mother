-- Argument checks for the world API. `level` goes straight to error(): world methods pass 3, so
-- the message points at the cart code that called them.
local check = {}

function check.finite(v)
    return type(v) == "number" and v - v == 0
end

function check.number(v, what, level)
    if not check.finite(v) then
        error(`{what} must be a finite number, got {v}`, level)
    end
    return v
end

function check.integer(v, what, min, level)
    if not check.finite(v) or v ~= math.floor(v) or v < min then
        error(`{what} must be a whole number of at least {min}, got {v}`, level)
    end
    return v
end

function check.one_of(v, set, what, level)
    if set[v] == nil then
        local names = {}
        for name in set do
            table.insert(names, tostring(name))
        end
        table.sort(names)
        error(`{what} must be one of {table.concat(names, ", ")}, got {v}`, level)
    end
    return v
end

function check.table(v, what, level)
    if type(v) ~= "table" then
        error(`{what} must be a table, got {type(v)}`, level)
    end
    return v
end

-- Every key of t must be in `allowed` (a set). The order keys are visited in only decides which
-- unknown key is reported, so they are sorted first.
function check.keys(t, allowed, what, level)
    local unknown = {}
    for k in t do
        if allowed[k] == nil then
            table.insert(unknown, tostring(k))
        end
    end
    if #unknown > 0 then
        table.sort(unknown)
        error(`{what}: unknown key {unknown[1]}`, level)
    end
end

return table.freeze(check)
