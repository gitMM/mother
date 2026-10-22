-- Gravity and the speed limit (brief section 6, "Units").
local S = use("core/state")
local units = use("core/units")

local motion = {}

-- Gravity moves every body that isn't kinematic, under [env] gravity = "down".
function motion.falls(e)
    return S.env.gravity == "down" and not e.kinematic
end

local function finite(v)
    return v - v == 0
end

-- Applies gravity and clamps each axis to the projectile speed, the hard maximum.
function motion.integrate(e)
    if not (finite(e.vx) and finite(e.vy) and finite(e.x) and finite(e.y)) then
        error(`{e.name} (entity {e.id}) has a position or velocity that isn't a finite number`, 0)
    end
    if motion.falls(e) then
        e.vy = math.min(e.vy + units.gravity, units.terminal)
    end
    e.vx = math.clamp(e.vx, -units.max_speed, units.max_speed)
    e.vy = math.clamp(e.vy, -units.max_speed, units.max_speed)
end

return table.freeze(motion)
