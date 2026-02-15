#include "lightbar.h"
#include <stdlib.h>

void lightbar_init(LightbarState *state, const LightbarConfig *config) {
    state->position = config->num_leds / 2;
    state->direction = 1;
    state->phase = LIGHTBAR_STOPPED;
    state->pause_timer_ms = 0.0f;
    state->move_accum_ms = 0.0f;
    state->edges_remaining = 0;
}

void lightbar_start(LightbarState *state) {
    state->phase = LIGHTBAR_MOVING;
}

void lightbar_stop(LightbarState *state, const LightbarConfig *config) {
    if (state->phase == LIGHTBAR_STOPPED || state->phase == LIGHTBAR_STOPPING) {
        return;
    }

    int middle = config->num_leds / 2;

    if (state->phase == LIGHTBAR_PAUSED_END) {
        state->edges_remaining = (state->direction == 1) ? 1 : 0;
    } else {
        /* MOVING */
        if (state->direction == 1) {
            state->edges_remaining = (state->position >= middle) ? 2 : 0;
        } else {
            state->edges_remaining = 1;
        }
    }

    state->phase = LIGHTBAR_STOPPING;
}

void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms) {
    if (state->phase == LIGHTBAR_STOPPED) {
        return;
    }

    if (state->phase == LIGHTBAR_PAUSED_END) {
        state->pause_timer_ms -= dt_ms;
        if (state->pause_timer_ms <= 0.0f) {
            state->direction = -state->direction;
            state->phase = LIGHTBAR_MOVING;
            state->pause_timer_ms = 0.0f;
            state->move_accum_ms = 0.0f;
        }
        return;
    }

    if (state->phase == LIGHTBAR_MOVING) {
        if (config->speed <= 0.0f) return;
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
        }
    }
}

void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds) {
    for (int i = 0; i < config->num_leds; i++) {
        int distance = abs(i - state->position);
        if (distance == 0) {
            leds[i] = config->color;
        } else if (config->glow_radius > 0 && distance <= config->glow_radius) {
            int divisor = config->glow_radius + 1;
            int factor = divisor - distance;
            leds[i].r = (uint8_t)(config->color.r * factor / divisor);
            leds[i].g = (uint8_t)(config->color.g * factor / divisor);
            leds[i].b = (uint8_t)(config->color.b * factor / divisor);
        } else {
            leds[i].r = 0;
            leds[i].g = 0;
            leds[i].b = 0;
        }
    }
}
