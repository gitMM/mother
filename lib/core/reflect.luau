-- Reflection (brief section 6): a reflector sends objects and projectiles back and takes
-- ownership of them; a ball bounces off other bodies.
local act = use("core/act")
local factions = use("core/factions")

local reflect = {}

local MAX_ANGLE = math.pi / 3 -- off the reflector's facing, at the ends of its face

-- Objects and projectiles are what reflectors send back.
function reflect.reflectable(o)
    return o.arch.category == "object" or o.layer == "projectile"
end

-- obj touched reflector r, which faces (r.fx, r.fy). When obj is moving into r, it leaves along
-- r's facing at the same speed, angled by where it hit across r's face (up to 60° at the ends),
-- and r's faction now owns it. Returns true when it reflected.
function reflect.off(r, obj)
    local fx, fy = r.fx, r.fy
    local vx, vy = act.velocity(obj)
    if vx * fx + vy * fy >= 0 then
        return false
    end
    local tx, ty = -fy, fx
    local half = math.abs(tx) * r.hw + math.abs(ty) * r.hh
    local across = math.clamp(((obj.x - r.x) * tx + (obj.y - r.y) * ty) / half, -1, 1)
    local a = across * MAX_ANGLE
    local c, s = m.cos(a), m.sin(a)
    local speed = math.sqrt(vx * vx + vy * vy)
    act.push(obj, speed * (fx * c + tx * s), speed * (fy * c + ty * s))
    factions.transfer(obj, r)
    return true
end

-- obj bounced off body b: it reverses along the axis on which it is furthest from b's centre
-- (relative to their sizes), so that it moves away from b. Returns true when it changed course.
function reflect.bounce(obj, b)
    local dx = (obj.x - b.x) / (obj.hw + b.hw)
    local dy = (obj.y - b.y) / (obj.hh + b.hh)
    local vx, vy = act.velocity(obj)
    local nvx, nvy = vx, vy
    if math.abs(dx) >= math.abs(dy) then
        nvx = math.abs(vx) * act.sign(dx)
    else
        nvy = math.abs(vy) * act.sign(dy)
    end
    if nvx == vx and nvy == vy then
        return false
    end
    act.push(obj, nvx, nvy)
    return true
end

return table.freeze(reflect)
