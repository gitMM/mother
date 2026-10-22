-- The box the player moves: position, size and flash timer, kept on screen.

local box = { x = 0, y = 0, size = 16, flash = 0 }

function box.reset()
    box.x, box.y, box.flash = 152, 82, 0
end

function box.move(dx, dy)
    box.x = math.clamp(box.x + dx, 0, 320 - box.size)
    box.y = math.clamp(box.y + dy, 0, 180 - box.size)
end

return box
