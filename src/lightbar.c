#include "lightbar.h"

void lightbar_init(LightbarState *state, const LightbarConfig *config) {
    state->position = config->num_leds / 2;
    state->direction = 1;
    state->phase = LIGHTBAR_STOPPED;
    state->pause_timer_ms = 0.0f;
    state->move_accum_ms = 0.0f;
}

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

void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms) {
    if (state->phase == LIGHTBAR_STOPPED) {
        return;
    }

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

void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds) {
    (void)state;
    (void)config;
    (void)leds;
}
