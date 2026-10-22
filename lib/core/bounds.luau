-- The world's edges and the camera (brief section 6, "Bounds").
--   scroll none    the camera stays at (0, 0); the bounds are the world rectangle
--   scroll follow  the camera centres on the avatar inside the world rectangle (the bounds)
--   scroll auto    the camera rises units.auto_scroll px/tick from (0, 0); the bounds are its view
-- The world rectangle is layer 0 of the map from (0, 0), or the screen when the cart has no map.
-- An edge applies to an entity once it has overlapped the bounds, so things can enter from
-- outside. solid: kept inside, and blocked as by a wall; kill: dies once wholly past the edge;
-- despawn: removed silently once wholly past. Under follow and auto scroll, an entity more than
-- one screen outside the camera view is removed silently too, unless it is the avatar or an
-- objective entity.
local S = use("core/state")
local units = use("core/units")
local entity = use("core/entity")
local damage = use("core/damage")
local collision = use("core/collision")
local objective = use("core/objective")

local W, H = units.screen_w, units.screen_h
local bounds = {}

function bounds.setup()
    local w, h = host.map_size(0)
    if w > 0 and h > 0 then
        S.world_w, S.world_h = w * units.tile, h * units.tile
    else
        S.world_w, S.world_h = W, H
    end
end

function bounds.rect()
    if S.env.scroll == "auto" then
        return S.cam_x, S.cam_y, S.cam_x + W, S.cam_y + H
    end
    return 0, 0, S.world_w, S.world_h
end

-- Marks e as entered once it overlaps the bounds.
function bounds.enter(e)
    if not e.entered then
        local x0, y0, x1, y1 = bounds.rect()
        e.entered = entity.overlaps(e, x0, y0, x1, y1)
    end
end

local function follow(centre, view, size)
    if size <= view then
        return 0
    end
    return math.clamp(math.floor(centre - view / 2), 0, size - view)
end

function bounds.update_camera()
    local scroll = S.env.scroll
    if scroll == "follow" then
        local a = S.avatar
        if entity.live(a) then
            S.cam_x, S.cam_y = follow(a.x, W, S.world_w), follow(a.y, H, S.world_h)
        end
    elseif scroll == "auto" then
        S.cam_y = -units.auto_scroll * S.tick
    end
end

local function leave(e, rule)
    if rule == "kill" then
        damage.kill(e, nil)
    else
        entity.remove(e, "despawn")
    end
end

-- A solid edge pushes e back inside; it only counts as a block when e was moving outwards.
local function push(e, moving_out, nx, ny)
    if nx > 0 then
        e.left = true
    elseif nx < 0 then
        e.right = true
    elseif ny > 0 then
        e.up = true
    else
        e.down = true
    end
    if moving_out then
        collision.block(e, nx, ny, nil)
    end
end

function bounds.apply(e)
    if e.ghost then
        return
    end
    bounds.enter(e)
    if e.entered then
        local x0, y0, x1, y1 = bounds.rect()
        local edges = S.env.bounds
        if e.x - e.hw < x0 then
            if edges.left == "solid" then
                e.x = x0 + e.hw
                push(e, e.vx < 0, 1, 0)
            elseif e.x + e.hw <= x0 then
                return leave(e, edges.left)
            end
        end
        if e.x + e.hw > x1 then
            if edges.right == "solid" then
                e.x = x1 - e.hw
                push(e, e.vx > 0, -1, 0)
            elseif e.x - e.hw >= x1 then
                return leave(e, edges.right)
            end
        end
        if e.y - e.hh < y0 then
            if edges.top == "solid" then
                e.y = y0 + e.hh
                push(e, e.vy < 0, 0, 1)
            elseif e.y + e.hh <= y0 then
                return leave(e, edges.top)
            end
        end
        if e.y + e.hh > y1 then
            if edges.bottom == "solid" then
                e.y = y1 - e.hh
                push(e, e.vy > 0, 0, -1)
            elseif e.y - e.hh >= y1 then
                return leave(e, edges.bottom)
            end
        end
    end
    if S.env.scroll ~= "none" and e ~= S.avatar and not objective.counts(e) then
        local cx, cy = S.cam_x, S.cam_y
        if e.x + e.hw < cx - W or e.x - e.hw > cx + 2 * W or e.y + e.hh < cy - H or e.y - e.hh > cy + 2 * H then
            entity.remove(e, "despawn")
        end
    end
end

return table.freeze(bounds)
