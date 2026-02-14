# Oscillation Logic Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement the oscillation core logic (data structures, state machine, rendering) and web visualization with controls.

**Architecture:** Platform-agnostic C99 core in `src/lightbar.c` with types in `include/lightbar.h`. A WASM bridge in `web/wasm_bridge.c` exposes the core to JavaScript. The web page renders LEDs and provides controls. See `docs/plans/2026-02-14-oscillation-design.md` for full design.

**Tech Stack:** C99, Unity test framework, Emscripten (WASM), vanilla HTML/CSS/JS.

---

### Task 1: Data structures and lightbar_init

**Files:**
- Create: `include/lightbar.h`
- Create: `src/lightbar.c`
- Create: `test/test_lightbar.c`
- Modify: `Makefile`

**Step 1: Create the header with types and function declarations**

Create `include/lightbar.h`:

```c
#ifndef LIGHTBAR_H
#define LIGHTBAR_H

#include <stdint.h>

typedef struct {
    uint8_t r, g, b;
} Led;

typedef struct {
    uint8_t num_leds;
    float speed;
    uint16_t end_pause_ms;
    uint16_t mid_pause_ms;
    uint8_t glow_radius;
    Led color;
} LightbarConfig;

typedef enum {
    LIGHTBAR_STOPPED,
    LIGHTBAR_MOVING,
    LIGHTBAR_PAUSED_END,
    LIGHTBAR_PAUSED_MIDDLE
} LightbarPhase;

typedef struct {
    int position;
    int direction;
    LightbarPhase phase;
    float pause_timer_ms;
    float move_accum_ms;
} LightbarState;

void lightbar_init(LightbarState *state, const LightbarConfig *config);
void lightbar_start(LightbarState *state);
void lightbar_stop(LightbarState *state, const LightbarConfig *config);
void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms);
void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds);

#endif
```

**Step 2: Write failing tests for lightbar_init**

Create `test/test_lightbar.c`:

```c
#include "unity.h"
#include "lightbar.h"

void setUp(void) {}
void tearDown(void) {}

void test_init_sets_position_to_middle(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    TEST_ASSERT_EQUAL_INT(12, state.position);
}

void test_init_sets_direction_right(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    TEST_ASSERT_EQUAL_INT(1, state.direction);
}

void test_init_sets_phase_stopped(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPED, state.phase);
}

void test_init_clears_timers(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.pause_timer_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.move_accum_ms);
}

void test_init_odd_led_count(void) {
    LightbarConfig config = { .num_leds = 21 };
    LightbarState state;
    lightbar_init(&state, &config);
    TEST_ASSERT_EQUAL_INT(10, state.position);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_position_to_middle);
    RUN_TEST(test_init_sets_direction_right);
    RUN_TEST(test_init_sets_phase_stopped);
    RUN_TEST(test_init_clears_timers);
    RUN_TEST(test_init_odd_led_count);
    return UNITY_END();
}
```

**Step 3: Add test_lightbar target to Makefile**

Add these lines to the Makefile. The `test` target should run both test executables:

```makefile
LIGHTBAR_SRC = src/lightbar.c
LIGHTBAR_TEST_SRC = test/test_lightbar.c

test: build/test_main build/test_lightbar
	./build/test_main
	./build/test_lightbar

build/test_lightbar: $(LIGHTBAR_TEST_SRC) $(LIGHTBAR_SRC) include/lightbar.h | build
	$(CC) $(CFLAGS) $(UNITY_INC) -DUNITY_INCLUDE_DOUBLE -o $@ \
		$(LIGHTBAR_TEST_SRC) $(LIGHTBAR_SRC) $(UNITY_SRC)
```

**Step 4: Run tests to verify they fail**

Run: `make test`
Expected: Linker error — `lightbar_init` undefined (no implementation yet).

**Step 5: Create src/lightbar.c with lightbar_init**

Create `src/lightbar.c`:

```c
#include "lightbar.h"

void lightbar_init(LightbarState *state, const LightbarConfig *config) {
    state->position = config->num_leds / 2;
    state->direction = 1;
    state->phase = LIGHTBAR_STOPPED;
    state->pause_timer_ms = 0.0f;
    state->move_accum_ms = 0.0f;
}
```

Add stub implementations for the remaining functions so the header is satisfied
and future tests can link. Each stub has an empty body:

```c
void lightbar_start(LightbarState *state) {
    (void)state;
}

void lightbar_stop(LightbarState *state, const LightbarConfig *config) {
    (void)state;
    (void)config;
}

void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms) {
    (void)state;
    (void)config;
    (void)dt_ms;
}

void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds) {
    (void)state;
    (void)config;
    (void)leds;
}
```

**Step 6: Run tests to verify they pass**

Run: `make test`
Expected: All 5 lightbar tests PASS (plus existing test_main tests).

**Step 7: Commit**

```bash
git add include/lightbar.h src/lightbar.c test/test_lightbar.c Makefile
git commit -m "Add lightbar data structures and init function"
```

---

### Task 2: Implement start and stop

**Files:**
- Modify: `src/lightbar.c`
- Modify: `test/test_lightbar.c`

**Step 1: Write failing tests for start and stop**

Add to `test/test_lightbar.c` (before `main`):

```c
void test_start_sets_phase_moving(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
}

void test_stop_resets_to_middle(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 5;
    state.direction = -1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_INT(12, state.position);
    TEST_ASSERT_EQUAL_INT(1, state.direction);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPED, state.phase);
}

void test_stop_clears_accumulators(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.move_accum_ms = 50.0f;
    state.pause_timer_ms = 100.0f;
    lightbar_stop(&state, &config);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.pause_timer_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.move_accum_ms);
}
```

Register all three in `main`:

```c
RUN_TEST(test_start_sets_phase_moving);
RUN_TEST(test_stop_resets_to_middle);
RUN_TEST(test_stop_clears_accumulators);
```

**Step 2: Run tests to verify they fail**

Run: `make test`
Expected: `test_start_sets_phase_moving` FAILS (phase is still STOPPED).

**Step 3: Implement start and stop**

Replace the stubs in `src/lightbar.c`:

```c
void lightbar_start(LightbarState *state) {
    state->phase = LIGHTBAR_MOVING;
}

void lightbar_stop(LightbarState *state, const LightbarConfig *config) {
    state->position = config->num_leds / 2;
    state->direction = 1;
    state->phase = LIGHTBAR_STOPPED;
    state->pause_timer_ms = 0.0f;
    state->move_accum_ms = 0.0f;
}
```

**Step 4: Run tests to verify they pass**

Run: `make test`
Expected: All 8 tests PASS.

**Step 5: Commit**

```bash
git add src/lightbar.c test/test_lightbar.c
git commit -m "Implement lightbar start and stop"
```

---

### Task 3: Implement update — basic movement

**Files:**
- Modify: `src/lightbar.c`
- Modify: `test/test_lightbar.c`

**Step 1: Write failing tests for movement**

Add to `test/test_lightbar.c`:

```c
void test_update_stopped_does_nothing(void) {
    LightbarConfig config = { .num_leds = 24, .speed = 10.0f };
    LightbarState state;
    lightbar_init(&state, &config);
    int old_pos = state.position;
    lightbar_update(&state, &config, 1000.0f);
    TEST_ASSERT_EQUAL_INT(old_pos, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPED, state.phase);
}

void test_update_advances_position(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 0, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    int start_pos = state.position;
    /* 10 LEDs/s => 100ms per step. Feed exactly 100ms. */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(start_pos + 1, state.position);
}

void test_update_accumulates_partial_steps(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 0, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    int start_pos = state.position;
    lightbar_update(&state, &config, 50.0f);
    TEST_ASSERT_EQUAL_INT(start_pos, state.position);
    lightbar_update(&state, &config, 50.0f);
    TEST_ASSERT_EQUAL_INT(start_pos + 1, state.position);
}

void test_update_multiple_steps_in_one_frame(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 0, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    int start_pos = state.position;
    /* 300ms = 3 steps at 10 LEDs/s */
    lightbar_update(&state, &config, 300.0f);
    TEST_ASSERT_EQUAL_INT(start_pos + 3, state.position);
}
```

Register in `main`:

```c
RUN_TEST(test_update_stopped_does_nothing);
RUN_TEST(test_update_advances_position);
RUN_TEST(test_update_accumulates_partial_steps);
RUN_TEST(test_update_multiple_steps_in_one_frame);
```

**Step 2: Run tests to verify they fail**

Run: `make test`
Expected: Movement tests FAIL (update is a stub).

**Step 3: Implement basic movement in lightbar_update**

Replace the `lightbar_update` stub in `src/lightbar.c`:

```c
void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms) {
    if (state->phase == LIGHTBAR_STOPPED) {
        return;
    }

    if (state->phase == LIGHTBAR_MOVING) {
        float ms_per_step = 1000.0f / config->speed;
        state->move_accum_ms += dt_ms;
        while (state->move_accum_ms >= ms_per_step) {
            state->move_accum_ms -= ms_per_step;
            state->position += state->direction;

            /* End check */
            if (state->position <= 0 || state->position >= config->num_leds - 1) {
                if (state->position <= 0) state->position = 0;
                if (state->position >= config->num_leds - 1) state->position = config->num_leds - 1;
                if (config->end_pause_ms > 0) {
                    state->phase = LIGHTBAR_PAUSED_END;
                    state->pause_timer_ms = (float)config->end_pause_ms;
                    state->move_accum_ms = 0.0f;
                    return;
                }
                state->direction = -state->direction;
            }

            /* Middle check */
            if (state->position == config->num_leds / 2) {
                if (config->mid_pause_ms > 0) {
                    state->phase = LIGHTBAR_PAUSED_MIDDLE;
                    state->pause_timer_ms = (float)config->mid_pause_ms;
                    state->move_accum_ms = 0.0f;
                    return;
                }
            }
        }
    }
}
```

**Step 4: Run tests to verify they pass**

Run: `make test`
Expected: All 12 tests PASS.

**Step 5: Commit**

```bash
git add src/lightbar.c test/test_lightbar.c
git commit -m "Implement lightbar update with basic movement"
```

---

### Task 4: Implement update — end pause

**Files:**
- Modify: `src/lightbar.c`
- Modify: `test/test_lightbar.c`

**Step 1: Write failing tests for end pause**

Add to `test/test_lightbar.c`:

```c
void test_update_triggers_end_pause_at_right(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 22;
    /* One step moves to 23 (last LED) */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(23, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_END, state.phase);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, state.pause_timer_ms);
}

void test_update_triggers_end_pause_at_left(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 1;
    state.direction = -1;
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(0, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_END, state.phase);
}

void test_end_pause_expires_and_reverses(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 23;
    state.direction = 1;
    state.phase = LIGHTBAR_PAUSED_END;
    state.pause_timer_ms = 200.0f;
    /* Feed 200ms to expire the pause */
    lightbar_update(&state, &config, 200.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    TEST_ASSERT_EQUAL_INT(-1, state.direction);
}

void test_end_pause_partial_timer(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 23;
    state.phase = LIGHTBAR_PAUSED_END;
    state.pause_timer_ms = 200.0f;
    /* Feed 100ms — still paused */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_END, state.phase);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, state.pause_timer_ms);
}
```

Register in `main`:

```c
RUN_TEST(test_update_triggers_end_pause_at_right);
RUN_TEST(test_update_triggers_end_pause_at_left);
RUN_TEST(test_end_pause_expires_and_reverses);
RUN_TEST(test_end_pause_partial_timer);
```

**Step 2: Run tests to verify they fail**

Run: `make test`
Expected: Pause timer/expiry tests FAIL (PAUSED_END/PAUSED_MIDDLE not handled
in update yet).

**Step 3: Add pause handling to lightbar_update**

Add this block to the top of `lightbar_update`, after the STOPPED check:

```c
    if (state->phase == LIGHTBAR_PAUSED_END || state->phase == LIGHTBAR_PAUSED_MIDDLE) {
        state->pause_timer_ms -= dt_ms;
        if (state->pause_timer_ms <= 0.0f) {
            if (state->phase == LIGHTBAR_PAUSED_END) {
                state->direction = -state->direction;
            }
            state->phase = LIGHTBAR_MOVING;
            state->pause_timer_ms = 0.0f;
            state->move_accum_ms = 0.0f;
        }
        return;
    }
```

**Step 4: Run tests to verify they pass**

Run: `make test`
Expected: All 16 tests PASS.

**Step 5: Commit**

```bash
git add src/lightbar.c test/test_lightbar.c
git commit -m "Implement end pause in lightbar update"
```

---

### Task 5: Implement update — middle pause

**Files:**
- Modify: `test/test_lightbar.c`

**Step 1: Write failing tests for middle pause**

Add to `test/test_lightbar.c`:

```c
void test_update_triggers_middle_pause(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 100
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    /* Position one step before middle, moving left */
    state.position = 13;
    state.direction = -1;
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(12, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_MIDDLE, state.phase);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, state.pause_timer_ms);
}

void test_update_no_middle_pause_on_start(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 100
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    /* Start at middle (12), first step should move to 13 without pausing */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(13, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
}

void test_middle_pause_expires_and_resumes(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 100
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 12;
    state.direction = -1;
    state.phase = LIGHTBAR_PAUSED_MIDDLE;
    state.pause_timer_ms = 100.0f;
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    /* Direction unchanged after middle pause */
    TEST_ASSERT_EQUAL_INT(-1, state.direction);
}

void test_middle_pause_no_retrigger_after_resume(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 100
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    /* Simulate resuming from middle pause: at middle, moving left */
    state.position = 12;
    state.direction = -1;
    state.phase = LIGHTBAR_PAUSED_MIDDLE;
    state.pause_timer_ms = 100.0f;
    /* Expire the pause */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    /* Next step should move to 11, not re-trigger middle pause */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(11, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
}
```

Register in `main`:

```c
RUN_TEST(test_update_triggers_middle_pause);
RUN_TEST(test_update_no_middle_pause_on_start);
RUN_TEST(test_middle_pause_expires_and_resumes);
RUN_TEST(test_middle_pause_no_retrigger_after_resume);
```

**Step 2: Run tests to verify they pass (or fail)**

Run: `make test`
Expected: These should already PASS — the middle pause logic was implemented in
Task 3 (movement) and Task 4 (pause expiry). If any fail, fix the logic.

**Step 3: Commit**

```bash
git add test/test_lightbar.c
git commit -m "Add middle pause tests"
```

---

### Task 6: Implement update — zero-duration pauses

**Files:**
- Modify: `test/test_lightbar.c`

**Step 1: Write failing tests for zero-duration pauses**

Add to `test/test_lightbar.c`:

```c
void test_zero_end_pause_skips_pause(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 0, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 22;
    state.direction = 1;
    /* Step to 23 (end), should reverse without pausing */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(23, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    TEST_ASSERT_EQUAL_INT(-1, state.direction);
}

void test_zero_mid_pause_skips_pause(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 200, .mid_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 13;
    state.direction = -1;
    /* Step to 12 (middle), should continue moving */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(12, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
}
```

Register in `main`:

```c
RUN_TEST(test_zero_end_pause_skips_pause);
RUN_TEST(test_zero_mid_pause_skips_pause);
```

**Step 2: Run tests to verify they pass (or fail)**

Run: `make test`
Expected: These should already PASS — the `config->end_pause_ms > 0` and
`config->mid_pause_ms > 0` checks in the movement logic handle this. If any
fail, fix the logic.

**Step 3: Commit**

```bash
git add test/test_lightbar.c
git commit -m "Add zero-duration pause tests"
```

---

### Task 7: Implement render — basic dot and glow

**Files:**
- Modify: `src/lightbar.c`
- Modify: `test/test_lightbar.c`

**Step 1: Write failing tests for render**

Add to `test/test_lightbar.c`:

```c
void test_render_single_led_no_glow(void) {
    LightbarConfig config = {
        .num_leds = 10, .glow_radius = 0,
        .color = {255, 255, 255}
    };
    LightbarState state;
    lightbar_init(&state, &config);
    state.position = 5;
    Led leds[10];
    lightbar_render(&state, &config, leds);
    TEST_ASSERT_EQUAL_UINT8(255, leds[5].r);
    TEST_ASSERT_EQUAL_UINT8(255, leds[5].g);
    TEST_ASSERT_EQUAL_UINT8(255, leds[5].b);
    TEST_ASSERT_EQUAL_UINT8(0, leds[4].r);
    TEST_ASSERT_EQUAL_UINT8(0, leds[6].r);
    TEST_ASSERT_EQUAL_UINT8(0, leds[0].r);
    TEST_ASSERT_EQUAL_UINT8(0, leds[9].r);
}

void test_render_glow_radius_2(void) {
    LightbarConfig config = {
        .num_leds = 10, .glow_radius = 2,
        .color = {255, 255, 255}
    };
    LightbarState state;
    lightbar_init(&state, &config);
    state.position = 5;
    Led leds[10];
    lightbar_render(&state, &config, leds);
    /* distance 0: 255 */
    TEST_ASSERT_EQUAL_UINT8(255, leds[5].r);
    /* distance 1: 255 * (1 - 1/3) = 170 */
    TEST_ASSERT_EQUAL_UINT8(170, leds[4].r);
    TEST_ASSERT_EQUAL_UINT8(170, leds[6].r);
    /* distance 2: 255 * (1 - 2/3) = 85 */
    TEST_ASSERT_EQUAL_UINT8(85, leds[3].r);
    TEST_ASSERT_EQUAL_UINT8(85, leds[7].r);
    /* distance 3: off */
    TEST_ASSERT_EQUAL_UINT8(0, leds[2].r);
    TEST_ASSERT_EQUAL_UINT8(0, leds[8].r);
}

void test_render_glow_at_left_edge(void) {
    LightbarConfig config = {
        .num_leds = 10, .glow_radius = 2,
        .color = {255, 255, 255}
    };
    LightbarState state;
    lightbar_init(&state, &config);
    state.position = 0;
    Led leds[10];
    lightbar_render(&state, &config, leds);
    TEST_ASSERT_EQUAL_UINT8(255, leds[0].r);
    TEST_ASSERT_EQUAL_UINT8(170, leds[1].r);
    TEST_ASSERT_EQUAL_UINT8(85, leds[2].r);
    TEST_ASSERT_EQUAL_UINT8(0, leds[3].r);
}

void test_render_glow_at_right_edge(void) {
    LightbarConfig config = {
        .num_leds = 10, .glow_radius = 2,
        .color = {255, 255, 255}
    };
    LightbarState state;
    lightbar_init(&state, &config);
    state.position = 9;
    Led leds[10];
    lightbar_render(&state, &config, leds);
    TEST_ASSERT_EQUAL_UINT8(255, leds[9].r);
    TEST_ASSERT_EQUAL_UINT8(170, leds[8].r);
    TEST_ASSERT_EQUAL_UINT8(85, leds[7].r);
    TEST_ASSERT_EQUAL_UINT8(0, leds[6].r);
}

void test_render_colored_dot(void) {
    LightbarConfig config = {
        .num_leds = 10, .glow_radius = 1,
        .color = {0, 255, 100}
    };
    LightbarState state;
    lightbar_init(&state, &config);
    state.position = 5;
    Led leds[10];
    lightbar_render(&state, &config, leds);
    TEST_ASSERT_EQUAL_UINT8(0, leds[5].r);
    TEST_ASSERT_EQUAL_UINT8(255, leds[5].g);
    TEST_ASSERT_EQUAL_UINT8(100, leds[5].b);
    /* distance 1: brightness = 1 - 1/2 = 0.5 */
    TEST_ASSERT_EQUAL_UINT8(0, leds[4].r);
    TEST_ASSERT_EQUAL_UINT8(127, leds[4].g);
    TEST_ASSERT_EQUAL_UINT8(50, leds[4].b);
}
```

Register in `main`:

```c
RUN_TEST(test_render_single_led_no_glow);
RUN_TEST(test_render_glow_radius_2);
RUN_TEST(test_render_glow_at_left_edge);
RUN_TEST(test_render_glow_at_right_edge);
RUN_TEST(test_render_colored_dot);
```

**Step 2: Run tests to verify they fail**

Run: `make test`
Expected: Render tests FAIL (render is a stub).

**Step 3: Implement lightbar_render**

Replace the `lightbar_render` stub in `src/lightbar.c`. Add `#include <stdlib.h>`
at the top for `abs()`:

```c
void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds) {
    for (int i = 0; i < config->num_leds; i++) {
        int distance = abs(i - state->position);
        if (distance == 0) {
            leds[i] = config->color;
        } else if (config->glow_radius > 0 && distance <= config->glow_radius) {
            float brightness = 1.0f - ((float)distance / (config->glow_radius + 1.0f));
            leds[i].r = (uint8_t)(config->color.r * brightness);
            leds[i].g = (uint8_t)(config->color.g * brightness);
            leds[i].b = (uint8_t)(config->color.b * brightness);
        } else {
            leds[i].r = 0;
            leds[i].g = 0;
            leds[i].b = 0;
        }
    }
}
```

**Step 4: Run tests to verify they pass**

Run: `make test`
Expected: All tests PASS.

**Step 5: Commit**

```bash
git add src/lightbar.c test/test_lightbar.c
git commit -m "Implement lightbar render with glow"
```

---

### Task 8: Full oscillation integration test

**Files:**
- Modify: `test/test_lightbar.c`

**Step 1: Write integration test for a full cycle**

This test verifies the dot makes a complete round trip: middle → right end →
pause → reverse → middle → pause → left end → pause → reverse → middle.

Add to `test/test_lightbar.c`:

```c
void test_full_oscillation_cycle(void) {
    LightbarConfig config = {
        .num_leds = 10, .speed = 100.0f,
        .end_pause_ms = 50, .mid_pause_ms = 30,
        .glow_radius = 0, .color = {255, 255, 255}
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    /* Start at middle (5), moving right */
    TEST_ASSERT_EQUAL_INT(5, state.position);

    /* 100 LEDs/s => 10ms per step. Move 4 steps to reach position 9 */
    lightbar_update(&state, &config, 40.0f);
    TEST_ASSERT_EQUAL_INT(9, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_END, state.phase);

    /* Expire end pause (50ms) */
    lightbar_update(&state, &config, 50.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    TEST_ASSERT_EQUAL_INT(-1, state.direction);

    /* Move 4 steps left to reach middle (5) */
    lightbar_update(&state, &config, 40.0f);
    TEST_ASSERT_EQUAL_INT(5, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_MIDDLE, state.phase);

    /* Expire middle pause (30ms) */
    lightbar_update(&state, &config, 30.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);

    /* Move 5 steps left to reach position 0 */
    lightbar_update(&state, &config, 50.0f);
    TEST_ASSERT_EQUAL_INT(0, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_END, state.phase);

    /* Expire end pause, direction reverses to +1 */
    lightbar_update(&state, &config, 50.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    TEST_ASSERT_EQUAL_INT(1, state.direction);
}
```

Register in `main`:

```c
RUN_TEST(test_full_oscillation_cycle);
```

**Step 2: Run tests**

Run: `make test`
Expected: All tests PASS. If this test reveals bugs in the state machine, fix
them and re-run.

**Step 3: Commit**

```bash
git add test/test_lightbar.c
git commit -m "Add full oscillation cycle integration test"
```

---

### Task 9: WASM bridge and web frontend

**Files:**
- Create: `web/wasm_bridge.c`
- Modify: `web/index.html`
- Modify: `Makefile`

**Step 1: Create the WASM bridge**

Create `web/wasm_bridge.c`:

```c
#include "lightbar.h"
#include <emscripten.h>

#define MAX_LEDS 64

static LightbarConfig config;
static LightbarState state;
static Led leds[MAX_LEDS];

EMSCRIPTEN_KEEPALIVE
void wasm_init(int num_leds, float speed, int end_pause, int mid_pause,
               int glow_radius, int r, int g, int b) {
    config.num_leds = (uint8_t)num_leds;
    config.speed = speed;
    config.end_pause_ms = (uint16_t)end_pause;
    config.mid_pause_ms = (uint16_t)mid_pause;
    config.glow_radius = (uint8_t)glow_radius;
    config.color.r = (uint8_t)r;
    config.color.g = (uint8_t)g;
    config.color.b = (uint8_t)b;
    lightbar_init(&state, &config);
}

EMSCRIPTEN_KEEPALIVE
void wasm_start(void) {
    lightbar_start(&state);
}

EMSCRIPTEN_KEEPALIVE
void wasm_stop(void) {
    lightbar_stop(&state, &config);
}

EMSCRIPTEN_KEEPALIVE
void wasm_update(float dt_ms) {
    lightbar_update(&state, &config, dt_ms);
}

EMSCRIPTEN_KEEPALIVE
void wasm_render(void) {
    lightbar_render(&state, &config, leds);
}

EMSCRIPTEN_KEEPALIVE
uint8_t *wasm_get_leds_ptr(void) {
    return (uint8_t *)leds;
}

EMSCRIPTEN_KEEPALIVE
int wasm_get_num_leds(void) {
    return config.num_leds;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_speed(float speed) {
    config.speed = speed;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_end_pause(int ms) {
    config.end_pause_ms = (uint16_t)ms;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_mid_pause(int ms) {
    config.mid_pause_ms = (uint16_t)ms;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_color(int r, int g, int b) {
    config.color.r = (uint8_t)r;
    config.color.g = (uint8_t)g;
    config.color.b = (uint8_t)b;
}

int main(void) {
    return 0;
}
```

**Step 2: Update Makefile wasm target**

Replace the existing wasm target in the Makefile:

```makefile
WASM_BRIDGE = web/wasm_bridge.c

wasm: web/main.js
	@echo "WASM build complete: web/main.js web/main.wasm"

web/main.js: $(WASM_BRIDGE) $(LIGHTBAR_SRC) include/lightbar.h
	$(EMCC) $(CFLAGS) -s NO_EXIT_RUNTIME=1 -s EXPORTED_RUNTIME_METHODS='["ccall"]' \
		-o $@ $(WASM_BRIDGE) $(LIGHTBAR_SRC)
```

**Step 3: Rewrite web/index.html**

Replace the contents of `web/index.html` with the full LED visualization and
controls. The page creates LED rectangles dynamically, runs a
requestAnimationFrame loop calling the WASM functions, and wires slider/button
controls to the config setters.

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>EMDR Lightbar</title>
    <style>
        body {
            font-family: monospace;
            background: #1a1a2e;
            color: #e0e0e0;
            display: flex;
            flex-direction: column;
            align-items: center;
            margin: 0;
            padding: 2em;
        }
        h1 { color: #0ff; margin-bottom: 1em; }
        #strip {
            display: flex;
            gap: 2px;
            padding: 1em;
            background: #111;
            border-radius: 8px;
            margin-bottom: 2em;
        }
        .led {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #000;
        }
        #controls {
            display: flex;
            flex-direction: column;
            gap: 1em;
            width: 400px;
        }
        .control-row {
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        .control-row label { width: 120px; }
        .control-row input[type="range"] { flex: 1; margin: 0 1em; }
        .control-row .value { width: 60px; text-align: right; }
        #toggle {
            padding: 0.5em 2em;
            font-size: 1.2em;
            font-family: monospace;
            background: #0f3460;
            color: #0ff;
            border: 1px solid #0ff;
            border-radius: 4px;
            cursor: pointer;
        }
        #toggle:hover { background: #16213e; }
    </style>
</head>
<body>
    <h1>EMDR Lightbar</h1>
    <div id="strip"></div>
    <div id="controls">
        <div style="text-align: center;">
            <button id="toggle">Start</button>
        </div>
        <div class="control-row">
            <label>Speed</label>
            <input type="range" id="speed" min="1" max="60" value="15">
            <span class="value"><span id="speed-val">15</span> /s</span>
        </div>
        <div class="control-row">
            <label>End pause</label>
            <input type="range" id="end-pause" min="0" max="1000" step="10" value="200">
            <span class="value"><span id="end-pause-val">200</span> ms</span>
        </div>
        <div class="control-row">
            <label>Mid pause</label>
            <input type="range" id="mid-pause" min="0" max="1000" step="10" value="100">
            <span class="value"><span id="mid-pause-val">100</span> ms</span>
        </div>
        <div class="control-row">
            <label>Color</label>
            <input type="color" id="color" value="#00ffff">
        </div>
    </div>
    <script>
        var Module = {
            onRuntimeInitialized: function() {
                var NUM_LEDS = 24;
                var running = false;
                var lastTime = 0;

                Module._wasm_init(NUM_LEDS, 15.0, 200, 100, 2, 0, 255, 255);

                var strip = document.getElementById('strip');
                var ledEls = [];
                for (var i = 0; i < NUM_LEDS; i++) {
                    var el = document.createElement('div');
                    el.className = 'led';
                    strip.appendChild(el);
                    ledEls.push(el);
                }

                /* Render initial stopped state */
                Module._wasm_render();
                var ptr = Module._wasm_get_leds_ptr();
                for (var i = 0; i < NUM_LEDS; i++) {
                    var r = Module.HEAPU8[ptr + i * 3];
                    var g = Module.HEAPU8[ptr + i * 3 + 1];
                    var b = Module.HEAPU8[ptr + i * 3 + 2];
                    ledEls[i].style.backgroundColor = 'rgb(' + r + ',' + g + ',' + b + ')';
                }

                var toggleBtn = document.getElementById('toggle');
                toggleBtn.addEventListener('click', function() {
                    running = !running;
                    if (running) {
                        Module._wasm_start();
                        toggleBtn.textContent = 'Stop';
                        lastTime = 0;
                    } else {
                        Module._wasm_stop();
                        toggleBtn.textContent = 'Start';
                        /* Re-render stopped state */
                        Module._wasm_render();
                        ptr = Module._wasm_get_leds_ptr();
                        for (var j = 0; j < NUM_LEDS; j++) {
                            var r = Module.HEAPU8[ptr + j * 3];
                            var g = Module.HEAPU8[ptr + j * 3 + 1];
                            var b = Module.HEAPU8[ptr + j * 3 + 2];
                            ledEls[j].style.backgroundColor = 'rgb(' + r + ',' + g + ',' + b + ')';
                        }
                    }
                });

                document.getElementById('speed').addEventListener('input', function(e) {
                    var val = parseFloat(e.target.value);
                    document.getElementById('speed-val').textContent = val;
                    Module._wasm_set_speed(val);
                });

                document.getElementById('end-pause').addEventListener('input', function(e) {
                    var val = parseInt(e.target.value);
                    document.getElementById('end-pause-val').textContent = val;
                    Module._wasm_set_end_pause(val);
                });

                document.getElementById('mid-pause').addEventListener('input', function(e) {
                    var val = parseInt(e.target.value);
                    document.getElementById('mid-pause-val').textContent = val;
                    Module._wasm_set_mid_pause(val);
                });

                document.getElementById('color').addEventListener('input', function(e) {
                    var hex = e.target.value;
                    var r = parseInt(hex.substr(1, 2), 16);
                    var g = parseInt(hex.substr(3, 2), 16);
                    var b = parseInt(hex.substr(5, 2), 16);
                    Module._wasm_set_color(r, g, b);
                });

                function frame(time) {
                    if (running) {
                        var dt = lastTime > 0 ? time - lastTime : 0;
                        Module._wasm_update(dt);
                        Module._wasm_render();
                        ptr = Module._wasm_get_leds_ptr();
                        for (var i = 0; i < NUM_LEDS; i++) {
                            var r = Module.HEAPU8[ptr + i * 3];
                            var g = Module.HEAPU8[ptr + i * 3 + 1];
                            var b = Module.HEAPU8[ptr + i * 3 + 2];
                            ledEls[i].style.backgroundColor = 'rgb(' + r + ',' + g + ',' + b + ')';
                        }
                    }
                    lastTime = time;
                    requestAnimationFrame(frame);
                }
                requestAnimationFrame(frame);
            }
        };
    </script>
    <script src="main.js"></script>
</body>
</html>
```

**Step 4: Build WASM and verify**

Run: `make clean && make wasm`
Expected: Compiles without errors. Output files `web/main.js` and
`web/main.wasm` are created.

If Emscripten is not installed locally, verify the build compiles natively
first: `make test` (all tests pass), then rely on CI for the WASM build.

**Step 5: Commit**

```bash
git add web/wasm_bridge.c web/index.html Makefile
git commit -m "Add WASM bridge and web frontend with controls"
```

---

### Task 10: Clean up scaffold code

**Files:**
- Modify: `Makefile` (optional)

**Step 1: Verify all tests pass**

Run: `make test`
Expected: All lightbar tests and existing hello_message test pass.

**Step 2: Decide whether to remove hello_message scaffold**

The `src/main.c`, `include/main.h`, and `test/test_main.c` files contain the
original "Hello, World!" scaffold. They are no longer needed for the lightbar
feature but the native build target still uses them. Leave them in place for now
— they don't interfere and the native target may be repurposed for the MCU
driver later.

**Step 3: Final commit (if any cleanup was done)**

```bash
git add -A
git commit -m "Clean up scaffold code"
```

Skip this commit if no changes were made.
