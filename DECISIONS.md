# Decisions

One line per decision: date, decision, reason.

- 2026-10-22 · The repo root is the brief's `mother/` folder; vendored code lives in `vendor/` (SDL3 3.4.18, Luau 0.743, stb @ a7d4c217) with upstream tests, examples, bench and fuzz removed · "vendored" means plain in-repo copies that build offline; the removed parts are never built.
- 2026-10-22 · SDL3 is linked statically · gives the single self-contained `mother` binary from section 2.
- 2026-10-22 · Strict FP for Luau and all MOTHER code: MSVC `/fp:strict`, GCC/Clang `-ffp-contract=off -fno-fast-math`; SDL keeps its own flags · fused multiply-add on ARM64 (macOS CI) would otherwise change results and break cross-platform hashes; SDL never touches simulation state, and `/fp:strict` fails to compile SDL's YUV code.
- 2026-10-22 · Luau is added to the build but not linked until M1 · it is vendored now and has no use in M0.
- 2026-10-22 · Patched Luau's `luaconf.h` so its MSVC-only fast-math regions are empty (noted in `vendor/README.md`) · they cover `%`, `//`, `floor`, `sqrt` and others; fast math there could make Windows results differ, and `/fp:strict` refuses the pragma.
- 2026-10-22 · The palette is AAP-64 verbatim in its published order, so index 63 (#423934) stands in for Mother Gold · the brief says to use AAP-64 until the final palette arrives; swapping one table entry later is trivial.
- 2026-10-22 · Colour arguments are masked to 0–63 (`c & 63`) · simplest total behaviour; no error path in drawing code.
- 2026-10-22 · Stick axes are quantised to int8 −127..127 with y positive = down · integer pad state is exact to hash and record in traces; y down matches framebuffer coordinates.
- 2026-10-22 · The 8-way direction (and `btn("left")` etc.) is set when an axis reaches ±64 (about 0.5); a digital direction (key or d-pad) overrides its axis when exactly one of the pair is held · one rule for keyboard, d-pad and analog stick.
- 2026-10-22 · Gamepad: deadzone 8000/32767; south/east/west/north = A/B/X/Y, shoulders = L/R, Start = Start; the first connected pad is used · SDL's positional button names match the keyboard map's layout.
- 2026-10-22 · The keyboard is read by scancode (physical key position) · the brief's map (WASD, ZXCV, Q/E) stays in place on non-QWERTY layouts.
- 2026-10-22 · The tick clock counts ns × 60 so one tick is exactly 1e9 units; when more than 4 ticks are owed it runs 4, keeps the sub-tick remainder and drops the rest · no rounding drift, and the brief's "up to 4 ticks, then drop time" rule.
- 2026-10-22 · Integer scaling never goes below 1×; a window smaller than 320×180 shows the centre crop · the brief only asks for the largest integer fit.
- 2026-10-22 · The window opens at 3× (960×540), resizable, with vsync · a reasonable default; the size is not part of the hardware spec.
- 2026-10-22 · `mother run` with no cart shows a built-in palette test pattern (swatches, pad state, tick marker) · M0 has no cart loader; the pattern is a host diagnostic, not game code.
- 2026-10-22 · `--ticks N` also works windowed (exits after N ticks); headless requires `--ticks` until carts can finish themselves · lets the window be run in automated checks; a headless run with no end would never return.
- 2026-10-22 · Headless runs call draw once per tick into a framebuffer that is never presented · draw-time errors then show up in headless and fuzz runs.
- 2026-10-22 · The headless runner prints one line, `result=<win|lose|timeout> ticks=<n>` (final hash added in M4) · trivial to parse in tests and scripts.
- 2026-10-22 · Tests use a small in-repo harness (`tests/test.h`) run through CTest, one CTest entry per module · the brief allows no dependencies beyond SDL3 and Luau.
- 2026-10-22 · Text files are stored with LF line endings on every platform (`.gitattributes`) · asset text and traces must be byte-identical across Windows, macOS and Linux.

## M1

- 2026-10-22 · M1's cart API: the lifecycle, `cls pset pget line rect rectfill circ circfill pal camera clip`, `btn btnp stick pointer`, `tick`, `import`, `mother.log`/`mother.finish` and the `tri mesh cam3d` stubs. `spr sspr map print palt ramp` come in M2, audio in M3, and `rnd rndi m.*` in M4 · each part arrives with the milestone that builds what it needs (sprites, font, synth); the RNG and fixed math belong with the determinism hashes that prove them.
- 2026-10-22 · `cart.toml` must exist but isn't read until M5 · manifest loading is an M5 item.
- 2026-10-22 · A cart's Luau is every `*.luau` file in the cart folder itself (not subfolders), read into memory at load; together they must be ≤ 65,536 bytes; module names are letters, digits and `_` · nothing touches the disk after load, and `import()` can't leave the folder on any OS.
- 2026-10-22 · Each module (main included) runs in its own global table that reads through to the frozen API; modules share values through `import()` return values and the `cart` table · a fresh table at load time stops Luau's load-time global caching from going stale, and keeps modules independent.
- 2026-10-22 · `import()` runs a module once and caches its first return value; cycles and `import("main")` are errors · `require`-like semantics, the simplest that works.
- 2026-10-22 · `main.luau` must define all four of `cart.init`, `cart.update`, `cart.draw` and `cart.hash` · the brief says main defines them; a typo fails at load instead of silently doing nothing.
- 2026-10-22 · Opened libraries: base, coroutine, table, string, math, utf8, bit32, buffer. Removed: `print`, `gcinfo`, and from math `acos asin atan atan2 cos cosh exp log log10 pow random randomseed sin sinh tan tanh`. Never opened: `os`, `debug`, `vector`, `integer` (Luau 0.743 has no `io`, `require` or `loadstring` in the VM) · what remains in math is exact IEEE arithmetic.
- 2026-10-22 · The removed math functions and all `vector.*` builtins are also disabled at compile time (`disabledBuiltins`) · otherwise Luau's fast paths still run `math.sin(x)` after the member is removed (checked by temporarily turning the option off: the sandbox cart then fails).
- 2026-10-22 · The `^` operator uses a patched deterministic pow (`vendor/README.md`): integer exponents are exact repeated multiplies, `x ^ 0.5` is `sqrt`, any other exponent gives NaN · libm `pow` differs across platforms, and MOTHER has no fixed exp/log to build general powers from.
- 2026-10-22 · Every graphics function, including `camera`, `clip` and `pal`, raises an error outside `cart.draw()` · `pget` during update would let frame timing (windowed vs headless) leak into the simulation.
- 2026-10-22 · Drawing coordinates are floored and clamped to −32768..32767, with NaN as 0 · keeps every primitive's loop bounded.
- 2026-10-22 · `camera` also offsets `pget`; `clip` is in screen pixels; `cls` fills the whole screen ignoring camera, clip and palette; draw state persists across frames; `rect`/`rectfill` take width and height and draw nothing when either is ≤ 0 · Pico-8-like, matching the brief's signatures.
- 2026-10-22 · `tick()` counts ticks started: 0 during `cart.init`, 1 during the first `cart.update` · "ticks since cart start", counting the current one.
- 2026-10-22 · `pointer()` is the mouse while it is over the screen image, in screen pixels (not camera-adjusted), `down` = left button; nil otherwise and always nil headless · the brief marks the pointer optional.
- 2026-10-22 · `mother.finish` ends the run after the current tick; a Luau error ends it with `result=error`, the message and traceback on stderr, exit code 1 · same behaviour headless and windowed.
- 2026-10-22 · `mother.log` writes `[cart] ` plus its tab-separated arguments to stdout · "headless and debug output only".
- 2026-10-22 · The 8 ms CPU budget is wall time in `cart.update` (`world:step()` joins in M5), checked every 256 interrupts and after the call, and logged once per tick on stderr; runaway loops are logged, not killed · the brief says over-budget ticks log an error.
- 2026-10-22 · The 16 MB heap is 16,777,216 bytes counted by the Luau allocator; going over fails with "not enough memory" · the limit from section 3.
- 2026-10-22 · Test carts live in `tests/carts/` (`hello`, `sandbox`); `carts/` stays for the six game carts · the brief's layout lists only the six there.

## M2

- 2026-10-22 · Sprite memory is one 256×1024 atlas, the four sheets stacked (sheet s from y = 256·s); sprite n's top-left is ((n % 32)·8, (n / 32)·8), so n = 0–4095 and n / 1024 is its sheet · one numbering for `spr`, `sspr` and the map, and sheet 3 needs no special case.
- 2026-10-22 · `spr` accepts 0–4095, so sheet 3 can be drawn (MOTHERLIB draws imported objects from it); a cart's `sprites.spr` defines sheets 0–2 only, and map cells, `mset`, `fget` and `fset` take tiles 0–3071 · matches the brief's tile range, and imports still render.
- 2026-10-22 · `spr(n, x, y, w, h)` draws the w×h-tile rectangle of the atlas starting at sprite n; atlas pixels beyond the atlas edge are skipped; flips mirror the whole block · Pico-8 behaviour.
- 2026-10-22 · `sspr` takes atlas pixel coordinates, samples nearest-neighbour with integer maths, and draws nothing when any size is ≤ 0; it has no flip arguments · the brief's signature, deterministic.
- 2026-10-22 · Transparency (`palt`) is checked on the source colour before the draw palette (`pal`) maps it; by default only colour 0 is transparent; `palt()` with no arguments resets · Pico-8 behaviour, and index 0 = transparent from section 3.
- 2026-10-22 · `map()` never draws tile 0 (an empty cell); cells outside the layer are empty; only cells that can touch the clip rectangle are visited · Pico-8 convention; keeps huge `cw`/`ch` cheap.
- 2026-10-22 · `mget`/`mset`/`fget`/`fset` work outside `cart.draw()`; `mset` outside the layer does nothing; out-of-range tiles, layers or bits raise errors · the tilemap is simulation state; bad indices are bugs.
- 2026-10-22 · Palette ramps live in `sprites.spr` as `ramp N i i …` lines (N 0–7, 1–64 indices); `ramp(n)` returns a new table, empty when undefined · the brief gives ramps no home; they're art, and `.spr` is the cart's art file.
- 2026-10-22 · `sprites.spr` and `map.map` are optional (missing = empty sheets / no layers and zero flags); when present they parse strictly (a sheet is exactly 256 rows of exactly 256 characters, tiles exactly 4 digits, the flags section is required with exactly 3072 lines), with blank lines allowed between blocks and CRLF read as LF; errors name file and line · carts like BLOCKFALL need no art file; strictness catches LLM slips early.
- 2026-10-22 · The font is MOTHER's own 3×5 glyphs in 4×6 cells; lowercase letters draw as uppercase; characters outside 32–126 draw nothing but still take a cell; `\n` starts a new line; `print` goes through camera, clip and palette · a 3×5 cell can't fit lowercase descenders; own glyphs avoid licence questions.
- 2026-10-22 · `png2spr <in.png> <out.spr>`: up to 256×768 px, rows 0–255 to sheet 0 and so on, writing every sheet the image touches; alpha < 128 becomes index 0; other pixels go to the nearest of indices 1–63 in OKLab (the lower index wins a tie) · index 0 is transparent, so an opaque pixel must never land on it.
- 2026-10-22 · The palette and both format parsers are a library (`mother_assets`) with no SDL or Luau, shared by the host and `png2spr`; `stb_image` is compiled only into `png2spr_lib` (the unit tests link it to check the converter) · keeps PNG decoding out of the runtime.
- 2026-10-22 · Acceptance fixture (M2): `tests/carts/roundtrip/art.png` (128×96, every palette colour, alpha-threshold cases), made once by a throwaway script and committed with its `sprites.spr`. CI checks png2spr reproduces that file byte for byte and that the cart draws the PNG pixel-exact · a binary fixture is simpler than generating PNGs in CI.

## M3

- 2026-10-22 · Audio is split in two. The sequencer runs on the simulation thread after `cart.update`, plays songs and sfx on the 60 Hz tick, and turns them into per-tick voice commands. The synth renders each tick's commands into exactly 800 frames: on SDL's audio thread when windowed, inline when headless · `beat()` reads the sequencer, so it's deterministic; the samples depend only on the command stream, never on thread timing.
- 2026-10-22 · The synth is all integer: 32-bit phase, Q16 envelopes, samples about ±8192 a voice. The pitch table is built by repeated IEEE multiplication by 2^(1/192), with no libm · identical samples on every platform and build. The test song's audio hash is pinned in CTest (`cli.cart_song`), so Debug, Release and every CI OS must match it.
- 2026-10-22 · The `.trk` grammar lives in `host/assets/trk_format.h` (blocks) and `host/assets/trk_cell.h` (cells). Blocks are `wavetable`, `instrument`, `sfx`, `pattern`, `song`, `fx`, with `#` comment lines; cells are `NOTE INST VOL FX` with hex instrument, volume and effect values; instrument numbers are hex everywhere, all other numbers decimal; effects are `Pxx` pan, `Uxx`/`Dxx` slide, `Vxy` vibrato, `Txx` tempo · the brief's `C-4 01 40 ...` row, kept hand-writable.
- 2026-10-22 · Limits: 64 instruments, 64 sfx (1–64 rows each, with their own speed), 64 patterns, 8 songs (`loop` or `once`, 1–64 patterns), 32 wavetables of 32 signed samples · the brief fixes only patterns, rows, channels and wavetables; the rest are round numbers.
- 2026-10-22 · Global effects (delay, low-pass, bitcrush) are set once per cart in the `.trk` `fx` block; the chain is bitcrush → one-pole low-pass → delay → clamp · the brief gives them no API.
- 2026-10-22 · Music channel c always plays on voice c (1–6). An sfx takes over its voice until it ends, then the music channel speaks again at its next note. `sfx(n)` with no voice picks the lowest voice 1–6 without an sfx, or else the one whose sfx started first. Voices 7–8 raise "reserved" errors from the cart API · six channels, eight voices, two reserved, as the brief says.
- 2026-10-22 · Row semantics: a note starts from its instrument's settings, with the row's volume and effect applied on top; effects last until the voice's next note; an instrument with no note sets the channel's instrument; a volume with no note changes the playing note; `===` releases. Undefined instruments play with defaults (square50, no envelope, full volume, centre) · the simplest MOD-like model.
- 2026-10-22 · ADSR times are ticks with per-sample linear ramps; a note with sustain 0 ends after its decay. Noise is a 15-bit LFSR clocked at 32× the note frequency; vibrato is a triangle LFO · chip-style and integer-only.
- 2026-10-22 · `beat()` returns `bar, beat, row`: 4 rows a beat and 4 beats a bar, counted from the song's start across patterns and loops, plus the row in the current pattern; it returns nil when no music is playing · the brief doesn't define bars and beats.
- 2026-10-22 · `music(song, fade)` fades in over `fade` ticks; `music(-1, fade)` fades out, then stops; starting a song releases the old one's notes. `stop()` cuts voices 1–6 and ends music and every sfx; `stop(v)` cuts voice v and ends its sfx · Pico-8-like.
- 2026-10-22 · `song_get`/`song_set` values: note 0 empty, −1 note off, 1–96 for C-0..B-7; instrument −1 (none) or 0–63; volume −1 (none) or 0–64; effect as a string, `"..."` for none (nil also accepted by `song_set`); patterns and rows are 0-based as in the file, channels 1–6 like voices · numbers for arithmetic (transposing), the file's own spelling for effects.
- 2026-10-22 · `sfx`, `music`, `stop` and `song_set` raise an error inside `cart.draw()`; `beat` and `song_get` work anywhere · what plays is simulation state, like the tilemap.
- 2026-10-22 · Headless runs always render audio and print `audio=<FNV-1a 64 hex>` after the tick count; `--wav FILE` (headless only) also writes the samples as a 48 kHz 16-bit stereo WAV · the hash is the M3 acceptance check; the WAV lets a person hear a headless render.
- 2026-10-22 · Windowed audio: the simulation queues each tick's commands, and the audio callback renders queued ticks in order. When the simulation is behind, the callback outputs silence; when more than 6 ticks pile up, it renders the extra ticks and drops their samples. Commands are never dropped · keeps latency near 100 ms without changing what a tick sounds like.
- 2026-10-22 · Test cart (M3) `tests/carts/song`: its `audio.trk` (C–Am–F–G loop, a one-shot fanfare, two sfx, every wave type and effect block) was written once by a throwaway script and is committed as plain text · the file itself is the hand-editable source.

## M4

- 2026-10-22 · The per-tick state hash is FNV-1a 64 over the tick count, the RNG's four state words, the tilemap hash and `cart.hash()`'s value; the MOTHERLIB world joins it in M5. The audio sequencer is not hashed (a cart can fold `beat()` into `cart.hash()`) · the brief's list, minus the world that doesn't exist yet.
- 2026-10-22 · Tilemap edits reach the hash through a Zobrist hash: an XOR of one mixed key per cell and per flag byte, updated in O(1) by `mset`/`fset` · rehashing up to 131,072 cells every tick would cost gigabytes per 10-minute run.
- 2026-10-22 · `cart.hash()` is called every tick, after `cart.update` and the audio tick. It may return any number: the value is hashed by its IEEE bits, with NaN and −0 folded; anything that isn't a number is a cart error · the brief says "int", but floats work and catch more drift.
- 2026-10-22 · The RNG is xoshiro128** seeded through splitmix64. `rnd(x = 1)` is next / 2^32 · x; `rndi(a, b)` is uniform and unbiased (rejection), with a ≤ b, both whole numbers in the 32-bit range. The sequence is pinned against an independent Python port · integer-only, so every platform draws alike.
- 2026-10-22 · `rnd` and `rndi` raise an error inside `cart.draw()` · draw runs a different number of times windowed and headless, so drawing must not consume random numbers.
- 2026-10-22 · `m.sin`, `m.cos`, `m.atan2`, `m.exp` take radians and use Cody–Waite range reduction plus Taylor polynomials, built from + − × ÷, `floor` and `ldexp` only; `atan2(0, 0)` is 0. Their bits are pinned against an independent Python port and agree with libm to about 1 ulp. `m.sqrt`, `m.floor`, `m.clamp` and `m.lerp` are exact IEEE operations · the brief's "fixed algorithm"; radians match `math`.
- 2026-10-22 · The seed comes from `--seed S`; by default it is 0 headless and the clock in a window. Every result line prints it, and `record` stores it · headless runs repeat; games vary from launch to launch.
- 2026-10-22 · Fuzz input has its own RNG from `fuzz:S`. A direction (none a third of the time) is held for 10–90 ticks. Buttons press in bursts: 70% taps of 1–6 ticks, 30% holds of 15–60 ticks. Idle gaps are A 5–60, B 10–120, L/R 30–300, X/Y 60–600 and Start 1200–3600 ticks. It never uses the pointer · "held directions, button bursts"; X/Y are included for the import system.
- 2026-10-22 · Traces are text: `mother-trace 1`, `cart`, `seed` and `ticks` header lines, then one `HELD SX SY PTR HASH` line per tick. Each line stores the tick's state hash as well as its pad, so a replay checks itself on any platform without a reference run · the brief asks for seed + pads; with the hashes in the trace, CI's cross-platform comparison is just a replay.
- 2026-10-22 · `replay` is always headless, runs exactly the trace's ticks and exits 1 on a hash mismatch (it reports the first) or if the cart stops early; `--hash` prints `TICK HASH` for every tick. `record` works windowed (play) or headless with fuzz input. `run --input trace:FILE` plays a trace's input and seed, windowed or headless, without checking · one checker, flexible ways to make and watch traces.
- 2026-10-22 · The result line is `result=… ticks=… seed=… budget=N hash=H`, followed by `audio=A` when headless and `replay=…` for replays; budget counts the ticks that ran over 8 ms · the brief's result, tick count, budget warnings and final hash on one parseable line.
- 2026-10-22 · Stored traces: `tests/traces/<cart>.trace` for each test cart, recorded from fuzz input in the Windows Release build. CTest replays them in every build on every CI platform; that is the cross-platform hash comparison · "one stored trace per cart".

## M5

- 2026-10-22 · MOTHERLIB is Luau in `lib/` (`core/`, `archetypes/`), compiled into the binary as byte arrays by `cmake/embed_lib.cmake`. Its modules load each other with a private `use("core/x")` and see a private `host` table (`modules`, `map_size`, `in_draw`); carts reach only the frozen `world` global · "shipped with the runtime, loaded read-only into every cart VM", with no files to find at run time.
- 2026-10-22 · The world is created from `cart.toml` before `main.luau` runs. `world:step()` runs after `cart.update` and is inside the same 8 ms budget. A step runs: behaviour (with statuses), gravity and movement, status timers, camera and bounds, contacts (damage, `on_contact`), spawners, the objective, then events · the brief's order from section 4, with behaviour first because it sets the velocities.
- 2026-10-22 · `cart.toml` is a TOML subset parsed by the host (`host/assets/toml.h`) and checked strictly against the schema in `host/assets/manifest.h`; unknown keys are errors, and every test cart now has a full manifest · the import harness (M8) reads manifests in C++, and strictness catches LLM slips.
- 2026-10-22 · Manifest additions: `[avatar]` has `x`, `y` (the start position) and optional `params`. `[objective]` takes `ticks` (survive), `laps` (race), `exit = { x, y, w, h }` (escape) or `archetype` (collect). `[avatar]` is present exactly when `env.avatar` isn't none; `[[spawn_points]]` (one or more) exactly when `env.spawn` isn't none · the brief's example has no avatar position and no objective parameters.
- 2026-10-22 · Entity positions are collider centres; colliders are even, 2–32 px a side; `world:spawn(archetype, params, x, y, faction)` requires the faction · centres make distances and spawn points independent of size.
- 2026-10-22 · An archetype module returns a table (format in `lib/core/archetypes.luau`) with a required `kinematic` flag, a damage response of `hp`, `stun` or `none` (immune), a contact damage `tier`, and hooks `init`, `update`, `draw`, `on_contact`, `on_block`. Spawns may override only the declared params, with values of the same type · the brief asks for explicit declarations; strict params catch typos.
- 2026-10-22 · Every spawn (`world:spawn`, spawners, the avatar) checks the archetype's `requires` against `[env]` and fails with e.g. `needs surface=ground, host has none`; equipment also needs an avatar and deployables need `deploy = true` · the same check the import harness needs; applying it to native spawns keeps manifests honest.
- 2026-10-22 · Velocity is clamped to ±4 px/tick on each axis, not by magnitude. Gravity adds 0.25 px/tick² to non-kinematic bodies, up to a 4 px/tick fall · per-axis limits bound each sweep to 4 px, and running while falling keeps its horizontal speed.
- 2026-10-22 · Bodies move x then y, each axis swept against solid tiles and `solid` entities. Only map layer 0 collides (tile 0 is always empty; flag bit 0 = solid); air bodies pass over both. A block calls the archetype's `on_block(nx, ny)`, or stops that axis · axis-separated sweeps are exact on the tile grid and can't tunnel.
- 2026-10-22 · After moving, every pair whose straight start-to-end paths overlapped during the step (touching doesn't count), or where one stopped the other, is in contact once that step. Pairs are found by sweep-and-prune on x; contact damage goes both ways, then each side's `on_contact` · relative-motion swept AABBs catch hits between whole-tick positions, including two bodies closing at 4 px/tick each.
- 2026-10-22 · After a hit lands, a body takes no damage for 30 ticks; a stun body's stun is set to at least 60 ticks per hit · the brief gives no number, and without it contact damage would land on every tick of an overlap.
- 2026-10-22 · The world applies statuses itself. Stun skips `update` and zeroes the body's own velocity (it still falls). Slip runs `update` but restores the velocity it had. Knockback skips `update` and moves the body at run speed (2 px/tick) away from where its last hit came from (backwards along its facing if never hit). Timers count steps, and re-applying a status never shortens it · every archetype honours all three without code of its own; `world:status` has no direction argument, so the last hit's source gives the direction.
- 2026-10-22 · Bounds: the world rectangle is map layer 0 from (0, 0), or the 320×180 screen without a map; under `scroll = "auto"` the bounds are the camera view. An edge applies to an entity once it has overlapped the bounds, so things can enter from outside. solid clamps and blocks; kill and despawn act once the entity is wholly past · auto-scroll can't use a fixed rectangle: a map is at most 256 tiles (2,048 px), and STARFIGHTER scrolls 2,700 px in 90 s.
- 2026-10-22 · Under follow and auto scroll, entities more than one screen outside the camera view are removed silently, except the avatar and objective entities · "behind the camera" has no fixed direction for a follow camera (RALLY's loop), so it reads as "away from"; racers and kill_all targets must never vanish.
- 2026-10-22 · Camera: `none` stays at (0, 0); `follow` centres on the avatar (floored) inside the world rectangle; `auto` rises 0.5 px/tick from (0, 0), with no manifest key. Carts set `camera(world:camera())` themselves; `world:draw()` draws in world coordinates · 0.5 is STARFIGHTER's speed; the brief gives auto-scroll no parameters.
- 2026-10-22 · New `world:spawner{ archetype, faction, params, count = 1, every = 0, points = all }`: releases `count` entities at the cart's spawn points in turn (camera-relative under `spawn = "camera"`), the first on the next step and then one every `every` steps, all at once when 0 · the brief's step "runs spawners", but its world API has no way to make one.
- 2026-10-22 · Objectives: kill_all is met when no host-faction `hp` entity is left and no host spawner has spawns left; survive after `ticks` steps; escape when the avatar overlaps `exit`; race when the avatar or a host racer has `laps` laps (the event names the winner); collect when no entity of `archetype` is left. The `objective` event fires each time the objective becomes met; the cart decides win or lose · KNIGHTFALL's waves re-arm kill_all; result screens are the cart's.
- 2026-10-22 · Objective entities (those the objective counts) can't be damaged or blocked by imported entities; the `imported` flag exists now and M8 sets it · the brief's rule, kept in the core where damage and blocking are decided.
- 2026-10-22 · Events (`spawn(id)`, `hit(id, tier, source)`, `death(id, view, source)`, `objective(type, id)`) queue up during `cart.update` and the step and are delivered at the end of `world:step()`, handlers in the order added; events raised by handlers are delivered in the same pass · deterministic, and safe for handlers that spawn or damage.
- 2026-10-22 · `world:get` returns a frozen snapshot (`id archetype faction layer imported x y vx vy w h hp status{stun slip knockback}`), or nil once the entity is dead or removed. Gone entities leave the list at the end of the step; ones spawned during a step first act in the next. `world:query(x, y, w, h, filter)` returns ids in id order; filters are tables (`faction`, `enemy_of`, `layer`, `archetype`, `damageable`, `exclude`); `nearest` breaks ties by the lower id · views can't leak mutable state; tables keep filters data, not code.
- 2026-10-22 · World methods must be called with a colon. `spawn`, `remove`, `damage`, `status`, `on` and `spawner` raise errors inside `cart.draw()`; `world:draw()` works only there and draws ground, object, projectile then air layers, each in id order · the same draw/update split as the rest of the API.
- 2026-10-22 · The world's state joins the per-tick hash: step count, next id, camera, objective state, spawners, and for every entity in id order its id, faction, position, velocity, HP, status and hurt timers and laps. They are written little-endian into a buffer with NaN and −0 made canonical; the host hashes the bytes. The stored traces were re-recorded · the brief's list; x64 and ARM64 produce different NaN bits.
- 2026-10-22 · `lib/archetypes/dummy.luau` is a configurable body (collider, layer, damage, tier, constant velocity, bounce, racer laps) for MOTHERLIB's tests and M8's axis tests · M5 has no game archetypes, and section 9 calls for "other-faction dummies".
- 2026-10-22 · `damage.tile` (for archetypes) breaks a damageable tile (flag bit 1) on layer 0 to tile 0 unless it is critical (bit 2); MOTHERLIB never overwrites critical tiles, while a cart's own `mset` is unrestricted · the cart owns its map.
- 2026-10-22 · MOTHERLIB tests are small carts in the unit-test binary (`tests/world_cart.h`), with checks written in Luau; `tests/carts/world` is a smoke cart with a stored trace and a 10-minute fuzz run in CTest · the world is Luau, so its tests are too.

## M6

- 2026-10-22 · An archetype's `requires` now names all six axes, each a list of accepted values or `axes.ANY` · the brief says to declare requirements "on each [env] axis".
- 2026-10-22 · Art and sound are params. An archetype lists its sprite params with their block size and frame count (`sprites`) and its sfx params (`sounds`). Sprite 0 draws a box in the faction's colour, and sfx −1 is silent, so tile 0 of a cart's sheets stays empty. Carts pass their own tiles and sfx in spawns and the same values in `[[exports]]` · "carry its own sprite and sfx references"; the import harness (M8) copies exactly the named tiles and sfx.
- 2026-10-22 · Archetype sounds alternate between voices 5 and 6; the carts' music uses channels 1–4 · M3's automatic voice is the lowest free one, voice 1, which would cut the music's lead.
- 2026-10-22 · Core additions: an `on_leave(e, reason)` hook (death, despawn or remove); `on_block` receives the blocking entity; `static` bodies never move; a `reflector` flag; and a `ghost` state (out of play: no contacts, blocking, queries or bounds), used by a ball waiting to be served · needed by brick drops, ball re-serves and paddles.
- 2026-10-22 · Entities that MOTHERLIB spawns itself (shots, drops, re-served balls) go through `spawn.child`: they must suit `[env]` like any spawn, and they are imported when their parent is · imported things stay imported (M8).
- 2026-10-22 · A self-propelled object keeps the velocity it wants in its archetype state and reapplies it every update, so the velocity survives a stun or knockback; reflectors and bounces change that wanted velocity · statuses stay honoured without losing momentum for good.
- 2026-10-22 · Equipment is an entity held by the avatar. Its params:
  - `button`: a, b, l or r when the cart binds it; x for the import system;
  - `ticks`: how long it stays held, 0 for good;
  - `loose`: it starts as a pickup drifting down at `drop` px/tick, taken by touching the avatar.

  Taking a second copy renews the held one. Equipment can't act while its holder is stunned or knocked back. MOTHERLIB reads X and Y through a private `host.btn` and `host.btnp` · native pickups are bound by the cart to A, B, L or R, and imports to X, as the brief says.
- 2026-10-22 · A `shot` archetype (not exported) is the projectile that firing archetypes spawn. It flies straight, is spent on its first hit on another faction or on anything solid (hurting a solid it hits), and expires after `life` ticks · the brief's firing archetypes need an entity for their shots, and the table has no projectile.
- 2026-10-22 · Melee and blasts:
  - a swing (sword, skeleton) hits every damageable entity of another faction whose centre lies inside the arc and within reach plus both half sizes;
  - a blast (bomb) hits every damageable entity whose collider comes within the radius, any faction, and every layer-0 tile whose centre does;
  - whatever a swing or blast hits is also knocked back for 6 ticks.

  The brief gives arcs, reaches and radii but no hit test.
- 2026-10-22 · Reflection: an object or projectile moving into a reflector leaves along the reflector's facing at the same speed, up to 60° off depending on where it hit across the face, and the reflector's faction takes ownership. A ball bounces off other player-faction bodies by reversing on the axis where it is furthest from the body's centre · "angle off the paddle depends on hit position".
- 2026-10-22 · Chasing AI walks straight at its target and slides along walls when blocked, turning back at corners · straight-line skeletons stuck fast on KNIGHTFALL's crates and pillars.
- 2026-10-22 · Numbers the brief leaves open:
  - knight: walks 1 px/tick; dodge moves at fast speed;
  - skeleton: 0.5 px/tick, 8 px reach, 20-tick wind-up, 60-tick cooldown;
  - paddle avatar: 3 px/tick;
  - sword: 20-tick cooldown;
  - ball: served heading up within 30° of vertical, from just above the avatar (or where it was first spawned, with no avatar).

  These are the brief's speed tiers, picked so the games play fairly.
- 2026-10-22 · Requirements:
  - knight: gravity none, avatar 8dir;
  - paddle_avatar: avatar up;
  - skeleton: ground surface and spawn points;
  - bomb: deployable;
  - sword, laser, paddle: equipment, which needs an avatar;
  - ball, brick and shot: accept everything.

  Matches the expected matrix in section 9; the ball's "bounds" requirement reads as "works under every bounds value".
- 2026-10-22 · KNIGHTFALL:
  - a 40×22 arena: walls and pillars are solid and critical, crates solid and damageable;
  - 4 edge spawn points; each wave's spawner releases a skeleton every 40 ticks;
  - A or Start leaves the title; the result shows for 4 s, then the cart finishes.

  BREAKOUT:
  - side and top walls in the map, open at the bottom over the kill edge;
  - each brick drops a laser with chance 0.1 (`rnd`);
  - the first serve comes 60 ticks after start; 3 balls.

  Native spawns repeat the `[[exports]]` params · the brief's layouts, with title and result screens.
- 2026-10-22 · Cart art, maps and audio (`sprites.spr`, `map.map`, `audio.trk`) were written once by throwaway Python scripts and are committed as the hand-editable source · as with the M2 and M3 fixtures.
- 2026-10-22 · "Runs 10 minutes under fuzz" means up to 36,000 ticks with no error or budget overrun; a cart that wins or loses sooner has finished. CTest runs 3 seeds per cart, twice each, and replays a stored trace per cart · random input loses both carts within a few minutes.
- 2026-10-22 · "Winnable and losable" is shown by bots in the unit tests. A bot is a Luau function that runs inside the cart's VM between ticks, reads the world and returns the next pad (`tests/bot.h`, through a new `CartVM::state()`); it only reads, so it plays as a player would · a real win needs real input; recorded play wasn't possible here.

## M7

- 2026-10-22 · Archetypes may define `guard(e, other)`: true shields e from other's contact damage in that contact. Both sides are asked before either takes damage · a stomp (runner) and a ram (cars) must land on one side only, and contact damage is otherwise symmetric.
- 2026-10-22 · Archetypes may list `public` mem fields that `world:get` views show (falling_piece `shape rot col row`, piece_controller `lines piece`, car `angle`, nitro `charges`), and every view now has `laps` · HUDs and bots need that state (lines, charges, laps, a car's heading), and the world API has no other way to read it.
- 2026-10-22 · Entities have a `boost` timer (nitro), hashed and counted down with the statuses; `act.speed(e, base)` doubles a speed while it runs, and every avatar archetype (knight, paddle_avatar, runner, ship, car) takes its speeds through it · nitro is equipment that works in five carts, so every avatar has to honour "speed ×2" the way it honours a status.
- 2026-10-22 · A race objective needs `line = [[x, y], ...]` (px, at least 2 points), the racing line. A racer heads for one point at a time and has passed it within 64 px; point 1 is the start line, and each return to it after the rest is a lap. Progress is hashed · the brief has rivals "follow a waypoint racing line" and races count laps, but defines neither; 64 px covers RALLY's track (42 px to the grass edge) even across an inside corner.
- 2026-10-22 · `survive` counts from the cart's first `world:spawn` or `world:spawner` call · otherwise time on a title screen would count; BLOCKFALL and STARFIGHTER start their games by spawning.
- 2026-10-22 · Tile flag bit 3 = offroad (MOTHERLIB's own) · RALLY's offroad tiles need a tile property; bits 0–2 are the brief's.
- 2026-10-22 · `fireball` (fire_flower) and `missile` (homing) are projectile archetypes that no cart exports, like `shot` · the brief's weapons need entities for what they fire.
- 2026-10-22 · Numbers the brief leaves open:
  - runner: accelerates and stops at 0.125 px/tick² up to run speed; jumps at 4 px/tick (30 px high); letting go of A cuts upward speed to 1; a stomp is landing on another faction (or anything that hurts its owner) with its feet at most 4 px into the other's top, then a bounce at 3 px/tick (4 with A held); a head-bump hits the solid tile over its head nearest its middle;
  - walker and turtle: crawl speed along x, setting off towards their target, walking off ledges; a turtle killed by a hit from above (the source's centre above its top) leaves a shell sliding away from the hitter;
  - fire_flower: an `hp` param (the holder's HP once taken, JUMPER 2); under gravity its fireballs set off level at fast speed, bounce 2 px/tick off floors and end at walls; without gravity they fly along the facing and ricochet 3 times; 150-tick life;
  - cars (`core/drive.luau`): top speed run, acceleration 0.05, braking 0.1, reversing at walk speed, drag 0.01, turning 0.06 rad/tick, and grip removes 20% of the sideways slide each tick; a landed ram also knocks the hit car back for 8 ticks;
  - rival_ai: brakes for turns over about 70°; after 60 ticks stuck it backs off for 30;
  - nitro: the 300-tick recharge starts at the boost; a `charges` param (0 = no limit, RALLY 3) after which it is gone;
  - ship: run speed; shots at projectile speed live 60 ticks;
  - diver: swoops for 90 ticks (1 px/tick down, up to 1.5 px/tick sideways), dives at fast speed at where its target was, bullet at run speed;
  - homing missile: fast speed, turns 0.1 rad/tick, 120-tick life;
  - piece_controller: auto-repeat after 12 ticks then every 4; soft drop a cell every 2 ticks; the lock delay doesn't reset on moves.

  These keep to the brief's speed tiers and play fairly.
- 2026-10-22 · Rams: a car is hurt only when the other car closes in on it at least as fast as it closes in on the other (a rear-ender hurts the car in front, a head-on both, two cars at rest neither) · cars aren't solid, and without this, cars resting on top of each other spun each other out forever.
- 2026-10-22 · Requirements:
  - runner: gravity down, avatar lr;
  - car: gravity none, surface ground, avatar heading (like every avatar body);
  - ship: avatar up;
  - piece_controller: gravity down, avatar none;
  - walker: surface ground, spawn; turtle: gravity down too;
  - rival_ai: gravity none, surface ground, spawn;
  - diver, mine: spawn;
  - falling_piece: gravity down;
  - shell: deployable; oil: deployable, surface ground;
  - fire_flower, nitro, homing: equipment.

  Matches the expected matrix in section 9.
- 2026-10-22 · Mines take hits without dying (stun response): any contact or hit sets one off, and the blast (radius 1 tile, tier 10, any faction) is its end · "detonates on any contact" leaves it no other way to die, and a stun body can still be hit and homed on.
- 2026-10-22 · Under auto scroll the ship and divers add the camera's motion to their own (`act.drift`), and the ship keeps inside the view the camera will have after the step. A diver comes back in from its spawn point (relative to the camera) once its centre has left the view, or a solid edge stops it · "8-dir within the screen" and "exit and re-enter" are relative to a moving screen.
- 2026-10-22 · BLOCKFALL: shapes I O T S Z J L with SRS rotation states turned in their box; pieces start centred on the well's top row; the wall kick tries one column left, then right; the controller clears full rows after each landing and dies when the next piece can't fit (the cart's top-out); it draws the next piece at its own position, the cart's preview box. A falling piece moves itself along the grid (static to statuses), never overwrites critical tiles, and with `tiles = 0` lands as nothing · the brief's rules; "N set by the piece" for the fall rate.
- 2026-10-22 · STARFIGHTER: no map (the cart draws a starfield); every edge despawns, and the cart loses if the ship ever leaves; a formation of 4–8 divers from one random top spawn point every 300 ticks, down to 150 by the end, skipped while 10 divers are about; a mine every 90–180 ticks · formations "of 4–8 from the top" and mines "with the scroll".
- 2026-10-22 · JUMPER:
  - ground 3 tiles thick; 5 pits 3 tiles wide; pipes at most 3 tiles tall; stairs before the flag at column 192 (the exit);
  - enemies appear as the camera comes within 40 px of their spawn points;
  - the runner is held (stunned) behind the title screen;
  - a ?-block is a damageable tile the cart watches: once broken, it becomes a used block and a fire flower drops out of its underside.

  A 30 px jump can't reach the top of a block 4 tiles up, where Mario's flower sits.
- 2026-10-22 · RALLY:
  - the 120×80-tile track is drawn round the racing line: road within 30 px, offroad grass to 42, walls to 52, solid scenery beyond;
  - the start line is the line's first point, with the player's car on pole and the rivals behind;
  - a 3-second countdown holds the car with `slip` (a stun would show a spin-out), and rivals spawn at "GO";
  - oil is dropped 14 px behind the car.
- 2026-10-22 · Bots are `tests/bots/*.luau` files read at run time · editable without rebuilding, and too long to keep as C++ strings.
- 2026-10-22 · All stored traces were re-recorded from the same fuzz seeds (the hash gained the game start, boosts and racing progress), and each new cart has one; RALLY's stops at 3,600 ticks, as a fuzzed race runs to about 4,000 · same inputs and outcomes, new hashes; a smaller file.
- 2026-10-22 · Imported falling pieces should decay after 1,200 ticks, and can't land as sheet-3 tiles (map cells take tiles 0–3071); both are left for the import harness (M8) · they only arise with imports. (Done in M8: see below.)

## M8

- 2026-10-22 · `mother import CART SOURCE:EXPORT [...]` takes up to 3 exports and the options of `run`. SOURCE is a cart folder next to CART, or a path. MOTHERLIB checks each export once the world exists, before main.luau runs, with the same check as native spawns. When CART refuses one, it prints e.g. `can't import jumper:walker: needs surface=ground, host has none` and exits 3 · the brief's message; a distinct exit code lets scripts tell refusals from errors.
- 2026-10-22 · An import's art and sounds are copied once into the host cart, and its params are pointed at the copies:
  - the sprite blocks its archetype declares (`w × frames` by `h` tiles) go into sheet 3, packed left to right in rows;
  - its sfx go into sfx slots 48–63, and the instruments they use into instruments 48–63; a note that relied on the default instrument 0 gets the source's instrument 0;
  - wavetables aren't copied (no cart uses them).

  The brief puts imported art in sheet 3 and gives imported sounds voices 7–8, but an sfx must be in the host's tables to play; reserved slots mirror the reserved sheet.
- 2026-10-22 · Imported entities' sounds take turns on voices 7 and 8 (`act.sound(e, n)` reads `e.imported`). The sequencer now plays sfx on all 8 voices; the cart API's `sfx` still takes voices 1–6 · brief section 3.
- 2026-10-22 · How each import enters the world:
  - equipment: held by the avatar from the start, on X (`button = "x"`, not loose, held for good);
  - deployables: dropped on Y just behind the avatar, against its facing; a `dir` param (the shell's) points away from it; several deployables take turns;
  - creatures: one at the cart's first spawn point at once, then one every 300 ticks, cycling through the spawn points;
  - other objects (auto-objects): one at the start, at the top of the view over the avatar (the view's centre without one);
  - bodies (avatars) are refused.

  Creatures and auto-objects are faction `wild`; equipment and deployables are `player`'s. The brief names the modes, but not where or when.
- 2026-10-22 · Caps: 3 exports per run, and 8 live instances spawned by the import system; the shots and drops of imported entities are imported too, but don't count · "8 imported instances" read as what the harness places.
- 2026-10-22 · Imported solids decay. Solid imported entities are removed 1,200 ticks after they appear. An imported falling piece's cells decay 1,200 ticks after it lands: the piece stays, out of play, as a solid, and gives back what its cells held unless something broke them · brief section 6.
- 2026-10-22 · Map cells and tile flags take tiles 0–4095 at run time, so sheet 3's tiles can be placed; `map.map` still holds 0–3071 and 3,072 flags. MOTHERLIB flags an imported piece's tile solid and damageable when it lands, and the tilemap hash covers all 4,096 flags · an imported falling_piece (✓ in JUMPER) must land as its own art.
- 2026-10-22 · Mother Gold outlines: `world:draw()` queues a 1 px rectangle round each imported entity's collider, 1 px out, for entities in play and not in a holder's hand; the host draws them in colour 63 after `cart.draw()`, over everything and ignoring clip and palette · brief section 4, step 5. AAP-64's index 63 is dark brown, so the outlines are faint until the final palette arrives.
- 2026-10-22 · Entities spawned by other entities (shots, drops) and imported deployables count as inside the bounds from the start, so the edges apply at once · the matrix test found shots fired by an avatar against a wall flying off for good, and oil dropped behind one falling forever.
- 2026-10-22 · A ball with no avatar re-serves from its start point relative to the camera, and a diver re-enters once its centre leaves the view or the bounds, whichever comes first · found by the matrix test: under auto scroll the ball served from a point far behind the camera, and in BREAKOUT (176 px tall) divers died at the kill edge before they ever left the 180 px screen.
- 2026-10-22 · Falling pieces stay inside the world rectangle (its sides and bottom), and line clears never move or overwrite critical tiles · found by the axis tests: in a world without a well, a piece moved off the map fell forever in a hard drop, and a floor of critical tiles was cleared as a full row.
- 2026-10-22 · `mother matrix [CARTS] [--out FILE] [--seeds N] [--ticks N]`, defaults `carts`, `matrix.md`, 3 and 36,000:
  - the carts are CARTS' subfolders with a cart.toml, in name order; their exports come in manifest order, under the carts' titles;
  - a pair is compatible when MOTHERLIB accepts the import;
  - each compatible pair runs headless for fuzz seeds 1–N (the same number seeds the cart's RNG) until the cart finishes or the ticks run out, then again: the "replay", which must give the same hash on every tick.

  It fails a pair on:
  - a Luau error (a NaN position is one);
  - ticks over the CPU budget in both runs; one run alone is a warning, because the budget is wall time and a busy machine went over it once in testing;
  - an entity wholly outside the bounds for more than 60 ticks in a row;
  - a replay mismatch;
  - no objective progress for 1,800 ticks while an imported solid is in the world.

  Progress is a count that grows: with each objective entity (or spawn) fewer under kill_all and collect, each step under survive, each px nearer the exit under escape, and each racing-line point passed under race · the brief's checks, made measurable.
- 2026-10-22 · The computed table equals the brief's expected one (`tests/matrix_expected.md`, with export ids for names: `rival_car` for "rival car"); neither a manifest nor the brief's table needed changing. CTest runs the matrix; Debug builds run 3,600 ticks per run instead of 36,000 · a full Debug matrix would take most of an hour; Release covers the brief's 10 minutes.
- 2026-10-22 · Archetype axis tests (`tests/test_axes.cpp`) read every archetype's declarations from MOTHERLIB (a `describe` hook). For each archetype there is:
  - a base world: the first value it accepts on each axis, from gravity none, surface ground, scroll none, avatar 8dir, spawn world, deploy true;
  - one world per accepted value of each axis, and one per bounds type.

  Equipment needs an avatar and deployables need deploy, so those values are left out. A body that is an avatar is the [avatar]; otherwise a dummy is, and the archetype is spawned (creatures as host, the rest as player). Three dummies, one per faction, bounce about with contact tier 1, and fuzz input drives the pad for 600 ticks. Checked every tick:
  - no errors;
  - nothing wholly outside the bounds for more than 60 ticks;
  - its damage response to a hit (hp loses 1 and dies at 0, stun is stunned 60 ticks, none takes nothing);
  - no hit within one faction unless the source hurts its owner or is a blast (bomb, mine).

  28 archetypes, 508 worlds; a deliberately wrong expectation fails every stun and none archetype · "a generated test world for each [env] value it accepts".
- 2026-10-22 · The state hash gained the import timers and an imported bit per entity; every stored trace was re-recorded from the same seeds, with the same outcomes.

## Run layer (after v1)

- 2026-10-22 · `mother rogue [CARTS]` plays a run: every cart in CARTS (default `carts`) once, in a random order, in one window. It lives in the host, built on the import harness like `import` and `matrix`, not in a shell cart · carts are sandboxed and can't start other carts, and a shell cart would need host functions beyond the frozen v1 API. The brief puts meta-games out of scope; the user asked for this after v1.
- 2026-10-22 · The order is a shuffle drawn from the run seed, which also seeds every cart, so `mother run CART --seed S` replays a run's cart without its imports. The seed comes from the clock in a window and is 0 headless, as for `run`; a new run after the last one takes seed + 1.
- 2026-10-22 · One loss ends the run; there are no lives and no difficulty curve yet · the user's call for later ("we'll see").
- 2026-10-22 · Loot: after each win except the last, you take one of that cart's exports or skip. You hold at most 3, the import cap; taking a fourth means dropping one. Every export can be taken, creatures and auto-objects too, which come in wild and attack anyone · the user's rule ("you get to pick after winning").
- 2026-10-22 · Each cart gets the held exports it can host, in the order they were taken; the rest sit it out, shown with MOTHERLIB's reason. The check is MOTHERLIB's own: a new `check` hook runs the test `imports.add` makes, against any cart's [env], so the run and `mother import` always agree. A unit test holds it to the expected matrix.
- 2026-10-22 · Screens between carts (intro, next cart, loot, swap, run over, run complete) are drawn by the host with the built-in font. Each export shows the first frame of its main sprite (`sprite`, else its first sprite block), copied from its cart and drawn at 2×; `describe` now lists each archetype's sprite blocks. The screens are silent.
- 2026-10-22 · Screens: left/right choose, A confirms, B skips or goes back. They ignore input for their first 20 ticks, so a button still held at the end of a cart doesn't choose. A cart starts on the tick after every face button is up, and its first tick sees the pad as a fresh run would: the press that left the screen never reaches its title screen.
- 2026-10-22 · `Program::audio_generation()` changes when a program's audio setup does. Both runners then restart their synth with the new setup before that tick's commands; the windowed one queues the restart on the audio thread, in order with the ticks. The run changes it when a cart opens and when it closes, so notes never ring on into a screen or the next cart.
- 2026-10-22 · A run's state hash covers the screen, the stage, the cursor, the held exports, the seed and the cart's own hash. At the end, A starts a new run and B quits with `result=win` or `result=lose`.
