/* A start/pause/reset stopwatch, and the minute arithmetic for showing one.
 *
 * The caller supplies a monotonic millisecond clock, so nothing here depends on
 * ESP-IDF: controller/test drives it with made-up times, which is the only way to
 * check the rounding at exact minute boundaries without waiting for them.
 *
 * Monotonic, not wall-clock, on purpose. A stopwatch measures a duration, so it
 * must not care whether NTP has synced yet, or jump when it does, or lose an hour
 * at the end of October.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool    running;
    int64_t banked_ms;    /* run time from earlier start/pause pairs */
    int64_t since_ms;     /* when the current run began; meaningful only if running */
} stopwatch_t;

/* All three are idempotent: start while running, or pause while paused, does
 * nothing -- so a stale page or a double click cannot double-count. */
void    sw_start(stopwatch_t *s, int64_t now_ms);
void    sw_pause(stopwatch_t *s, int64_t now_ms);
void    sw_reset(stopwatch_t *s);

int64_t sw_elapsed_ms(const stopwatch_t *s, int64_t now_ms);

/* Count-up: whole minutes elapsed, rounded DOWN. A watch started just now shows
 * 0, and 1 appears only once a full minute has passed.
 *
 * *due_ms is set to the milliseconds until that number next changes, or 0 if it
 * never will -- the watch is paused. That is what render() returns to the
 * scheduler, so a paused watch costs no wake-ups at all. */
int sw_up_minutes(const stopwatch_t *s, int64_t now_ms, uint32_t *due_ms);

/* Countdown: whole minutes remaining, rounded UP. A fresh 10-minute timer shows
 * 10, not 9, and 0 appears only when the time is genuinely up -- so "0 min" on
 * the sign means finished, never "under a minute left".
 *
 * On reaching zero it stops the watch and banks exactly the duration, so the
 * overshoot between the deadline and the next render is not carried forward if
 * the duration is later lengthened. *due_ms as above. */
int sw_down_minutes(stopwatch_t *s, int64_t duration_ms, int64_t now_ms,
                    uint32_t *due_ms);

/* True if a countdown of this duration has no time left. */
bool sw_down_finished(const stopwatch_t *s, int64_t duration_ms, int64_t now_ms);

/* Parse a duration typed into the web form: "90" is ninety minutes, "1:30" is an
 * hour and a half. Accepts 1 to 5999 minutes -- 99h59, the most the right-hand
 * field can show. Returns false for anything else. */
bool sw_parse_minutes(const char *text, int *minutes);
