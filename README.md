# MOTHER

A fantasy console with fixed virtual hardware that runs deterministic, sandboxed Luau carts.
The full spec is [BRIEF.md](BRIEF.md); choices the brief leaves open are in [DECISIONS.md](DECISIONS.md).

## Build

Needs CMake 3.25+, Ninja and a C++20 compiler. Every dependency is vendored in `vendor/`.
On Windows, run these from "Developer PowerShell for VS 2026" so MSVC is on the PATH.

```
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Use the `debug` preset for a debug build. The binary is `build/<preset>/bin/mother`.

## Run

```
mother run carts/knightfall                        # a cart in a window, with sound
mother run carts/breakout
mother run tests/carts/world
mother run tests/carts/dice --headless --ticks 18000 --input fuzz:42 --seed 7
                                                   # no window or audio device; fuzzed input
mother run tests/carts/song --headless --ticks 600 --wav song.wav   # also write the audio
mother record tests/carts/dice out.trace           # play in a window; writes seed, pads, hashes
mother record tests/carts/dice out.trace --headless --ticks 1800 --input fuzz:1
mother replay tests/carts/dice out.trace --hash    # re-runs it; fails on any state-hash mismatch
mother run tests/carts/dice --input trace:out.trace   # watch a trace
mother run                                         # palette test pattern
```

Each run ends with one line: `result=… ticks=… seed=… budget=… hash=…`, plus `audio=…` headless
and `replay=ok` or `replay=mismatch …` for replays. `tests/traces/` holds one trace per cart; CI
replays them on Windows, macOS and Linux.

## Carts

- **KNIGHTFALL** (`carts/knightfall`): survive three waves of skeletons. A sword, B dodge, R bomb.
- **BREAKOUT** (`carts/breakout`): clear 108 bricks with 3 balls. Some bricks drop a laser (A).

A cart is a folder with `cart.toml` and `main.luau` (plus any other `*.luau` modules, loaded with
`import("name")`). `main.luau` defines `cart.init`, `cart.update`, `cart.draw` and `cart.hash`.
`cart.toml` is the manifest: `[cart]`, `[env]`, `[objective]`, and `[avatar]`, `[[spawn_points]]`
and `[[exports]]` as the env requires; its schema is in `host/assets/manifest.h`.

Everything that moves, collides or takes damage lives in the MOTHERLIB world (`lib/`, Luau, built
into the binary): `world:spawn`, `get`, `query`, `nearest`, `damage`, `status`, `on`, `spawner`,
`avatar`, `camera` and `draw`, documented in `lib/core/world.luau`. Archetypes are modules in
`lib/archetypes/`; their format is in `lib/core/archetypes.luau`.
Optional `sprites.spr` (sheets 0–2 and palette ramps), `map.map` (two tile layers and tile flags)
and `audio.trk` (instruments, wavetables, sfx, patterns, songs, global effects) are plain text;
their grammars are in `host/assets/spr_format.h`, `map_format.h`, `trk_format.h` and `trk_cell.h`.

Convert a PNG (up to 256×768, alpha below 128 is transparent) to sprite sheets:

```
png2spr art.png sprites.spr
```

Keys: arrows/WASD = stick, Z = A, X = B, C = X, V = Y, Q = L, E = R, Enter = Start. Gamepads work too.
