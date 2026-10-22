-- The simulation standard's numbers (brief section 6). All are the brief's, except hurt_ticks
-- and flag_offroad (DECISIONS.md).
return table.freeze({
    -- speed tiers, px/tick; projectile is the hard maximum on each axis
    crawl = 0.5,
    walk = 1,
    run = 2,
    fast = 3,
    projectile = 4,
    max_speed = 4,

    gravity = 0.25, -- px/tick², under [env] gravity = "down"
    terminal = 4, -- fall speed limit
    auto_scroll = 0.5, -- px/tick upwards, under [env] scroll = "auto"

    stun_ticks = 60, -- a stun-response body is stunned this long per hit
    hurt_ticks = 30, -- no further damage for this long after a hit

    tile = 8,
    screen_w = 320,
    screen_h = 180,

    -- tile flag bits; offroad is MOTHERLIB's own (DECISIONS.md)
    flag_solid = 0,
    flag_damageable = 1,
    flag_critical = 2,
    flag_offroad = 3,
})
