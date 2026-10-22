# Vendored dependencies

Plain source copies, built from this repo with no network access.

| Library | Version | Source | License |
| --- | --- | --- | --- |
| SDL3 | 3.4.18 (git 829a65d7) | https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18 (`SDL3-3.4.18.tar.gz`) | zlib, `SDL/LICENSE.txt` |
| Luau | 0.743 | https://github.com/luau-lang/luau/releases/tag/0.743 (source tarball) | MIT, `luau/LICENSE.txt` |
| stb_image | nothings/stb @ a7d4c217 | https://github.com/nothings/stb/blob/a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d/stb_image.h | MIT / public domain, in the header |

Removed from the upstream copies because MOTHER never builds them:
SDL `test/`, `examples/`; Luau `tests/`, `bench/`, `fuzz/`, `.github/`.

`stb_image.h` is used only by `tools/png2spr` (brief section 2).

## Local patches

- `luau/VM/include/luaconf.h`: `LUAU_FASTMATH_BEGIN`/`END` are always empty. Upstream turns on
  `/fp:fast` inside a few VM math functions on MSVC (`%`, `//`, `floor`, `ceil`, `sqrt`, `round`);
  that breaks the `/fp:strict` build and risks Windows-only results.
- `luau/Common/include/Luau/MotherPow.h` (new): `luau_mother_pow`, a deterministic `pow`.
  Integer exponents use exponentiation by squaring, `0.5` uses `sqrt`, anything else is NaN.
  It replaces libm `pow` for the `^` operator in `luau/VM/src/lnumutils.h` (`luai_numpow`) and
  two fast paths in `luau/VM/src/lvmexecute.cpp` (`LOP_POW`, `LOP_POWK`), and in constant folding
  in `luau/Compiler/src/ConstantFolding.cpp`. libm `pow` differs between platforms.
  (`luau/Bytecode/include/Luau/Sccp.h` still calls `pow`; only the JIT inliner uses it and
  MOTHER does not build that.)
