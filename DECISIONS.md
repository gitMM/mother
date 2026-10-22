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
