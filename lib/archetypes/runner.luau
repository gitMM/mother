-- RUNNER (body, JUMPER): the avatar. Runs left and right, speeding up to run speed; A jumps from
-- the ground at 4 px/tick, and letting go of A while rising cuts the jump short. Landing on an
-- entity of another faction (or anything that hurts its owner) from above is a stomp: tier 1 to
-- it, a bounce (higher with A held), and its contact damage can't hurt the runner that time. A
-- head-bump hits the tile above the runner's head (the one nearest its middle) and breaks it if
-- it's damageable. Needs gravity down and avatar lr.
local axes = use("core/axes")
local units = use("core/units")
local act = use("core/act")
local look = use("core/look")
local damage = use("core/damage")
local collision = use("core/collision")

local T = units.tile
local STOMP_SLACK = 4 -- px its feet may be below the other's top at the start of the step

-- Coming down on `other` from above?
local function stomping(e, other)
    local hostile = other.faction ~= e.faction or other.hurts_owner
    return hostile and e.sy + e.hh <= other.sy - other.hh + STOMP_SLACK and e.y - e.sy > other.y - other.sy
end

-- The tile row above its head: the solid cell nearest its middle takes the hit.
local function head_bump(e)
    local ty = math.floor((e.y - e.hh) / T) - 1
    for _, x in { e.x, e.x - e.hw, e.x + e.hw - 0.01 } do
        local tx = math.floor(x / T)
        if collision.solid_tile(tx, ty) then
            damage.tile(tx, ty)
            act.sound(e, e.params.sfx_bump)
            return
        end
    end
end

return {
    category = "body",
    layer = "ground",
    size = "M",
    w = 10,
    h = 14,
    kinematic = false, -- it falls
    damage = "hp",
    hp = 1,
    tier = 0,
    requires = {
        gravity = { "down" },
        surface = axes.ANY,
        scroll = axes.ANY,
        avatar = { "lr" },
        spawn = axes.ANY,
        deploy = axes.ANY,
    },
    params = {
        speed = units.run,
        accel = 0.125, -- px/tick², running and stopping
        jump = 4,
        cut = 1, -- the most upward speed left when A is let go early
        stomp_tier = 1,
        bounce = 3,
        sprite = 0, -- standing, two running frames, jumping; facing right
        sfx_jump = -1,
        sfx_stomp = -1,
        sfx_bump = -1,
    },
    sprites = { sprite = { w = 2, h = 2, frames = 4 } },
    sounds = { "sfx_jump", "sfx_stomp", "sfx_bump" },

    init = function(e)
        e.fx, e.fy = 1, 0
        e.mem.jumping = false
    end,

    update = function(e)
        local p, mem = e.params, e.mem
        local x = (if btn("right") then 1 else 0) - (if btn("left") then 1 else 0)
        local speed = act.speed(e, p.speed)
        if x ~= 0 then
            act.face(e, x, 0)
            e.vx = math.clamp(e.vx + x * p.accel, -speed, speed)
        elseif math.abs(e.vx) <= p.accel then
            e.vx = 0
        else
            e.vx -= act.sign(e.vx) * p.accel
        end
        if e.down then
            mem.jumping = false
            if btnp("a") then
                e.vy = -p.jump
                mem.jumping = true
                act.sound(e, p.sfx_jump)
            end
        end
        if mem.jumping and not btn("a") and e.vy < -p.cut then
            e.vy = -p.cut
        end
    end,

    on_block = function(e, nx, ny, other)
        if nx ~= 0 then
            e.vx = 0
        else
            e.vy = 0
            if ny > 0 and other == nil then
                head_bump(e)
            end
        end
    end,

    guard = function(e, other)
        return stomping(e, other)
    end,

    on_contact = function(e, other)
        if not stomping(e, other) then
            return
        end
        if other.damage ~= "none" then
            damage.hit(other, e.params.stomp_tier, e)
        end
        e.vy = -(if btn("a") then e.params.jump else e.params.bounce)
        e.mem.jumping = false
        act.sound(e, e.params.sfx_stomp)
    end,

    draw = function(e)
        if look.blinking(e) then
            return
        end
        local frame = 0
        if not e.down then
            frame = 3
        elseif e.vx ~= 0 then
            frame = 1 + (tick() // 6) % 2
        end
        look.sprite(e, "sprite", frame, e.fx < 0)
    end,
}
