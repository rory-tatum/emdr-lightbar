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
    uint8_t glow_radius;
    Led color;
} LightbarConfig;

typedef enum {
    LIGHTBAR_STOPPED,
    LIGHTBAR_MOVING,
    LIGHTBAR_PAUSED_END,
    LIGHTBAR_STOPPING
} LightbarPhase;

typedef struct {
    int position;
    int direction;
    LightbarPhase phase;
    float pause_timer_ms;
    float move_accum_ms;
    uint8_t edges_remaining;
} LightbarState;

void lightbar_init(LightbarState *state, const LightbarConfig *config);
void lightbar_start(LightbarState *state);
void lightbar_stop(LightbarState *state, const LightbarConfig *config);
void lightbar_update(LightbarState *state, const LightbarConfig *config, float dt_ms);
void lightbar_render(const LightbarState *state, const LightbarConfig *config, Led *leds);

#endif
