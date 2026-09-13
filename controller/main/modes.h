/* Modes: the things that decide what 16 characters the panel should show.
 *
 * A mode is a small table entry, not a subclass of anything. It renders a line
 * and says when to ask again; the scheduler owns the timing and panel.c owns
 * the wire. Adding one means writing a file and putting a pointer in MODES[].
 *
 * That is the seam the ADIF real-time feed is meant to arrive through: a
 * mode_realtime.c holding a WebSocket task, whose render() reads the newest
 * departure and formats it with the same layout("<label>|<minutes>") the two
 * counter modes already use.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_http_server.h"

#include "cp437.h"

typedef struct {
    const char *id;         /* stable: it is the NVS value and the form value */
    const char *label;      /* shown in the web UI */

    void (*start)(void);    /* optional: entering this mode */
    void (*stop)(void);     /* optional: leaving it */

    /* Produce the line. Returns milliseconds until the next render is due;
     * 0 means "static, only re-render when the configuration changes". */
    uint32_t (*render)(char out[PANEL_CHARS + 1]);

    /* Emit this mode's configuration rows as HTML table rows. */
    void (*form)(httpd_req_t *r);
} panel_mode_t;

extern const panel_mode_t *const MODES[];
extern const int MODE_COUNT;

/* The active mode, from configuration. Never NULL -- an unknown id falls back
 * to the first entry. */
const panel_mode_t *mode_active(void);

/* Re-read the active mode from configuration, calling stop()/start() if it
 * changed. */
void mode_refresh(void);

/* Wake the scheduler: render now rather than at the next due time. Called
 * whenever configuration changes. */
void mode_notify(void);

/* Start the scheduler task. */
esp_err_t modes_init(void);

/* Shared helpers for mode implementations. */

/* Format a counter line: label, elastic gap, then the minutes hard against the
 * right edge where the "min" sticker continues them. Past 99 minutes the field
 * becomes "2h05", which the sticker still completes.
 * Negative minutes render as a placeholder, for "the clock is not set yet". */
void mode_counter_line(const char *label, int minutes, char out[PANEL_CHARS + 1]);

/* An HTML form row: <tr><th>label<td><input name=... value=...>. */
void mode_row_text(httpd_req_t *r, const char *key, const char *label,
                   const char *value, int maxlen, const char *hint);
