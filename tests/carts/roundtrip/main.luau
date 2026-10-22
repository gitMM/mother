-- ROUNDTRIP: the M2 acceptance cart. art.png was converted to sprites.spr by tools/png2spr.
-- The cart draws it 1:1 at the top-left on a colour-0 background, so tests can compare every
-- pixel with the PNG. Beside it, the same art through spr, a 2x sspr, a mirrored 2x2 spr and
-- print.

function cart.init() end

function cart.update() end

function cart.draw()
    cls(0)
    sspr(0, 0, 128, 96, 0, 0)
    for n = 0, 15 do
        spr(n, n * 10, 104)
    end
    sspr(8, 8, 16, 16, 140, 16, 32, 32)
    spr(1, 180, 16, 2, 2, true, false)
    print("PNG2SPR ROUND TRIP", 140, 4, 22)
    print("1:1 SSPR, 16 SPR,\n2X SSPR, MIRRORED 2X2", 140, 56, 40)
end

function cart.hash()
    return 0
end
