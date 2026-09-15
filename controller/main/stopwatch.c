/* See stopwatch.h. Depends on nothing but the C library. */

#include "stopwatch.h"

#include <stdio.h>
#include <string.h>

#define MINUTE_MS 60000

void sw_start(stopwatch_t *s, int64_t now_ms)
{
    if (s->running)
        return;
    s->running = true;
    s->since_ms = now_ms;
}

void sw_pause(stopwatch_t *s, int64_t now_ms)
{
    if (!s->running)
        return;
    s->banked_ms += now_ms - s->since_ms;
    s->running = false;
}

void sw_reset(stopwatch_t *s)
{
    s->running = false;
    s->banked_ms = 0;
    s->since_ms = 0;
}

int64_t sw_elapsed_ms(const stopwatch_t *s, int64_t now_ms)
{
    int64_t e = s->banked_ms + (s->running ? now_ms - s->since_ms : 0);
    return e < 0 ? 0 : e;
}

int sw_up_minutes(const stopwatch_t *s, int64_t now_ms, uint32_t *due_ms)
{
    int64_t e = sw_elapsed_ms(s, now_ms);
    if (due_ms)
        *due_ms = s->running ? (uint32_t)(MINUTE_MS - e % MINUTE_MS) : 0;
    return (int)(e / MINUTE_MS);
}

int sw_down_minutes(stopwatch_t *s, int64_t duration_ms, int64_t now_ms,
                    uint32_t *due_ms)
{
    int64_t rem = duration_ms - sw_elapsed_ms(s, now_ms);
    if (rem <= 0) {
        if (s->running) {
            s->running = false;
            s->banked_ms = duration_ms > 0 ? duration_ms : 0;
        }
        if (due_ms)
            *due_ms = 0;
        return 0;
    }

    /* Rounded up, the shown number drops when rem crosses a whole minute. From
     * 4:30 left that is in 30 s; from exactly 5:00 left it is a full minute. */
    if (due_ms) {
        int64_t part = rem % MINUTE_MS;
        *due_ms = s->running ? (uint32_t)(part ? part : MINUTE_MS) : 0;
    }
    return (int)((rem + MINUTE_MS - 1) / MINUTE_MS);
}

bool sw_down_finished(const stopwatch_t *s, int64_t duration_ms, int64_t now_ms)
{
    return duration_ms - sw_elapsed_ms(s, now_ms) <= 0;
}

bool sw_parse_minutes(const char *text, int *minutes)
{
    if (!text)
        return false;

    char buf[16];
    while (*text == ' ')
        text++;
    snprintf(buf, sizeof buf, "%s", text);
    size_t n = strlen(buf);
    while (n && buf[n - 1] == ' ')
        buf[--n] = '\0';

    int a, b, total;
    char tail;
    if (strchr(buf, ':')) {
        if (sscanf(buf, "%d:%d%c", &a, &b, &tail) != 2)
            return false;
        if (a < 0 || b < 0 || b > 59)
            return false;
        total = a * 60 + b;
    } else {
        if (sscanf(buf, "%d%c", &a, &tail) != 1)
            return false;
        total = a;
    }

    if (total < 1 || total > 5999)
        return false;
    *minutes = total;
    return true;
}
