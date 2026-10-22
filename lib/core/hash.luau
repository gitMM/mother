-- The world's part of the per-tick state hash (brief section 4): the step count, camera,
-- objective, spawners and, for every entity in id order, its id, faction, position, velocity,
-- HP, status timers and laps. Written little-endian into a buffer that the host hashes, with
-- every NaN written as one value and -0 as 0 so the bytes match on every platform.
local S = use("core/state")

local hash = {}

local FACTION = { player = 1, host = 2, wild = 3 }
local ENTITY_BYTES = 4 + 1 + 5 * 8 + 5 * 4
local SPAWNER_BYTES = 4 * 4

local buf = buffer.create(1024)

local function f64(o, v)
    if v ~= v then
        buffer.writeu32(buf, o, 0)
        buffer.writeu32(buf, o + 4, 0x7FF80000)
    elseif v == 0 then
        buffer.writef64(buf, o, 0)
    else
        buffer.writef64(buf, o, v)
    end
    return o + 8
end

local function u32(o, v)
    buffer.writeu32(buf, o, v)
    return o + 4
end

-- Returns the buffer and how many bytes of it to hash.
function hash.write()
    local list, spawners = S.list, S.spawners
    local size = 64 + SPAWNER_BYTES * #spawners + ENTITY_BYTES * #list
    if buffer.len(buf) < size then
        buf = buffer.create(math.max(size, 2 * buffer.len(buf)))
    end
    local o = u32(0, S.tick)
    o = u32(o, S.next_id)
    o = f64(o, S.cam_x)
    o = f64(o, S.cam_y)
    o = u32(o, if S.objective_met then 1 else 0)
    o = u32(o, #spawners)
    for i = 1, #spawners do
        local sp = spawners[i]
        o = u32(o, sp.id)
        o = u32(o, sp.left)
        o = u32(o, sp.timer)
        o = u32(o, sp.next)
    end
    o = u32(o, #list)
    for i = 1, #list do
        local e = list[i]
        o = u32(o, e.id)
        buffer.writeu8(buf, o, FACTION[e.faction])
        o += 1
        o = f64(o, e.x)
        o = f64(o, e.y)
        o = f64(o, e.vx)
        o = f64(o, e.vy)
        o = f64(o, e.hp)
        o = u32(o, e.stun)
        o = u32(o, e.slip)
        o = u32(o, e.knock)
        o = u32(o, e.hurt)
        o = u32(o, e.laps)
    end
    return buf, o
end

return table.freeze(hash)
