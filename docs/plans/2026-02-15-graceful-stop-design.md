# Graceful Stop Design

## Overview

Change stop behavior so the LED completes its current oscillation cycle
before stopping at the middle, rather than immediately snapping to middle.
Also remove mid_pause entirely.

## Change 1: Remove mid_pause

Remove `mid_pause_ms` from `LightbarConfig`, `LIGHTBAR_PAUSED_MIDDLE` from
`LightbarPhase`, and all related logic, tests, bridge functions, and UI
controls.

## Change 2: Graceful stop with LIGHTBAR_STOPPING phase

### State changes

- Replace `LIGHTBAR_PAUSED_MIDDLE` with `LIGHTBAR_STOPPING` in the
  `LightbarPhase` enum.
- Add `uint8_t edges_remaining` to `LightbarState`.

### lightbar_stop() behavior

Instead of immediately resetting, initiate a graceful wind-down:

- If already STOPPED or STOPPING: no-op.
- Otherwise: set phase to STOPPING and calculate `edges_remaining`:

| Current state                         | edges_remaining |
|---------------------------------------|-----------------|
| Going right, position >= middle       | 2               |
| Going left (any position)             | 1               |
| Going right, position < middle        | 0               |
| PAUSED_END, direction right (at right)| 1               |
| PAUSED_END, direction left (at left)  | 0               |

### lightbar_update() STOPPING behavior

Behaves identically to normal oscillation (movement + end pauses) with two
additions:

1. Each time an edge is reached, decrement `edges_remaining`.
2. After each step, if `position == middle && edges_remaining == 0`:
   finalize stop (phase = STOPPED, direction = 1, clear timers).

End pauses are respected during wind-down.

### lightbar_start() cancels pending stop

If phase is STOPPING, set phase back to MOVING (resume normal oscillation).

### Web frontend

- Remove mid pause slider and event handler.
- Remove mid_pause parameter from wasm_init().
- Remove wasm_set_mid_pause() bridge function.
- Change animation loop to always run (requestAnimationFrame always active).
  The loop is already a no-op when STOPPED since wasm_update returns
  immediately. This lets the STOPPING phase animate without extra state in JS.
- Button click for "Stop" calls wasm_stop() and changes text to "Start".
  Button click for "Start" calls wasm_start() (which cancels STOPPING if
  active) and changes text to "Stop".

## Testing

### Tests to remove

- test_update_triggers_middle_pause
- test_update_no_middle_pause_on_start
- test_middle_pause_expires_and_resumes
- test_middle_pause_no_retrigger_after_resume
- test_zero_mid_pause_skips_pause

### Tests to update

- test_stop_resets_to_middle: rewrite for new STOPPING behavior
- test_stop_clears_accumulators: rewrite (accumulators not cleared on stop)
- test_full_oscillation_cycle: remove mid_pause references

### New tests

1. test_stop_sets_stopping_phase
2. test_stop_edges_remaining_going_right_from_middle (= 2)
3. test_stop_edges_remaining_going_left (= 1)
4. test_stop_edges_remaining_going_right_below_middle (= 0)
5. test_stop_edges_remaining_paused_right_edge (= 1)
6. test_stop_edges_remaining_paused_left_edge (= 0)
7. test_stopping_continues_movement
8. test_stopping_decrements_edges_at_end
9. test_stopping_respects_end_pause
10. test_stopping_finalizes_at_middle
11. test_start_cancels_stopping
12. test_stop_while_already_stopped_is_noop
13. test_stop_while_already_stopping_is_noop
14. test_stopping_full_cycle (integration)
