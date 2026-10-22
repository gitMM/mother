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
