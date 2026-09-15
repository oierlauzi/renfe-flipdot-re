/* Time until, and time since, a time of day.
 *
 * Minutes to the next occurrence of an "HH:MM", or since the last one, against
 * the wall clock. They were once called countdown and count-up, which undersold
 * what distinguishes them: they need no button, run by themselves from NTP, and
 * roll over every day. The start/pause/reset stopwatches are mode_stopwatch.c.
 *
 * One file and one layout, because they are the same thing with the sign of the
 * subtraction flipped: a label on the left, minutes held against the right edge
 * where the "min" vinyl sticker continues them. That is the layout the sign used
 * in service -- destination plus ETA -- so it is also the shape the future ADIF
 * real-time mode will produce, through the same mode_counter_line().
 */

#include <stdio.h>

#include "config.h"
#include "modes.h"
#include "timesrc.h"

static uint32_t render_common(char out[PANEL_CHARS + 1],
                              cfg_key_t label_key, cfg_key_t time_key,
                              bool until)
{
    int h, m;
    if (!timesrc_parse_hhmm(cfg_get(time_key), &h, &m)) {
        layout("HORA NO VALIDA", out);
        return 0;
    }
    int mins = until ? timesrc_minutes_until(h, m) : timesrc_minutes_since(h, m);
    mode_counter_line(cfg_get(label_key), mins, out);

    /* mins < 0 means the clock is not set yet: mode_counter_line() shows "--"
     * and we look again shortly rather than once a minute. */
    return mins < 0 ? 5000 : (uint32_t)timesrc_ms_to_next_minute();
}

/* ------------------------------------------------------------ time until */

static uint32_t until_render(char out[PANEL_CHARS + 1])
{
    return render_common(out, CFG_TU_LABEL, CFG_TU_TIME, true);
}

static void until_form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_TU_LABEL), "Label", cfg_get(CFG_TU_LABEL),
                  16, "anything: a destination, CAFE, HORNO");
    mode_row_text(r, cfg_name(CFG_TU_TIME), "Until", cfg_get(CFG_TU_TIME),
                  5, "HH:MM, the next time it comes round");
}

const panel_mode_t mode_until = {
    .id     = "until",
    .label  = "Time until",
    .render = until_render,
    .form   = until_form,
};

/* ------------------------------------------------------------ time since */

static uint32_t since_render(char out[PANEL_CHARS + 1])
{
    return render_common(out, CFG_TS_LABEL, CFG_TS_TIME, false);
}

static void since_form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_TS_LABEL), "Label", cfg_get(CFG_TS_LABEL),
                  16, NULL);
    mode_row_text(r, cfg_name(CFG_TS_TIME), "Since", cfg_get(CFG_TS_TIME),
                  5, "HH:MM, the last time it came round");
}

const panel_mode_t mode_since = {
    .id     = "since",
    .label  = "Time since",
    .render = since_render,
    .form   = since_form,
};
