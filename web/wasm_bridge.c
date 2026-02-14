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
