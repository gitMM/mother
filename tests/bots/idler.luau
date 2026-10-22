-- Any cart: starts the game, then stands still.
return function(t)
    return 0, 0, if t == 5 then "a" else ""
end
