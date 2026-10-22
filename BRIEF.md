# MOTHER console — build brief for an LLM

Oct 22, 2026 · @Transhuman Michał Marcinkowski

You are building MOTHER: a fantasy console (virtual hardware, runtime, standard library and tools) and six small classic games that prove it works. Follow this brief exactly. Where it is silent, pick the simplest option and record it in `DECISIONS.md`.

## 1. The brief

**Goal.** A native fantasy console that runs small, deterministic, sandboxed Luau games ("carts") on fixed virtual hardware, plus six classic carts built on it. The carts are the proof: if all six are fun, deterministic and built from one shared library, the console is done.

**Second proof.** Carts share one simulation standard (units, factions, damage), so a game object from one cart can run inside another unchanged. A debug harness must demonstrate this (section 9). This is a console feature, not a game feature.

**In scope:** host runtime, Luau sandbox, cart API, audio synth and tracker, MOTHERLIB (core services + archetypes), asset text formats and converters, headless runner, trace record/replay, the six carts, the import harness.

**Out of scope:** any meta-game, menus beyond a cart picker, 3D rasterizer (reserve the API names only), networking, editors (assets are text or converted from PNG), console platform SDKs.

### Working rules

1. Build milestone by milestone (section 10). Don't start a milestone until the previous one's acceptance tests pass headless.
2. Use the exact numbers in this brief. Don't add features, options or abstractions it doesn't ask for.
3. When something is underspecified, choose the simplest option, implement it, and add one line to `DECISIONS.md`: date, decision, reason.
4. Every module gets tests. Tests run headless in CI with no window and no audio device.
5. Keep files under \~400 lines. One responsibility per file.
6. Determinism is non-negotiable. Anything that can make two runs differ is a bug (section 4).
7. Carts are data plus Luau. Never put game-specific code in the host.

## 2. Stack and repo layout

- **Host:** C++20, CMake, one binary `mother`. Dependencies, vendored: SDL3 and Luau. Nothing else; write PNG decoding with a single-header library such as stb\_image, only in the converter tool.
- **Platforms:** Windows, macOS, Linux. Headless mode on all three.
- **MOTHERLIB:** written in Luau, shipped with the runtime, loaded read-only into every cart VM.

```
mother/
  host/            C++: window, framebuffer, palette, input, audio, VM, loop
    gfx/  audio/  input/  vm/  runner/
  lib/             MOTHERLIB (Luau)
    core/          world, collision, factions, damage, status, spawn, objective
    archetypes/    one file per archetype
  carts/
    blockfall/  breakout/  jumper/  knightfall/  rally/  starfighter/
  tools/           png2spr, trace tools, matrix runner
  tests/           host unit tests, determinism tests, cart smoke tests
  DECISIONS.md
```

**Commands:**

```
mother run carts/knightfall            # windowed
mother run carts/knightfall --headless --ticks 18000 --input fuzz:42
mother record carts/rally out.trace
mother replay carts/rally out.trace --hash   # prints per-tick state hash
mother import carts/knightfall breakout:ball # import harness, section 9
mother matrix                                # all imports x all carts, section 9
```

## 3. Hardware spec

| Subsystem | Spec |
| --- | --- |
| Framebuffer | 320×180, 8-bit indexed. Presented as RGBA through the palette, integer-scaled to the largest fit, letterboxed in black |
| Palette | 64 fixed colours; use AAP-64 as a placeholder until the final palette is supplied. Index 0 = transparent in sprites and maps. **Index 63 = Mother Gold**, reserved: carts may use it only for the player avatar; the host uses it for import outlines |
| Palette ramps | Up to 8 per cart, each an ordered list of indices; used for fog, fades, dithered lighting |
| Tiles | 8×8 px. Tilemap: 2 layers per cart, up to 256×256 tiles each. Tile flags: 8 bits per tile index |
| Sprites | 4 sheets of 256×256 px (1,024 8×8 tiles each). Sheets 0–2 for the cart; **sheet 3 reserved for imported objects** |
| Sprite tiers | S 8×8, M 16×16, L 32×32. Colliders are multiples of 2 px, up to 32×32 |
| Font | Built-in 4×6 px font, ASCII 32–126 |
| Audio | 48 kHz stereo out. 8 voices. **Voices 7–8 reserved** for imports and the host |
| Waveforms | square (duty 12.5/25/50 %), saw, triangle, noise, plus 32 user wavetables of 32 samples |
| Voice | per-voice ADSR, volume, pan, pitch slide, vibrato |
| Global FX | delay, low-pass filter, bitcrush |
| Music | Tracker: 64 patterns × 64 rows × 6 channels; song = list of up to 64 pattern indices; tempo in ticks per row. Readable and writable at runtime |
| Input | Virtual pad: stick (analog, plus 8-way), A, B, X, Y, L, R, Start. Pointer optional. **X and Y are reserved** for the import system; carts use A, B, L, R |
| Keyboard map | arrows/WASD = stick, Z = A, X = B, C = X, V = Y, Q = L, E = R, Enter = Start |
| Tick | Fixed 60 Hz simulation; render decoupled; no frame-time dependence anywhere in cart code |
| Code | Luau, ≤64 KB total source per cart |
| CPU budget | Luau interrupt callback; a cart exceeding 8 ms per tick on reference hardware logs a budget error |
| Memory | 16 MB Luau heap per cart |
| 3D | Out of scope. Reserve the API names `tri`, `mesh`, `cam3d` as stubs that raise "not implemented" |

## 4. Runtime

### Main loop (one tick)

1. Poll input into the virtual pad state (or read it from a trace).
2. `cart.update()`
3. `world:step()`: MOTHERLIB moves bodies, resolves collision, applies damage and statuses, runs spawners, checks the objective.
4. `cart.draw()`; the cart calls `world:draw()` where it wants entities layered.
5. Host draws Mother Gold outlines around imported entities.
6. Present the framebuffer (skipped headless). Audio is mixed on its own thread from a command queue written during the tick.

If rendering falls behind, run up to 4 simulation ticks per frame, then drop time. Simulation never runs at a variable step.

### Sandbox

- One Luau VM per cart. Remove `io`, `os`, `debug`, `require` of arbitrary paths, `loadstring`, `math.random`, `math.sin` and other libm functions.
- Cart modules are loaded with `import("name")` from the cart folder only.
- MOTHERLIB tables are frozen (read-only).

### Determinism

- RNG: xoshiro128\*\*, seeded by the host per run. Carts use only `rnd`, `rndi`.
- Math: `+ − × ÷` and `sqrt` on doubles are IEEE-exact. All transcendental functions (`m.sin`, `m.cos`, `m.atan2`, `m.exp`) are host-implemented with a fixed algorithm and strict floating-point compile flags.
- No iteration over hash tables in simulation logic. MOTHERLIB uses arrays with stable ordering; entity ids are monotonic integers.
- GC timing must not affect simulation: no weak tables or finalizers in simulation code.
- **State hash:** after every tick, hash the world state (entity ids, positions, velocities, HP, statuses, tilemap edits, RNG state) plus a cart-provided `cart.hash()` value. Two runs with the same seed and input trace must produce identical hash sequences on all three platforms.

### Cart format

A cart is a folder (packed later as a zip with the `.mcart` extension):

```
carts/knightfall/
  cart.toml        manifest
  main.luau        entry: defines cart.init, cart.update, cart.draw, cart.hash
  *.luau           modules
  sprites.spr      sheets 0-2
  map.map          tilemap layers + flags
  audio.trk        instruments, sfx, patterns, songs
```

### Asset text formats

- **`.spr`:** one pixel per character. The 64 palette indices map to the base64 alphabet (`A`=0 … `/`=63). Each sheet is 256 lines of 256 characters, preceded by `sheet N`.
- **`.map`:** per layer, a header `layer N W H`, then H lines of W tile indices as 4-digit decimal (0–3071, sheets 0–2) separated by spaces. Then `flags`, 3,072 lines of 8-bit flags as two hex digits.
- **`.trk`:** plain-text tracker: `instrument`, `sfx`, `pattern` and `song` blocks. One row per line: `C-4 01 40 ...` (note, instrument, volume, effect). Define the exact grammar in `DECISIONS.md` and keep it hand-writable.
- **Converter:** `tools/png2spr` quantises a PNG to the palette (nearest colour in OKLab) and writes `.spr`.

All formats are designed so an LLM can write and edit them directly.

## 5. Cart API reference

Pico-8-style globals. Coordinates are framebuffer pixels after `camera()`. Colours are palette indices 0–63.

```lua
-- lifecycle (defined by the cart)
cart.init()            -- once, after the world is created from cart.toml
cart.update()          -- every tick, before world:step()
cart.draw()            -- every rendered frame
cart.hash() -> int     -- cart-only state for the determinism hash

-- graphics
cls(c)  pset(x,y,c)  pget(x,y)  line(x0,y0,x1,y1,c)
rect(x,y,w,h,c)  rectfill(x,y,w,h,c)  circ(x,y,r,c)  circfill(x,y,r,c)
spr(n, x, y, [w=1], [h=1], [flip_x], [flip_y])  -- n = 8x8 tile index; w,h in tiles
sspr(sx, sy, sw, sh, dx, dy, [dw], [dh])
map(cx, cy, sx, sy, cw, ch, [layer=0])
print(str, x, y, c)
pal(src, dst)  palt(c, transparent)  ramp(n) -> {indices}
camera(x, y)  clip(x, y, w, h)

-- tilemap
mget(tx, ty, [layer])  mset(tx, ty, tile, [layer])  fget(tile, [bit])  fset(tile, bit, on)

-- input
btn(b) btnp(b)          -- b: "left","right","up","down","a","b","l","r","start"
stick() -> x, y         -- -1..1 analog
pointer() -> x, y, down -- nil if no pointer

-- audio
sfx(n, [voice])  music(song, [fade_ticks])  stop([voice])
beat() -> bar, beat, row
song_get(pattern, row, ch) -> note, inst, vol, fx
song_set(pattern, row, ch, note, inst, vol, fx)

-- math and time
rnd(x) -> float in [0,x)   rndi(a, b) -> int in [a,b]
m.sin(a) m.cos(a) m.atan2(y,x) m.sqrt(x) m.floor(x) m.clamp(x,a,b) m.lerp(a,b,t)
tick() -> int             -- ticks since cart start

-- host
mother.finish("win" | "lose")
mother.log(...)           -- headless and debug output only
world                     -- the MOTHERLIB world, section 6
```

The cart never reads `btn("x")` or `btn("y")`; those are reserved for the import system (section 6).

## 6. Simulation standard and the MOTHERLIB world

Every cart's game objects live in one MOTHERLIB world with shared units and rules. This is what lets any archetype run in any cart. Carts may keep extra private state, but anything that moves, collides or takes damage must be a world entity.

### Units

| Quantity | Standard |
| --- | --- |
| Speed tiers (px/tick) | crawl 0.5 · walk 1 · run 2 · fast 3 · projectile 4 (hard max) |
| HP and damage tiers | 1 · 3 · 10 · 30 |
| Gravity | 0.25 px/tick², terminal velocity 4 |
| Grid-step fall | pieces fall one tile per N ticks (N set by the piece) |
| Collision | swept AABB against tiles and entities; no tunnelling at 4 px/tick |

### Cart manifest (`cart.toml`)

```toml
[cart]
id = "knightfall"
title = "KNIGHTFALL"
family = "action"            # action | platform | driving | shooter | puzzle

[env]
gravity = "none"             # none | down
surface = "ground"           # none | ground
scroll  = "none"             # none | follow | auto
avatar  = "8dir"             # none | up | lr | 8dir | heading
spawn   = "world"            # none | world | camera
deploy  = true
bounds  = { left="solid", right="solid", top="solid", bottom="solid" }   # solid | kill | despawn

[objective]
type = "kill_all"            # kill_all | escape | survive | race | collect

[avatar]
archetype = "knight"
damage = "hp"                 # hp | stun
hp = 30

[[spawn_points]]
x = 8
y = 88

[[exports]]
id = "sword"
archetype = "sword"
params = { tier = 3, arc = 90, reach = 12 }
```

### Rules

- **Collision layers:** ground bodies, air bodies, objects, projectiles. Solid tiles block everything except air bodies. All entities interact with each other by contact regardless of layer. Surface effects (oil) apply to ground bodies only.
- **Factions:** `player`, `host`, `wild`. Every AI targets the nearest entity of another faction, including native enemies in their home cart. Contact damage applies only between different factions.
- **Ownership:** each object has an owner faction and a `hurts_owner` flag. A reflector transfers ownership of what it reflects.
- **Damage response:** each body is `hp` (dies at 0) or `stun` (stunned for 60 ticks per hit, never dies).
- **Statuses:** `stun` (no actions), `slip` (no steering, keeps velocity), `knockback` (impulse). Every archetype honours all three.
- **Damageable tiles:** tile flag bit 0 = solid, bit 1 = damageable (HP 1), bit 2 = indestructible-critical (never damaged or overwritten by anything).
- **Bounds:** per edge `solid`, `kill` (entity dies) or `despawn` (removed silently). Scrolling carts also despawn entities more than one screen behind the camera.
- **Objectives:** `kill_all` counts host-faction entities only. `race` counts host racers only. Objective entities can't be destroyed or blocked by non-native entities.
- **Imported entities:** faction `wild` for creatures and hazards, `player` for equipment and deployables. Imported solids decay after 1,200 ticks. Caps: 3 imported exports, 8 imported instances.
- **Import input:** `X` uses the equipped import, `Y` deploys the deployable import. Handled by MOTHERLIB, never by the cart.

### World API

```lua
world:spawn(archetype, params, x, y, faction) -> id
world:remove(id)
world:get(id) -> entity            -- read-only view: x, y, vx, vy, hp, faction, status
world:query(x, y, w, h, [filter]) -> {ids}
world:nearest(x, y, filter) -> id
world:damage(id, tier, source)
world:status(id, kind, ticks)
world:on(event, fn)                -- "death", "hit", "spawn", "objective"
world:avatar() -> id
world:camera() -> x, y
world:draw()
```

## 7. Archetypes to build

An archetype is a MOTHERLIB module: a parameterised entity with its own behaviour, written only against the world API and the simulation standard. Carts compose archetypes; they don't reimplement them.

**Every archetype must:**

- declare its category (`body`, `creature`, `object`, `equipment`), layer, size tier and default params;
- declare its requirements on each `[env]` axis, and behave correctly on every value it accepts (for example, the Ball under `gravity = "down"` ignores gravity because it is kinematic; that must be explicit in code);
- target "nearest other faction", never "the player";
- honour stun, slip and knockback;
- carry its own sprite and sfx references, so it renders and sounds right inside any cart;
- pass the axis tests in section 9.

| Archetype | Category | Used by | Behaviour | Requires |
| --- | --- | --- | --- | --- |
| knight | body | KNIGHTFALL | 8-dir walk, A = sword swing, B = dodge (12-tick invulnerability, 30-tick cooldown) | avatar 8dir |
| runner | body | JUMPER | run, jump (variable height), stomp damage tier 1 from above, head-bump breaks damageable tiles | gravity down, avatar lr |
| paddle\_avatar | body | BREAKOUT | 1-axis movement along the bottom, 32×8 collider, reflects objects | avatar up |
| car | body | RALLY (player and rivals) | accel, brake, steer, momentum, grip; offroad tiles halve max speed; stun = spin-out | gravity none, surface ground |
| ship | body | STARFIGHTER | 8-dir within the screen, A = 2 shots on screen max | avatar up |
| piece\_controller | body | BLOCKFALL | spawns tetrominoes, rotate, move, soft/hard drop, lock delay 30 ticks | gravity down, avatar none |
| skeleton | creature | KNIGHTFALL | chase nearest other faction, swing tier 3; HP 3 | surface ground, spawn |
| walker | creature | JUMPER | walk one axis, reverse at solids; contact tier 1; HP 1 | surface ground, spawn |
| turtle | creature | JUMPER | as walker; when stomped becomes a shell object | gravity down, surface ground, spawn |
| diver | creature | STARFIGHTER | air; swoop from spawn, dive at nearest other faction, exit and re-enter; HP 1; fires 1 bullet per dive | spawn |
| rival\_ai | creature (drives a car) | RALLY | follow waypoint racing line; swerve to ram other-faction within 4 tiles; ram-only if no line exists | surface ground, gravity none, spawn |
| brick | creature (static) | BREAKOUT | static body, HP 1, may drop a pickup on death | — |
| mine | creature (hazard) | STARFIGHTER | static in world space; detonates on any contact, tier 10, radius 1 tile | spawn |
| ball | object | BREAKOUT | kinematic, speed fast; reflects off solids and player-faction bodies; tier 1 on contact; re-serve 180 ticks after loss | bounds |
| shell | object | JUMPER | slides along facing at fast, reverses at solids, falls under gravity; tier 3 to all factions after 30 ticks | — |
| bomb | object | KNIGHTFALL | 120-tick fuse, radius 2 tiles, tier 10 to all, breaks damageable tiles | — |
| oil | object | RALLY | ground bodies crossing it slip for 60 ticks; 3 alive max | surface ground |
| falling\_piece | object | BLOCKFALL | grid-step fall, lands as solid damageable tiles that decay | gravity down |
| sword | equipment | KNIGHTFALL | 90° arc along facing, reach 12 px, tier 3 | avatar |
| paddle | equipment | BREAKOUT | 24 px reflector on the avatar's facing; reflects objects and projectiles, transfers ownership | avatar |
| laser | equipment | BREAKOUT | 2 shots along facing, tier 1, 30-tick cooldown | avatar |
| fire\_flower | equipment | JUMPER | fireball tier 3, max 2; bounces along floors under gravity, ricochets 3× without | avatar |
| nitro | equipment | RALLY | speed ×2 for 60 ticks, 300-tick cooldown | avatar |
| homing | equipment | STARFIGHTER | missile seeks nearest damageable other faction, tier 3, 60-tick cooldown | avatar |

Category requirements apply on top of the table: deployable objects (shell, bomb, oil) also require `deploy = true`; equipment requires an avatar; auto-objects (ball, falling\_piece) need neither.

Native pickups (Laser from bricks, Fire Flower from blocks, Nitro and Oil charges in RALLY, Homing in STARFIGHTER) are bound by the cart to A, B, L or R. The same archetypes are bound to X/Y only by the import system.

## 8. Proof-of-concept carts

Six classics, each 2–5 minutes, each a complete game with a win and a loss. Together they exercise every environment axis value.

| Cart | Family | Gravity | Surface | Scroll | Avatar | Objective |
| --- | --- | --- | --- | --- | --- | --- |
| BLOCKFALL | puzzle | down | none | none | none | survive 120 s |
| BREAKOUT | action | none | none | none | up | kill\_all |
| JUMPER | platform | down | ground | follow | lr | escape |
| KNIGHTFALL | action | none | ground | none | 8dir | kill\_all |
| RALLY | driving | none | ground | follow | heading | race 3 laps |
| STARFIGHTER | shooter | none | none | auto | up | survive 90 s |

### BLOCKFALL (Tetris)

- Well 10×20 cells (8 px) centred; next-piece preview to the right.
- 7 tetrominoes, 7-bag randomiser from `rndi`. Rotation with a 1-cell wall kick.
- Fall 1 cell per 48 ticks, −4 ticks every 10 lines, minimum 8. Lock delay 30 ticks.
- Controls: left/right move, down soft drop, up hard drop, A rotate clockwise, B rotate anticlockwise.
- Settled cells are damageable tiles (HP 1). A removed cell leaves a hole; only full lines collapse.
- Lose: a new piece can't spawn. Win: survive 7,200 ticks.

### BREAKOUT

- 6 rows × 18 bricks of 16×8 px; paddle at the bottom; 3 lives.
- Ball at speed `fast`; angle off the paddle depends on hit position.
- 10% of bricks drop a Laser pickup (15 s, fires on A).
- Lose: 3 balls lost through the bottom (`kill` edge). Win: all bricks destroyed.

### JUMPER (Mario)

- One level, 200 × 22 tiles, follow camera on x. Pits, bricks, ?-blocks, a flag at the end.
- Avatar HP 1; a Fire Flower from a ?-block gives HP 2 and fireballs on B.
- 12 walkers, 6 turtles. Bricks are damageable tiles; floors are critical tiles.
- Lose: death or 300 s timer. Win: touch the flag.

### KNIGHTFALL (Zelda/Gauntlet)

- One 40 × 22-tile arena with walls and pillars; 4 spawn points at the edges.
- 3 waves of skeletons: 4, 6, 8. Knight HP 30.
- A sword, B dodge, R drops a bomb (3 per wave).
- Lose: HP 0. Win: all 3 waves cleared.

### RALLY (Micro Machines, combat)

- One looping track, about 120 × 80 tiles, follow camera; offroad tiles; walls.
- Player and 3 rivals use the `car` archetype; rivals use `rival_ai`.
- A accelerate, B brake, L nitro (3 charges), R drop oil (3 charges).
- Lose: a rival finishes 3 laps first. Win: you finish first.

### STARFIGHTER (Galaga)

- Vertical auto-scroll at 0.5 px/tick; ship HP 3.
- Divers arrive in formations of 4–8 from the top; mines drift in with the scroll.
- A fire, B homing missile (60-tick cooldown).
- Lose: HP 0. Win: survive 5,400 ticks.

**Every cart must:** define its manifest fully; use only archetypes for entities; use only its own art (`.spr`), map and audio; have a title screen and a result screen; and run 10 minutes headless under fuzz input without errors.

## 9. Tooling and tests

### Headless runner

- Runs any cart with no window or audio device: `--ticks N`, `--seed S`, `--input fuzz:S | trace:file`.
- Prints the result (`win`, `lose`, `timeout`), tick count, budget warnings and the final state hash.
- **Fuzz input:** random but plausible pad input (held directions, button bursts), seeded.

### Traces

- `record` writes seed + per-tick pad state. `replay` must reproduce the exact per-tick hash sequence.
- CI replays one stored trace per cart on Windows, macOS and Linux and compares hash sequences across platforms.

### Import harness

Proves the simulation standard: any export can run inside any compatible cart, unchanged.

1. `mother import <cart> <source>:<export>` loads the host cart, reads the export's archetype and params from the source cart's manifest, and checks its requirements against the host's `[env]`.
2. If incompatible, it prints which axis failed (e.g. `needs surface=ground, host has none`) and exits.
3. If compatible, it loads the export's sprites into sheet 3 and its sfx into voices 7–8, spawns or equips it per its category (creatures at host spawn points, deployables on `Y`, equipment on `X`, auto-objects at start), and runs the cart.
4. Imported entities get the Mother Gold outline.

### Matrix test

`mother matrix` runs every export of every cart against every other cart:

- computes compatible / incompatible from the manifests and writes the 16 × 6 table to `matrix.md`;
- for each compatible cell, runs 3 seeds × 10 minutes headless under fuzz input;
- fails on any Luau error, budget overrun, NaN position, entity outside all bounds for more than 60 ticks, determinism mismatch on replay, or an objective made impossible (no progress for 1,800 ticks while an imported solid blocks the path).

Expected compatibility for the six carts (✓ compatible, ✗ incompatible, H home):

| Export | BLOCKFALL | BREAKOUT | JUMPER | KNIGHTFALL | RALLY | STARFIGHTER |
| --- | --- | --- | --- | --- | --- | --- |
| falling\_piece | H | ✗ | ✓ | ✗ | ✗ | ✗ |
| ball | ✓ | H | ✓ | ✓ | ✓ | ✓ |
| paddle | ✗ | H | ✓ | ✓ | ✓ | ✓ |
| laser | ✗ | H | ✓ | ✓ | ✓ | ✓ |
| walker | ✗ | ✗ | H | ✓ | ✓ | ✗ |
| shell | ✗ | ✓ | H | ✓ | ✓ | ✓ |
| fire\_flower | ✗ | ✓ | H | ✓ | ✓ | ✓ |
| skeleton | ✗ | ✗ | ✓ | H | ✓ | ✗ |
| bomb | ✗ | ✓ | ✓ | H | ✓ | ✓ |
| sword | ✗ | ✓ | ✓ | H | ✓ | ✓ |
| rival car | ✗ | ✗ | ✗ | ✓ | H | ✗ |
| oil | ✗ | ✗ | ✓ | ✓ | H | ✗ |
| nitro | ✗ | ✓ | ✓ | ✓ | H | ✓ |
| diver | ✗ | ✓ | ✓ | ✓ | ✓ | H |
| mine | ✗ | ✓ | ✓ | ✓ | ✓ | H |
| homing | ✗ | ✓ | ✓ | ✓ | ✓ | H |

If the computed table differs from this one, either a manifest or this table is wrong. Stop and record which in `DECISIONS.md`.

### Archetype axis tests

For every archetype, a generated test world for each `[env]` value it accepts (gravity on/off, surface on/off, each scroll mode, each bounds type). Spawn it with a few other-faction dummies and run 600 ticks: no errors, stays in bounds or is correctly removed, damages and takes damage per the standard.

## 10. Milestones and acceptance criteria

Build in this order. Each milestone is done only when its acceptance tests pass headless in CI.

| # | Milestone | Acceptance |
| --- | --- | --- |
| M0 | Host skeleton: SDL3 window, 320×180 indexed framebuffer, palette, integer scaling, fixed 60 Hz tick, virtual pad from keyboard and gamepad | Palette test pattern renders correctly; 3,600 ticks headless take the same tick count on every run |
| M1 | Luau sandbox, cart loader, graphics and input API, `import()` | A hello cart draws, reads input, and cannot access `io`, `os` or `math.random` |
| M2 | Sprites, tilemap, font, `.spr` and `.map` formats, `png2spr` | A test cart round-trips a PNG through `png2spr` and draws it pixel-exact |
| M3 | Audio synth, `.trk` format, tracker playback, sfx, `beat()`, `song_get`/`song_set` | A test song plays; headless audio render produces identical samples on every run |
| M4 | Headless runner, fuzz input, traces, per-tick state hash | Replay of a recorded trace matches its hash sequence on Windows, macOS and Linux |
| M5 | MOTHERLIB core: world, swept collision, layers, factions, damage, statuses, spawners, bounds, objectives, manifest loading | Core unit tests; no tunnelling at 4 px/tick; `kill_all` counts host faction only |
| M6 | Archetypes for KNIGHTFALL and BREAKOUT; both carts | Both carts are winnable and losable, run 10 minutes under fuzz, replay deterministically |
| M7 | Remaining archetypes; JUMPER, RALLY, STARFIGHTER, BLOCKFALL | Same acceptance for all six carts |
| M8 | Archetype axis tests, import harness, matrix test | Computed matrix equals the expected table; every compatible cell passes 3 seeds × 10 minutes |

**The console is done when** all six carts pass M7 and the matrix passes M8. Then the archetype library, manifest format and API are frozen as version 1.

**Report back after each milestone:** what was built, test results, every new line in `DECISIONS.md`, and anything in this brief that turned out to be wrong.
