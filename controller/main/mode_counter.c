/* Countdown and count-up.
 *
 * One file and one layout, because they are the same thing with the sign of the
 * subtraction flipped: a label on the left, minutes held against the right edge
 * where the "min" vinyl sticker continues them. That is the layout the sign
 * actually used in service -- destination plus ETA -- so it is also the shape
 * the future ADIF real-time mode will produce, through the same
 * mode_counter_line().
 *
 * Both take a wall-clock "HH:MM" rather than a date, which keeps the two
 * symmetric and needs no date picker: the countdown runs to its next
 * occurrence, the count-up from its last.
 */

#include <stdio.h>

#include "config.h"
#include "modes.h"
#include "timesrc.h"

static uint32_t render_common(char out[PANEL_CHARS + 1],
                              cfg_key_t label_key, cfg_key_t time_key,
                              bool down)
{
    int h, m;
    if (!timesrc_parse_hhmm(cfg_get(time_key), &h, &m)) {
        layout("HORA NO VALIDA", out);
        return 0;
    }
    int mins = down ? timesrc_minutes_until(h, m) : timesrc_minutes_since(h, m);
    mode_counter_line(cfg_get(label_key), mins, out);

    /* mins < 0 means the clock is not set yet: mode_counter_line() shows "--"
     * and we look again shortly rather than once a minute. */
    return mins < 0 ? 5000 : (uint32_t)timesrc_ms_to_next_minute();
}

/* ------------------------------------------------------------- countdown */

static uint32_t cd_render(char out[PANEL_CHARS + 1])
{
    return render_common(out, CFG_CD_LABEL, CFG_CD_TIME, true);
}

static void cd_form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_CD_LABEL), "Label", cfg_get(CFG_CD_LABEL),
                  16, "anything: a destination, CAFE, HORNO");
    mode_row_text(r, cfg_name(CFG_CD_TIME), "Counts down to", cfg_get(CFG_CD_TIME),
                  5, "HH:MM, the next time it comes round");
}

const panel_mode_t mode_countdown = {
    .id     = "countdown",
    .label  = "Countdown",
    .render = cd_render,
    .form   = cd_form,
};

/* --------------------------------------------------------------- count-up */

static uint32_t cu_render(char out[PANEL_CHARS + 1])
{
    return render_common(out, CFG_CU_LABEL, CFG_CU_TIME, false);
}

static void cu_form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_CU_LABEL), "Label", cfg_get(CFG_CU_LABEL),
                  16, NULL);
    mode_row_text(r, cfg_name(CFG_CU_TIME), "Counts up from", cfg_get(CFG_CU_TIME),
                  5, "HH:MM, the last time it came round");
}

const panel_mode_t mode_countup = {
    .id     = "countup",
    .label  = "Count-up",
    .render = cu_render,
    .form   = cu_form,
};
