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
mother run carts/jumper                            # also breakout, blockfall, rally, starfighter
mother run tests/carts/world
mother run tests/carts/dice --headless --ticks 18000 --input fuzz:42 --seed 7
                                                   # no window or audio device; fuzzed input
mother run tests/carts/song --headless --ticks 600 --wav song.wav   # also write the audio
mother record tests/carts/dice out.trace           # play in a window; writes seed, pads, hashes
mother record tests/carts/dice out.trace --headless --ticks 1800 --input fuzz:1
mother replay tests/carts/dice out.trace --hash    # re-runs it; fails on any state-hash mismatch
mother run tests/carts/dice --input trace:out.trace   # watch a trace
mother run                                         # palette test pattern
mother import carts/knightfall breakout:ball       # a cart with up to 3 exports of other carts
mother matrix                                      # every export against every cart; writes matrix.md
mother rogue                                       # a run: all six carts in a random order
```

**Runs.** `mother rogue [CARTS]` plays every cart in CARTS (default `carts`) once, in an order
shuffled from the seed (which also seeds each cart). Win a cart to go on; lose one and the run is
over. After each win you take one of that cart's exports along, or skip (B). You hold up to three,
and taking a fourth means dropping one. Every later cart that can host what you hold gets it, as
with `mother import`: tools on X (key C), drops on Y (key V), and wild things that attack anyone.
Exports a cart can't host sit that cart out, and the screen before it says why. When the run ends,
A starts a new one (seed + 1) and B quits.

**Imports.** `mother import` runs a cart with exports of others (`SOURCE:EXPORT`, SOURCE a cart
folder next to it). It refuses one the cart's `[env]` can't host, saying which axis failed, and
exits 3. Imported art goes into sheet 3, and imported sounds play on voices 7–8. Equipment is used
with X, and deployables are dropped with Y. Creatures come in at the cart's spawn points, and other
objects appear at the start. Imported things get a Mother Gold outline. `mother matrix` tries every
export in every other cart: it writes the compatibility table ([matrix.md](matrix.md)) and runs
each compatible pair for 3 fuzz seeds of 10 minutes under checks (errors, CPU budget, bounds,
replay, objective progress).

Each run ends with one line: `result=… ticks=… seed=… budget=… hash=…`, plus `audio=…` headless
and `replay=ok` or `replay=mismatch …` for replays. `tests/traces/` holds one trace per cart; CI
replays them on Windows, macOS and Linux.

## Carts

- **BLOCKFALL** (`carts/blockfall`): survive two minutes of tetrominoes. Left/right move, down soft
  drop, up hard drop, A/B rotate.
- **BREAKOUT** (`carts/breakout`): clear 108 bricks with 3 balls. Some bricks drop a laser (A).
- **JUMPER** (`carts/jumper`): reach the flag within 300 s. A jump (hold for height), stomp enemies;
  ?-blocks drop a fire flower: HP 2 and fireballs on B.
- **KNIGHTFALL** (`carts/knightfall`): survive three waves of skeletons. A sword, B dodge, R bomb.
- **RALLY** (`carts/rally`): win a 3-lap race against three rival cars. A gas, B brake, left/right
  steer, L nitro, R oil (3 of each).
- **STARFIGHTER** (`carts/starfighter`): survive 90 s of diving formations and mines. A fire,
  B homing missile.

Every cart has a title screen (A or Start to play) and a result screen, then exits (or, in a
run, hands back to the run).

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
