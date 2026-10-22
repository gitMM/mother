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
mother run                                # palette test pattern in a window
mother run --headless --ticks 3600        # no window or audio; prints result and tick count
```

Keys: arrows/WASD = stick, Z = A, X = B, C = X, V = Y, Q = L, E = R, Enter = Start. Gamepads work too.
