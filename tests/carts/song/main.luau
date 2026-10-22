-- SONG: the M3 test cart. Loops song 0 from audio.trk, plays sfx 0 on A and the one-shot
-- fanfare (song 1) on B, then returns to song 0 when the fanfare ends. Shows the music position
-- from beat() and pattern 0's current row read back with song_get(). From bar 8 the bass in
-- pattern 0 plays an octave up: song_set rewrites its notes while the song runs.

local lifted = false

function cart.init()
    music(0)
end

function cart.update()
    if btnp("a") then
        sfx(0)
    end
    if btnp("b") then
        music(1)
    end
    local bar = beat()
    if bar == nil then
        music(0)
    elseif bar >= 8 and not lifted then
        for row = 0, 63 do
            local note, inst, vol, fx = song_get(0, row, 2)
            if note > 0 then
                song_set(0, row, 2, note + 12, inst, vol, fx)
            end
        end
        lifted = true
    end
end

local NAMES = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" }

local function note_name(note)
    if note == 0 then
        return "---"
    elseif note == -1 then
        return "==="
    end
    return NAMES[(note - 1) % 12 + 1] .. (note - 1) // 12
end

function cart.draw()
    cls(17)
    print("SONG  A: SFX  B: FANFARE", 4, 4, 22)
    local bar, b, row = beat()
    if bar == nil then
        return
    end
    print("BAR " .. bar .. "  BEAT " .. b .. "  ROW " .. row, 4, 14, 9)
    for i = 0, 3 do
        rectfill(4 + i * 12, 26, 10, 6, if i == b then 9 else 40)
    end
    for ch = 1, 6 do
        local note, inst = song_get(0, row, ch)
        print(note_name(note) .. " " .. (if inst >= 0 then inst else ".."), 4 + (ch - 1) * 52, 40, if note ~= 0 then 22 else 40)
    end
    if lifted then
        print("BASS LIFTED BY SONG_SET", 4, 52, 12)
    end
end

function cart.hash()
    local bar = beat()
    return bar or -1
end
