/* Host-side checks for stopwatch.c: exact minute boundaries, which cannot be
 * tested on the board without waiting for them.
 *
 * The two rounding directions are the whole point. A count-up rounds down, so it
 * reads 0 until a full minute has passed; a countdown rounds up, so a fresh
 * 10-minute timer reads 10 and "0 min" on the sign always means finished. Getting
 * either backwards is an off-by-one that looks plausible on the panel.
 */

#include <stdio.h>

#include "stopwatch.h"

#define MIN 60000

static int failures;

#define EXPECT(cond, ...) do {                                                \
        if (!(cond)) {                                                        \
            printf("FAIL %s:%d: ", __func__, __LINE__);                       \
            printf(__VA_ARGS__);                                              \
            printf("\n");                                                     \
            failures++;                                                       \
        }                                                                     \
    } while (0)

static void up(const stopwatch_t *s, int64_t now, int want_min, uint32_t want_due)
{
    uint32_t due;
    int got = sw_up_minutes(s, now, &due);
    EXPECT(got == want_min && due == want_due,
           "up at t=%lld: want %d min due %u, got %d min due %u",
           (long long)now, want_min, want_due, got, due);
}

static void down(stopwatch_t *s, int64_t dur, int64_t now, int want_min,
                 uint32_t want_due)
{
    uint32_t due;
    int got = sw_down_minutes(s, dur, now, &due);
    EXPECT(got == want_min && due == want_due,
           "down at t=%lld: want %d min due %u, got %d min due %u",
           (long long)now, want_min, want_due, got, due);
}

static void test_countup(void)
{
    stopwatch_t s = { 0 };

    up(&s, 0, 0, 0);                   /* never started: 0, and never changes */

    sw_start(&s, 1000);
    up(&s, 1000,          0, MIN);     /* just started */
    up(&s, 1000 + MIN - 1, 0, 1);      /* one millisecond short of a minute */
    up(&s, 1000 + MIN,    1, MIN);     /* exactly one minute: rounds down to 1 */
    up(&s, 1000 + 90000,  1, 30000);   /* 1:30 in: next change in 30 s */

    sw_pause(&s, 1000 + 90000);        /* banked 1:30 */
    up(&s, 1000000,       1, 0);       /* paused: frozen, and no wake-ups */

    sw_start(&s, 2000000);             /* resume after a long pause */
    up(&s, 2000000,       1, 30000);   /* the pause did not count */
    up(&s, 2000000 + 30000, 2, MIN);

    /* Idempotence: a stale page or a double click must not double-count. */
    sw_start(&s, 2100000);             /* already running: ignored */
    up(&s, 2000000 + 30000, 2, MIN);
    sw_pause(&s, 2000000 + 30000);
    sw_pause(&s, 9999999);             /* already paused: ignored */
    up(&s, 9999999,       2, 0);

    sw_reset(&s);
    up(&s, 9999999,       0, 0);
    EXPECT(!s.running, "reset should stop the watch");
}

static void test_countdown(void)
{
    stopwatch_t s = { 0 };
    const int64_t ten = 10 * MIN;

    down(&s, ten, 0, 10, 0);           /* fresh: shows the full 10, not running */

    sw_start(&s, 0);
    down(&s, ten, 0,        10, MIN);  /* rounds UP: a fresh timer reads 10 */
    down(&s, ten, 1,        10, MIN - 1);
    down(&s, ten, MIN,       9, MIN);  /* exactly 9:00 left */
    down(&s, ten, 5 * MIN + 30000, 5, 30000);   /* 4:30 left reads 5 */
    down(&s, ten, ten - 1,   1, 1);    /* one millisecond left still reads 1 */

    /* Zero only when it is genuinely up, and reaching it stops the watch. */
    down(&s, ten, ten,       0, 0);
    EXPECT(!s.running, "countdown should stop itself at zero");
    EXPECT(s.banked_ms == ten, "should bank exactly the duration, got %lld",
           (long long)s.banked_ms);

    /* Overshoot: rendered late, well past the deadline. */
    stopwatch_t late = { 0 };
    sw_start(&late, 0);
    down(&late, ten, ten + 45000, 0, 0);
    EXPECT(late.banked_ms == ten,
           "a late render must not carry the overshoot, banked %lld",
           (long long)late.banked_ms);
    /* ...which matters if the duration is then lengthened: 15 - 10 = 5 left,
     * not 15 - 10:45. */
    down(&late, 15 * MIN, ten + 45000, 5, 0);

    EXPECT(sw_down_finished(&s, ten, ten), "should report finished");
}

static void test_countdown_pause(void)
{
    stopwatch_t s = { 0 };
    const int64_t five = 5 * MIN;

    sw_start(&s, 0);
    sw_pause(&s, 2 * MIN + 15000);     /* 2:45 left */
    down(&s, five, 10 * MIN,  3, 0);   /* frozen at 3 while paused */

    sw_start(&s, 20 * MIN);
    down(&s, five, 20 * MIN,  3, 45000);
    down(&s, five, 20 * MIN + 45000, 2, MIN);

    /* Shortening the duration below what has already run finishes it at once. */
    sw_pause(&s, 20 * MIN + 45000);    /* 3:00 elapsed */
    down(&s, 2 * MIN, 30 * MIN, 0, 0);
}

static void test_parse(void)
{
    struct { const char *in; int ok, want; } cases[] = {
        { "10",     1,   10 },
        { "1",      1,    1 },
        { "90",     1,   90 },
        { "1:30",   1,   90 },
        { "0:45",   1,   45 },
        { "2:00",   1,  120 },
        { " 15 ",   1,   15 },         /* a browser may leave spaces */
        { "5999",   1, 5999 },         /* 99h59, the widest the field shows */
        { "99:59",  1, 5999 },
        { "0",      0,    0 },
        { "0:00",   0,    0 },
        { "6000",   0,    0 },
        { "100:00", 0,    0 },
        { "1:60",   0,    0 },
        { "-5",     0,    0 },
        { "abc",    0,    0 },
        { "10min",  0,    0 },
        { "1:30:00",0,    0 },
        { "",       0,    0 },
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        int got = -1;
        bool ok = sw_parse_minutes(cases[i].in, &got);
        EXPECT(ok == (bool)cases[i].ok && (!ok || got == cases[i].want),
               "parse \"%s\": want %s %d, got %s %d", cases[i].in,
               cases[i].ok ? "ok" : "refused", cases[i].want,
               ok ? "ok" : "refused", got);
    }
    EXPECT(!sw_parse_minutes(NULL, &(int){0}), "NULL must be refused");
}

int main(void)
{
    test_countup();
    test_countdown();
    test_countdown_pause();
    test_parse();
    if (!failures)
        printf("stopwatch: minute rounding, pause, finish and parsing ok\n");
    return failures ? 1 : 0;
}
