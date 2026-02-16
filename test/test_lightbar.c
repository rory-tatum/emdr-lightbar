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

void test_stop_preserves_position(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 5;
    state.direction = -1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_INT(5, state.position);
    TEST_ASSERT_EQUAL_INT(-1, state.direction);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
}

void test_stop_preserves_accumulators(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.move_accum_ms = 50.0f;
    state.pause_timer_ms = 100.0f;
    lightbar_stop(&state, &config);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, state.move_accum_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, state.pause_timer_ms);
}

void test_stop_sets_stopping_phase(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
}

void test_stop_edges_remaining_going_right_from_middle(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 12;
    state.direction = 1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(2, state.edges_remaining);
}

void test_stop_edges_remaining_going_left(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 8;
    state.direction = -1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(1, state.edges_remaining);
}

void test_stop_edges_remaining_going_right_below_middle(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 5;
    state.direction = 1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(0, state.edges_remaining);
}

void test_stop_edges_remaining_paused_right_edge(void) {
    LightbarConfig config = { .num_leds = 24, .end_pause_ms = 200 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 23;
    state.direction = 1;
    state.phase = LIGHTBAR_PAUSED_END;
    state.pause_timer_ms = 100.0f;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(1, state.edges_remaining);
}

void test_stop_edges_remaining_paused_left_edge(void) {
    LightbarConfig config = { .num_leds = 24, .end_pause_ms = 200 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 0;
    state.direction = -1;
    state.phase = LIGHTBAR_PAUSED_END;
    state.pause_timer_ms = 100.0f;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(0, state.edges_remaining);
}

void test_stop_while_already_stopped_is_noop(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPED, state.phase);
    TEST_ASSERT_EQUAL_INT(12, state.position);
}

void test_stop_while_already_stopping_is_noop(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 15;
    state.direction = 1;
    lightbar_stop(&state, &config);
    uint8_t saved_edges = state.edges_remaining;
    /* Simulate some movement, then stop again */
    state.position = 3;
    state.direction = 1;
    lightbar_stop(&state, &config);
    /* Should be no-op: edges_remaining not recalculated */
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
    TEST_ASSERT_EQUAL_UINT8(saved_edges, state.edges_remaining);
    TEST_ASSERT_EQUAL_INT(3, state.position);
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
        .end_pause_ms = 0
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
        .end_pause_ms = 0
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
        .end_pause_ms = 0
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
        .end_pause_ms = 200
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
        .end_pause_ms = 200
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
        .end_pause_ms = 200
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
        .end_pause_ms = 200
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

void test_zero_end_pause_skips_pause(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f,
        .end_pause_ms = 0
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

void test_stopping_continues_movement(void) {
    LightbarConfig config = {
        .num_leds = 24, .speed = 10.0f, .end_pause_ms = 200
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 15;
    state.direction = 1;
    lightbar_stop(&state, &config);
    /* 10 LEDs/s => 100ms per step */
    lightbar_update(&state, &config, 100.0f);
    TEST_ASSERT_EQUAL_INT(16, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
}

void test_stopping_decrements_edges_at_end(void) {
    LightbarConfig config = {
        .num_leds = 10, .speed = 100.0f, .end_pause_ms = 50
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 2;
    state.direction = -1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(1, state.edges_remaining);
    /* 2 steps to left edge */
    lightbar_update(&state, &config, 20.0f);
    TEST_ASSERT_EQUAL_INT(0, state.position);
    TEST_ASSERT_EQUAL_UINT8(0, state.edges_remaining);
}

void test_stopping_respects_end_pause(void) {
    LightbarConfig config = {
        .num_leds = 10, .speed = 100.0f, .end_pause_ms = 50
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 1;
    state.direction = -1;
    lightbar_stop(&state, &config);
    /* 1 step to left edge */
    lightbar_update(&state, &config, 10.0f);
    TEST_ASSERT_EQUAL_INT(0, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
    /* Partial pause: direction not yet reversed */
    lightbar_update(&state, &config, 25.0f);
    TEST_ASSERT_EQUAL_INT(0, state.position);
    TEST_ASSERT_EQUAL_INT(-1, state.direction);
    /* Expire remaining pause: direction reverses */
    lightbar_update(&state, &config, 25.0f);
    TEST_ASSERT_EQUAL_INT(1, state.direction);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
}

void test_stopping_finalizes_at_middle(void) {
    LightbarConfig config = {
        .num_leds = 10, .speed = 100.0f, .end_pause_ms = 0
    };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 3;
    state.direction = 1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_UINT8(0, state.edges_remaining);
    /* 2 steps: 3 -> 4 -> 5 (middle). Finalize. */
    lightbar_update(&state, &config, 20.0f);
    TEST_ASSERT_EQUAL_INT(5, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPED, state.phase);
    TEST_ASSERT_EQUAL_INT(1, state.direction);
}

void test_start_cancels_stopping(void) {
    LightbarConfig config = { .num_leds = 24 };
    LightbarState state;
    lightbar_init(&state, &config);
    lightbar_start(&state);
    state.position = 15;
    state.direction = 1;
    lightbar_stop(&state, &config);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_STOPPING, state.phase);
    lightbar_start(&state);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    TEST_ASSERT_EQUAL_INT(15, state.position);
}

void test_full_oscillation_cycle(void) {
    LightbarConfig config = {
        .num_leds = 10, .speed = 100.0f,
        .end_pause_ms = 50,
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

    /* Move 9 steps left to reach position 0 (passes through middle) */
    lightbar_update(&state, &config, 90.0f);
    TEST_ASSERT_EQUAL_INT(0, state.position);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_PAUSED_END, state.phase);

    /* Expire end pause, direction reverses to +1 */
    lightbar_update(&state, &config, 50.0f);
    TEST_ASSERT_EQUAL_INT(LIGHTBAR_MOVING, state.phase);
    TEST_ASSERT_EQUAL_INT(1, state.direction);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_position_to_middle);
    RUN_TEST(test_init_sets_direction_right);
    RUN_TEST(test_init_sets_phase_stopped);
    RUN_TEST(test_init_clears_timers);
    RUN_TEST(test_init_odd_led_count);
    RUN_TEST(test_start_sets_phase_moving);
    RUN_TEST(test_stop_preserves_position);
    RUN_TEST(test_stop_preserves_accumulators);
    RUN_TEST(test_stop_sets_stopping_phase);
    RUN_TEST(test_stop_edges_remaining_going_right_from_middle);
    RUN_TEST(test_stop_edges_remaining_going_left);
    RUN_TEST(test_stop_edges_remaining_going_right_below_middle);
    RUN_TEST(test_stop_edges_remaining_paused_right_edge);
    RUN_TEST(test_stop_edges_remaining_paused_left_edge);
    RUN_TEST(test_stop_while_already_stopped_is_noop);
    RUN_TEST(test_stop_while_already_stopping_is_noop);
    RUN_TEST(test_update_stopped_does_nothing);
    RUN_TEST(test_update_advances_position);
    RUN_TEST(test_update_accumulates_partial_steps);
    RUN_TEST(test_update_multiple_steps_in_one_frame);
    RUN_TEST(test_update_triggers_end_pause_at_right);
    RUN_TEST(test_update_triggers_end_pause_at_left);
    RUN_TEST(test_end_pause_expires_and_reverses);
    RUN_TEST(test_end_pause_partial_timer);
    RUN_TEST(test_zero_end_pause_skips_pause);
    RUN_TEST(test_render_single_led_no_glow);
    RUN_TEST(test_render_glow_radius_2);
    RUN_TEST(test_render_glow_at_left_edge);
    RUN_TEST(test_render_glow_at_right_edge);
    RUN_TEST(test_render_colored_dot);
    RUN_TEST(test_stopping_continues_movement);
    RUN_TEST(test_stopping_decrements_edges_at_end);
    RUN_TEST(test_stopping_respects_end_pause);
    RUN_TEST(test_stopping_finalizes_at_middle);
    RUN_TEST(test_start_cancels_stopping);
    RUN_TEST(test_full_oscillation_cycle);
    return UNITY_END();
}
