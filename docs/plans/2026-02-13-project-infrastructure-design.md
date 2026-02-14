# EMDR Lightbar — Project Infrastructure Design

## Overview

Set up the build infrastructure, CI/CD pipeline, and project conventions for an
EMDR lightbar project. The project drives an oscillating LED strip from a
microcontroller. The oscillation logic is written in platform-agnostic C so it
can be compiled natively (for the MCU) and to WebAssembly (for browser-based
visualization via GitHub Pages).

This initial milestone delivers a "hello world" C program that goes through
the full pipeline: native build, Unity tests, Emscripten WASM build, and
deployment to GitHub Pages.

## Architecture

Three layers:

1. **Core logic** (`src/`, `include/`) — Platform-agnostic C (C99). Pure
   functions implementing oscillation state: position, speed, pause durations.
   No I/O or hardware dependencies.

2. **Platform drivers** (future) — Thin wrappers that call core logic and
   interface with hardware (Arduino Uno LED strip) or browser rendering
   (Emscripten exports). Not part of this milestone.

3. **Web visualization** (`web/`) — HTML page loading WASM-compiled core
   logic. Renders LED state visually. Deployed to GitHub Pages.

## Directory Structure

```
emdr-lightbar/
├── CLAUDE.md
├── Makefile
├── src/
│   └── main.c
├── include/
├── test/
│   └── test_main.c
├── lib/
│   └── unity/              (git submodule)
├── web/
│   ├── index.html
│   └── (main.js, main.wasm)  (generated, gitignored)
├── build/                   (gitignored)
├── .github/
│   └── workflows/
│       └── ci.yml
└── .gitignore
```

## Build System

Makefile with these targets:

- `make` / `make native` — compile `src/` to native binary in `build/`
- `make test` — compile and run Unity tests
- `make wasm` — compile `src/` via `emcc` to `web/main.js` + `web/main.wasm`
- `make clean` — remove `build/` and generated WASM files

## Testing

- **Framework**: Unity (vendored as git submodule in `lib/unity/`)
- **Test files**: `test/test_*.c`
- All core logic must have corresponding tests
- Tests run natively via `make test`

## CI/CD Pipeline

GitHub Actions workflow (`ci.yml`):

1. **`test` job** — `ubuntu-latest`
   - Checkout with submodules
   - `make test`

2. **`build-and-deploy` job** — `ubuntu-latest`, depends on `test`
   - Checkout with submodules
   - Install Emscripten via `mymindstorm/setup-emsdk`
   - `make wasm`
   - Deploy `web/` to GitHub Pages via `actions/deploy-pages`
   - Deploys only on pushes to `main`

## Conventions

- C99 standard
- No platform-specific code in `src/` core logic
- Unity for all tests
- Target MCU: Arduino Uno (platform-agnostic core, driver layer added later)

## Hello World Milestone

Deliverables:
- `src/main.c` prints "Hello, World!"
- `test/test_main.c` has a passing Unity test
- `web/index.html` Emscripten shell displays the output
- `ci.yml` runs tests, builds WASM, deploys to Pages
- `CLAUDE.md` documents all of the above
