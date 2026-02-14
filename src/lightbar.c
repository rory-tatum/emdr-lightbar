#include "lightbar.h"

void lightbar_init(LightbarState *state, const LightbarConfig *config) {
    state->position = config->num_leds / 2;
    state->direction = 1;
    state->phase = LIGHTBAR_STOPPED;
    state->pause_timer_ms = 0.0f;
    state->move_accum_ms = 0.0f;
}

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
