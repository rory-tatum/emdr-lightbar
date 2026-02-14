# Oscillation Logic Design

## Overview

Oscillating LED dot controller for EMDR therapy. A single bright dot with
configurable glow travels back and forth across an LED strip, pausing at each
end and at the middle. The core logic is platform-agnostic C99 — the same code
drives both a web visualization (via WASM) and an eventual MCU LED driver.

## Data Structures

```c
typedef struct {
    uint8_t r, g, b;
} Led;

typedef struct {
    uint8_t num_leds;       // total LED count
    float speed;            // LEDs per second (runtime changeable)
    uint16_t end_pause_ms;  // pause duration at each end (runtime changeable)
    uint16_t mid_pause_ms;  // pause duration at middle (runtime changeable)
    uint8_t glow_radius;    // neighbor count for glow on each side (init-time)
    Led color;              // dot color at full brightness (runtime changeable)
} LightbarConfig;

typedef enum {
    LIGHTBAR_STOPPED,
    LIGHTBAR_MOVING,
    LIGHTBAR_PAUSED_END,
    LIGHTBAR_PAUSED_MIDDLE
} LightbarPhase;

typedef struct {
    int position;           // current LED index (0 to num_leds-1)
    int direction;          // +1 (right) or -1 (left)
    LightbarPhase phase;    // current phase
    float pause_timer_ms;   // time remaining in current pause
    float move_accum_ms;    // accumulated time toward next LED step
} LightbarState;
```

- `Led` is reused in config (dot color) and output (rendered LED values).
- Config fields can be mutated between frames. The next update/render picks up
  changes immediately.
- Middle position is derived as `num_leds / 2`.

## State Machine

```
STOPPED (at middle)
  |
  (start)
  v
MOVING --(reaches middle)--> PAUSED_MIDDLE --(expires)--> MOVING
  |                                                         |
  (reaches end)                                       (reaches end)
  v                                                         v
PAUSED_END --(expires, reverse dir)--> MOVING <-------------+
```

Init sets `position = num_leds / 2`, `direction = +1`,
`phase = LIGHTBAR_STOPPED`.

Stop (from any state) resets to middle, direction = +1, clears accumulators.

Start (from STOPPED) sets phase to MOVING. Always starts moving right.

### Update Rules

**STOPPED:** Ignore dt, do nothing.

**PAUSED_END / PAUSED_MIDDLE:** Decrement `pause_timer_ms` by dt. When it
reaches zero, transition to MOVING. On exit from PAUSED_END, reverse direction
first.

**MOVING:** Accumulate dt into `move_accum_ms`. Each time the accumulator
exceeds `1000.0 / speed` (ms per LED step), consume that amount and advance
position by direction. After each step check:
- Position is 0 or num_leds - 1: transition to PAUSED_END, set pause timer.
- Position is num_leds / 2 (crossing middle, not resuming from middle pause):
  transition to PAUSED_MIDDLE, set pause timer.

Middle pause is not re-triggered immediately after resuming because the first
step moves the dot off the middle position before pause checks run.

**Zero-duration pauses:** If end_pause_ms or mid_pause_ms is 0, skip the pause
and stay in MOVING.

## API

```c
void lightbar_init(LightbarState *state, const LightbarConfig *config);
void lightbar_start(LightbarState *state);
void lightbar_stop(LightbarState *state, const LightbarConfig *config);
void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms);
void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds);
```

- `render` is pure: reads state + config, writes LED array, no side effects.
- Caller owns the Led array (no dynamic allocation — important for MCU).
- `stop` takes config to recompute middle position.

## Glow Rendering

For each LED i:
1. `distance = abs(i - position)`
2. If `distance > glow_radius`: off (0, 0, 0)
3. If `distance == 0`: full color
4. Otherwise: `brightness = 1.0 - (distance / (glow_radius + 1.0))`

Brightness scales each color channel. Integer truncation. No wrapping at edges.

`glow_radius = 0` means only the center LED is lit.

## Web Integration

**Rendering:** Horizontal row of rectangles, one per LED. Each frame: call
update with requestAnimationFrame delta, call render, set rectangle background
colors from RGB values.

**Controls:**
- Start/Stop button: calls lightbar_start / lightbar_stop
- Speed slider: writes config.speed (LEDs/second)
- End pause slider: writes config.end_pause_ms
- Middle pause slider: writes config.mid_pause_ms
- Color picker: writes config.color

**WASM bridge:** Emscripten exposes C functions to JS. JS allocates state,
config, and LED array in WASM memory. Config fields written directly from JS
when controls change.

**Defaults:**
- num_leds: 24
- speed: 15.0 LEDs/s
- end_pause_ms: 200
- mid_pause_ms: 100
- glow_radius: 2
- color: {0, 255, 255} (cyan)
