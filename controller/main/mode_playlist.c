/* A list of lines, one at a time.
 *
 * The dwell is floored at 30 seconds by config.c, and the panel's settle time
 * bounds it again below that. Without the floor this becomes a scroller with
 * extra steps, flipping the whole line every few seconds -- the same wear
 * argument that rules out a marquee in mode_message.c.
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "modes.h"
#include "webui.h"

static int cursor;

static void start(void)
{
    cursor = 0;
}

/* Copy line `n` of a newline-separated list. Returns false past the end. */
static bool nth_line(const char *s, int n, char *out, size_t cap)
{
    for (int i = 0; i < n; i++) {
        s = strchr(s, '\n');
        if (!s)
            return false;
        s++;
    }
    if (!*s)
        return false;
    const char *end = strchr(s, '\n');
    size_t len = end ? (size_t)(end - s) : strlen(s);
    if (len >= cap)
        len = cap - 1;
    memcpy(out, s, len);
    out[len] = '\0';
    /* Tolerate CRLF from a browser textarea. */
    while (len && (out[len - 1] == '\r' || out[len - 1] == ' '))
        out[--len] = '\0';
    return true;
}

static uint32_t render(char out[PANEL_CHARS + 1])
{
    const char *items = cfg_get(CFG_PL_ITEMS);
    char line[PANEL_CHARS * 4];

    if (!nth_line(items, cursor, line, sizeof line)) {
        cursor = 0;
        if (!nth_line(items, 0, line, sizeof line)) {
            layout("LISTA VACIA", out);
            return 0;
        }
    }
    layout(line, out);
    cursor++;
    return (uint32_t)cfg_get_int(CFG_PL_DWELL) * 1000u;
}

static void form(httpd_req_t *r)
{
    char esc[1024];
    char buf[1400];

    html_escape(cfg_get(CFG_PL_ITEMS), esc, sizeof esc);
    snprintf(buf, sizeof buf,
             "<tr><th>Lines<td><textarea name=\"%s\" rows=\"6\" cols=\"32\">%s"
             "</textarea> <small>one per line, 16 characters each</small>",
             cfg_name(CFG_PL_ITEMS), esc);
    httpd_resp_sendstr_chunk(r, buf);

    mode_row_text(r, cfg_name(CFG_PL_DWELL), "Seconds each",
                  cfg_get(CFG_PL_DWELL), 5,
                  "30 s minimum: the dots are mechanical");
}

const panel_mode_t mode_playlist = {
    .id     = "playlist",
    .label  = "Playlist",
    .start  = start,
    .render = render,
    .form   = form,
};
