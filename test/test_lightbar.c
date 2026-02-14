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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_position_to_middle);
    RUN_TEST(test_init_sets_direction_right);
    RUN_TEST(test_init_sets_phase_stopped);
    RUN_TEST(test_init_clears_timers);
    RUN_TEST(test_init_odd_led_count);
    RUN_TEST(test_start_sets_phase_moving);
    RUN_TEST(test_stop_resets_to_middle);
    RUN_TEST(test_stop_clears_accumulators);
    RUN_TEST(test_update_stopped_does_nothing);
    RUN_TEST(test_update_advances_position);
    RUN_TEST(test_update_accumulates_partial_steps);
    RUN_TEST(test_update_multiple_steps_in_one_frame);
    RUN_TEST(test_update_triggers_end_pause_at_right);
    RUN_TEST(test_update_triggers_end_pause_at_left);
    RUN_TEST(test_end_pause_expires_and_reverses);
    RUN_TEST(test_end_pause_partial_timer);
    RUN_TEST(test_update_triggers_middle_pause);
    RUN_TEST(test_update_no_middle_pause_on_start);
    RUN_TEST(test_middle_pause_expires_and_resumes);
    RUN_TEST(test_middle_pause_no_retrigger_after_resume);
    RUN_TEST(test_zero_end_pause_skips_pause);
    RUN_TEST(test_zero_mid_pause_skips_pause);
    return UNITY_END();
}
