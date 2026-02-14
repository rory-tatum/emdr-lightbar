# EMDR Lightbar

Oscillating LED strip controller for EMDR therapy with web-based visualization.

## Architecture

- **Core logic** (`src/`, `include/`) — Platform-agnostic C99. Pure functions
  implementing oscillation state. No I/O or hardware dependencies.
- **Web visualization** (`web/`) — Core logic compiled to WASM via Emscripten.
  HTML page renders LED state. Deployed to GitHub Pages.
- **MCU driver** (future) — Thin wrapper calling core logic to drive LED strip
  on Arduino Uno.

## Build Commands

- `make native` — compile to native binary in `build/`
- `make test` — compile and run Unity tests
- `make wasm` — compile to WASM via Emscripten into `web/`
- `make clean` — remove all build artifacts

## Project Structure

- `src/` — C source files (core logic)
- `include/` — C headers
- `test/` — Unity test files (`test_*.c`)
- `web/` — HTML page + generated WASM (`.js`/`.wasm` are gitignored)
- `lib/unity/` — Unity test framework (git submodule)
- `build/` — Native build output (gitignored)

## Conventions

- C99 standard, `-Wall -Wextra`
- No platform-specific code in `src/` — keep core logic portable
- Every function in core logic must have a Unity test
- Tests go in `test/test_<module>.c`
- Frequent, small commits

## CI/CD

GitHub Actions (`.github/workflows/ci.yml`):
1. Runs `make test` on every push and PR
2. Builds WASM via Emscripten
3. Deploys `web/` to GitHub Pages on pushes to `main`

## Dependencies

- GCC (native build)
- Emscripten (WASM build — installed automatically in CI)
- Unity test framework (vendored as git submodule in `lib/unity/`)
